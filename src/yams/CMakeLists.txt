set(SRC_FILES main.cpp Logging.cpp)

set(SRC_HEADERS Logging.hpp)

add_executable(yams ${SRC_FILES} ${SRC_HEADERS})

target_include_directories(yams PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(
	yams PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets slog++::slog++
)
