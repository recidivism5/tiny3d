cmake_minimum_required(VERSION 3.5)
project(tiny3d C)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(include)
add_executable(${PROJECT_NAME} src/tiny3d.c src/game.c src/tinymath.c src/whereami.c ${PLATFORM_SRC})
if(MSVC)
	set_target_properties(
		${PROJECT_NAME}
		PROPERTIES
			LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE"
			LINK_FLAGS_RELEASE "/SUBSYSTEM:windows /ENTRY:mainCRTStartup"
			LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:windows /ENTRY:mainCRTStartup"
			LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:windows /ENTRY:mainCRTStartup"
	)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	if(${CMAKE_VERSION} VERSION_LESS "3.6.0") 
		message( "\n\t[ WARNING ]\n\n\tCMake version lower than 3.6.\n\n\t - Please update CMake and rerun; OR\n\t - Manually set StartUp Project in Visual Studio.\n" )
	else()
		set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
	endif()
endif()

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})

if (WIN32)
	target_sources(${PROJECT_NAME} PRIVATE src/platform/windows.c)
	target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES} dwmapi uuid windowscodecs mfplat.lib mfreadwrite.lib mfuuid.lib)
elseif(APPLE)
	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/app.icns
		PRE_BUILD
		COMMAND iconutil -c icns ${CMAKE_CURRENT_SOURCE_DIR}/app.iconset
	)
	set_target_properties(${PROJECT_NAME} PROPERTIES
		MACOSX_BUNDLE TRUE
		MACOSX_FRAMEWORK_IDENTIFIER org.cmake.ExecutableTarget
		RESOURCE app.icns)
	set(MACOSX_BUNDLE_ICON_FILE app.icns)
	target_sources(${PROJECT_NAME} PRIVATE src/platform/mac.m ${CMAKE_CURRENT_SOURCE_DIR}/app.icns)
	target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES} m "-framework Cocoa -framework CoreVideo -framework AudioToolbox")
	set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE TRUE)
	# what a G: https://stackoverflow.com/questions/45048882/how-can-i-set-nshighresolutioncapable-in-info-plist-via-cmake
	add_custom_command(
		TARGET ${PROJECT_NAME}
		POST_BUILD
		COMMAND plutil -replace NSHighResolutionCapable -bool true "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.app/Contents/Info.plist"
    )
elseif(UNIX)
	target_sources(${PROJECT_NAME} PRIVATE src/platform/unix.c)
	target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES} m X11 pulse pulse-simple)
endif()

if (NOT (APPLE))
	#https://stackoverflow.com/questions/7747857/in-cmake-how-do-i-work-around-the-debug-and-release-directories-visual-studio-2
	set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/$<0:>)
endif()
