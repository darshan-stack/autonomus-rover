#!/usr/bin/env python3
"""
RViz Configuration Launcher for Lunar Habitat Robot Control Panel
"""

from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
import os
from ament_index_python.packages import get_package_share_directory

def generate_launch_description():
    # Get package directory
    pkg_dir = get_package_share_directory('mars_rover_navigation')
    
    # RViz config file (we'll create this)
    rviz_config_file = os.path.join(pkg_dir, 'config', 'lunar_habitat_visualization.rviz')
    
    return LaunchDescription([
        DeclareLaunchArgument(
            'use_sim_time',
            default_value='true',
            description='Use simulation time'
        ),
        
        # Launch RViz with custom config
        Node(
            package='rviz2',
            executable='rviz2',
            name='rviz2',
            output='screen',
            arguments=['-d', rviz_config_file] if os.path.exists(rviz_config_file) else [],
            parameters=[{
                'use_sim_time': LaunchConfiguration('use_sim_time')
            }]
        ),
    ])
