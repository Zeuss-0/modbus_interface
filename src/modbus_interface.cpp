/*
 * Author: Jiayuan Sun
 * Copyright: 2019 Shanghai Yikun Electrical Engineering Co，Ltd
 */
#include "modbus_interface/modbus_interface.h"
namespace modbus_interface
{
  ModbusInterafce::ModbusInterafce(ros::NodeHandle &nh, const std::string name)
      : ip_addr_("192.168.1.71"),
        port_("502"),
        socket_(-1),
        isconnected_(false),
        nh_(nh)
  {
    ros::NodeHandle private_nh("~");
    private_nh.param("ip_addr", ip_addr_, std::string("192.168.1.71"));
    private_nh.param("port", port_, std::string("502"));
    private_nh.param("timesec", timesec_, 60);
    flag_pub = nh_.advertise<std_msgs::String>("/obsignal", 10);
    std::string service_name;
    service_name.append("/");
    service_name.append(name);
    service_name.append("/Command");
    command_server_ = private_nh.advertiseService(service_name, &ModbusInterafce::command_callback, this);

    timeout_.tv_sec = timesec_;
    timeout_.tv_usec = 0;
    flag_str.data = "0";

    ROS_INFO("Modbus Interface %s(%s)", ip_addr_.data(), name.data());
  }

  ModbusInterafce::~ModbusInterafce()
  {
    if (isconnected_)
    {
      close(socket_);
      socket_ = -1;
    }
  }

  bool ModbusInterafce::command_callback(yikun_msgs::ModbusCmdRequest &req, yikun_msgs::ModbusCmdResponse &res)
  {
    switch (req.cmd)
    {
    case 1:
      res.result = this->getDigitalInput(req.number);
      break;
    case 2:
      res.result = this->setDigitalOutput(req.number, req.value);
      break;
    case 3:
      res.result = this->waitForDigitalInput(req.number, req.value, req.duration);
      break;
    case 4:
      res.result = this->getDigitalOutput(req.number);
      break;
    default:
      res.result = -1;
      break;
    }
    return true;
  }

  bool ModbusInterafce::modbusConnect(const std::string addr, const std::string port)
  {
    if (addr != "" && port != "")
    {
      std::cout << "new addr" << std::endl;
      ip_addr_ = addr;
      port_ = port;
    }
    socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //PF_INET
    if (socket_ == -1)
    {
      exit(1);
    }
    //timeout
    if (setsockopt(socket_, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout_, sizeof(timeout_)) == -1)
      exit(1);
    if (setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout_, sizeof(timeout_)) == -1)
      exit(1);

    memset(&serv_addr_, 0, sizeof(serv_addr_));
    serv_addr_.sin_family = AF_INET;
    serv_addr_.sin_addr.s_addr = inet_addr(ip_addr_.data());
    serv_addr_.sin_port = htons(atoi(port_.data()));

    if (connect(socket_, (struct sockaddr *)&serv_addr_, sizeof(serv_addr_)) == -1)
    {
      return false;
    }
    else
    {
      isconnected_ = true;
      return true;
    }
  }

  void ModbusInterafce::modbusDisconnect()
  {
    close(socket_);
    socket_ = -1;
    isconnected_ = false;
  }

  int ModbusInterafce::getDigitalInput(const int number)
  {
    boost::mutex::scoped_lock lock(mutex_in_);
    char tmp[100];
    memset(tmp, 0, sizeof(tmp));
    if (!isconnected_)
    {
      ROS_ERROR("ModBus has not connected,please call modbusConnect function first");
      return -1;
    }
    //00 01 00 00 00 06 FF 02 00 C8 00 04
    unsigned char ctrDO[12] = {0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0xFF, 0x02, 0x00, 0xC8, 0x00, 0x04};
    std::cout << ctrDO << std::endl;
    recv(socket_, tmp, sizeof(tmp), 1);
    if (send(socket_, ctrDO, 12, 0) == -1 && errno == EAGAIN)
    {
      return -1;
    }
    if (recv(socket_, tmp, 10, 0) == -1 && errno == EAGAIN)
    {
      ROS_ERROR("[Modbus] Recv error");
      return -1;
    }
    for (int i = 0; i < 10; i++)
    {
      printf("%02X ", tmp[i]);
    }
    printf("\n");
    std::bitset<4> bin(tmp[9]);
    std::cout << "number: " << number << "value: " << bin[number] << std::endl;
    return bin[number];
  }

  int ModbusInterafce::getDigitalOutput(const int number)
  {
    boost::mutex::scoped_lock lock(mutex_in_);
    char tmp[100];
    memset(tmp, 0, sizeof(tmp));
    if (!isconnected_)
    {
      ROS_ERROR("ModBus has not connected,please call modbusConnect function first");
      return -1;
    }
    //00 01 00 00 00 06 FF 01 00 64 00 04
    unsigned char ctrDO[12] = {0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0xFF, 0x01, 0x00, 0x64, 0x00, 0x04};
    recv(socket_, tmp, sizeof(tmp), 1);
    if (send(socket_, ctrDO, 12, 0) == -1 && errno == EAGAIN)
    {
      return -1;
    }
    if (recv(socket_, tmp, sizeof(tmp), 0) == -1 && errno == EAGAIN)
    {
      ROS_ERROR("[Modbus] Recv error");
      return -1;
    }
    for (int i = 0; i < 10; i++)
    {
      printf("%02X ", tmp[i]);
    }
    printf("\n");
    std::bitset<4> bin(tmp[9]);
    std::cout << "number: " << number << "value: " << bin[number] << std::endl;
    return bin[number];
  }

  int ModbusInterafce::setDigitalOutput(const int number, const int value)
  {
    boost::mutex::scoped_lock lock(mutex_out_);
    char tmp[12];
    memset(tmp, 0, sizeof(tmp));
    if (!isconnected_)
    {
      ROS_ERROR("ModBus has not connected,please call modbusConnect function first");
      return -1;
    }
    //0x00 01 00 00 00 06 FF 05 00 64 FF 00
    unsigned char ctrDO[12] = {0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0xFF, 0x05, 0x00, 0x64, 0xFF, 0x00};
    std::cout << ctrDO << std::endl;
    bool send_flag = false;
    switch (number)
    {
    case 0:
      ctrDO[9] = 0x64;
      break;
    case 1:
      ctrDO[9] = 0x65;
      send_flag = true;
      break;
    case 2:
      ctrDO[9] = 0x66;
      break;
    case 3:
      ctrDO[9] = 0x67;
      break;
    }

    if (value == 0)
    {
      ctrDO[10] = 0x00; //00 or FF
      if (send_flag == true)
      {
        flag_str.data = "0";
      }
    }
    else if (value == 1)
    {
      ctrDO[10] = 0xFF;
      if (send_flag == true)
      {
        flag_str.data = "1";
      }
    }
    else
    {
      return -1;
    }
    send_flag = false;
    if (send(socket_, ctrDO, 12, 0) == -1 && errno == EAGAIN)
    {
      return -1;
    }
    if (recv(socket_, tmp, sizeof(tmp), 0) == -1 && errno == EAGAIN)
    {
      ROS_ERROR("[Modbus] Recv error");
      return -1;
      ;
    }
    return 1; //then,check it...
  }

  void ModbusInterafce::publishSignal()
  {
    flag_pub.publish(flag_str);
  }

  int ModbusInterafce::setDigitalOutputs(const int outputs)
  {
    char tmp[12];
    memset(tmp, 0, sizeof(tmp));
    if (!isconnected_)
    {
      ROS_ERROR("ModBus has not connected,please call modbusConnect function first");
      return -1;
    }

    unsigned char ctrDO[14] = {0x00, 0x01, 0x00, 0x00, 0x00, 0x08, 0xFF, 0x0F, 0x00, 0x64, 0x00, 0x04, 0x01, 0x03};
    ctrDO[13] = outputs; //00 to FF
    std::cout << ctrDO << std::endl;
    if (send(socket_, ctrDO, 14, 0) == -1 && errno == EAGAIN)
    {
      return -1;
    }

    if (recv(socket_, tmp, sizeof(tmp), 0) == -1 && errno == EAGAIN)
    {
      ROS_ERROR("[Modbus] Recv error");
      return -1;
      ;
    }
    return 1; //then,check it...
  }

  int ModbusInterafce::waitForDigitalInput(const int number, const int value, const float duration)
  {
    double now = ros::Time::now().toSec();
    while (ros::Time::now().toSec() - now < duration)
    {
      if (value == getDigitalInput(number))
      {
        return -1;
      }
      ros::Duration(0.1).sleep();
    }
    return 1;
  }

  std::string ModbusInterafce::getIpAddr()
  {
    return ip_addr_;
  }
  bool ModbusInterafce::isConnected()
  {
    return isconnected_;
  }

} // namespace modbus_interface
