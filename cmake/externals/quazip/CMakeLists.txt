set(EXTERNAL_NAME quazip)

# we need to find qmake inside QT_DIR
find_program(QMAKE_COMMAND NAME qmake PATHS /usr/bin ${QT_DIR}/bin $ENV{QTTOOLDIR} NO_DEFAULT_PATH)

if (NOT QMAKE_COMMAND)
  message(FATAL_ERROR "Could not find qmake. Qxmpp cannot be compiled without qmake.")
endif ()

if (ANDROID)
  set(ANDROID_CMAKE_ARGS "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}" "-DANDROID_NATIVE_API_LEVEL=19")
endif ()

include(ExternalProject)

# URL http://hifi-public.s3.amazonaws.com/dependencies/quazip-0.7.1.tar.gz
# URL_MD5 e4a14ad41b9a1ce494f8dd9add56429e

SET(QUAZIP_URL http://headache.hungry.com/~seth/hifi/quazip-0.7.1.tar.gz CACHE STRING "url for QuaZIP")
SET(QUAZIP_MD5 "2a9b985d10b29347873186d8bf3f049d" CACHE STRING "md5 for QuaZIP")

if (WIN32)
  if(EXISTS "${QT_DIR}/include/QtZlib/zlib.h")
    SET(ZLIB_INCLUDE_DIRS "${QT_DIR}/include/QtZlib" CACHE STRING "Path to ZLIB headers of Qt")
  else()
    SET(ZLIB_INCLUDE_DIRS $ENV{HIFI_LIB_DIR}/zlib/include CACHE STRING "Path to ZLIB headers of Qt")
  endif()

  ExternalProject_Add(
    ${EXTERNAL_NAME}
    URL ${QUAZIP_URL}
    URL_MD5 ${QUAZIP_MD5}
    # CONFIGURE_COMMAND ${QMAKE_COMMAND} PREFIX=<INSTALL_DIR>
    # CMAKE_ARGS ${ANDROID_CMAKE_ARGS} -DZLIB_INCLUDE_DIRS=${ZLIB_INCLUDE_DIRS} -DZLIB_LIBRARIES=${ZLIB_LIBRARIES} -DQt5Core_DIR=${QT_DIR}/lib/cmake/Qt5Core -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    CMAKE_ARGS ${ANDROID_CMAKE_ARGS} -DZLIB_INCLUDE_DIRS=${ZLIB_INCLUDE_DIRS} -DQt5Core_DIR=${QT_DIR}/lib/cmake/Qt5Core -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> -DQT_ROOT=${QT_DIR}
    BINARY_DIR ${EXTERNAL_PROJECT_PREFIX}/build
    LOG_DOWNLOAD 1
    LOG_CONFIGURE 1
    LOG_BUILD 1
  )
else()
  ExternalProject_Add(
    ${EXTERNAL_NAME}
    URL ${QUAZIP_URL}
    URL_MD5 ${QUAZIP_MD5}
    # CONFIGURE_COMMAND ${QMAKE_COMMAND} PREFIX=<INSTALL_DIR>
    CMAKE_ARGS ${ANDROID_CMAKE_ARGS} -DQt5Core_DIR=${QT_DIR}/lib/cmake/Qt5Core -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    BINARY_DIR ${EXTERNAL_PROJECT_PREFIX}/build
    LOG_DOWNLOAD 1
    LOG_CONFIGURE 1
    LOG_BUILD 1
  )
endif()

# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")

if (APPLE)
  set(${EXTERNAL_NAME}_DEPENDENCY_QT_MODULES ${ARGN})
  list(APPEND ${EXTERNAL_NAME}_DEPENDENCY_QT_MODULES Core)
  foreach(QT_MODULE ${${EXTERNAL_NAME}_DEPENDENCY_QT_MODULES})
    target_link_libraries(${EXTERNAL_NAME} Qt5::${QT_MODULE})
  endforeach()
endif()

ExternalProject_Get_Property(${EXTERNAL_NAME} INSTALL_DIR)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

if (WIN32)
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG ${INSTALL_DIR}/lib/Debug/QUAZIP_LIB.lib CACHE FILEPATH "Path to QuaZIP debug library")
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${INSTALL_DIR}/lib/Release/QUAZIP_LIB.lib CACHE FILEPATH "Path to QuaZIP release library")
else ()
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG "" CACHE FILEPATH "Path to QuaZIP debug library")
  set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${INSTALL_DIR}/lib/libquazip.so CACHE FILEPATH "Path to QuaZIP release library")
endif ()

set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${INSTALL_DIR}/include CACHE FILEPATH "Path to QuaZIP include directory")
