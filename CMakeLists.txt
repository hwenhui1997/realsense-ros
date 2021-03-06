CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT(myrealsense)
add_definitions(-std=c++11)

# PACKAGES
FIND_PACKAGE(OpenCV REQUIRED)
FIND_PACKAGE(Eigen3 REQUIRED)
FIND_PACKAGE(realsense2 REQUIRED)
INCLUDE_DIRECTORIES(${EIGEN3_INCLUDE_DIR})
FIND_PACKAGE(
  catkin
  REQUIRED COMPONENTS
  roscpp
  std_msgs
  geometry_msgs
  cv_bridge
  image_transport
)
CATKIN_PACKAGE(LIBRARIES ${PROJECT_NAME})
INCLUDE_DIRECTORIES(${catkin_INCLUDE_DIRS}
                    ${realsense2_INCLUDE_DIR})

# EXECUTABLES
SET(DEPS
  ${realsense2_LIBRARY}
  ${OpenCV_LIBS}
  ${catkin_LIBRARIES}
)

ADD_EXECUTABLE(intel_d435i src/intel_cam.cpp)
TARGET_LINK_LIBRARIES(intel_d435i ${DEPS})
