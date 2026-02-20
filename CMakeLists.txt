cmake_minimum_required(VERSION 4.2.0)
project(prism C)

set(C_STANDARD 23)

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

#if ("${PLATFORM}" STREQUAL "Web")
#    add_custom_command(
#        TARGET ${PROJECT_NAME} PRE_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/resources $<TARGET_FILE_DIR:${PROJECT_NAME}>/../resources
#    )
#else()
#    add_custom_command(
#        TARGET ${PROJECT_NAME} POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/resources $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
#    )
#endif()

# Link libraylib and libm
target_link_libraries(${PROJECT_NAME} raylib m)

target_compile_options(prism PRIVATE
    -O3 -flto -fno-math-errno -fomit-frame-pointer -s
)

# target_compile_options(prism PRIVATE
#     -g -O1
#     -fsanitize=address,undefined
#     -fno-omit-frame-pointer
# )
    
# target_link_options(prism PRIVATE
#     -fsanitize=address,undefined
# )
