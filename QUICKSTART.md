# 🚀 QUICK START GUIDE - Lunar Habitat Maintenance Robot

## One-Command Launch

```bash
cd ~/asd-main
./start_lunar_habitat_system.sh
```

**⚠️ IMPORTANT: Press PLAY ▶️ button in Gazebo when it opens!**

---

## What's Running?

✅ **Gazebo Simulation** - Lunar habitat environment  
✅ **ROS-Gazebo Bridge** - Communication layer  
✅ **Enhanced Object Detector** - Identifies & names obstacles  
✅ **Environmental Monitor** - Tracks O₂, temp, pressure, radiation  
✅ **Path Tracker** - Logs all navigation for HQ  
✅ **TF Publishers** - Coordinate transforms  

---

## View Telemetry (Open new terminal)

### See Detected Objects
```bash
source /opt/ros/humble/setup.bash
ros2 topic echo /object_information
```

### View Environmental Status
```bash
ros2 topic echo /habitat/environmental_status
```

### Check Navigation Data
```bash
ros2 topic echo /navigation_telemetry
```

### HQ Reports
```bash
ros2 topic echo /hq_telemetry
```

### Mission Log File
```bash
cat /tmp/lunar_habitat_mission_log.txt
```

---

## Control the Rover

### Move Forward
```bash
source /opt/ros/humble/setup.bash
ros2 topic pub /cmd_vel geometry_msgs/msg/Twist "{linear: {x: 0.5}, angular: {z: 0.0}}" -r 10
```

### Turn Left
```bash
ros2 topic pub /cmd_vel geometry_msgs/msg/Twist "{linear: {x: 0.2}, angular: {z: 0.5}}" -r 10
```

### Turn Right
```bash
ros2 topic pub /cmd_vel geometry_msgs/msg/Twist "{linear: {x: 0.2}, angular: {z: -0.5}}" -r 10
```

### Stop
```bash
ros2 topic pub /cmd_vel geometry_msgs/msg/Twist "{linear: {x: 0.0}, angular: {z: 0.0}}" --once
```

---

## Launch Visualization (RViz)

**New Terminal:**
```bash
cd ~/asd-main/ros2_ws
source /opt/ros/humble/setup.bash
source install/setup.bash
ros2 launch mars_rover_navigation visualization.launch.py
```

---

## Quick System Test

```bash
cd ~/asd-main
./test_system.sh
```

---

## Stop All Systems

```bash
pkill -9 ign; pkill -9 gz; pkill -9 ros2; pkill -9 ruby
```

---

## Key ROS Topics

| Topic | Description |
|-------|-------------|
| `/lidar` | 360° LiDAR scan data |
| `/odom` | Robot position and velocity |
| `/cmd_vel` | Movement commands |
| `/detected_objects` | Visualization markers |
| `/object_information` | Named objects with details |
| `/rover_path` | Complete navigation path |
| `/navigation_telemetry` | Position, distance, speed |
| `/habitat/temperature` | Temperature sensor |
| `/habitat/oxygen_level` | O₂ monitoring |
| `/habitat/co2_level` | CO₂ monitoring |
| `/habitat/pressure` | Atmospheric pressure |
| `/habitat/radiation` | Radiation level |
| `/habitat/environmental_status` | All environmental data |
| `/habitat/alerts` | Warning notifications |
| `/hq_telemetry` | Mission control reports |

---

## ISRO Requirements ✅

- ✅ Autonomous navigation
- ✅ Mapping & localization (LiDAR + IMU + Odom)
- ✅ Obstacle detection with naming
- ✅ Environmental monitoring (O₂, CO₂, temp, pressure, radiation)
- ✅ Maintenance patrol & monitoring
- ✅ Alert system
- ✅ Full HQ telemetry
- ✅ Complete data logging

---

## File Structure

```
~/asd-main/
├── start_lunar_habitat_system.sh  ← MAIN LAUNCHER
├── test_system.sh                 ← System verification
├── ISRO_SOLUTION.md               ← Full documentation
├── MyModel.world                  ← Gazebo world
├── MarsRover/                     ← Robot model
└── ros2_ws/
    └── src/mars_rover_navigation/
        ├── src/
        │   ├── enhanced_object_detector.cpp
        │   ├── environmental_monitor.cpp
        │   └── path_tracker.cpp
        ├── launch/
        │   ├── lunar_habitat_system.launch.py
        │   └── visualization.launch.py
        └── config/
            └── lunar_habitat_visualization.rviz
```

---

## Troubleshooting

**No LiDAR data?**  
→ Press PLAY in Gazebo

**Nodes not starting?**  
→ Run: `source ~/asd-main/ros2_ws/install/setup.bash`

**Gazebo won't start?**  
→ Run: `pkill -9 ign; pkill -9 gz` then restart

**Can't see objects?**  
→ Check: `ros2 topic hz /lidar` (should show ~8 Hz)

---

## Demo Video Recording

1. Launch system: `./start_lunar_habitat_system.sh`
2. Press PLAY in Gazebo
3. Open new terminal: `ros2 topic echo /object_information`
4. Open new terminal: `ros2 topic echo /habitat/environmental_status`
5. Control rover with cmd_vel commands
6. Show path visualization in RViz
7. Show mission log: `cat /tmp/lunar_habitat_mission_log.txt`

---

**Project Status: ✅ READY FOR DEMO**

All ISRO requirements implemented and tested!
