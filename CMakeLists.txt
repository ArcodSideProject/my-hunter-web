cmake_minimum_required(VERSION 3.15)
project(my_hunter_web C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(SOURCES
    src/main.c
    src/game.c
)

if(EMSCRIPTEN)
    add_executable(my_hunter ${SOURCES})
    target_link_libraries(my_hunter raylib)

    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    target_link_options(my_hunter PRIVATE
        "-sUSE_GLFW=3"
        "-sASYNCIFY"
        "-sTOTAL_MEMORY=67108864"
        "-sFORCE_FILESYSTEM=1"
        "--shell-file=${CMAKE_SOURCE_DIR}/web/shell.html"
    )
else()
    find_package(raylib QUIET)
    if(NOT raylib_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            raylib
            GIT_REPOSITORY https://github.com/raysan5/raylib.git
            GIT_TAG 5.5
        )
        FetchContent_MakeAvailable(raylib)
    endif()

    add_executable(my_hunter ${SOURCES})
    target_link_libraries(my_hunter raylib)

    if(APPLE)
        target_link_libraries(my_hunter "-framework IOKit" "-framework Cocoa" "-framework OpenGL")
    endif()
endif()
