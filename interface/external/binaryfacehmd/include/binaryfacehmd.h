/**************************************************************************************************
BINARYVR, INC. PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with BinaryVR, Inc. and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2016 BinaryVR, Inc. All Rights Reserved.
**************************************************************************************************/
#ifndef _BINARYVR_BINARYFACEHMD_H_
#define _BINARYVR_BINARYFACEHMD_H_

#ifndef _WIN32
# ifndef BINARYFACEHMD_API
#   define BINARYFACEHMD_API __attribute__((visibility("default")))
# endif // BINARYFACEHMD_API
#else
# //  TODO: support win32
#endif // _WIN32

/**
 *  BinaryFaceHMD API version 0.1
 */
#include <cstdint>

#define BINARYFACEHMD_SDK_DEFAULT_API_KEY "api_key"
#define BINARYFACEHMD_SDK_VERSION 0x0001

#ifndef BINARYFACEHMD_API
#  define BINARYFACEHMD_API extern
#endif  // BINARYFACEHMD_API

typedef enum _binaryfacehmd_ret {
  BINARYFACEHMD_OK = 0,                   /// Success
  BINARYFACEHMD_SDK_VERSION_MISMATCH = 1, /// binaryfacehmd.h is not up-to-date
  BINARYFACEHMD_FILE_NOT_FOUND = 2,       /// Data file is not found
  BINARYFACEHMD_FILE_CORRUPTED = 3,       /// Api key is not valid
  BINARYFACEHMD_INVALID_PARAMETER = 4,    /// Passed parameters are not valid

  BINARYFACEHMD_DEVICE_NOT_CONNECTED = 100, /// Failed to find a connected camera
  BINARYFACEHMD_DEVICE_OPEN_FAILED = 101,   /// Failed to open the camera device 
  BINARYFACEHMD_DEVICE_NOT_OPENED = 102,    /// The camera device is not open yet
  BINARYFACEHMD_FRAME_NOT_CAPTURED = 103,   /// Image frame is not captured 
  
  BINARYFACEHMD_PROCESSOR_READY = 200,          /// Processor is ready to start calibration or tracking
  BINARYFACEHMD_ON_CALIBRATION = 201,           /// Processor is running calibration
  BINARYFACEHMD_ON_TRACKING = 202,              /// Processor is tracking a user's facial expression 
  BINARYFACEHMD_TRACKING_START_FAILED = 203,    /// Failed to start tracking
  BINARYFACEHMD_TRACKING_NOT_STARTED_YET = 204, /// Tracking is not started yet

  BINARYFACEHMD_TRACKING_FAILED = 301,          /// Failed to track a user's facial expression for the current frame 

  BINARYFACEHMD_USER_MODEL_LOADED = 400,      /// User model is found in the cache directory
  BINARYFACEHMD_USER_MODEL_NOT_FOUND = 401,  /// User model is not found. Calibration should be done for accurate tracking

  BINARYFACEHMD_UNKNOWN_ERROR = 1000      /// Unknown error

} binaryfacehmd_ret;

typedef int32_t binaryfacehmd_context_t;

/**
*  sdk_version: SDK version. Should be equal to 'BINARYFACEHMD_SDK_VERSION' constant.
*  data_file_version: .bfh file version
*  num_blendshapes: Number of facial expressions the context supports.
*/
typedef struct _binaryfacehmd_context_info_t {
  int32_t sdk_version;
  int32_t data_file_version;
  int32_t num_blendshapes;
} binaryfacehmd_context_info_t;

/**
*  id for camera device
*  it has to be 0 as we currently assume there is only one pmd camera attached to the machine.
*  will be interpreted as the parameter 'fd' for android 
*/
typedef int32_t binaryfacehmd_device_id_t;

/**
*  general information of images captured by camera device
*  includes camera intrinsic parameters
*/
typedef struct _binaryfacehmd_image_info_t {
  int32_t width;
  int32_t height;
  int32_t num_bytes_per_pixel;
  float   focal_length;
  float   principal_point_x;
  float   principal_point_y;
} binaryfacehmd_image_info_t;

/**
*  confidence: face expression tracking score, which ranges between 0(fail) and 1(success). 
*/
typedef struct _binaryfacehmd_face_info_t {
  float confidence;
} binaryfacehmd_face_info_t;


#ifdef __cplusplus
extern "C" {
#endif

/**
 *  @brief  Open a BinaryFaceHMD context. You will need to call this function only once in your runtime.
 *          Error if BINARYFACEHMD_OK is not returned
 *
 *  @param  model_file_path .bfh model file path
 *  @param  cache_dir_path  directory path for storing user model files
 *  @param  api_key         BinaryFaceHMD API key of your app. See the license document.
 *  @param  sdk_version     SDK version. Simply pass 'BINARYFACEHMD_SDK_VERSION'. This function returns
 *                          BINARYFACEHMD_SDK_VERSION_MISMATCH if the SDK binary version mismatches with
 *                          this parameter.
 *  @param  context_out     The context created
 *  @param  info_out        The context information
 */
BINARYFACEHMD_API binaryfacehmd_ret binaryfacehmd_open_context(
  const char *model_file_path,
  const char *cache_dir_path,
  const char *api_key,
  int32_t sdk_version,
  binaryfacehmd_context_t *context_out,
  binaryfacehmd_context_info_t *info_out);

/**
*  @brief  Close the BinaryFaceHMD context and release all the resources allocated.
*          Error if BINARYFACEHMD_OK is not returned
*
*  @param  context_id BinaryFaceHMD context id
*/
BINARYFACEHMD_API binaryfacehmd_ret binaryfacehmd_close_context(
  binaryfacehmd_context_t context_id);

/**
*  @brief  Open camera device.
*          Error if BINARYFACEHMD_OK is not returned
*
*  @param  context_id     BinaryFaceHMD context id
*  @param  device_id      0 
*  @param  image_info_out The output image information captured by camera device 
*/
BINARYFACEHMD_API binaryfacehmd_ret binaryfacehmd_open_device(
  binaryfacehmd_context_t context_id,
  binaryfacehmd_device_id_t device_id,
  binaryfacehmd_image_info_t *image_info_out);

/**
*  @brief  Open recorded file
*          Error if BINARYFACEHMD_OK is not returned
*
*  @param  context_id       BinaryFaceHMD context id
*  @param  device_file_path Recorded file path
*  @param  image_info_out   The output image information captured by camera device
*/
BINARYFACEHMD_API binaryfacehmd_ret binaryfacehmd_open_device_file(
  binaryfacehmd_context_t context_id,
  const char *device_file_path,
  binaryfacehmd_image_info_t *image_info_out);

/**
*  @brief  Close the camera device if opened.
*          Error if BINARYFACEHMD_OK is not returned
*
*  @param  context_id BinaryFaceHMD context id
*/
BINARYFACEHMD_API binaryfacehmd_ret binaryfacehmd_close_device(
  binaryfacehmd_context_t context_id);

/**
*  @brief  Set user id and load user model if exists
*          Error if BINARYFACEHMD_USER_MODEL_LOADED 
*                or BINARYFACEHMD_USER_MODEL_NOT_FOUND is not returned
*
*  @param  context_id BinaryFaceHMD context id
*/
BINARYFACEHMD_API binaryfacehmd_ret binaryfacehmd_set_user(
  binaryfacehmd_context_t context_id,
  const char *user_id);

/**
*  @brief  Signal to start calibration process and then tracking
*          Error if BINARYFACEHMD_OK is not returned
*
*  @param  context_id BinaryFaceHMD context id
*/
BINARYFACEHMD_API binaryfacehmd_ret binaryfacehmd_start_calibration_and_tracking(
  binaryfacehmd_context_t context_id);

/**
*  @brief  Signal to start tracking
*          Error if BINARYFACEHMD_OK is not returned
*
*  @param  context_id BinaryFaceHMD context id
*/
BINARYFACEHMD_API binaryfacehmd_ret binaryfacehmd_start_tracking(
  binaryfacehmd_context_t context_id);

/**
*  @brief  Get the most recent image captured by camera device
*          Error if BINARYFACEHMD_OK is not returned
*
*  @param  context_id BinaryFaceHMD context id
*  @param  image_out  RGB image snapshot to be processed if valid pointer is provided. 
*                     Memory buffer for image data should be allocated in advance. 
*                     If an error occurs, image data in buffer are not changed
*/
BINARYFACEHMD_API binaryfacehmd_ret binaryfacehmd_get_image(
  binaryfacehmd_context_t context_id,
  unsigned char *image_out); 

/**
*  @brief  Get tracking result for the most recent frame
*          Error if BINARYFACEHMD_OK is not returned
*
*  @param  context_id             BinaryFaceHMD context id
*  @param  face_info_out          face tracking information
*  @param  blendshape_weights_out face expression parameters. 
*                                 Memory buffer for weights should be allocated in advance
*                                 w_0, w_1, ..., w_(N-1), N: # of blendshapes, 0 <= w_i <= 1
*                                 If an error occurs, weight values in buffer are not changed
*/
BINARYFACEHMD_API binaryfacehmd_ret binaryfacehmd_get_face_info(
  binaryfacehmd_context_t context_id,
  binaryfacehmd_face_info_t *face_info_out,
  float *blendshape_weights_out 
);

/**
*  @brief  Get processing status
*          Error if BINARYFACEHMD_ON_CALIBRATION
*                or BINARYFACEHMD_ON_TRACKING
*                or BINARYFACEHMD_PROCESSOR_READY is not returned
*
*  @param  context_id BinaryFaceHMD context id
*/
BINARYFACEHMD_API binaryfacehmd_ret binaryfacehmd_get_processing_status(
  binaryfacehmd_context_t context_id);

#ifdef __cplusplus
}
#endif

#endif // _BINARYVR_BINARYFACEHMD_H_
