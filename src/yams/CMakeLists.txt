set(SRC_FILES Logging.cpp)

set(SRC_HEADERS Logging.hpp)

set(SRC_TESTS_FILES LoggingTest.cpp)

add_library(yams-common STATIC ${SRC_FILES} ${SRC_HEADERS})

target_include_directories(yams-common PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(
	yams-common PUBLIC Qt6::Core Qt6::Gui Qt6::Widgets slog++::slog++
)

add_executable(yams main.cpp)
target_link_libraries(yams PRIVATE yams-common)

add_executable(yams-tests ${SRC_TESTS_FILES})

target_link_libraries(yams-tests yams-common GTest::gmock_main)

set_target_properties(
	yams-common yams yams-tests
	PROPERTIES AUTOMOC ON
			   AUTOUIC ON
			   AUTORCC ON
)

add_test(NAME yams-all-tests COMMAND yams-tests)
add_dependencies(check yams-tests)

if(WIN32)
	set_tests_properties(
		yams-all-tests
		PROPERTIES ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
	)

endif()

install(
	TARGETS yams
	BUNDLE DESTINATION .
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(WIN32)
	set_target_properties(yams PROPERTIES WIN32_EXECUTABLE TRUE)
	qt_generate_deploy_script(
		TARGET
		yams
		OUTPUT_SCRIPT
		yams_deploy_script
		CONTENT
		"qt_deploy_runtime_dependencies(EXECUTABLE \"$<TARGET_FILE:yams>\" GENERATE_QT_CONF)"
	)
	install(SCRIPT ${yams_deploy_script})
endif()
