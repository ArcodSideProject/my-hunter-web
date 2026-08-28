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

# Box2D v3, used for real barrel physics (gravity, floor contact, walls,
# barrel-vs-barrel collision) -- the original CSFML game's hand-rolled
# gravity/bounce/friction code had no collision between barrels at all.
find_package(box2d QUIET)
if(NOT box2d_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        box2d
        GIT_REPOSITORY https://github.com/erincatto/box2d.git
        GIT_TAG v3.1.1
    )
    set(BOX2D_SAMPLES OFF CACHE BOOL "" FORCE)
    set(BOX2D_UNIT_TESTS OFF CACHE BOOL "" FORCE)
    set(BOX2D_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(box2d)
endif()

if(EMSCRIPTEN)
    add_executable(my_hunter ${SOURCES})
    target_link_libraries(my_hunter raylib box2d)

    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    target_link_options(my_hunter PRIVATE
        "-sUSE_GLFW=3"
        "-sASYNCIFY"
        "-sTOTAL_MEMORY=134217728"
        "-sFORCE_FILESYSTEM=1"
        "--shell-file=${CMAKE_SOURCE_DIR}/web/shell.html"
        "--preload-file=${CMAKE_SOURCE_DIR}/assets@assets"
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
    target_link_libraries(my_hunter raylib box2d)

    if(APPLE)
        target_link_libraries(my_hunter "-framework IOKit" "-framework Cocoa" "-framework OpenGL")
    endif()

    # copy assets next to the built binary so the relative "assets/..."
    # paths used in LoadGameAssets() resolve at runtime
    add_custom_command(TARGET my_hunter POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:my_hunter>/assets
    )
endif()
