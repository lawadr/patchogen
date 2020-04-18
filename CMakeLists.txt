cmake_minimum_required(VERSION 3.15)
include(FetchContent)

project(patchogen)

set(PRODUCT_NAME "" CACHE STRING "Name of the product")
set(EXECUTABLE_NAME patch CACHE STRING "Name of the patch executable")

set(OLD_VERSION "" CACHE STRING "Old version number")
set(OLD_PATH ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "Path to the old version")

set(NEW_VERSION "" CACHE STRING "New version number")
set(NEW_PATH ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "Path to the new version")

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

FetchContent_Declare(
    zlib
    GIT_REPOSITORY https://github.com/madler/zlib.git
    GIT_TAG v1.2.11
)

FetchContent_GetProperties(zlib)

if(NOT zlib_POPULATED)
    FetchContent_Populate(zlib)
    add_subdirectory(${zlib_SOURCE_DIR} ${zlib_BINARY_DIR})
	target_include_directories(zlibstatic INTERFACE ${zlib_SOURCE_DIR} ${zlib_BINARY_DIR})
    target_compile_definitions(zlibstatic PUBLIC ZLIB_CONST)
endif()

FetchContent_Declare(
    bsdiff
    GIT_REPOSITORY https://github.com/drake127/bsdiff.git
    GIT_TAG origin/cpp-compatibility
)

FetchContent_GetProperties(bsdiff)

if(NOT bsdiff_POPULATED)
    FetchContent_Populate(bsdiff)
    add_library(bsdiff STATIC)
    set_target_properties(bsdiff PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${bsdiff_BINARY_DIR})
    target_sources(bsdiff PRIVATE ${bsdiff_SOURCE_DIR}/bsdiff.c ${bsdiff_SOURCE_DIR}/bspatch.c)
    target_include_directories(bsdiff INTERFACE ${bsdiff_SOURCE_DIR})
endif()

add_library(common STATIC)
target_sources(common PRIVATE common.cpp common.h)
target_link_libraries(common PRIVATE zlibstatic bsdiff)

add_executable(zip)
target_sources(zip PRIVATE zip.cpp)
target_link_libraries(zip PRIVATE common)

add_executable(diff)
target_sources(diff PRIVATE diff.cpp)
target_link_libraries(diff PRIVATE common)

add_executable(crc)
target_sources(crc PRIVATE crc.cpp)
target_link_libraries(crc PRIVATE common)

add_executable(wrap)
target_sources(wrap PRIVATE wrap.cpp)

set(GEN_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(GEN_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/src)

add_executable(patch)
set_target_properties(patch PROPERTIES OUTPUT_NAME ${EXECUTABLE_NAME})
target_sources(patch PRIVATE patch.cpp patch.h)
target_link_libraries(patch PRIVATE common)
target_include_directories(patch PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${GEN_SRC_DIR})
set_target_properties(patch PROPERTIES CXX_STANDARD 17)

configure_file(config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
target_include_directories(patch PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_sources(patch PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/config.h)

file(GLOB_RECURSE OLD_FILES RELATIVE ${OLD_PATH} CONFIGURE_DEPENDS ${OLD_PATH}/*)
file(GLOB_RECURSE NEW_FILES RELATIVE ${NEW_PATH} CONFIGURE_DEPENDS ${NEW_PATH}/*)

# Removed files
foreach(FILE ${OLD_FILES})
    if (NOT ${FILE} IN_LIST NEW_FILES)
        message(STATUS "Removed file: " ${FILE})
        string(MAKE_C_IDENTIFIER g_${FILE} OBJECT_VAR)
        string(MAKE_C_IDENTIFIER g_${FILE}_bin BINARY_VAR)
        set(BINARY_HEADER ${FILE}.h)

        add_custom_command(
            OUTPUT ${GEN_BIN_DIR}/${FILE}.bin
            COMMAND crc ${OLD_PATH}/${FILE} ${GEN_BIN_DIR}/${FILE}.bin
            MAIN_DEPENDENCY ${OLD_PATH}/${FILE}
            DEPENDS crc
        )

        add_custom_command(
            OUTPUT ${GEN_SRC_DIR}/${FILE}.h
            COMMAND wrap ${GEN_BIN_DIR}/${FILE}.bin ${GEN_SRC_DIR}/${FILE}.h ${BINARY_VAR}
            MAIN_DEPENDENCY ${GEN_BIN_DIR}/${FILE}.bin
            DEPENDS wrap
        )

        configure_file(removed.cpp.in ${GEN_SRC_DIR}/${FILE}.cpp)
        target_sources(patch PRIVATE ${GEN_SRC_DIR}/${FILE}.cpp ${GEN_SRC_DIR}/${FILE}.h)
    endif()
endforeach()

#Added files
foreach(FILE ${NEW_FILES})
    if (NOT ${FILE} IN_LIST OLD_FILES)
        message(STATUS "Added file: " ${FILE})
        string(MAKE_C_IDENTIFIER g_${FILE} OBJECT_VAR)
        string(MAKE_C_IDENTIFIER g_${FILE}_bin BINARY_VAR)
        set(BINARY_HEADER ${FILE}.h)
    
        add_custom_command(
            OUTPUT ${GEN_BIN_DIR}/${FILE}.bin
            COMMAND zip ${NEW_PATH}/${FILE} ${GEN_BIN_DIR}/${FILE}.bin
            DEPENDS zip
        )

        add_custom_command(
            OUTPUT ${GEN_SRC_DIR}/${FILE}.h
            COMMAND wrap ${GEN_BIN_DIR}/${FILE}.bin ${GEN_SRC_DIR}/${FILE}.h ${BINARY_VAR}
            MAIN_DEPENDENCY ${GEN_BIN_DIR}/${FILE}.bin
            DEPENDS wrap
        )

        configure_file(added.cpp.in ${GEN_SRC_DIR}/${FILE}.cpp)
        target_sources(patch PRIVATE ${GEN_SRC_DIR}/${FILE}.cpp ${GEN_SRC_DIR}/${FILE}.h)
    endif()
endforeach()

# Modified files
foreach(FILE ${OLD_FILES})
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E compare_files ${OLD_PATH}/${FILE} ${NEW_PATH}/${FILE}
        RESULT_VARIABLE RESULT
		OUTPUT_QUIET ERROR_QUIET
    )

    if(${FILE} IN_LIST NEW_FILES AND RESULT)
        message(STATUS "Modified file: " ${FILE})
        string(MAKE_C_IDENTIFIER g_${FILE} OBJECT_VAR)
        string(MAKE_C_IDENTIFIER g_${FILE}_bin BINARY_VAR)
        string(MAKE_C_IDENTIFIER g_${FILE}_crc CRC_VAR)
        set(BINARY_HEADER ${FILE}.h)
        set(CRC_HEADER ${FILE}.crc.h)

        add_custom_command(
            OUTPUT ${GEN_BIN_DIR}/${FILE}.bin
            COMMAND diff ${OLD_PATH}/${FILE} ${NEW_PATH}/${FILE} ${GEN_BIN_DIR}/${FILE}.bin
            DEPENDS diff
        )

        add_custom_command(
            OUTPUT ${GEN_SRC_DIR}/${FILE}.h
            COMMAND wrap ${GEN_BIN_DIR}/${FILE}.bin ${GEN_SRC_DIR}/${FILE}.h ${BINARY_VAR}
            MAIN_DEPENDENCY ${GEN_BIN_DIR}/${FILE}.bin
            DEPENDS wrap
        )

        add_custom_command(
            OUTPUT ${GEN_BIN_DIR}/${FILE}.crc 
            COMMAND crc ${OLD_PATH}/${FILE} ${GEN_BIN_DIR}/${FILE}.crc
            MAIN_DEPENDENCY ${OLD_PATH}/${FILE}
            DEPENDS crc
		)

        add_custom_command(
            OUTPUT  ${GEN_SRC_DIR}/${FILE}.crc.h
            COMMAND wrap ${GEN_BIN_DIR}/${FILE}.crc ${GEN_SRC_DIR}/${FILE}.crc.h ${CRC_VAR}
            MAIN_DEPENDENCY ${GEN_BIN_DIR}/${FILE}.crc
            DEPENDS wrap
        )

        configure_file(modified.cpp.in ${GEN_SRC_DIR}/${FILE}.cpp)
        target_sources(patch PRIVATE ${GEN_SRC_DIR}/${FILE}.cpp ${GEN_SRC_DIR}/${FILE}.h ${GEN_SRC_DIR}/${FILE}.crc.h)
    endif()
endforeach()
