set(SRC_FILES main.cpp)

add_executable(yams ${SRC_FILES})

target_link_libraries(
	yams PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets slog++::slog++
)
