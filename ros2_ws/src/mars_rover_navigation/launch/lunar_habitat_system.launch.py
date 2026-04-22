#!/usr/bin/env python3
"""
Lunar Habitat Maintenance Robot - Complete System Launch
Includes navigation, mapping, object detection, environmental monitoring, and telemetry
"""

from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration

def generate_launch_description():
    return LaunchDescription([
        # Launch Arguments
        DeclareLaunchArgument(
            'use_sim_time',
            default_value='true',
            description='Use simulation time'
        ),
        
        # Enhanced Object Detector - identifies and names objects
        Node(
            package='mars_rover_navigation',
            executable='enhanced_object_detector',
            name='enhanced_object_detector',
            output='screen',
            parameters=[{
                'use_sim_time': LaunchConfiguration('use_sim_time'),
                'min_cluster_size': 3,
                'max_cluster_distance': 0.5,
                'rock_size_threshold': 0.4,
                'large_obstacle_threshold': 1.0
            }]
        ),
        
        # Environmental Monitor - tracks habitat parameters
        Node(
            package='mars_rover_navigation',
            executable='environmental_monitor',
            name='environmental_monitor',
            output='screen',
            parameters=[{
                'use_sim_time': LaunchConfiguration('use_sim_time'),
                'nominal_temperature': 22.0,
                'nominal_pressure': 101.3,
                'nominal_o2': 21.0,
                'nominal_co2': 0.04,
                'nominal_humidity': 45.0,
                'nominal_radiation': 0.5,
                'temp_low_threshold': 18.0,
                'temp_high_threshold': 26.0,
                'o2_low_threshold': 19.5,
                'co2_high_threshold': 0.5,
                'pressure_low_threshold': 95.0,
                'radiation_high_threshold': 2.0
            }]
        ),
        
        # Path Tracker - logs navigation data for HQ
        Node(
            package='mars_rover_navigation',
            executable='path_tracker',
            name='path_tracker',
            output='screen',
            parameters=[{
                'use_sim_time': LaunchConfiguration('use_sim_time'),
                'log_interval': 2.0,
                'min_distance_threshold': 0.05,
                'log_file_path': '/tmp/lunar_habitat_mission_log.txt'
            }]
        ),
        
        # Do not publish fake static odom/base transforms here.
        # Gazebo diff-drive publishes odom->base_link and SLAM provides map->odom.
        
        # Autonomous Patrol - Makes rover move autonomously
        Node(
            package='mars_rover_navigation',
            executable='autonomous_patrol',
            name='autonomous_patrol',
            output='screen',
            parameters=[{
                'use_sim_time': LaunchConfiguration('use_sim_time'),
                'forward_speed': 0.3,
                'turn_speed': 0.5,
                'obstacle_distance': 2.0,
                'safe_distance': 3.0,
                'patrol_enabled': True
            }]
        ),
        
        # HQ Control Panel - Displays all data
        Node(
            package='mars_rover_navigation',
            executable='hq_control_panel',
            name='hq_control_panel',
            output='screen',
            parameters=[{
                'use_sim_time': LaunchConfiguration('use_sim_time')
            }]
        ),
    ])
