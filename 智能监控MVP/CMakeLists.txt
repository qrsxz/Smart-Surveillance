cmake_minimum_required(VERSION 3.10)
project(SmartSurveillanceMVP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(OpenCV REQUIRED)

# 优先 Qt6，回退 Qt5（v5: 需要 Network 模块做局域网推流）
find_package(Qt6 COMPONENTS Widgets Network QUIET)
if (NOT Qt6_FOUND)
    find_package(Qt5 REQUIRED COMPONENTS Widgets Network)
endif()

add_executable(SmartSurveillanceMVP
    src/main.cpp
    src/MainWindow.cpp
    src/VideoThread.cpp
    src/PersonDetector.cpp
    src/StreamServer.cpp
)

target_include_directories(SmartSurveillanceMVP PRIVATE src)

if (Qt6_FOUND)
    target_link_libraries(SmartSurveillanceMVP PRIVATE Qt6::Widgets Qt6::Network ${OpenCV_LIBS})
else()
    target_link_libraries(SmartSurveillanceMVP PRIVATE Qt5::Widgets Qt5::Network ${OpenCV_LIBS})
endif()
