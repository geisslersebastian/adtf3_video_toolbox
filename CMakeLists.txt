project(ADTF3_Video_Toolbox)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

if (CONAN_COMPILER)
    include(${CMAKE_CURRENT_SOURCE_DIR}/../conanbuildinfo.cmake)
    conan_basic_setup(TARGETS NO_OUTPUT_DIRS)
endif()

add_subdirectory(src)