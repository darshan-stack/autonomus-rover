#!/bin/bash

# Quick Test Script for Lunar Habitat System
# Verifies all components are working

source /opt/ros/humble/setup.bash

echo "========================================="
echo "LUNAR HABITAT SYSTEM - QUICK TEST"
echo "========================================="
echo ""

echo "📋 Checking Active Nodes..."
ros2 node list
echo ""

echo "📡 Checking Active Topics..."
ros2 topic list
echo ""

echo "🔍 Testing Topic Rates..."
echo "LiDAR:"
timeout 3 ros2 topic hz /lidar 2>/dev/null || echo "  No data (is Gazebo running and PLAY pressed?)"

echo "Odometry:"
timeout 3 ros2 topic hz /odom 2>/dev/null || echo "  No data"

echo ""
echo "📊 Sample Telemetry (5 seconds each)..."
echo ""
echo "--- Object Information ---"
timeout 5 ros2 topic echo /object_information --once 2>/dev/null || echo "No objects detected yet"
echo ""

echo "--- Environmental Status ---"
timeout 5 ros2 topic echo /habitat/environmental_status --once 2>/dev/null || echo "Monitor not running"
echo ""

echo "--- Navigation Telemetry ---"
timeout 5 ros2 topic echo /navigation_telemetry --once 2>/dev/null || echo "Tracker not running"
echo ""

echo "--- HQ Telemetry ---"
timeout 5 ros2 topic echo /hq_telemetry --once 2>/dev/null || echo "No telemetry yet"
echo ""

echo "========================================="
echo "✅ Test Complete"
echo "========================================="
echo ""
echo "To control rover:"
echo "  Forward: ros2 topic pub /cmd_vel geometry_msgs/msg/Twist \"{linear: {x: 0.5}, angular: {z: 0.0}}\" -r 10"
echo "  Stop:    ros2 topic pub /cmd_vel geometry_msgs/msg/Twist \"{linear: {x: 0.0}, angular: {z: 0.0}}\" --once"
echo ""
echo "View mission log:"
echo "  cat /tmp/lunar_habitat_mission_log.txt"
echo ""
