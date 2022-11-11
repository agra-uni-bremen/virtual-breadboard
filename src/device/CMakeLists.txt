file(GLOB_RECURSE SRC *.cpp)
file(GLOB_RECURSE INC *.h*)

add_library(device
	${SRC} ${INC}
	$<TARGET_OBJECTS:c-devices>		# This keeps otherwise unreferenced objects in this library
	# TODO: Somehow, this expression is empty
)
target_include_directories(device
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
	INTERFACE
		${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_compile_features(device PUBLIC cxx_std_17)

target_link_libraries(device
	PRIVATE
		virtual-breadboard-common
	PUBLIC
		LuaBridge ${LUA_LIB}
		Qt5::Widgets
		breadboard
)

add_library(device-interface INTERFACE)
target_include_directories(device-interface INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}/
	${CMAKE_CURRENT_SOURCE_DIR}/factory
)
target_link_libraries(device-interface
	INTERFACE
		Qt5::Widgets
		virtual-breadboard-common
)