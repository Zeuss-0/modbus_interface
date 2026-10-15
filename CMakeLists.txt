cmake_minimum_required(VERSION 2.8.3)
project(modbus_interface)

add_compile_options(-std=c++11)

find_package(catkin REQUIRED COMPONENTS
  roscpp
  yikun_msgs
)

catkin_package(
  INCLUDE_DIRS include
  CATKIN_DEPENDS roscpp
)

include_directories(include ${catkin_INCLUDE_DIRS})
add_executable( modbus_interface_node
  src/modbus_interface.cpp
  src/modbus_interface_node.cpp)
add_dependencies(
  modbus_interface_node
  ${catkin_EXPORTED_TARGETS}
)
target_link_libraries(modbus_interface_node ${catkin_LIBRARIES} ${Boost_LIBRARIES})


install(
    TARGETS
        modbus_interface_node
    DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION})
