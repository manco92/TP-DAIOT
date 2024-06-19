# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

#/ project(generic_gpio)

#set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/common_components/protocol_examples_common)
set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/common_components/protocol_examples_common $ENV{IDF_PATH}/components/)
include_directories($ENV{IDF_PATH}/examples/common_components/protocol_examples_common/include)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(mqtt_ssl_mutual_auth)

target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "main/client.crt" TEXT)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "main/client.key" TEXT)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "main/ca.crt" TEXT)