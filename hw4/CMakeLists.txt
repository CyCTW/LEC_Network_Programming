# Minimal CMakeLists.txt for the AWS SDK for C++.
cmake_minimum_required(VERSION 3.2)


# Use shared libraries, which is the default for the AWS C++ SDK build.
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

project(client)

# Locate the AWS SDK for c++ package.
find_package(AWSSDK REQUIRED COMPONENTS s3)

add_executable(client client.cpp)


target_compile_features(client PUBLIC cxx_std_11)
target_link_libraries(client ${AWSSDK_LINK_LIBRARIES})

target_link_libraries(client "-lrdkafka")
target_link_libraries(client "-lpthread")
	
