cmake_minimum_required(VERSION 3.5)
project(tiny3d C)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(include)
file(GLOB_RECURSE SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")
	 
add_executable(${PROJECT_NAME} ${SRC})
if(MSVC)
	set_target_properties(
		${PROJECT_NAME}
		PROPERTIES
			LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE"
			LINK_FLAGS_RELEASE "/SUBSYSTEM:windows /ENTRY:mainCRTStartup"
			LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:windows /ENTRY:mainCRTStartup"
			LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:windows /ENTRY:mainCRTStartup"
	)
endif()
if (WIN32)
	target_link_libraries(${PROJECT_NAME} dxguid d3d11 d3dcompiler dwmapi uuid windowscodecs)
else()
	find_package(OpenGL REQUIRED)
	include_directories(${OPENGL_INCLUDE_DIRS})
endif()
if(APPLE)	
	target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES} m)
elseif(UNIX)
	target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES} m X11)
endif()

if( MSVC )
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	if(${CMAKE_VERSION} VERSION_LESS "3.6.0") 
		message( "\n\t[ WARNING ]\n\n\tCMake version lower than 3.6.\n\n\t - Please update CMake and rerun; OR\n\t - Manually set StartUp Project in Visual Studio.\n" )
	else()
		set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
	endif()
endif()

#https://stackoverflow.com/questions/7747857/in-cmake-how-do-i-work-around-the-debug-and-release-directories-visual-studio-2
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/$<0:>)
