cmake_minimum_required(VERSION 3.5)

# Set the custom partition table
set(PARTITION_TABLE ${CMAKE_CURRENT_LIST_DIR}/partitions.csv)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(mqtt_espnow_bridge)

# Add components
set(EXTRA_COMPONENT_DIRS components)

# Enable unit testing
if(CONFIG_UNITY_ENABLE_IDF_TEST_RUNNER)
    set(EXTRA_COMPONENT_DIRS ${EXTRA_COMPONENT_DIRS} test)
endif()
