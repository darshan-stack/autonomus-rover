#!/bin/bash

echo "========================================="
echo "ROVER MOTION TEST"
echo "========================================="
echo ""
echo "This script will:"
echo "1. Launch Gazebo with the rover"
echo "2. Start the ROS-Gazebo bridge"
echo "3. Send forward motion commands"
echo ""
echo "Watch the rover - it should move FORWARD!"
echo ""

# Kill existing processes
pkill -9 ign 2>/dev/null
pkill -9 gz 2>/dev/null
pkill -9 parameter_bridge 2>/dev/null
sleep 2

# Source ROS 2
source /opt/ros/humble/setup.bash

# Resolve repo root from this script location
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Set up Gazebo paths
cd "$SCRIPT_DIR"
export IGN_GAZEBO_RESOURCE_PATH=$IGN_GAZEBO_RESOURCE_PATH:$(pwd)
export GZ_SIM_RESOURCE_PATH=$GZ_SIM_RESOURCE_PATH:$(pwd)

echo "Starting Gazebo..."
ign gazebo -r MyModel.world &
GAZEBO_PID=$!
sleep 8

echo "Starting ROS-Gazebo Bridge..."
ros2 run ros_gz_bridge parameter_bridge \
    /lidar@sensor_msgs/msg/LaserScan[ignition.msgs.LaserScan \
    /odom@nav_msgs/msg/Odometry[ignition.msgs.Odometry \
    /cmd_vel@geometry_msgs/msg/Twist]ignition.msgs.Twist &
BRIDGE_PID=$!
sleep 3

echo ""
echo "✅ System ready!"
echo ""
echo "🚀 Gazebo is already running. Press Enter to continue..."
read

echo ""
echo "📡 Sending FORWARD motion command..."
echo "   linear.x = 0.5 m/s (forward)"
echo "   angular.z = 0.0 rad/s (no rotation)"
echo ""

ros2 topic pub /cmd_vel geometry_msgs/msg/Twist "{linear: {x: 0.5, y: 0.0, z: 0.0}, angular: {x: 0.0, y: 0.0, z: 0.0}}" &
CMD_PID=$!

echo ""
echo "⏱️  Testing for 10 seconds..."
sleep 10

echo ""
echo "🎯 Now testing ROTATION..."
echo "   linear.x = 0.0 m/s"
echo "   angular.z = 0.5 rad/s (turning)"
echo ""

kill $CMD_PID 2>/dev/null
ros2 topic pub /cmd_vel geometry_msgs/msg/Twist "{linear: {x: 0.0, y: 0.0, z: 0.0}, angular: {x: 0.0, y: 0.0, z: 0.5}}" &
CMD_PID=$!

sleep 5

echo ""
echo "✅ Test complete!"
echo ""
echo "Did the rover:"
echo "  ✓ Move FORWARD in the first test?"
echo "  ✓ Rotate in place in the second test?"
echo ""
echo "If YES - The fix worked! 🎉"
echo "If NO - Check Gazebo console for errors"
echo ""

kill $CMD_PID 2>/dev/null
kill $BRIDGE_PID 2>/dev/null
kill $GAZEBO_PID 2>/dev/null

echo "Press Enter to exit..."
read
