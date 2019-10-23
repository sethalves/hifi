include(vcpkg_common_functions)
set(VIDEO_CODEC_SDK_VERSION 9.1.23)
set(MASTER_COPY_SOURCE_PATH ${CURRENT_BUILDTREES_DIR}/src)

if (ANDROID)
elseif (WIN32)
    vcpkg_download_distfile(
        VIDEO_CODEC_SDK_SOURCE_ARCHIVE
        URLS https://hifi-public.s3.amazonaws.com/seth/video-codec-sdk-9.1.23-windows.tar.gz
        SHA512 c61900b5191907cfa7af3090523d79cefb3998f53f2d896efeefb237009c223cc24ce5767d279da9ddf0baea578007a5ae9797fd5d353cd3cd55775f865cee73
        FILENAME video-codec-sdk-9.1.23-windows.tar.gz
    )
elseif (APPLE)
else ()
    # else Linux desktop
    vcpkg_download_distfile(
        VIDEO_CODEC_SDK_SOURCE_ARCHIVE
        URLS https://hifi-public.s3.amazonaws.com/seth/video-codec-sdk-9.1.23-linux.tar.gz
        SHA512 d6634ed7a5b3b10b3f70e3fdaae8ea973cc307ecef8df840453b4a19cf74505fe4c79e4210cd2d4c314cc1257d8f6bf8f031a2c8c6fa770d17371684d00ada17
        FILENAME video-codec-sdk-9.1.23-linux.tar.gz
    )
endif ()


if (VIDEO_CODEC_SDK_SOURCE_ARCHIVE)
    vcpkg_extract_source_archive(${VIDEO_CODEC_SDK_SOURCE_ARCHIVE})
    file(COPY ${MASTER_COPY_SOURCE_PATH}/video-codec-sdk/include DESTINATION ${CURRENT_PACKAGES_DIR})
    file(COPY ${MASTER_COPY_SOURCE_PATH}/video-codec-sdk/lib DESTINATION ${CURRENT_PACKAGES_DIR})
    file(COPY ${MASTER_COPY_SOURCE_PATH}/video-codec-sdk/share DESTINATION ${CURRENT_PACKAGES_DIR})
    file(COPY ${MASTER_COPY_SOURCE_PATH}/video-codec-sdk/debug DESTINATION ${CURRENT_PACKAGES_DIR})
endif ()


# -- Linux --
# set -x
# mkdir ~/build/video_codec_sdk
# cd ~/build/video_codec_sdk
# unzip ~/Downloads/Video_Codec_SDK_9.1.23.zip
# rm -rf video-codec-sdk
# mkdir -p video-codec-sdk/include/video-codec-sdk/
# cp Video_Codec_SDK_9.1.23/include/* video-codec-sdk/include/video-codec-sdk/
# mkdir -p video-codec-sdk/lib
# cp Video_Codec_SDK_9.1.23/Lib/linux/stubs/x86_64/* video-codec-sdk/lib/
# mkdir -p video-codec-sdk/debug/lib
# cp Video_Codec_SDK_9.1.23/Lib/linux/stubs/x86_64/* video-codec-sdk/debug/lib/
# mkdir -p video-codec-sdk/share/video-codec-sdk
# cp Video_Codec_SDK_9.1.23/LicenseAgreement.pdf video-codec-sdk/share/video-codec-sdk/copyright
#
# tar cf video-codec-sdk.tar video-codec-sdk
# gzip -9 video-codec-sdk.tar
# mv video-codec-sdk.tar.gz video-codec-sdk-9.1.23-linux.tar.gz
# sha512sum video-codec-sdk-9.1.23-linux.tar.gz
# set +x

# -- Windows --
# set -x
# mkdir ~/build/video_codec_sdk
# cd ~/build/video_codec_sdk
# unzip ~/Downloads/Video_Codec_SDK_9.1.23.zip
# rm -rf video-codec-sdk
# mkdir -p video-codec-sdk/include/video-codec-sdk/
# cp Video_Codec_SDK_9.1.23/include/* video-codec-sdk/include/video-codec-sdk/
# mkdir -p video-codec-sdk/lib
# cp Video_Codec_SDK_9.1.23/Lib/Win32/* video-codec-sdk/lib/
# mkdir -p video-codec-sdk/debug/lib
# cp Video_Codec_SDK_9.1.23/Lib/Win32/* video-codec-sdk/debug/lib/
# mkdir -p video-codec-sdk/share/video-codec-sdk
# cp Video_Codec_SDK_9.1.23/LicenseAgreement.pdf video-codec-sdk/share/video-codec-sdk/copyright
#
# tar cf video-codec-sdk.tar video-codec-sdk
# gzip -9 video-codec-sdk.tar
# mv video-codec-sdk.tar.gz video-codec-sdk-9.1.23-windows.tar.gz
# sha512sum video-codec-sdk-9.1.23-windows.tar.gz
# set +x
