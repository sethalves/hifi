include(ExternalProject)
include(SelectLibraryConfigurations)

set(EXTERNAL_NAME leopoly)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

if (WIN32)

  ExternalProject_Add(
        ${EXTERNAL_NAME}
        URL https://s3.amazonaws.com/hifi-public/dependencies/LeoPolySDK-3.zip
        URL_MD5 cdbf0668c75c4b8e68b1d11ace97f26a
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        LOG_DOWNLOAD 1
  )

  # Hide this external target (for ide users)
  set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")
  ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)
  set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR} CACHE TYPE INTERNAL)
  set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${SOURCE_DIR}/SculptAppDLL.lib CACHE TYPE INTERNAL)

  set(${EXTERNAL_NAME_UPPER}_DLL_PATH "${SOURCE_DIR}" CACHE TYPE INTERNAL)

elseif (APPLE)

  ExternalProject_Add(
        ${EXTERNAL_NAME}
        URL https://s3.amazonaws.com/hifi-public/dependencies/LeoPolySDK-Stub-3.zip
        URL_MD5 debaf95ff5cc470e496790c9a7fae19a
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        LOG_DOWNLOAD 1
  )

  # Hide this external target (for ide users)
  set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")
  ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)
  set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR} CACHE TYPE INTERNAL)

elseif (ANDROID)
else () # Linux

  ExternalProject_Add(
        ${EXTERNAL_NAME}
        URL https://s3.amazonaws.com/hifi-public/dependencies/LeoPolySDK-Stub-3.zip
        URL_MD5 debaf95ff5cc470e496790c9a7fae19a
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        LOG_DOWNLOAD 1
  )

  # Hide this external target (for ide users)
  set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")
  ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)
  set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR} CACHE TYPE INTERNAL)

endif()
