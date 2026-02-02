cmake_minimum_required(VERSION 3.20)

include(cmake/GenerateVersion.cmake)

project(
	yams
	VERSION ${GIT_PROJECT_VERSION}
	LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.9 REQUIRED COMPONENTS Core Gui Widgets OpenGL)
qt_standard_project_setup()
set(CMAKE_AUTOMOC OFF)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

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
		GIT_TAG 0cc9700b78f3fa0d6bae05c29d254414b73c91f2
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

# Prevent Installation of Googletest. We won't deploy tests in CPack
set(INSTALL_GTEST
	OFF
	CACHE BOOL "" FORCE
)
set(INSTALL_GMOCK
	OFF
	CACHE BOOL "" FORCE
)

FetchContent_Declare(
	cmake_git_version_tracking
	GIT_REPOSITORY
		https://github.com/andrew-hardin/cmake-git-version-tracking.git
	GIT_TAG 6c0cb87edd029ddfb403a8e24577c144a03605a6
)

FetchContent_MakeAvailable(slogpp googletest cmake_git_version_tracking)

message(STATUS "Found current version: ${GIT_DESCRIBE}")

# Enable testing
enable_testing()

include(GoogleTest)

if(WIN32)
	add_custom_target(check ${CMAKE_CTEST_COMMAND} ARGS --verbose)
else()
	add_custom_target(check ALL ${CMAKE_CTEST_COMMAND} ARGS --verbose)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(
	gstreamer
	REQUIRED
	IMPORTED_TARGET
	glib-2.0
	gio-2.0
	gobject-2.0
	gstreamer-1.0
	gstreamer-app-1.0
	gstreamer-video-1.0
)

# Add source subdirectory
add_subdirectory(src/yams)

add_subdirectory(resources/icons)

add_subdirectory(misc)

if(WIN32)
	add_subdirectory(src/yams_launcher)
	# CPack configuration (NSIS on Windows)
	set(CPACK_GENERATOR "NSIS")
	set(CPACK_PACKAGE_NAME "YAMS")
	set(CPACK_PACKAGE_VENDOR "io.github.atuleu")
	set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
	set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Yet Another Media Server")
	set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/atuleu/yams")
	set(CPACK_PACKAGE_INSTALL_DIRECTORY
		"YAMS ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
	)
	set(CPACK_NSIS_DISPLAY_NAME "YAMS: Yet Another Media Server")
	set(CPACK_NSIS_PACKAGE_NAME "YAMS: Yet Another Media Server")
	set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
	set(CPACK_PACKAGE_EXECUTABLES "yams_launcher"
								  "YAMS: Yet Another Media Server"
	)

	set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")

	set(CPACK_NSIS_MODIFY_PATH OFF)

	# Add "Run YAMS" checkbox on finish page
	set(CPACK_NSIS_MUI_FINISHPAGE_RUN "yams_launcher")

	set(CPACK_NSIS_MUI_ICON
		"${PROJECT_SOURCE_DIR}\\\\resources\\\\icons\\\\yams.ico"
	)
	set(CPACK_NSIS_MUI_UNIICON
		"${PROJECT_SOURCE_DIR}\\\\resources\\\\icons\\\\yams-uninstall.ico"
	)

	set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\yams_launcher.exe")

	# Define all NSIS extra commands in one place to avoid overwriting
	set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS
		"
	ExecWait 'msiexec /i \\\"$INSTDIR\\\\${CMAKE_INSTALL_DATADIR}\\\\gstreamer-1.0-runtime.msi\\\" /passive INSTALLDIR=\\\"$INSTDIR\\\\gstreamer\\\" ADDLOCAL=ALL'
	CreateShortCut '$DESKTOP\\\\YAMS: Yet Another Media Server ${CPACK_PACKAGE_VERSION}.lnk' '$INSTDIR\\\\bin\\\\yams_launcher.exe'
"
	)

	set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS
		"
	Delete '$DESKTOP\\\\YAMS: Yet Another Media Server ${CPACK_PACKAGE_VERSION}.lnk'
	ExecWait 'msiexec /x \\\"$INSTDIR\\\\${CMAKE_INSTALL_DATADIR}\\\\gstreamer-1.0-runtime.msi\\\" /passive'
"
	)

	set(CPACK_PACKAGE_ICON
		"${PROJECT_SOURCE_DIR}\\\\resources\\\\icons\\\\yams.ico"
	)

	include(CPack)
endif()
