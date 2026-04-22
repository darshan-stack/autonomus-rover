#!/usr/bin/env python3
"""
Master Launch File - Complete Lunar Habitat Maintenance Robot System
Launches: Gazebo, ROS Bridge, All Monitoring Nodes, and Visualization
"""

from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, ExecuteProcess, TimerAction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
import os

def generate_launch_description():
    
    return LaunchDescription([
        # Note: Gazebo must be started separately with:
        # cd ~/asd-main && export IGN_GAZEBO_RESOURCE_PATH=$IGN_GAZEBO_RESOURCE_PATH:$(pwd) && export GZ_SIM_RESOURCE_PATH=$GZ_SIM_RESOURCE_PATH:$(pwd) && ign gazebo MyModel.world
        
        # ROS-Gazebo Bridge
        ExecuteProcess(
            cmd=['ros2', 'run', 'ros_gz_bridge', 'parameter_bridge',
                 '/lidar@sensor_msgs/msg/LaserScan[ignition.msgs.LaserScan',
                 '/odom@nav_msgs/msg/Odometry[ignition.msgs.Odometry',
                 '/cmd_vel@geometry_msgs/msg/Twist]ignition.msgs.Twist'],
            output='screen',
            shell=False
        ),
        
        # Wait a bit for bridge to start
        TimerAction(
            period=2.0,
            actions=[
                # Enhanced Object Detector
                Node(
                    package='mars_rover_navigation',
                    executable='enhanced_object_detector',
                    name='enhanced_object_detector',
                    output='screen',
                    parameters=[{
                        'use_sim_time': True,
                        'min_cluster_size': 3,
                        'max_cluster_distance': 0.5,
                        'rock_size_threshold': 0.4,
                        'large_obstacle_threshold': 1.0
                    }]
                ),
                
                # Environmental Monitor
                Node(
                    package='mars_rover_navigation',
                    executable='environmental_monitor',
                    name='environmental_monitor',
                    output='screen',
                    parameters=[{
                        'use_sim_time': True
                    }]
                ),
                
                # Path Tracker
                Node(
                    package='mars_rover_navigation',
                    executable='path_tracker',
                    name='path_tracker',
                    output='screen',
                    parameters=[{
                        'use_sim_time': True,
                        'log_interval': 2.0
                    }]
                ),
                
                # TF Publishers
                Node(
                    package='tf2_ros',
                    executable='static_transform_publisher',
                    name='map_to_odom',
                    arguments=['0', '0', '0', '0', '0', '0', 'map', 'odom']
                ),
            ]
        ),
    ])
