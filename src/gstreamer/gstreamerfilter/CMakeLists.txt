project (GStreamerFilter)

find_package(ADTF COMPONENTS filtersdk)

set (PROJECT_NAME gstreamer_filter)


adtf_add_filter(${PROJECT_NAME}
                gstreamer_reflection.h
                gstreamer_filter.cpp
                gstreamer_base.h
                gstreamer_appsink.h
                gstreamer_appsource.h
                gstreamer_service.h)

if (WIN32)
    set (GSTREAMER_DIR "C:/gstreamer/1.0/x86_64")

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
else (WIN32)
    set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH ON)
    find_package(PkgConfig REQUIRED)
    if ( NOT (PKGCONFIG_FOUND))
          message(FATAL_ERROR "Please Install PkgConfig")
    endif()
    pkg_check_modules(GST REQUIRED gstreamer-1.0>=1.8)
    if ( NOT (GST_FOUND))
          message(FATAL_ERROR "Please Install Gstreamer Dev: CMake will Exit")
    endif()
    set(ENV{PKG_CONFIG_PATH})
    
    target_include_directories(${PROJECT_NAME} PRIVATE ${GST_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${GST_LIBRARIES})
    
endif (WIN32)

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


