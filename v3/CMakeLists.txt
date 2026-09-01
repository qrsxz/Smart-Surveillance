cmake_minimum_required(VERSION 3.10)
project(SmartSurveillanceV3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(OpenCV REQUIRED)

# 优先 Qt6，回退 Qt5
find_package(Qt6 COMPONENTS Widgets QUIET)
if (NOT Qt6_FOUND)
    find_package(Qt5 REQUIRED COMPONENTS Widgets)
endif()

add_executable(SmartSurveillanceV3
    src/main.cpp
    src/ui/MainWindow.cpp
    src/ui/VideoWidget.cpp
    src/ui/StatusLed.cpp
    src/core/VideoThread.cpp
    src/algorithm/MotionDetector.cpp
)

target_include_directories(SmartSurveillanceV3 PRIVATE src)

if (Qt6_FOUND)
    target_link_libraries(SmartSurveillanceV3 PRIVATE Qt6::Widgets ${OpenCV_LIBS})
else()
    target_link_libraries(SmartSurveillanceV3 PRIVATE Qt5::Widgets ${OpenCV_LIBS})
endif()
