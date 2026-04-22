#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <visualization_msgs/msg/marker_array.hpp>
#include <nav_msgs/msg/path.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <iostream>
#include <iomanip>
#include <vector>
#include <sstream>

struct DetectedObject {
  std::string name;
  std::string type;
  double distance;
  double angle;
  std::string timestamp;
};

class HQControlPanel : public rclcpp::Node
{
public:
  HQControlPanel() : Node("hq_control_panel"), object_count_(0), total_distance_(0.0)
  {
    // Subscribers for all telemetry
    object_info_sub_ = this->create_subscription<std_msgs::msg::String>(
      "/object_information", 10,
      std::bind(&HQControlPanel::objectInfoCallback, this, std::placeholders::_1));
    
    env_status_sub_ = this->create_subscription<std_msgs::msg::String>(
      "/habitat/environmental_status", 10,
      std::bind(&HQControlPanel::envStatusCallback, this, std::placeholders::_1));
    
    nav_telemetry_sub_ = this->create_subscription<std_msgs::msg::String>(
      "/navigation_telemetry", 10,
      std::bind(&HQControlPanel::navTelemetryCallback, this, std::placeholders::_1));
    
    patrol_status_sub_ = this->create_subscription<std_msgs::msg::String>(
      "/patrol_status", 10,
      std::bind(&HQControlPanel::patrolStatusCallback, this, std::placeholders::_1));
    
    hq_telemetry_sub_ = this->create_subscription<std_msgs::msg::String>(
      "/hq_telemetry", 10,
      std::bind(&HQControlPanel::hqTelemetryCallback, this, std::placeholders::_1));
    
    alerts_sub_ = this->create_subscription<std_msgs::msg::String>(
      "/habitat/alerts", 10,
      std::bind(&HQControlPanel::alertsCallback, this, std::placeholders::_1));
    
    odom_sub_ = this->create_subscription<nav_msgs::msg::Odometry>(
      "/odom", 10,
      std::bind(&HQControlPanel::odomCallback, this, std::placeholders::_1));
    
    path_sub_ = this->create_subscription<nav_msgs::msg::Path>(
      "/rover_path", 10,
      std::bind(&HQControlPanel::pathCallback, this, std::placeholders::_1));
    
    // Timer to refresh display
    display_timer_ = this->create_wall_timer(
      std::chrono::seconds(3),
      std::bind(&HQControlPanel::displayDashboard, this));
    
    RCLCPP_INFO(this->get_logger(), "🖥️  HQ CONTROL PANEL ONLINE");
    RCLCPP_INFO(this->get_logger(), "📡 Monitoring: Navigation | Objects | Environment | Path");
    
    last_x_ = 0.0;
    last_y_ = 0.0;
    displayHeader();
  }

private:
  void displayHeader()
  {
    std::cout << "\033[2J\033[1;1H";  // Clear screen
    std::cout << "╔══════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║    🌙 LUNAR HABITAT MAINTENANCE ROBOT - HQ CONTROL PANEL 🖥️             ║\n";
    std::cout << "║           ISRO Mission Control - Live Telemetry Dashboard                ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════════════╝\n";
    std::cout << std::endl;
  }
  
  void odomCallback(const nav_msgs::msg::Odometry::SharedPtr msg)
  {
    double dx = msg->pose.pose.position.x - last_x_;
    double dy = msg->pose.pose.position.y - last_y_;
    double distance = std::sqrt(dx*dx + dy*dy);
    
    if (distance > 0.01)  // Only count significant movement
    {
      total_distance_ += distance;
      last_x_ = msg->pose.pose.position.x;
      last_y_ = msg->pose.pose.position.y;
    }
  }
  
  void pathCallback(const nav_msgs::msg::Path::SharedPtr msg)
  {
    path_length_ = msg->poses.size();
  }
  
  void objectInfoCallback(const std_msgs::msg::String::SharedPtr msg)
  {
    latest_object_info_ = msg->data;
    
    // Parse detected objects
    detected_objects_.clear();
    std::stringstream ss(latest_object_info_);
    std::string line;
    DetectedObject current_obj;
    
    while (std::getline(ss, line))
    {
      if (line.find("Object:") != std::string::npos)
      {
        size_t pos = line.find("Object:");
        current_obj.name = line.substr(pos + 8);
        current_obj.name.erase(0, current_obj.name.find_first_not_of(" "));
      }
      else if (line.find("Type:") != std::string::npos)
      {
        size_t pos = line.find("Type:");
        current_obj.type = line.substr(pos + 6);
        current_obj.type.erase(0, current_obj.type.find_first_not_of(" "));
      }
      else if (line.find("Distance:") != std::string::npos)
      {
        size_t pos = line.find("Distance:");
        std::string dist_str = line.substr(pos + 10);
        try {
          current_obj.distance = std::stod(dist_str);
        } catch (...) {
          current_obj.distance = 0.0;
        }
      }
      else if (line.find("Angle:") != std::string::npos)
      {
        size_t pos = line.find("Angle:");
        std::string angle_str = line.substr(pos + 7);
        try {
          current_obj.angle = std::stod(angle_str);
        } catch (...) {
          current_obj.angle = 0.0;
        }
        
        // Object info complete
        if (!current_obj.name.empty())
        {
          current_obj.timestamp = getCurrentTimestamp();
          detected_objects_.push_back(current_obj);
        }
        current_obj = DetectedObject();  // Reset
      }
    }
    
    if (detected_objects_.size() != object_count_)
    {
      object_count_ = detected_objects_.size();
      RCLCPP_INFO(this->get_logger(), "📦 OBJECT UPDATE: %zu objects in database", object_count_);
    }
  }
  
  std::string getCurrentTimestamp()
  {
    auto now = this->now();
    int total_seconds = static_cast<int>(now.seconds());
    int seconds = total_seconds % 60;
    int minutes = (total_seconds / 60) % 60;
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(2) << minutes << ":" 
       << std::setw(2) << seconds;
    return ss.str();
  }
  
  void envStatusCallback(const std_msgs::msg::String::SharedPtr msg)
  {
    latest_env_status_ = msg->data;
  }
  
  void navTelemetryCallback(const std_msgs::msg::String::SharedPtr msg)
  {
    latest_nav_telemetry_ = msg->data;
  }
  
  void patrolStatusCallback(const std_msgs::msg::String::SharedPtr msg)
  {
    latest_patrol_status_ = msg->data;
  }
  
  void hqTelemetryCallback(const std_msgs::msg::String::SharedPtr msg)
  {
    latest_hq_telemetry_ = msg->data;
  }
  
  void alertsCallback(const std_msgs::msg::String::SharedPtr msg)
  {
    std::cout << "\n🚨 ALERT RECEIVED:\n";
    std::cout << "════════════════════════════════════════\n";
    std::cout << msg->data << "\n";
    std::cout << "════════════════════════════════════════\n" << std::endl;
    
    RCLCPP_WARN(this->get_logger(), "🚨 ALERT: %s", msg->data.c_str());
  }
  
  void displayDashboard()
  {
    std::cout << "\033[2J\033[1;1H";  // Clear screen
    displayHeader();
    
    auto now = this->now();
    int mission_seconds = now.seconds();
    int hours = mission_seconds / 3600;
    int minutes = (mission_seconds % 3600) / 60;
    int seconds = mission_seconds % 60;
    
    std::cout << "📅 Mission Time: " << std::setfill('0') << std::setw(2) << hours << ":"
              << std::setw(2) << minutes << ":" << std::setw(2) << seconds << "\n";
    std::cout << "📏 Total Distance Traveled: " << std::fixed << std::setprecision(2) 
              << total_distance_ << " m\n";
    std::cout << "📍 Path Points Recorded: " << path_length_ << "\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════\n\n";
    
    // Display patrol status
    if (!latest_patrol_status_.empty())
    {
      std::cout << latest_patrol_status_ << "\n";
    }
    
    // Display detected objects in a table
    if (!detected_objects_.empty())
    {
      std::cout << "\n╔═══════════════════════════════════════════════════════════════════════╗\n";
      std::cout << "║              🎯 DETECTED OBJECTS DATABASE (HQ RECORDS)               ║\n";
      std::cout << "╚═══════════════════════════════════════════════════════════════════════╝\n";
      std::cout << "┌──────┬──────────────────┬──────────────┬──────────┬─────────┬─────────┐\n";
      std::cout << "│ # │ Object Name      │ Type         │ Distance │ Angle   │ Time    │\n";
      std::cout << "├──────┼──────────────────┼──────────────┼──────────┼─────────┼─────────┤\n";
      
      size_t max_display = std::min((size_t)15, detected_objects_.size());
      for (size_t i = 0; i < max_display; ++i)
      {
        const auto& obj = detected_objects_[i];
        
        // Determine icon based on type
        std::string icon = "📦";
        if (obj.type.find("ROCK") != std::string::npos) icon = "🪨";
        else if (obj.type.find("BOULDER") != std::string::npos) icon = "🗿";
        else if (obj.type.find("STRUCTURE") != std::string::npos) icon = "🏗️";
        
        std::cout << "│ " << std::setw(2) << (i+1) << " │ "
                  << icon << " " << std::left << std::setw(13) << obj.name.substr(0, 13) << " │ "
                  << std::setw(12) << obj.type.substr(0, 12) << " │ "
                  << std::right << std::setw(6) << std::fixed << std::setprecision(2) << obj.distance << "m │ "
                  << std::setw(6) << std::setprecision(0) << obj.angle << "° │ "
                  << obj.timestamp << " │\n";
      }
      
      std::cout << "└──────┴──────────────────┴──────────────┴──────────┴─────────┴─────────┘\n";
      
      if (detected_objects_.size() > max_display)
      {
        std::cout << "   ... and " << (detected_objects_.size() - max_display) << " more objects\n";
      }
      
      std::cout << "\n💾 Total Objects Catalogued: " << detected_objects_.size() << "\n";
      std::cout << "📡 All object data transmitted to HQ successfully!\n";
    }
    else
    {
      std::cout << "\n📡 Scanning for objects... No objects detected yet.\n";
    }
    
    // Environmental status
    if (!latest_env_status_.empty())
    {
      std::cout << "\n╔═══════════════════════════════════════════════════════════════════════╗\n";
      std::cout << "║                🌡️  ENVIRONMENTAL MONITORING                           ║\n";
      std::cout << "╚═══════════════════════════════════════════════════════════════════════╝\n";
      
      std::stringstream ss(latest_env_status_);
      std::string line;
      while (std::getline(ss, line))
      {
        if (!line.empty() && line.find("===") == std::string::npos)
        {
          std::cout << line << "\n";
        }
      }
    }
    
    std::cout << "\n═══════════════════════════════════════════════════════════════════════════\n";
    std::cout << "🔄 Dashboard auto-refreshes every 3 seconds | Press Ctrl+C to stop\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════\n";
  }
    
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr object_info_sub_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr env_status_sub_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr nav_telemetry_sub_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr patrol_status_sub_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr hq_telemetry_sub_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr alerts_sub_;
  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_sub_;
  rclcpp::Subscription<nav_msgs::msg::Path>::SharedPtr path_sub_;
  rclcpp::TimerBase::SharedPtr display_timer_;
  
  std::string latest_object_info_;
  std::string latest_env_status_;
  std::string latest_nav_telemetry_;
  std::string latest_patrol_status_;
  std::string latest_hq_telemetry_;
  std::vector<DetectedObject> detected_objects_;
  size_t object_count_;
  size_t path_length_;
  double total_distance_;
  double last_x_, last_y_;
};

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<HQControlPanel>());
  rclcpp::shutdown();
  return 0;
}
