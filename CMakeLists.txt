cmake_minimum_required(VERSION 3.20)

project(
	yams
	VERSION 0.1.0
	LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6 (>= 6.4 for Ubuntu 22.04 compatibility)
find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Widgets)

# Fetch slogpp header-only logging library
include(FetchContent)
FetchContent_Declare(
	slogpp
	GIT_REPOSITORY https://github.com/atuleu/slogpp.git
	GIT_TAG 2aac2dd844471323d8b62704768610b85ef31f3e
	GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(slogpp)

# Add source subdirectory
add_subdirectory(src/yams)
