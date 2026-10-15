/*
 * Author: Jiayuan Sun
 * Copyright: 2019 Shanghai Yikun Electrical Engineering Co，Ltd
 */
#include "modbus_interface/modbus_interface.h"
#include <unistd.h>

using namespace modbus_interface;
#define Modbus "Modbus"

int main(int argc, char *argv[])
{
  ros::init(argc, argv, "modbus");
  ros::NodeHandle nh;
  ros::NodeHandle private_nh("~");
  std::string group_name;
  private_nh.param("group_name", group_name, std::string(Modbus));
  ModbusInterafce::Ptr interface;
  ros::Rate loop(10);
  interface = boost::make_shared<ModbusInterafce>(private_nh, group_name);
  int send_count = 3;
  while (nh.ok())
  {
    if (!interface->isConnected())
    {
      if (interface->modbusConnect())
      {
        ROS_INFO("Successfully connnected to Modbus with ip=%s", interface->getIpAddr().data());
      }
      else
      {
        ROS_ERROR("Unable to connnect to Modbus with ip=%s", interface->getIpAddr().data());
      }
    }
    ros::spinOnce();
    loop.sleep();
  }
  return 0;
}
