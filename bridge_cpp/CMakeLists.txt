cmake_minimum_required(VERSION 3.10)
project(einscan_legacy_bridge CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SDK_ROOT "${CMAKE_SOURCE_DIR}/../third_party/deprecated-EinScan-SDK" CACHE PATH "Deprecated EinScan SDK root")
file(TO_CMAKE_PATH "${SDK_ROOT}" SDK_ROOT_CMAKE)
set(SDK_INCLUDE "${SDK_ROOT_CMAKE}/SDK/include")
set(SDK_LIB_DIR "${SDK_ROOT_CMAKE}/SDK/x64_lib")
set(SDK_DLL_DIR "${SDK_ROOT_CMAKE}/SDK/x64_dll")

message(STATUS "SDK_ROOT=${SDK_ROOT_CMAKE}")
message(STATUS "SDK_INCLUDE=${SDK_INCLUDE}")
message(STATUS "SDK_LIB_DIR=${SDK_LIB_DIR}")

if(NOT EXISTS "${SDK_INCLUDE}/sn3d_pro_sdk.h")
    message(FATAL_ERROR "sn3d_pro_sdk.h not found at ${SDK_INCLUDE}/sn3d_pro_sdk.h")
endif()
if(NOT EXISTS "${SDK_LIB_DIR}/sn3d_pro_sdk.lib")
    message(FATAL_ERROR "sn3d_pro_sdk.lib not found at ${SDK_LIB_DIR}/sn3d_pro_sdk.lib")
endif()

add_executable(einscan_legacy_bridge einscan_legacy_bridge.cpp)
target_include_directories(einscan_legacy_bridge PRIVATE "${SDK_INCLUDE}")
target_link_directories(einscan_legacy_bridge PRIVATE "${SDK_LIB_DIR}")
target_link_libraries(einscan_legacy_bridge sn3d_pro_sdk)

add_custom_command(TARGET einscan_legacy_bridge POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:einscan_legacy_bridge>"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${SDK_DLL_DIR}"
        "$<TARGET_FILE_DIR:einscan_legacy_bridge>"
)
