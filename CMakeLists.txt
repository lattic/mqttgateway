cmake_minimum_required(VERSION 3.0.0)
project(mqttgateway VERSION 0.1.0)

include(CTest)
enable_testing()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

add_executable(iotgw main_iotgw.c mqtt/iotgw_mqtt_client.c utils/iotgw_utils.c iotcmft/iotgw_cmft.c)
add_executable(iotutils main_iotutils.c utils/iotgw_utils.c)

find_package(json-c CONFIG)
target_link_libraries(iotgw PRIVATE json-c::json-c)
target_link_libraries(iotutils PRIVATE json-c::json-c)

find_package(MbedTLS)
target_link_libraries(iotgw PUBLIC MbedTLS::mbedtls MbedTLS::mbedcrypto MbedTLS::mbedx509)
target_link_libraries(iotutils PUBLIC MbedTLS::mbedtls MbedTLS::mbedcrypto MbedTLS::mbedx509)

find_package(eclipse-paho-mqtt-c)
target_link_libraries(iotgw PUBLIC eclipse-paho-mqtt-c::paho-mqtt3a)
target_link_libraries(iotutils PUBLIC eclipse-paho-mqtt-c::paho-mqtt3a)

SET_TARGET_PROPERTIES(iotgw iotutils PROPERTIES COMPILE_DEFINITIONS "DBG_PRINTF=1;PAHO_MQTT_IMPORTS=1")

target_include_directories(iotgw PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/mqtt
    ${CMAKE_CURRENT_SOURCE_DIR}/iotcmft
)

target_include_directories(iotutils PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/mqtt
)
