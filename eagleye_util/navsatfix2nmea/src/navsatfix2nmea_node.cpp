#include <rclcpp/rclcpp.hpp>

#include <geographic_msgs/msg/geo_pose_with_covariance_stamped.hpp>
#include <geometry_msgs/msg/twist_stamped.hpp>

#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>
#include <tf2/utils.h>

#include <cmath>
#include <sstream>
#include <string>
#include <iomanip>
#include <chrono>

// UDP
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>

#define RAD2DEG(x) ((x) * 180.0 / M_PI)

class NMEAUdpPublisher : public rclcpp::Node
{
public:
  NMEAUdpPublisher()
    : Node("nmea_udp_publisher")
    , lat_(0.0)
    , lon_(0.0)
    , heading_deg_(0.0)
    , vel_(0.0)
    , alt_(0.0)
    , has_fix_(false)
  {
    gps_sub_ = this->create_subscription<geographic_msgs::msg::GeoPoseWithCovarianceStamped>(
        "/eagleye/geo_pose_with_covariance", 10,
        std::bind(&NMEAUdpPublisher::gpsCallback, this, std::placeholders::_1));

    vel_sub_ = this->create_subscription<geometry_msgs::msg::TwistStamped>(
        "/eagleye/vehicle/twist", 10, std::bind(&NMEAUdpPublisher::velCallback, this, std::placeholders::_1));
  }

  ~NMEAUdpPublisher()
  {
  }

private:
  // ---------------- Callbacks ----------------

  void velCallback(const geometry_msgs::msg::TwistStamped::SharedPtr msg)
  {
    vel_ = msg->twist.linear.x / 0.514444;
  }

  void gpsCallback(const geographic_msgs::msg::GeoPoseWithCovarianceStamped::SharedPtr msg)
  {
    lat_ = msg->pose.pose.position.latitude;
    lon_ = msg->pose.pose.position.longitude;
    has_fix_ = true;
    sec_ = msg->header.stamp.sec;
    alt_ = msg->pose.pose.position.longitude;

    heading_deg_ = RAD2DEG(tf2::getYaw(msg->pose.pose.orientation));

    publishNMEA();
  }

  // ---------------- Helpers ----------------

  std::pair<std::string, char> degToNMEA(double deg, bool is_lat)
  {
    double abs_deg = std::abs(deg);
    int d = static_cast<int>(abs_deg);
    double m = (abs_deg - d) * 60.0;

    std::ostringstream oss;

    if (is_lat)
      oss << std::setw(2) << std::setfill('0') << d;
    else
      oss << std::setw(3) << std::setfill('0') << d;

    oss << std::fixed << std::setprecision(4) << std::setw(7) << std::setfill('0') << m;

    char dir = (deg >= 0.0) ? (is_lat ? 'N' : 'E') : (is_lat ? 'S' : 'W');

    return { oss.str(), dir };
  }

  std::string getChecksum(const std::string& s)
  {
    uint8_t cs = 0;
    for (auto c : s)
      cs ^= static_cast<uint8_t>(c);

    std::ostringstream oss;
    oss << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(cs);

    return oss.str();
  }

  std::string navSatFixToGPGGA(int num_satellites = 8, double hdop = 0.9)
  {
    std::time_t t = sec_;
    std::tm* gmt = std::gmtime(&t);

    std::stringstream time_ss;
    time_ss << std::setfill('0') << std::setw(2) << gmt->tm_hour << std::setw(2) << gmt->tm_min << std::setw(2)
            << gmt->tm_sec;

    // Latitude / Longitude
    auto [lat_str, lat_dir] = degToNMEA(lat_, true);
    auto [lon_str, lon_dir] = degToNMEA(lon_, false);

    // Build sentence (without $ and checksum)
    std::stringstream body;
    body << "GPGGA," << time_ss.str() << "," << lat_str << "," << lat_dir << "," << lon_str << "," << lon_dir << ","
         << has_fix_ << "," << num_satellites << "," << std::fixed << std::setprecision(1) << hdop << "," << std::fixed
         << std::setprecision(1) << alt_ << ",M,"
         << "0.0,M,"  // TODO geoid separation (unknown → 0)
         << ","       // DGPS age
         << "";       // DGPS station ID

    std::string sentence_body = body.str();
    std::string checksum = getChecksum(sentence_body);

    return "$" + sentence_body + "*" + checksum + "\r\n";
  }

  void publishNMEA()
  {
    if (!has_fix_)
      return;

    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm* gmt = std::gmtime(&t);

    char time_str[16];
    char date_str[16];

    std::strftime(time_str, sizeof(time_str), "%H%M%S", gmt);
    std::strftime(date_str, sizeof(date_str), "%d%m%y", gmt);

    auto [lat_str, lat_dir] = degToNMEA(lat_, true);
    auto [lon_str, lon_dir] = degToNMEA(lon_, false);

    std::ostringstream body;
    body << "GPRMC," << time_str << ".00,A," << lat_str << "," << lat_dir << "," << lon_str << "," << lon_dir << ","
         << std::fixed << std::setprecision(2) << vel_ << ","  // speed
         << heading_deg_ << "," << date_str << ",,,A";

    std::string body_str = body.str();
    std::string full = "$" + body_str + "*" + getChecksum(body_str) + "\r\n";
    std::string gpgga = navSatFixToGPGGA();

    // TODO Send
    // send(full);
    // send(gpgga);

    RCLCPP_INFO(this->get_logger(), "%s", full.c_str());
    RCLCPP_INFO(this->get_logger(), "%s", gpgga.c_str());
  }

  // ---------------- Members ----------------

  rclcpp::Subscription<geographic_msgs::msg::GeoPoseWithCovarianceStamped>::SharedPtr gps_sub_;
  rclcpp::Subscription<geometry_msgs::msg::TwistStamped>::SharedPtr vel_sub_;

  double lat_, lon_, alt_;
  double heading_deg_;
  double vel_;
  bool has_fix_;
  uint16_t sec_;
  
};

// ---------------- Main ----------------

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<NMEAUdpPublisher>());
  rclcpp::shutdown();
  return 0;
}