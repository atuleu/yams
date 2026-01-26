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

# Fetch dependencies
include(FetchContent)
if(EXISTS ${PROJECT_SOURCE_DIR}/3rdparty/slogpp)
	# Use local slogpp for in-tree development
	FetchContent_Declare(
		slogpp SOURCE_DIR ${PROJECT_SOURCE_DIR}/3rdparty/slogpp
	)
else()
	# Fetch slogpp header-only logging library
	FetchContent_Declare(
		slogpp
		GIT_REPOSITORY https://github.com/atuleu/slogpp.git
		GIT_TAG 5d2d9c1acbf8e797b032e855b48b1cbe38b30484
		GIT_SHALLOW TRUE
	)
endif()
# Fetch GoogleTest framework
FetchContent_Declare(
	googletest
	GIT_REPOSITORY https://github.com/google/googletest.git
	GIT_TAG 52eb8108c5bdec04579160ae17225d66034bd723
	GIT_SHALLOW TRUE
)

# Prevent GoogleTest from overriding compiler/linker options
set(gtest_force_shared_crt
	ON
	CACHE BOOL "" FORCE
)

FetchContent_MakeAvailable(slogpp googletest)

# Enable testing
enable_testing()

include(GoogleTest)

add_custom_target(check ALL ${CMAKE_CTEST_COMMAND} ARGS --output-on-failure)

# Add source subdirectory
add_subdirectory(src/yams)
