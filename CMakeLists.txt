cmake_minimum_required(VERSION 3.5)
project(tiny3d C)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(include)
file(GLOB SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
add_executable(${PROJECT_NAME} ${SRC})

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})

if(WIN32)
	add_custom_command(
		OUTPUT "icon.ico"
		COMMAND iconwriter app.iconset
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		COMMENT "Generating icon from app.iconset"
		VERBATIM
	)
	add_custom_target(generate_icon ALL
		DEPENDS "icon.ico"
	)
	add_dependencies(${PROJECT_NAME} generate_icon)
	set(COMPANY_NAME "Dark Software")
	configure_file(res.rc ${CMAKE_SOURCE_DIR}/reso.rc @ONLY)
	target_sources(${PROJECT_NAME} PRIVATE reso.rc src/platform/windows.c)
	target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES} dwmapi uuid windowscodecs mfplat mfreadwrite mfuuid shlwapi)
	if(MSVC)
		add_definitions(-D_CRT_SECURE_NO_WARNINGS)
		set_target_properties(
			${PROJECT_NAME}
			PROPERTIES
				LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE"
				LINK_FLAGS_RELEASE "/SUBSYSTEM:windows /ENTRY:mainCRTStartup"
				LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:windows /ENTRY:mainCRTStartup"
				LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:windows /ENTRY:mainCRTStartup"
		)
		if(${CMAKE_VERSION} VERSION_LESS "3.6.0") 
			message( "\n\t[ WARNING ]\n\n\tCMake version lower than 3.6.\n\n\t - Please update CMake and rerun; OR\n\t - Manually set StartUp Project in Visual Studio.\n" )
		else()
			set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
		endif()
	endif()
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
	target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/app.icns src/platform/mac.m)
	target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES} m "-framework Cocoa -framework CoreVideo -framework AudioToolbox")
	set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE TRUE)
	# what a G: https://stackoverflow.com/questions/45048882/how-can-i-set-nshighresolutioncapable-in-info-plist-via-cmake
	add_custom_command(
		TARGET ${PROJECT_NAME}
		POST_BUILD
		COMMAND plutil -replace NSHighResolutionCapable -bool true "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.app/Contents/Info.plist"
    )
	file(COPY res DESTINATION ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.app/Contents/MacOS)
elseif(UNIX)
	target_sources(${PROJECT_NAME} PRIVATE src/platform/linux.c)
	target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES} m X11 Xi pulse pulse-simple avformat avcodec avutil swscale swresample)
endif()

#https://stackoverflow.com/questions/7747857/in-cmake-how-do-i-work-around-the-debug-and-release-directories-visual-studio-2
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/$<0:>)