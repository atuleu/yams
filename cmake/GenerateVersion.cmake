find_package(Git REQUIRED)

execute_process(
	COMMAND ${GIT_EXECUTABLE} describe --tags --dirty
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	OUTPUT_VARIABLE _GIT_DESCRIBE_RESULT
	RESULT_VARIABLE _GIT_DESCRIBE
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(_GIT_DESCRIBE GREATER 0)
	message(FATAL_ERROR "Could not describe repository")
endif()

string(REGEX REPLACE "^v?([0-9\\.]+)(-.*)?" "\\1" GIT_PROJECT_VERSION
					 ${_GIT_DESCRIBE_RESULT}
)
message(STATUS "Version from git describe: ${GIT_PROJECT_VERSION}")
