// Copyright (c) 2019, Map IV, Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name of the Map IV, Inc. nor the names of its contributors
//   may be used to endorse or promote products derived from this software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDER BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

/*
 * can_velocity_converter.cpp
 * Author MapIV Sekino
 * Updated by LMA Toffanetto
 */

#include "rclcpp/rclcpp.hpp"
#include "can_msgs/msg/frame.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"

rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr pub;
static geometry_msgs::msg::TwistStamped msg_velocity;

static int can_id = 0x123;
static std::string node_name = "can_velocity_converter";

void can_callback(const can_msgs::msg::Frame::ConstSharedPtr msg)
{
  if (msg->id == static_cast<uint32_t>(can_id))
  {
    msg_velocity.header.stamp = msg->header.stamp;
    msg_velocity.header.frame_id = "base_link";

    msg_velocity.twist.linear.x = ((msg->data[0] << 8) + msg->data[1]) / 100.0;  // cm/s to m/s

    RCLCPP_INFO(rclcpp::get_logger(node_name), "%lf m/s", msg_velocity.twist.linear.x);

    pub->publish(msg_velocity);
  }
}

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  auto node = rclcpp::Node::make_shared("can_velocity_converter");

  node->declare_parameter("can_id", can_id);

  node->get_parameter("can_id", can_id);

  std::cout << "can_id " << can_id << std::endl;

  auto sub1 = node->create_subscription<can_msgs::msg::Frame>("/vehicle/can_tx", rclcpp::QoS(10), can_callback);
  pub = node->create_publisher<geometry_msgs::msg::TwistStamped>("/can_twist", rclcpp::QoS(10));

  rclcpp::spin(node);

  return 0;
}
