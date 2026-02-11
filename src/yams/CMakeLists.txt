set(SRC_FILES
	utils/Logging.cpp #
	utils/Version.cpp #
	utils/ObjectPool.cpp #
	gstreamer/Thread.cpp #
	gstreamer/QOpenGL.cpp #
	gstreamer/Pipeline.cpp
	Frame.cpp
	MediaPipeline.cpp
	Compositor.cpp
	VideoOutput.cpp
	VideoThread.cpp
)

set(SRC_HEADERS
	utils/Logging.hpp #
	utils/defer.hpp #
	utils/slogQt.hpp #
	utils/Version.hpp #
	utils/ObjectPool.hpp #
	utils/fractional.hpp #
	gstreamer/Memory.hpp #
	gstreamer/QOpenGL.hpp #
	gstreamer/Thread.hpp #
	gstreamer/Pipeline.hpp
	gstreamer/Factory.hpp
	MediaPlayInfo.hpp
	MediaPipeline.hpp
	Frame.hpp
	Compositor.hpp
	VideoOutput.hpp
	VideoThread.hpp
)

set(SRC_TESTS_FILES
	utest/main.cpp #
	gstreamer/MemoryTest.cpp #
	gstreamer/PipelineTest.cpp #
	utils/LoggingTest.cpp #
	utils/ObjectPoolTest.cpp #
	utils/fractionalTest.cpp #
	gstreamer/ThreadTest.cpp #
)

add_library(yams-common STATIC ${SRC_FILES} ${SRC_HEADERS})

target_include_directories(yams-common PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(
	yams-common
	PUBLIC Qt6::Core #
		   Qt6::Gui #
		   Qt6::Widgets
		   Qt6::OpenGL
		   slog++::slog++
		   cmake_git_version_tracking
		   PkgConfig::gstreamer
		   cpptrace::cpptrace
		   concurrentqueue
)

qt_add_resources(
	yams-common
	"shaders"
	PREFIX
	"/"
	FILES
	shaders/sprite.vertex
	shaders/sprite.fragment
	shaders/frame.vertex
	shaders/frame.fragment
)

if(WIN32)
	configure_file(yams.rc.in yams.rc @ONLY)
endif()

add_executable(yams $<$<PLATFORM_ID:Windows>:yams.rc> main.cpp)

target_link_libraries(yams PRIVATE yams-common)

add_executable(yams-tests ${SRC_TESTS_FILES})

target_link_libraries(yams-tests yams-common GTest::gmock concurrentqueue)

set_target_properties(
	yams-common yams yams-tests
	PROPERTIES AUTOMOC ON
			   AUTOUIC ON
			   AUTORCC ON
)

add_test(NAME yams-all-tests COMMAND yams-tests)
add_dependencies(check yams-tests)

if(WIN32)
	# Your mileage may not work, especially in CI.
	set(gstreamer_BIN_DIR "${gstreamer_LIBRARY_DIRS}/../bin")
	cmake_path(ABSOLUTE_PATH gstreamer_BIN_DIR NORMALIZE)
	set_tests_properties(
		yams-all-tests
		PROPERTIES
			ENVIRONMENT
			"PATH=$<TARGET_FILE_DIR:Qt6::Core>;${gstreamer_BIN_DIR};$ENV{PATH}"
	)
	message(STATUS "${gstreamer_BIN_DIR} added to test path")
endif()

install(
	TARGETS yams
	BUNDLE DESTINATION .
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

option(
	GSTREAMER_RUNTIME_INSTALLER
	"Path to GStreamer runtimer installer for repack. Required for deploying on Windows."
	""
)

if(WIN32)
	if(NOT GSTREAMER_RUNTIME_INSTALLER)
		message(
			WARNING
				"On windows, GSTREAMER_RUNTIME_INSTALLER option is required for install/package targets. These targets will fail."
		)
	else()
		cmake_path(
			ABSOLUTE_PATH GSTREAMER_RUNTIME_INSTALLER BASE_DIRECTORY
			"${PROJECT_SOURCE_DIR}" NORMALIZE
		)
		if(EXISTS ${GSTREAMER_RUNTIME_INSTALLER})
			message(
				STATUS
					"Will package private gstreamer: ${GSTREAMER_RUNTIME_INSTALLER}"
			)
			install(
				FILES ${GSTREAMER_RUNTIME_INSTALLER}
				DESTINATION ${CMAKE_INSTALL_DATADIR}
				RENAME gstreamer-1.0-runtime.msi
			)
		else()
			message(
				FATAL_ERROR
					"GStreamer runtime installer not found: ${GSTREAMER_RUNTIME_INSTALLER}"
			)
		endif()
	endif()

	set_target_properties(yams PROPERTIES WIN32_EXECUTABLE TRUE)
	qt_generate_deploy_script(
		TARGET
		yams
		OUTPUT_SCRIPT
		yams_deploy_script
		CONTENT
		"
qt_deploy_runtime_dependencies(EXECUTABLE \"$<TARGET_FILE:yams>\" GENERATE_QT_CONF)
"
	)

	install(SCRIPT ${yams_deploy_script})
endif()
