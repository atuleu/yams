cmake_policy(SET CMP0048 NEW)

project(yams VERSION 0.1.0
	         LANGUAGES C CXX)

cmake_minimum_required(VERSION 3.11)


find_package(Qt5 COMPONENTS Widgets MultimediaWidgets REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)


add_subdirectory(src)
