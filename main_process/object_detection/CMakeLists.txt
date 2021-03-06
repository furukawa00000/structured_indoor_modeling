cmake_minimum_required(VERSION 2.8)
project(object_detection)

FIND_PACKAGE(OpenCV REQUIRED)
FIND_PACKAGE(CGAL QUIET COMPONENTS Core)

INCLUDE( ${CGAL_USE_FILE} )
link_directories(/usr/local/lib)

if(UNIX)
set(CMAKE_CXX_FLAGS "-Wno-c++11-extensions -std=c++11")
endif(UNIX)

if(${CMAKE_SYSTEM} MATCHES "Darwin")
   set( CMAKE_CXX_FLAGS "-Wno-c++11-extensions -Wno-gnu-static-float-init -Wno-sign-compare" )
   include_directories("/usr/local/include/eigen3")			  		     
endif(${CMAKE_SYSTEM} MATCHES "Darwin")

if(${CMAKE_SYSTEM} MATCHES "Linux")
   include_directories("/usr/include/eigen3")
endif(${CMAKE_SYSTEM} MATCHES "Linux")


if (WIN32)
	include_directories("C:\\Eigen3.2.2")
	include_directories("C:\\gflags-2.1.1\\include")
	link_directories("C:\\gflags-2.1.1\\lib")	
endif (WIN32)

add_executable( generate_object_icons_cli generate_object_icons_cli.cc generate_object_icons.cc polygon_triangulation2.cc ../../base/detection.cc ../../base/floorplan.cc ../../base/indoor_polygon.cc ../../base/panorama.cc ../../base/point_cloud.cc )
target_link_libraries( generate_object_icons_cli ${OpenCV_LIBS} )
target_link_libraries( generate_object_icons_cli gflags )
target_link_libraries( generate_object_icons_cli ${CGAL_LIBRARIES} ${CGAL_3RDPARTY_LIBRARIES})

if(${CMAKE_SYSTEM} MATCHES "Linux")
  target_link_libraries(generate_object_icons_cli pthread)
endif(${CMAKE_SYSTEM} MATCHES "Linux")

