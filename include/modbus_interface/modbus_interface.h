/*
 * @Descripttion: 
 * @version: 
 * @Author: wzy
 * @Date: 2021-06-02 10:12:49
 * @LastEditors: wzy
 * @LastEditTime: 2021-06-02 11:08:23
 */
/*
 * Author: Jiayuan Sun
 * Copyright: 2019 Shanghai Yikun Electrical Engineering Co，Ltd
 */
#ifndef MODBUS_INTERFACE_H
#define MODBUS_INTERFACE_H
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ros/ros.h>
#include "yikun_msgs/ModbusCmd.h"
#include "std_msgs/String.h"
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/thread.hpp>

#define DIGITAL_INPUTS 7
#define DIGITAL_OUTPUTS 7
#define MAXSIZE 225
namespace modbus_interface
{

  class ModbusInterafce
  {
  public:
    typedef boost::shared_ptr<modbus_interface::ModbusInterafce> Ptr;
    ModbusInterafce(ros::NodeHandle &nh, const std::string name);
    ~ModbusInterafce();

    bool modbusConnect(const std::string addr = "", const std::string port = "");
    void modbusDisconnect();
    int getDigitalInput(const int number);
    int getDigitalOutput(const int number);
    int setDigitalOutput(const int number, const int value);
    int setDigitalOutputs(const int outputs);
    int waitForDigitalInput(const int number, const int value, const float duration = 1.0);

    std::string getIpAddr();
    std_msgs::String flag_str;
    bool isConnected();
    void publishSignal();

  private:
    bool command_callback(yikun_msgs::ModbusCmdRequest &req, yikun_msgs::ModbusCmdResponse &res);

    ros::NodeHandle nh_;
    std::string ip_addr_, port_;
    struct sockaddr_in serv_addr_;
    int socket_;
    bool isconnected_;
    int timesec_;
    struct timeval timeout_;
    boost::mutex mutex_in_, mutex_out_;
    ros::ServiceServer command_server_;
    ros::Publisher flag_pub;
  };

} // namespace modbus_interface

#endif // MODBUS_INTERFACE_H
