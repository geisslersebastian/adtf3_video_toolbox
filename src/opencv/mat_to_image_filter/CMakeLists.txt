project(mat_to_image_filter)

find_package(ADTF COMPONENTS filtersdk REQUIRED)
find_package(OpenCV REQUIRED)

set (PROJECT_NAME mat_to_image_filter)

adtf_add_filter(${PROJECT_NAME}
                image_to_mat_filter.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE 
				opencv_base_filter
				opencv_videoio)

adtf_install_filter(${PROJECT_NAME} bin )

set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER opencv)

adtf_create_plugindescription(
    TARGET 
        ${PROJECT_NAME}
    PLUGIN_SUBDIR 
        "bin"
)

add_subdirectory(test)
