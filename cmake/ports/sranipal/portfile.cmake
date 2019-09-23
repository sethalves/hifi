include(vcpkg_common_functions)
set(SRANIPAL_VERSION 1.1.0.1)
set(MASTER_COPY_SOURCE_PATH ${CURRENT_BUILDTREES_DIR}/src)

if (WIN32)
    vcpkg_download_distfile(
        SRANIPAL_SOURCE_ARCHIVE
        URLS https://hifi-public.s3.amazonaws.com/seth/sranipal-1.1.0.1-windows.zip
        SHA512 c5a339506bdac9735c98d6405816800ea8f14b4338b2f86e57e4b033e4999052154a5f9b9e9ad7ce1140e8f3d795868fd1841c70a34ded69d9b7cb26c74540de
        FILENAME sranipal-1.1.0.1-windows.zip
    )

    vcpkg_extract_source_archive(${SRANIPAL_SOURCE_ARCHIVE})
    file(COPY ${MASTER_COPY_SOURCE_PATH}/sranipal/include DESTINATION ${CURRENT_PACKAGES_DIR})
    file(COPY ${MASTER_COPY_SOURCE_PATH}/sranipal/lib DESTINATION ${CURRENT_PACKAGES_DIR})
    file(COPY ${MASTER_COPY_SOURCE_PATH}/sranipal/debug DESTINATION ${CURRENT_PACKAGES_DIR})
    file(COPY ${MASTER_COPY_SOURCE_PATH}/sranipal/bin DESTINATION ${CURRENT_PACKAGES_DIR})
    file(COPY ${MASTER_COPY_SOURCE_PATH}/sranipal/share DESTINATION ${CURRENT_PACKAGES_DIR})

endif ()
