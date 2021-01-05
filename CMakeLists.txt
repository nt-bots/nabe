cmake_minimum_required(VERSION 3.14)

#set(NABE_BUILD_VERSION 2021.01.01) # If you need to manually set this for local build, etc.
project(nabe VERSION $ENV{NABE_BUILD_VERSION})

set(NABE_COMMIT $ENV{NABE_THIS_COMMIT_SHA})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/NabeConfig.h.in ${CMAKE_CURRENT_SOURCE_DIR}/include/NabeConfig.h @ONLY)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)

if (WIN32)
    if(NOT DEFINED SQLite3_INCLUDE_DIR)
        set(SQLite3_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/thirdparty/sqlite-amalgamation)
    endif()
    
    if(NOT DEFINED SQLite3_LIBRARY)
        set(SQLite3_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/lib/thirdparty/$ENV{NABE_BUILD_ARCH}/sqlite3.lib)
    endif()
endif()

configure_file(include/NabeConfig.h.in include/NabeConfig.h)

add_executable(${PROJECT_NAME}
               main.cpp

               include/interrupt_handler.cpp
               include/nabe_keyvalues.cpp
               include/nabe_nav_coordinator.cpp
               include/nabe_pathfinder.cpp
               include/nav_parser.cpp
               include/print_helpers.cpp
               include/python_auto_initializer.cpp

               include/thirdparty/KeyValues/src/keyvalues.cpp

               include/thirdparty/source-sdk-stubs/nav.cpp
               include/thirdparty/source-sdk-stubs/nav_area.cpp
               include/thirdparty/source-sdk-stubs/nav_mesh.cpp
               include/thirdparty/source-sdk-stubs/nav_node.cpp
               include/thirdparty/source-sdk-stubs/nav_spot.cpp
               include/thirdparty/source-sdk-stubs/mathlib/math-pfns.cpp
)

#set(NABE_PYTHON_VERSION 3.9) # If you need to manually set this for local build, etc.
find_package(PythonLibs $ENV{NABE_PYTHON_VERSION} REQUIRED)

find_package(SQLite3 REQUIRED)
find_package(Threads REQUIRED)

target_include_directories(${PROJECT_NAME} PUBLIC
                          ${CMAKE_CURRENT_SOURCE_DIR}/include
                          ${CMAKE_CURRENT_BINARY_DIR}/include
                          ${PYTHON_INCLUDE_DIRS}
                          ${SQLite3_INCLUDE_DIRS}
)

if (MSVC)
target_link_libraries(${PROJECT_NAME}
                      Threads::Threads
                      ${PYTHON_LIBRARIES}
                      ${SQLite3_LIBRARIES}
)
else()
target_link_libraries(${PROJECT_NAME}
                      stdc++fs
                      Threads::Threads
                      ${PYTHON_LIBRARIES}
                      ${SQLite3_LIBRARIES}
)
endif()

include(FindFilesystem.cmake)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
if (MSVC)
    # Some libraries used query this define for std 17+ features support.
    # Need to set this flag manually on some MSVC versions to support it.
    target_compile_options(${PROJECT_NAME} PUBLIC
                           "/Zc:__cplusplus"
    )
    set(CMAKE_EXE_LINKER_FLAGS
        "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:libcmt.lib")
endif()
