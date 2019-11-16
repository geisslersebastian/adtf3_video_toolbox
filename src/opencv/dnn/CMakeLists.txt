project (DnnFilter)

find_package(ADTF COMPONENTS filtersdk REQUIRED)
find_package(OpenCV REQUIRED)

set (PROJECT_NAME dnn_filter)

adtf_add_filter(${PROJECT_NAME}
                dnn_filter.cpp)


target_include_directories(${PROJECT_NAME} PRIVATE
            ${OpenCV_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} PRIVATE
            ${OpenCV_LIBS})

adtf_install_filter(${PROJECT_NAME} bin )

set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER opencv)

adtf_create_plugindescription(
    TARGET 
        ${PROJECT_NAME}
    PLUGIN_SUBDIR 
        "bin"
)

# We use the installed gstreamer libs 
#FILE(GLOB DLLS ${CMAKE_CURRENT_SOURCE_DIR}/../gstreamer_1.0/x86_64/bin/*.dll)
#install(FILES ${DLLS}
#              DESTINATION bin/debug CONFIGURATIONS Debug)
#install(FILES ${DLLS}
#              DESTINATION bin/ CONFIGURATIONS RelWithDebInfo)


