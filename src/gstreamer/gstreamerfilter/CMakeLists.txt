project (GStreamerFilter)

find_package(ADTF COMPONENTS filtersdk)

set (PROJECT_NAME gstreamer_filter)
set (GSTREAMER_DIR "C:/gstreamer/1.0/x86_64")

adtf_add_filter(${PROJECT_NAME}
                gstreamer_reflection.h
                gstreamer_filter.cpp)


target_include_directories(${PROJECT_NAME} PRIVATE
            ${GSTREAMER_DIR}/include/gstreamer-1.0
            ${GSTREAMER_DIR}/include/glib-2.0
            ${GSTREAMER_DIR}/lib/glib-2.0/include)

target_link_libraries(${PROJECT_NAME} PRIVATE
            ${GSTREAMER_DIR}/lib/gstreamer-1.0.lib
            ${GSTREAMER_DIR}/lib/gobject-2.0.lib  
            ${GSTREAMER_DIR}/lib/glib-2.0.lib
            ${GSTREAMER_DIR}/lib/gstvideo-1.0.lib
            ${GSTREAMER_DIR}/lib/gstapp-1.0.lib)

adtf_install_filter(${PROJECT_NAME} bin)

set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER gstreamer)

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


