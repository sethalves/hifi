include(vcpkg_common_functions)
set(SRANIPAL_VERSION 1.0.1.0)
set(MASTER_COPY_SOURCE_PATH ${CURRENT_BUILDTREES_DIR}/src)

if (WIN32)
    vcpkg_download_distfile(
        SRANIPAL_SOURCE_ARCHIVE
        URLS https://hifi-public.s3.amazonaws.com/seth/sranipal-1.0.1.0-windows.zip
        SHA512 8b7cb175f33f14b02159f9215f2791f87b067869d837d1a51d7895cf880d282515860bd850ac74d3491534c92bcd4473bb7884e68a05847689c56469fb4e122d
        FILENAME sranipal-1.0.1.0-windows.zip
    )

    vcpkg_extract_source_archive(${SRANIPAL_SOURCE_ARCHIVE})
    file(COPY ${MASTER_COPY_SOURCE_PATH}/sranipal/include DESTINATION ${CURRENT_PACKAGES_DIR})
    file(COPY ${MASTER_COPY_SOURCE_PATH}/sranipal/lib DESTINATION ${CURRENT_PACKAGES_DIR})
    file(COPY ${MASTER_COPY_SOURCE_PATH}/sranipal/debug DESTINATION ${CURRENT_PACKAGES_DIR})
    file(COPY ${MASTER_COPY_SOURCE_PATH}/sranipal/bin DESTINATION ${CURRENT_PACKAGES_DIR})
    file(COPY ${MASTER_COPY_SOURCE_PATH}/sranipal/share DESTINATION ${CURRENT_PACKAGES_DIR})

endif ()
