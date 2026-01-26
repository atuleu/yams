# Toolchain file for cross-compiling to Windows using MSVC via Wine
#
# This toolchain enables building Windows binaries on Linux using the actual
# MSVC compiler running under Wine, providing exact parity with Windows CI builds.
#
# Usage:
#   cmake -S . -B build-win \
#     -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-msvc-wine.cmake \
#     -DCMAKE_BUILD_TYPE=RelWithDebInfo \
#     -G Ninja

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x64)

# Prevent CMake from searching host system paths for libraries and includes
# This is CRITICAL to avoid mixing Linux and Windows headers
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Set MSVC Wine compilers (can be overridden via command line)
if(NOT DEFINED CMAKE_C_COMPILER)
	set(CMAKE_C_COMPILER "$ENV{HOME}/my_msvc/opt/msvc/bin/x64/cl")
endif()
if(NOT DEFINED CMAKE_CXX_COMPILER)
	set(CMAKE_CXX_COMPILER "$ENV{HOME}/my_msvc/opt/msvc/bin/x64/cl")
endif()
if(NOT DEFINED CMAKE_RC_COMPILER)
	set(CMAKE_RC_COMPILER "$ENV{HOME}/my_msvc/opt/msvc/bin/x64/rc")
endif()
if(NOT DEFINED CMAKE_AR)
	set(CMAKE_AR "$ENV{HOME}/my_msvc/opt/msvc/bin/x64/lib")
endif()
if(NOT DEFINED CMAKE_LINKER)
	set(CMAKE_LINKER "$ENV{HOME}/my_msvc/opt/msvc/bin/x64/link")
endif()

# CRITICAL: Ignore host system include directories
# This prevents CMake from adding /usr/include and other Linux paths
# which would cause MSVC to try compiling Linux glibc headers
set(CMAKE_SYSTEM_IGNORE_PATH
	/usr/include
	/usr/local/include
	/opt/include
	/usr/include/x86_64-linux-gnu
)

# Qt6 location in Wine prefix
if(NOT DEFINED Qt6_DIR)
	set(Qt6_DIR
		"$ENV{HOME}/.wine/drive_c/Qt/6.9.3/msvc2022_64/lib/cmake/Qt6"
	)
endif()

# Disable Vulkan detection on host system
# Qt6 tries to find Vulkan headers, which on Linux points to /usr/include
# We explicitly disable this to prevent Linux headers from leaking in
set(Vulkan_INCLUDE_DIR
	"Vulkan_INCLUDE_DIR-NOTFOUND"
	CACHE PATH "Disabled for cross-compilation" FORCE
)
set(Vulkan_LIBRARY
	"Vulkan_LIBRARY-NOTFOUND"
	CACHE PATH "Disabled for cross-compilation" FORCE
)

# Confirm we're cross-compiling
set(CMAKE_CROSSCOMPILING TRUE)

# Suppress warnings about missing sysroot
set(CMAKE_SYSROOT "")
