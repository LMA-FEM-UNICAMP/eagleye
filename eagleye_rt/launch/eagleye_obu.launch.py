# TODO:
    
#     - launch eagleye_rt
#     - launch nmea_ros_driver
#     - launch can_velocity_converter
#     - launch socket_can_receiver

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_xml.launch_description_sources import XMLLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from launch.actions import ExecuteProcess


def generate_launch_description():
    
    eagleye_rt_launch = IncludeLaunchDescription(
      XMLLaunchDescriptionSource([os.path.join(
         get_package_share_directory('eagleye_rt')),
         '/launch/eagleye_rt_lite.launch.xml']))
    
    nmea_ros_driver_launch = IncludeLaunchDescription(
      PythonLaunchDescriptionSource([os.path.join(
         get_package_share_directory('nmea_ros_bridge')),
         '/launch/nmea_tcp.launch.py']))
    
    eagleye_can_velocity_converter_launch = IncludeLaunchDescription(
      XMLLaunchDescriptionSource([os.path.join(
         get_package_share_directory('eagleye_can_velocity_converter')),
         '/launch/can_velocity_converter.xml']))
    
    socket_can_receiver_launch = IncludeLaunchDescription(
      PythonLaunchDescriptionSource([os.path.join(
         get_package_share_directory('eagleye_rt')),
         '/launch/socket_can_receiver.launch.py']))
    
    navsat2nmea = ExecuteProcess(
      cmd=["/bin/bash", "-c", os.path.join(get_package_share_directory("navsatfix2nmea"), "scripts", "restart_launch.sh")],
      output="screen"
    )
    
    return LaunchDescription([
        eagleye_rt_launch,
        nmea_ros_driver_launch,
        socket_can_receiver_launch,
        eagleye_can_velocity_converter_launch,
        navsat2nmea
    ])
