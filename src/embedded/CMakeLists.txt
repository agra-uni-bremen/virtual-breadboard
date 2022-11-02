file(GLOB_RECURSE SRC *.cpp)
file(GLOB_RECURSE INC *.h*)


add_library(embedded ${SRC} ${INC})
target_include_directories(embedded INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(embedded PUBLIC
	virtual-breadboard-client
	Qt5::Widgets
)
set_target_properties(embedded PROPERTIES
	AUTOMOC ON
)