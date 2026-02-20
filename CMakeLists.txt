cmake_minimum_required(VERSION 4.2.0)
project(prism C)

set(CMAKE_C_STANDARD 11)

# Collect all .c files recursively
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.c"
)

add_executable(${PROJECT_NAME} ${SOURCES})

# good to have
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include
)

# Link libraylib and libm
target_link_libraries(${PROJECT_NAME} raylib m)

target_compile_options(prism PRIVATE
    -O3 -flto -fno-math-errno -fomit-frame-pointer -s
)

