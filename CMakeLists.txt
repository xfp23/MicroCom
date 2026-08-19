project(MICROCOM C)

file(GLOB_RECURSE MICROCOM_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

add_library(MICROCOM STATIC
    ${MICROCOM_SOURCES}
)


target_include_directories(MICROCOM PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(MICROCOM PRIVATE
    "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>"
)