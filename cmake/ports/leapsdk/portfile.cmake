include(vcpkg_common_functions)
set(LEAPSDK_VERSION 4.0.0+52173)
set(MASTER_COPY_SOURCE_PATH ${CURRENT_BUILDTREES_DIR}/src)

if (WIN32)
    vcpkg_download_distfile(
        LEAPSDK_SOURCE_ARCHIVE
        URLS https://hifi-public.s3.amazonaws.com/seth/leapsdk-4.0.0%2B52173-windows.zip
        SHA512 a6b1553ceeee10cffa46ea93c1da29606a4c06ea93a2e3d19a124ff6fd7f8b6cff1e0fc0fba6c47476217dec491f1112aa6342a4aecb25bf4bc6f502fb8c245b
        FILENAME leapsdk-4.0.0+52173-windows.zip
    )

    vcpkg_extract_source_archive(${LEAPSDK_SOURCE_ARCHIVE})
    file(COPY ${MASTER_COPY_SOURCE_PATH}/leapsdk/include DESTINATION ${CURRENT_PACKAGES_DIR})
    file(COPY ${MASTER_COPY_SOURCE_PATH}/leapsdk/lib DESTINATION ${CURRENT_PACKAGES_DIR})
    file(COPY ${MASTER_COPY_SOURCE_PATH}/leapsdk/debug DESTINATION ${CURRENT_PACKAGES_DIR})
    file(COPY ${MASTER_COPY_SOURCE_PATH}/leapsdk/bin DESTINATION ${CURRENT_PACKAGES_DIR})
    file(COPY ${MASTER_COPY_SOURCE_PATH}/leapsdk/share DESTINATION ${CURRENT_PACKAGES_DIR})

endif ()
