#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <sensor_msgs/msg/laser_scan.hpp>
#include <std_msgs/msg/string.hpp>
#include <cmath>
#include <vector>
#include <random>
#include <sstream>
#include <iomanip>

enum class PatrolState {
  MOVING_FORWARD,
  TURNING_LEFT,
  TURNING_RIGHT,
  AVOIDING_OBSTACLE,
  EXPLORING,
  REVERSING
};

class AutonomousPatrol : public rclcpp::Node
{
public:
  AutonomousPatrol() : Node("autonomous_patrol"), 
    state_(PatrolState::MOVING_FORWARD),
    obstacle_detected_(false),
    turn_direction_(1.0),
    random_gen_(std::random_device{}())
  {
    // Publishers
    cmd_vel_pub_ = this->create_publisher<geometry_msgs::msg::Twist>("/cmd_vel", 10);
    status_pub_ = this->create_publisher<std_msgs::msg::String>("/patrol_status", 10);
    
    // Subscribers
    scan_sub_ = this->create_subscription<sensor_msgs::msg::LaserScan>(
      "/lidar", 10,
      std::bind(&AutonomousPatrol::scanCallback, this, std::placeholders::_1));
    
    odom_sub_ = this->create_subscription<nav_msgs::msg::Odometry>(
      "/odom", 10,
      std::bind(&AutonomousPatrol::odomCallback, this, std::placeholders::_1));
    
    // Parameters
    this->declare_parameter("forward_speed", 0.4);
    this->declare_parameter("turn_speed", 0.5);
    this->declare_parameter("obstacle_distance", 2.0);
    this->declare_parameter("safe_distance", 3.0);
    this->declare_parameter("patrol_enabled", true);
    
    // Control timer (20 Hz for smooth control)
    control_timer_ = this->create_wall_timer(
      std::chrono::milliseconds(50),
      std::bind(&AutonomousPatrol::controlLoop, this));
    
    // Status timer (2 Hz)
    status_timer_ = this->create_wall_timer(
      std::chrono::milliseconds(500),
      std::bind(&AutonomousPatrol::publishStatus, this));
    
    current_x_ = 0.0;
    current_y_ = 0.0;
    current_yaw_ = 0.0;
    turn_start_time_ = this->now();
    forward_start_time_ = this->now();
    exploration_cycles_ = 0;
    min_obstacle_distance_ = 100.0;
    left_clearance_ = 100.0;
    right_clearance_ = 100.0;
    
    RCLCPP_INFO(this->get_logger(), "🤖 AUTONOMOUS NAVIGATION SYSTEM - ONLINE");
    RCLCPP_INFO(this->get_logger(), "🚀 Beginning intelligent exploration with obstacle avoidance...");
    RCLCPP_INFO(this->get_logger(), "📡 Lidar active - Scanning 360° for obstacles");
  }

private:
  void odomCallback(const nav_msgs::msg::Odometry::SharedPtr msg)
  {
    current_x_ = msg->pose.pose.position.x;
    current_y_ = msg->pose.pose.position.y;
    
    // Extract yaw from quaternion
    double siny_cosp = 2 * (msg->pose.pose.orientation.w * msg->pose.pose.orientation.z +
                             msg->pose.pose.orientation.x * msg->pose.pose.orientation.y);
    double cosy_cosp = 1 - 2 * (msg->pose.pose.orientation.y * msg->pose.pose.orientation.y +
                                 msg->pose.pose.orientation.z * msg->pose.pose.orientation.z);
    current_yaw_ = std::atan2(siny_cosp, cosy_cosp);
  }
  
  void scanCallback(const sensor_msgs::msg::LaserScan::SharedPtr msg)
  {
    double obstacle_dist = this->get_parameter("obstacle_distance").as_double();
    double safe_dist = this->get_parameter("safe_distance").as_double();
    
    // Check front sector (±45 degrees)
    int front_start = msg->ranges.size() * 0.375;  // -45 deg
    int front_end = msg->ranges.size() * 0.625;    // +45 deg
    
    // Check left and right sectors
    int left_start = msg->ranges.size() * 0.625;
    int left_end = msg->ranges.size() * 0.875;
    int right_start = msg->ranges.size() * 0.125;
    int right_end = msg->ranges.size() * 0.375;
    
    double min_front_distance = 100.0;
    double min_left_distance = 100.0;
    double min_right_distance = 100.0;
    obstacle_detected_ = false;
    
    // Check front
    for (int i = front_start; i < front_end; ++i)
    {
      float range = msg->ranges[i];
      if (!std::isnan(range) && !std::isinf(range) && 
          range >= msg->range_min && range <= msg->range_max)
      {
        min_front_distance = std::min(min_front_distance, (double)range);
        if (range < obstacle_dist) obstacle_detected_ = true;
      }
    }
    
    // Check left
    for (int i = left_start; i < left_end; ++i)
    {
      float range = msg->ranges[i];
      if (!std::isnan(range) && !std::isinf(range) && 
          range >= msg->range_min && range <= msg->range_max)
      {
        min_left_distance = std::min(min_left_distance, (double)range);
      }
    }
    
    // Check right
    for (int i = right_start; i < right_end; ++i)
    {
      float range = msg->ranges[i];
      if (!std::isnan(range) && !std::isinf(range) && 
          range >= msg->range_min && range <= msg->range_max)
      {
        min_right_distance = std::min(min_right_distance, (double)range);
      }
    }
    
    min_obstacle_distance_ = min_front_distance;
    left_clearance_ = min_left_distance;
    right_clearance_ = min_right_distance;
    
    // Intelligent decision making
    if (obstacle_detected_ && state_ == PatrolState::MOVING_FORWARD)
    {
      // Choose best direction based on clearance
      if (min_left_distance > min_right_distance && min_left_distance > obstacle_dist)
      {
        state_ = PatrolState::TURNING_LEFT;
        turn_direction_ = 1.0;
        RCLCPP_WARN(this->get_logger(), "⚠️  Obstacle at %.2fm - Turning LEFT (clearance: %.2fm)", 
                    min_front_distance, min_left_distance);
      }
      else if (min_right_distance > min_left_distance && min_right_distance > obstacle_dist)
      {
        state_ = PatrolState::TURNING_RIGHT;
        turn_direction_ = -1.0;
        RCLCPP_WARN(this->get_logger(), "⚠️  Obstacle at %.2fm - Turning RIGHT (clearance: %.2fm)", 
                    min_front_distance, min_right_distance);
      }
      else
      {
        // Reverse if boxed in
        state_ = PatrolState::REVERSING;
        RCLCPP_ERROR(this->get_logger(), "🚨 BOXED IN - REVERSING!");
      }
      turn_start_time_ = this->now();
    }
    else if (!obstacle_detected_ && min_front_distance > safe_dist)
    {
      auto elapsed = (this->now() - turn_start_time_).seconds();
      
      if (state_ == PatrolState::TURNING_LEFT || 
          state_ == PatrolState::TURNING_RIGHT || 
          state_ == PatrolState::REVERSING)
      {
        if (elapsed > 1.5)  // Minimum turn time
        {
          state_ = PatrolState::MOVING_FORWARD;
          forward_start_time_ = this->now();
          exploration_cycles_++;
          RCLCPP_INFO(this->get_logger(), "✅ Path clear - MOVING FORWARD (cycle #%d)", exploration_cycles_);
        }
      }
      else if (state_ == PatrolState::EXPLORING)
      {
        // Occasionally switch back to straight movement
        std::uniform_real_distribution<> dis(0.0, 1.0);
        if (dis(random_gen_) < 0.05)  // 5% chance per update
        {
          state_ = PatrolState::MOVING_FORWARD;
          RCLCPP_INFO(this->get_logger(), "🔄 Switching to straight path");
        }
      }
    }
    
    // Periodically add exploration behavior
    auto forward_time = (this->now() - forward_start_time_).seconds();
    if (state_ == PatrolState::MOVING_FORWARD && forward_time > 8.0)
    {
      std::uniform_real_distribution<> dis(0.0, 1.0);
      if (dis(random_gen_) < 0.3)  // 30% chance to explore
      {
        state_ = PatrolState::EXPLORING;
        RCLCPP_INFO(this->get_logger(), "🔍 Entering exploration mode");
      }
      forward_start_time_ = this->now();
    }
  }
  
  void controlLoop()
  {
    if (!this->get_parameter("patrol_enabled").as_bool())
    {
      stopRover();
      return;
    }
    
    geometry_msgs::msg::Twist cmd_vel;
    double forward_speed = this->get_parameter("forward_speed").as_double();
    double turn_speed = this->get_parameter("turn_speed").as_double();
    
    switch (state_)
    {
      case PatrolState::MOVING_FORWARD:
        cmd_vel.linear.x = forward_speed;
        cmd_vel.angular.z = 0.0;
        break;
        
      case PatrolState::TURNING_LEFT:
        cmd_vel.linear.x = 0.0;
        cmd_vel.angular.z = turn_speed;
        break;
        
      case PatrolState::TURNING_RIGHT:
        cmd_vel.linear.x = 0.0;
        cmd_vel.angular.z = -turn_speed;
        break;
        
      case PatrolState::AVOIDING_OBSTACLE:
        cmd_vel.linear.x = 0.0;
        cmd_vel.angular.z = turn_direction_ * turn_speed;
        break;
        
      case PatrolState::EXPLORING:
        cmd_vel.linear.x = forward_speed * 0.7;
        cmd_vel.angular.z = 0.15;  // Gentle curve for exploration
        break;
        
      case PatrolState::REVERSING:
        cmd_vel.linear.x = -forward_speed * 0.5;
        cmd_vel.angular.z = turn_speed * 0.5;  // Turn while reversing
        
        // Auto-recover after 2 seconds
        auto elapsed = (this->now() - turn_start_time_).seconds();
        if (elapsed > 2.0)
        {
          state_ = PatrolState::TURNING_LEFT;
          turn_start_time_ = this->now();
        }
        break;
    }
    
    cmd_vel_pub_->publish(cmd_vel);
  }
  
  void stopRover()
  {
    geometry_msgs::msg::Twist cmd_vel;
    cmd_vel.linear.x = 0.0;
    cmd_vel.angular.z = 0.0;
    cmd_vel_pub_->publish(cmd_vel);
  }
  
  void publishStatus()
  {
    std::stringstream ss;
    ss << "\n╔═══════════════════════════════════════════╗\n";
    ss << "║   AUTONOMOUS NAVIGATION STATUS REPORT    ║\n";
    ss << "╚═══════════════════════════════════════════╝\n";
    ss << "📍 Position: (" << std::fixed << std::setprecision(2) 
       << current_x_ << ", " << current_y_ << ") m\n";
    ss << "🧭 Heading: " << std::setprecision(1) << (current_yaw_ * 180.0 / M_PI) << "°\n";
    ss << "🔄 Exploration Cycles: " << exploration_cycles_ << "\n\n";
    
    ss << "🤖 Current State: ";
    switch (state_)
    {
      case PatrolState::MOVING_FORWARD:
        ss << "🚀 MOVING FORWARD\n";
        break;
      case PatrolState::TURNING_LEFT:
        ss << "↰ TURNING LEFT\n";
        break;
      case PatrolState::TURNING_RIGHT:
        ss << "↱ TURNING RIGHT\n";
        break;
      case PatrolState::AVOIDING_OBSTACLE:
        ss << "⚠️  AVOIDING OBSTACLE\n";
        break;
      case PatrolState::EXPLORING:
        ss << "🔍 EXPLORING (Curved Path)\n";
        break;
      case PatrolState::REVERSING:
        ss << "⬅️  REVERSING (Recovery)\n";
        break;
    }
    
    ss << "\n📡 Lidar Scan:\n";
    ss << "   Front:  " << std::setprecision(2) << min_obstacle_distance_ << " m ";
    if (obstacle_detected_)
      ss << "⚠️  [OBSTACLE DETECTED]\n";
    else
      ss << "✅ [CLEAR]\n";
    
    ss << "   Left:   " << left_clearance_ << " m\n";
    ss << "   Right:  " << right_clearance_ << " m\n";
    ss << "─────────────────────────────────────────────\n";
    
    std_msgs::msg::String msg;
    msg.data = ss.str();
    status_pub_->publish(msg);
  }
  
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_pub_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr status_pub_;
  rclcpp::Subscription<sensor_msgs::msg::LaserScan>::SharedPtr scan_sub_;
  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_sub_;
  rclcpp::TimerBase::SharedPtr control_timer_;
  rclcpp::TimerBase::SharedPtr status_timer_;
  
  PatrolState state_;
  bool obstacle_detected_;
  double min_obstacle_distance_;
  double left_clearance_;
  double right_clearance_;
  double turn_direction_;
  double current_x_, current_y_, current_yaw_;
  rclcpp::Time turn_start_time_;
  rclcpp::Time forward_start_time_;
  int exploration_cycles_;
  std::mt19937 random_gen_;
};

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<AutonomousPatrol>());
  rclcpp::shutdown();
  return 0;
}
