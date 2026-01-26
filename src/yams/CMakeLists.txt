set(SRC_FILES Logging.cpp)

set(SRC_HEADERS Logging.hpp)

set(SRC_TESTS_FILES LoggingTest.cpp #
					main-check.cpp #
)

add_library(yams-common STATIC ${SRC_FILES} ${SRC_HEADERS})

target_include_directories(yams-common PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(
	yams-common PUBLIC Qt6::Core Qt6::Gui Qt6::Widgets slog++::slog++
)

add_executable(yams main.cpp)
target_link_libraries(yams PRIVATE yams-common)

add_executable(yams-tests ${SRC_TESTS_FILES})

target_link_libraries(yams-tests yams-common GTest::gtest GTest::gmock)

gtest_discover_tests(yams-tests)

add_dependencies(check yams-tests)
