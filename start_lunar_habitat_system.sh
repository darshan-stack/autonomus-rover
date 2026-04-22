#!/bin/bash

# Lunar Habitat Maintenance Robot - Startup Script
# Launches all system components in sequence

echo "========================================="
echo "LUNAR HABITAT MAINTENANCE ROBOT SYSTEM"
echo "========================================="

# Kill any existing processes
echo "Stopping existing processes..."
pkill -9 ign 2>/dev/null
pkill -9 gz 2>/dev/null
pkill -9 ruby 2>/dev/null
pkill -9 object_detector 2>/dev/null
pkill -9 parameter_bridge 2>/dev/null
pkill -9 rviz2 2>/dev/null
sleep 2

# Source ROS 2
source /opt/ros/humble/setup.bash

# Resolve repo root from this script location
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/ros2_ws/install/setup.bash"

echo ""
echo "Step 1: Starting Gazebo Simulation..."
echo "(Simulation auto-starts in RUN mode)"
cd "$SCRIPT_DIR"
export IGN_GAZEBO_RESOURCE_PATH=$IGN_GAZEBO_RESOURCE_PATH:$(pwd)
export GZ_SIM_RESOURCE_PATH=$GZ_SIM_RESOURCE_PATH:$(pwd)
ign gazebo -r MyModel.world &
GAZEBO_PID=$!

sleep 5

echo ""
echo "Step 2: Starting anti-pause watchdog..."
(
  while true; do
    ign service -s /world/untitled_world/control \
      --reqtype ignition.msgs.WorldControl \
      --reptype ignition.msgs.Boolean \
      --timeout 1000 \
      --req 'pause: false' >/dev/null 2>&1 || true
    sleep 2
  done
) &
WATCHDOG_PID=$!

sleep 3

echo ""
echo "Step 3: Starting Navigation Stack (Bridge + SLAM + Nav2 + Path Tracker)..."
ros2 launch mars_rover_navigation rover_navigation.launch.py use_sim_time:=true &
NAV_PID=$!

sleep 2

echo ""
echo "Step 4: Starting Enhanced Object Detector..."
ros2 run mars_rover_navigation enhanced_object_detector &
DETECTOR_PID=$!

sleep 1

echo ""
echo "Step 5: Starting Environmental Monitor..."
ros2 run mars_rover_navigation environmental_monitor &
MONITOR_PID=$!

sleep 1

echo ""
echo "Step 6: Starting Autonomous Patrol..."
echo "(Rover will move autonomously)"
ros2 run mars_rover_navigation autonomous_patrol &
PATROL_PID=$!

sleep 1

echo ""
echo "Step 7: Starting HQ Control Panel..."
ros2 run mars_rover_navigation hq_control_panel &
PANEL_PID=$!

sleep 1

echo ""
echo "Step 8: Starting RViz..."
RVIZ_CONFIG="$SCRIPT_DIR/ros2_ws/src/mars_rover_navigation/config/lunar_habitat_visualization.rviz"
env -u LD_LIBRARY_PATH rviz2 -d "$RVIZ_CONFIG" &
RVIZ_PID=$!

echo ""
echo "========================================="
echo "✅ ALL SYSTEMS OPERATIONAL"
echo "========================================="
echo ""
echo "Process IDs:"
echo "  Gazebo:     $GAZEBO_PID"
echo "  Watchdog:   $WATCHDOG_PID"
echo "  Nav Stack:  $NAV_PID"
echo "  Detector:   $DETECTOR_PID"
echo "  Monitor:    $MONITOR_PID"
echo "  Patrol:     $PATROL_PID"
echo "  Panel:      $PANEL_PID"
echo "  RViz:       $RVIZ_PID"
echo ""
echo "🤖 AUTONOMOUS MODE: Rover is moving autonomously!"
echo "📊 HQ Control Panel: Displaying object names and telemetry"
echo "🗺️  Path Visualization: Check RViz for complete path"
echo ""
echo "📡 Object names will be sent to HQ panel when detected"
echo ""
echo "📊 View Telemetry:"
echo "  ros2 topic echo /object_information"
echo "  ros2 topic echo /habitat/environmental_status"
echo "  ros2 topic echo /navigation_telemetry"
echo "  ros2 topic echo /hq_telemetry"
echo ""
echo "🎮 Control Rover:"
echo "  ros2 topic pub /cmd_vel geometry_msgs/msg/Twist \"{linear: {x: 0.5}, angular: {z: 0.0}}\" -r 10"
echo ""
echo "🛑 To stop all: pkill -9 ign; pkill -9 ros2; pkill -9 rviz2"
echo ""
echo "Press Ctrl+C to stop all processes..."

# Wait for user interrupt
trap "echo 'Stopping all processes...'; kill $GAZEBO_PID $WATCHDOG_PID $NAV_PID $DETECTOR_PID $MONITOR_PID $PATROL_PID $PANEL_PID $RVIZ_PID 2>/dev/null; exit" INT TERM

wait
