cmake_minimum_required(VERSION 3.10)
project(MZZXLC_mod_loader)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

set(INIH_FILES
    "external/inih/ini.h"
    "external/inih/cpp/INIReader.h"
    "external/inih/ini.c"
    "external/inih/cpp/INIReader.cpp"
)
source_group("External\\inih" FILES ${INIH_FILES})

set(HEADER_FILES
    "source/stage0.h"
    "source/stage1.h"
    "source/gui.h"
    "source/mod.h"
)

set(SRC_FILES
    "source/stage0.cpp"
    "source/stage1.cpp"
    "source/gui.cpp"
    "source/mod.cpp"
    "source/dllmain.cpp"
)

set(
    CMAKE_CONFIGURATION_TYPES
    "Debug;Release;RelWithDebInfo;MinSizeRel;ForPub;ForPub2"
    CACHE STRING "" FORCE
)

add_compile_definitions(NOMINMAX UNICODE _UNICODE)

add_subdirectory("external/minhook")

add_library(${PROJECT_NAME} SHARED ${SRC_FILES} ${HEADER_FILES} ${INIH_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".asi")
target_compile_definitions(${PROJECT_NAME} PUBLIC TOML_EXCEPTIONS=0)

target_include_directories(
    ${PROJECT_NAME}
    BEFORE
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/external/inih/cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/external/semver
    ${CMAKE_CURRENT_SOURCE_DIR}/external/boost
    ${CMAKE_CURRENT_SOURCE_DIR}/external/tomlplusplus/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/source
)

target_link_libraries(
    ${PROJECT_NAME} PRIVATE
    minhook
    comctl32.lib
)

if(MSVC)
    set(
        CMAKE_CXX_FLAGS
        "/DWIN32 /D_WINDOWS /W3"
        CACHE STRING "C++ flags" FORCE
    )
    set(
        CMAKE_C_FLAGS
        "/DWIN32 /D_WINDOWS /W3"
        CACHE STRING "C flags" FORCE
    )
    set(
        CMAKE_CXX_FLAGS_FORPUB
        "${CMAKE_CXX_FLAGS_RELEASE} /W3 /Oi /Ot /Oy /GS- /GR- /GL /Gy /GT /GF /GR- /FAcs"
        CACHE STRING "" FORCE
    )
    set(
        CMAKE_C_FLAGS_FORPUB
        "${CMAKE_C_FLAGS_RELEASE} /W3 /Oi /Ot /Oy /GS- /GR- /GL /Gy /GT /GF /FAcs"
        CACHE STRING "" FORCE
    )
    set(
        CMAKE_CXX_FLAGS_FORPUB2
        "${CMAKE_CXX_FLAGS_FORPUB} /clang:-flto=full"
        CACHE STRING "" FORCE
    )
    set(
        CMAKE_C_FLAGS_FORPUB2
        "${CMAKE_C_FLAGS_FORPUB} /clang:-flto=full"
        CACHE STRING "" FORCE
    )
    set_target_properties(
        ${PROJECT_NAME} PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY
        "${CMAKE_CURRENT_SOURCE_DIR}"
    )
    set_property(
        DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}
        PROPERTY
        VS_STARTUP_PROJECT
        ${PROJECT_NAME}
    )
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/LTCG")
endif()