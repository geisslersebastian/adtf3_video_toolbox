project(image_source)

find_package(ADTF COMPONENTS filtersdk REQUIRED)
find_package(OpenCV REQUIRED)

set (PROJECT_NAME image_source)

adtf_add_filter(${PROJECT_NAME}
                image_source.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE 
				opencv_base_filter
				opencv_videoio
				opencv_highgui)

adtf_install_filter(${PROJECT_NAME} bin )

set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER opencv)

adtf_create_plugindescription(
    TARGET 
        ${PROJECT_NAME}
    PLUGIN_SUBDIR 
        "bin"
)
