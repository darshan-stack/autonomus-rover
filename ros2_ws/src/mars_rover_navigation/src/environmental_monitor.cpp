#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <sensor_msgs/msg/temperature.hpp>
#include <sensor_msgs/msg/fluid_pressure.hpp>
#include <std_msgs/msg/float32.hpp>
#include <random>
#include <sstream>
#include <iomanip>

class EnvironmentalMonitor : public rclcpp::Node
{
public:
  EnvironmentalMonitor() : Node("environmental_monitor"), gen_(rd_())
  {
    // Publishers for individual sensors
    temp_pub_ = this->create_publisher<sensor_msgs::msg::Temperature>(
      "/habitat/temperature", 10);
    
    pressure_pub_ = this->create_publisher<sensor_msgs::msg::FluidPressure>(
      "/habitat/pressure", 10);
    
    o2_pub_ = this->create_publisher<std_msgs::msg::Float32>(
      "/habitat/oxygen_level", 10);
    
    co2_pub_ = this->create_publisher<std_msgs::msg::Float32>(
      "/habitat/co2_level", 10);
    
    humidity_pub_ = this->create_publisher<std_msgs::msg::Float32>(
      "/habitat/humidity", 10);
    
    radiation_pub_ = this->create_publisher<std_msgs::msg::Float32>(
      "/habitat/radiation", 10);
    
    // Combined status publisher
    status_pub_ = this->create_publisher<std_msgs::msg::String>(
      "/habitat/environmental_status", 10);
    
    alert_pub_ = this->create_publisher<std_msgs::msg::String>(
      "/habitat/alerts", 10);
    
    // Parameters - nominal values
    this->declare_parameter("nominal_temperature", 22.0);  // Celsius
    this->declare_parameter("nominal_pressure", 101.3);     // kPa
    this->declare_parameter("nominal_o2", 21.0);            // %
    this->declare_parameter("nominal_co2", 0.04);           // %
    this->declare_parameter("nominal_humidity", 45.0);      // %
    this->declare_parameter("nominal_radiation", 0.5);      // μSv/h
    
    // Alert thresholds
    this->declare_parameter("temp_low_threshold", 18.0);
    this->declare_parameter("temp_high_threshold", 26.0);
    this->declare_parameter("o2_low_threshold", 19.5);
    this->declare_parameter("co2_high_threshold", 0.5);
    this->declare_parameter("pressure_low_threshold", 95.0);
    this->declare_parameter("radiation_high_threshold", 2.0);
    
    // Timer for monitoring (1 Hz)
    monitor_timer_ = this->create_wall_timer(
      std::chrono::seconds(1),
      std::bind(&EnvironmentalMonitor::monitorCallback, this));
    
    RCLCPP_INFO(this->get_logger(), "Environmental Monitor Node Started");
    RCLCPP_INFO(this->get_logger(), "Monitoring habitat environmental parameters...");
  }

private:
  void monitorCallback()
  {
    // Get nominal values
    double nom_temp = this->get_parameter("nominal_temperature").as_double();
    double nom_pressure = this->get_parameter("nominal_pressure").as_double();
    double nom_o2 = this->get_parameter("nominal_o2").as_double();
    double nom_co2 = this->get_parameter("nominal_co2").as_double();
    double nom_humidity = this->get_parameter("nominal_humidity").as_double();
    double nom_radiation = this->get_parameter("nominal_radiation").as_double();
    
    // Generate simulated sensor readings with small variations
    std::normal_distribution<> temp_dist(nom_temp, 0.5);
    std::normal_distribution<> pressure_dist(nom_pressure, 0.5);
    std::normal_distribution<> o2_dist(nom_o2, 0.3);
    std::normal_distribution<> co2_dist(nom_co2, 0.01);
    std::normal_distribution<> humidity_dist(nom_humidity, 2.0);
    std::normal_distribution<> radiation_dist(nom_radiation, 0.1);
    
    double temperature = temp_dist(gen_);
    double pressure = pressure_dist(gen_);
    double o2_level = o2_dist(gen_);
    double co2_level = co2_dist(gen_);
    double humidity = humidity_dist(gen_);
    double radiation = radiation_dist(gen_);
    
    // Publish individual readings
    auto now = this->now();
    
    sensor_msgs::msg::Temperature temp_msg;
    temp_msg.header.stamp = now;
    temp_msg.temperature = temperature;
    temp_msg.variance = 0.25;
    temp_pub_->publish(temp_msg);
    
    sensor_msgs::msg::FluidPressure pressure_msg;
    pressure_msg.header.stamp = now;
    pressure_msg.fluid_pressure = pressure * 1000.0;  // Convert kPa to Pa
    pressure_msg.variance = 250.0;
    pressure_pub_->publish(pressure_msg);
    
    std_msgs::msg::Float32 o2_msg;
    o2_msg.data = o2_level;
    o2_pub_->publish(o2_msg);
    
    std_msgs::msg::Float32 co2_msg;
    co2_msg.data = co2_level;
    co2_pub_->publish(co2_msg);
    
    std_msgs::msg::Float32 humidity_msg;
    humidity_msg.data = humidity;
    humidity_pub_->publish(humidity_msg);
    
    std_msgs::msg::Float32 radiation_msg;
    radiation_msg.data = radiation;
    radiation_pub_->publish(radiation_msg);
    
    // Check thresholds and generate alerts
    std::vector<std::string> alerts;
    checkThresholds(temperature, pressure, o2_level, co2_level, radiation, alerts);
    
    // Publish combined status
    publishStatus(temperature, pressure, o2_level, co2_level, humidity, radiation, alerts);
    
    // Publish alerts if any
    if (!alerts.empty())
    {
      publishAlerts(alerts);
    }
  }
  
  void checkThresholds(double temp, double pressure, double o2, double co2, 
                      double radiation, std::vector<std::string>& alerts)
  {
    double temp_low = this->get_parameter("temp_low_threshold").as_double();
    double temp_high = this->get_parameter("temp_high_threshold").as_double();
    double o2_low = this->get_parameter("o2_low_threshold").as_double();
    double co2_high = this->get_parameter("co2_high_threshold").as_double();
    double pressure_low = this->get_parameter("pressure_low_threshold").as_double();
    double radiation_high = this->get_parameter("radiation_high_threshold").as_double();
    
    if (temp < temp_low)
      alerts.push_back("WARNING: Temperature below threshold (" + 
                      std::to_string(temp) + "°C)");
    if (temp > temp_high)
      alerts.push_back("WARNING: Temperature above threshold (" + 
                      std::to_string(temp) + "°C)");
    if (o2 < o2_low)
      alerts.push_back("CRITICAL: Oxygen level low (" + std::to_string(o2) + "%)");
    if (co2 > co2_high)
      alerts.push_back("WARNING: CO2 level elevated (" + std::to_string(co2) + "%)");
    if (pressure < pressure_low)
      alerts.push_back("CRITICAL: Pressure drop detected (" + 
                      std::to_string(pressure) + " kPa)");
    if (radiation > radiation_high)
      alerts.push_back("WARNING: Elevated radiation (" + 
                      std::to_string(radiation) + " μSv/h)");
  }
  
  void publishStatus(double temp, double pressure, double o2, double co2,
                    double humidity, double radiation, 
                    const std::vector<std::string>& alerts)
  {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    
    ss << "=== HABITAT ENVIRONMENTAL STATUS ===" << "\n";
    ss << "Timestamp: " << this->now().seconds() << "\n\n";
    
    ss << "Temperature:    " << temp << " °C";
    if (alerts.size() > 0 && alerts[0].find("Temperature") != std::string::npos)
      ss << " [ALERT]";
    ss << "\n";
    
    ss << "Pressure:       " << pressure << " kPa";
    if (std::any_of(alerts.begin(), alerts.end(), 
        [](const std::string& s) { return s.find("Pressure") != std::string::npos; }))
      ss << " [ALERT]";
    ss << "\n";
    
    ss << "Oxygen (O2):    " << o2 << " %";
    if (std::any_of(alerts.begin(), alerts.end(), 
        [](const std::string& s) { return s.find("Oxygen") != std::string::npos; }))
      ss << " [ALERT]";
    ss << "\n";
    
    ss << "Carbon Dioxide: " << co2 << " %";
    if (std::any_of(alerts.begin(), alerts.end(), 
        [](const std::string& s) { return s.find("CO2") != std::string::npos; }))
      ss << " [ALERT]";
    ss << "\n";
    
    ss << "Humidity:       " << humidity << " %" << "\n";
    ss << "Radiation:      " << radiation << " μSv/h";
    if (std::any_of(alerts.begin(), alerts.end(), 
        [](const std::string& s) { return s.find("radiation") != std::string::npos; }))
      ss << " [ALERT]";
    ss << "\n\n";
    
    if (alerts.empty())
    {
      ss << "Status: ALL SYSTEMS NOMINAL" << "\n";
    }
    else
    {
      ss << "Status: ALERTS ACTIVE (" << alerts.size() << ")" << "\n";
    }
    
    std_msgs::msg::String msg;
    msg.data = ss.str();
    status_pub_->publish(msg);
  }
  
  void publishAlerts(const std::vector<std::string>& alerts)
  {
    std::stringstream ss;
    ss << "=== HABITAT ALERTS ===" << "\n";
    ss << "Time: " << this->now().seconds() << "\n\n";
    
    for (const auto& alert : alerts)
    {
      ss << "• " << alert << "\n";
      RCLCPP_WARN(this->get_logger(), "%s", alert.c_str());
    }
    
    std_msgs::msg::String msg;
    msg.data = ss.str();
    alert_pub_->publish(msg);
  }
  
  rclcpp::Publisher<sensor_msgs::msg::Temperature>::SharedPtr temp_pub_;
  rclcpp::Publisher<sensor_msgs::msg::FluidPressure>::SharedPtr pressure_pub_;
  rclcpp::Publisher<std_msgs::msg::Float32>::SharedPtr o2_pub_;
  rclcpp::Publisher<std_msgs::msg::Float32>::SharedPtr co2_pub_;
  rclcpp::Publisher<std_msgs::msg::Float32>::SharedPtr humidity_pub_;
  rclcpp::Publisher<std_msgs::msg::Float32>::SharedPtr radiation_pub_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr status_pub_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr alert_pub_;
  rclcpp::TimerBase::SharedPtr monitor_timer_;
  
  std::random_device rd_;
  std::mt19937 gen_;
};

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<EnvironmentalMonitor>());
  rclcpp::shutdown();
  return 0;
}
