if(NOT WIN32)
	message(
		FATAL_ERROR
			"Internal cmake configuration error. I should only be compiled for Windows"
	)
endif()

add_executable(
	yams_launcher WIN32 ${PROJECT_BINARY_DIR}/src/yams/yams.rc main.c
)

target_link_libraries(yams_launcher shlwapi shell32)

set_target_properties(yams_launcher PROPERTIES WIN32_EXECUTABLE TRUE)
install(
	TARGETS yams_launcher
	BUNDLE DESTINATION .
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
