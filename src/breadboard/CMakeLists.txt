file(GLOB_RECURSE SRC *.cpp)
file(GLOB_RECURSE INC *.h*)


add_library(breadboard ${SRC} ${INC})
target_include_directories(breadboard
	INTERFACE
		${CMAKE_CURRENT_SOURCE_DIR}/..
		${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(breadboard
	PUBLIC
	device
	virtual-breadboard-client
	embedded
	Qt5::Widgets
)
set_target_properties(breadboard PROPERTIES
	AUTOMOC ON
)