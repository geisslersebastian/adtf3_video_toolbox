cmake_minimum_required(VERSION 3.10.0)
project(opencv_filter_tester)

if (NOT TARGET adtf::testing)
    find_package(ADTF COMPONENTS filtersdk testing)
endif()

find_package(OpenCV REQUIRED)


adtf_add_catch_test(NAME opencv_filter_tester 
                    TIMEOUT 10
                    SOURCES opencv_filter_tester.cpp
                    ADDITIONAL_PLUGIN_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/bin$<$<CONFIG:Debug>:/debug>"
                    WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}/bin$<$<CONFIG:Debug>:/debug>")

target_link_libraries(opencv_filter_tester PRIVATE opencv_base_filter adtf::filtersdk ${OpenCV_LIBS})
target_include_directories(${PROJECT_NAME} PRIVATE
            ${OpenCV_INCLUDE_DIRS})

set_property(TARGET opencv_filter_tester PROPERTY FOLDER opencv/tests)
  