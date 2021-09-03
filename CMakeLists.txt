cmake_minimum_required (VERSION 3.14.5)

# Set project name
project (TTA)
set(TARGET_NAME TTA)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_subdirectory(Urho3D)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (DESKTOP)
    # Mobile/web toolchains manage runtime libraries themselves, we should not interfere.
    if (URHO3D_STATIC_RUNTIME)
        ucm_set_runtime(STATIC)
    else ()
        ucm_set_runtime(DYNAMIC)
    endif ()
endif ()

if (WEB)
	set (CMAKE_BUILD_TYPE Release)
else()
	set (CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

file(GLOB_RECURSE MY_CPP_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp)
file(GLOB_RECURSE MY_H_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h)

set(MY_SOURCE_FILES ${MY_CPP_FILES} ${MY_H_FILES})
add_executable(${TARGET_NAME} WIN32 ${MY_SOURCE_FILES})
target_link_libraries(${TARGET_NAME} PRIVATE Urho3D)

if (WEB OR MOBILE)
	# SET(PACKAGE_TOOL ${SDK_PATH}/bin/PackageTool)
	create_pak("${CMAKE_CURRENT_SOURCE_DIR}/bin/Data"               "${CMAKE_BINARY_DIR}/bin/Data.pak")
    create_pak("${CMAKE_CURRENT_SOURCE_DIR}/bin/CoreData"           "${CMAKE_BINARY_DIR}/bin/CoreData.pak")
endif ()

if (WEB)
	set (EMSCRIPTEN_MEMORY_LIMIT 512)
    web_executable(${TARGET_NAME})
    package_resources_web(
        FILES        "${CMAKE_CURRENT_BINARY_DIR}/bin/Data.pak"
                     "${CMAKE_CURRENT_BINARY_DIR}/bin/CoreData.pak"
        RELATIVE_DIR "${CMAKE_CURRENT_BINARY_DIR}/bin"
        OUTPUT       "Resources.js"
        INSTALL_TO   "${CMAKE_CURRENT_BINARY_DIR}/bin"
    )
    web_link_resources(${TARGET_NAME} Resources.js)
    target_link_libraries(${TARGET_NAME} PRIVATE "--shell-file ${CMAKE_SOURCE_DIR}/shell.html" "-s INITIAL_MEMORY=536870912" "-s ALLOW_MEMORY_GROWTH=1")
endif ()