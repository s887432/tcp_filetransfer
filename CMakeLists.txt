cmake_minimum_required(VERSION 3.5)

project(file_trans)

#SET(CMAKE_C_COMPILER   /home/user/WorkArea/buildroot_a5d27_som1_opcua-2023.10/buildroot-at91/output/host/bin/arm-buildroot-linux-gnueabihf-gcc)
#SET(CMAKE_CXX_COMPILER /home/user/WorkArea/buildroot_a5d27_som1_opcua-2023.10/buildroot-at91/output/host/bin/arm-buildroot-linux-gnueabihf-g++)

#SET(CMAKE_C_COMPILER   /home/user/WorkArea/buildroot_9x60_curiosity-2023.10/buildroot-at91/output/host/bin/arm-buildroot-linux-gnueabi-gcc)
#SET(CMAKE_CXX_COMPILER /home/user/WorkArea/buildroot_9x60_curiosity-2023.10/buildroot-at91/output/host/bin/arm-buildroot-linux-gnueabi-g++)

SET(CMAKE_C_COMPILER   gcc)
SET(CMAKE_CXX_COMPILER g++)

set (EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

add_executable(server ${PROJECT_SOURCE_DIR}/src/server.c)

add_executable(client ${PROJECT_SOURCE_DIR}/src/client.c)

