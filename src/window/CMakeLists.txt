file(GLOB_RECURSE SRC *.cpp)
file(GLOB_RECURSE INC *.h*)


add_library(window ${SRC} ${INC})
target_include_directories(window INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(window
PUBLIC
	embedded
	breadboard
	Qt5::Widgets
)
set_target_properties(window PROPERTIES
	AUTOMOC ON
)
