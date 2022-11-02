file(GLOB_RECURSE SRC *.cpp)
file(GLOB_RECURSE INC *.h*)


add_library(device-interface ${SRC} ${INC})
target_include_directories(device-interface
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
	INTERFACE
		${CMAKE_CURRENT_SOURCE_DIR}/factory
		${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_compile_features(device-interface PUBLIC cxx_std_17)
target_link_libraries(device-interface
	PUBLIC
		LuaBridge ${LUA_LIB}
		Qt5::Widgets
		breadboard
		virtual-breadboard-common
)