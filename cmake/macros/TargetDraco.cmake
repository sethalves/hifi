macro(TARGET_DRACO)
    set(LIBS draco dracodec dracoenc)
    find_library(LIBPATH ${LIB} PATHS )
    if (ANDROID)
        set(INSTALL_DIR ${HIFI_ANDROID_PRECOMPILED}/draco)
        set(DRACO_INCLUDE_DIRS "${INSTALL_DIR}/include" CACHE STRING INTERNAL)
        set(LIB_DIR ${INSTALL_DIR}/lib)
        list(APPEND DRACO_LIBRARIES ${LIB_DIR}/libdraco.a)
        list(APPEND DRACO_LIBRARIES ${LIB_DIR}/libdracodec.a)
        list(APPEND DRACO_LIBRARIES ${LIB_DIR}/libdracoenc.a)
        target_link_libraries(${TARGET_NAME} ${DRACO_LIBRARIES})

    elseif (DISABLE_VCPKG)

        set(LIB_SEARCH_PATH_RELEASE "")
        set(LIB_SEARCH_PATH_DEBUG "")
        foreach(LIB ${LIBS}) 
            find_library(${LIB}_LIBPATH ${LIB})
            list(APPEND DRACO_LIBRARY_RELEASE ${${LIB}_LIBPATH})
            find_library(${LIB}D_LIBPATH ${LIB})
            list(APPEND DRACO_LIBRARY_DEBUG ${${LIB}D_LIBPATH})
        endforeach()
        select_library_configurations(DRACO)
        target_link_libraries(${TARGET_NAME} ${DRACO_LIBRARY})

    else()
        set(LIB_SEARCH_PATH_RELEASE ${VCPKG_INSTALL_ROOT}/lib/)
        set(LIB_SEARCH_PATH_DEBUG ${VCPKG_INSTALL_ROOT}/debug/lib/)
        foreach(LIB ${LIBS}) 
            find_library(${LIB}_LIBPATH ${LIB} PATHS ${LIB_SEARCH_PATH_RELEASE} NO_DEFAULT_PATH)
            list(APPEND DRACO_LIBRARY_RELEASE ${${LIB}_LIBPATH})
            find_library(${LIB}D_LIBPATH ${LIB} PATHS ${LIB_SEARCH_PATH_DEBUG} NO_DEFAULT_PATH)
            list(APPEND DRACO_LIBRARY_DEBUG ${${LIB}D_LIBPATH})
        endforeach()
        select_library_configurations(DRACO)
        target_link_libraries(${TARGET_NAME} ${DRACO_LIBRARY})
    endif()
endmacro()
