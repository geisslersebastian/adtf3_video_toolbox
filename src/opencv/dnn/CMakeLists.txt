project (DnnFilter)

find_package(ADTF COMPONENTS filtersdk REQUIRED)
find_package(OpenCV REQUIRED)

set (PROJECT_NAME opencv_filter)

adtf_add_filter(${PROJECT_NAME}
				opencv_sample.h
                dnn_filter.cpp)


message("Include ${OpenCV_INCLUDE_DIRS}")
message("Libs ${OpenCV_LIBS}")

target_include_directories(${PROJECT_NAME} PRIVATE
            ${OpenCV_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} PRIVATE
            opencv_core
			opencv_dnn)

adtf_install_filter(${PROJECT_NAME} bin )

set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER opencv)

adtf_create_plugindescription(
    TARGET 
        ${PROJECT_NAME}
    PLUGIN_SUBDIR 
        "bin"
)

FILE(GLOB OPENCV_DLL "${OpenCV_DIR}/x64/*/bin/*.dll")
install(FILES ${OPENCV_DLL} DESTINATION bin CONFIGURATIONS RelWithDebInfo Release)
install(FILES ${OPENCV_DLL} DESTINATION bin/debug CONFIGURATIONS Debug)



