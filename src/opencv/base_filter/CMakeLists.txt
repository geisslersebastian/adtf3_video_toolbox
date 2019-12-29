project(opencv_base_filter)

find_package(ADTF COMPONENTS filtersdk REQUIRED)
find_package(OpenCV REQUIRED)

set (PROJECT_NAME opencv_base_filter)

add_library(${PROJECT_NAME} STATIC
            opencv_base_filter.cpp
            opencv_sample.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC
                ${OpenCV_INCLUDE_DIRS}
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_link_libraries(${PROJECT_NAME} PUBLIC
            opencv_core
            adtf::filtersdk)

set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER opencv)

FILE(GLOB OPENCV_DLL "${OpenCV_DIR}/x64/*/bin/*.dll")

foreach(source_file ${OPENCV_DLL})
    file(COPY ${source_file}
        DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
endforeach()

install(FILES ${OPENCV_DLL} DESTINATION bin CONFIGURATIONS RelWithDebInfo Release)
install(FILES ${OPENCV_DLL} DESTINATION bin/debug CONFIGURATIONS Debug)
