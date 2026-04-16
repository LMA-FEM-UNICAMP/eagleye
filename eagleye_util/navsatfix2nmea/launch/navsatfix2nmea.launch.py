import os
import sys

from ament_index_python.packages import get_package_share_directory

from launch import LaunchDescription
from launch.actions import RegisterEventHandler, EmitEvent
from launch.actions import ExecuteProcess
from launch.event_handlers import OnProcessExit
from launch.events import Shutdown
from launch_ros.actions import Node
from launch.actions import IncludeLaunchDescription, DeclareLaunchArgument, OpaqueFunction
from launch.substitutions import LaunchConfiguration


def generate_launch_description():
    
    gps_ip_arg = DeclareLaunchArgument(
            name='gps_ip',
            default_value='127.0.0.1',
            description='IP of the GPS server')
    
    local_gps_port_arg = DeclareLaunchArgument(
            name='local_gps_port',
            default_value='5000',
            description='Local GPS port for NMEA')
    
    gps_ip = LaunchConfiguration('gps_ip')
    local_gps_port = LaunchConfiguration('local_gps_port')
    gpsd_script_path = os.path.join(get_package_share_directory("navsatfix2nmea"), "scripts", "gpsd_tcp.sh")

    navsatfix2nmea = Node(
        package='navsatfix2nmea',
        executable='navsatfix2nmea_node',
        name='navsatfix2nmea',
        output='both'
    )
    
    gpsd = ExecuteProcess(
        cmd=[gpsd_script_path, gps_ip, local_gps_port],
            output='screen'
        )

    # When the node exits, trigger a shutdown of the whole launch system
    restart_on_gpsd_exit = RegisterEventHandler(
        OnProcessExit(
            target_action=gpsd,
            on_exit=[
                EmitEvent(event=Shutdown(reason='gpsd died, restarting launch'))
            ]
        )
    )

    return LaunchDescription([
        gps_ip_arg,
        local_gps_port_arg,
        navsatfix2nmea,
        gpsd,
        restart_on_gpsd_exit
    ])