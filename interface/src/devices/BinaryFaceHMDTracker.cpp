//
//  BinaryFaceHMDTracker.cpp
//
//
//  Created by Jungwoon Park on 11/11/16.
//  Copyright 2016 High Fidelity, Inc. ???
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <SharedUtil.h>

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QTimer>

#include <GLMHelpers.h>
#include <NumericalConstants.h>

#include "Application.h"
#include "avatar/AvatarManager.h"
#include "FaceshiftConstants.h"
#include "BinaryFaceHMDTracker.h"
#include "InterfaceLogging.h"
#include "Menu.h"
#include "OffscreenUi.h"

using namespace std;

#ifdef HAVE_BINARYFACEHMD
#define BINARYFACEHMD_MESSAGEBOX_TITLE "BinaryFaceHMD"

static const int BINARYFACEHMD_TO_FACESHIFT_MAPPING[] = {
    21, // "JawOpen"
    20, // "JawLeft"
    23, // "JawWRight"
    19, // "JawFwd"
    36, // "LipsUpperUp_L"
    37, // "LipsUpperUp_R"
    32, // "LipsLowerDown_L"
    33, // "LipsLowerDown_R"
    34, // "LipsUpperClose"
    35, // "LipsLowerClose"
    28, // "MouthSmile_L"
    29, // "MouthSmile_R"
    44, // "LipStretch_L"
    45, // "LipStretch_R"
    26, // "MouthFrown_L"
    27, // "MouthFrown_R"
    41, // "LipsPucker"
    40, // "LipsFunnel"
    24, // "MouthLeft"
    25, // "MouthRight"
    48  // "Puff"
};

BinaryFaceHMDTracker::BinaryFaceHMDTracker() :
    _isContextOpen(false),
    _isDeviceOpen(false),
    _isCalibrating(false),
    _isUserModelLoaded(false)
{
}

BinaryFaceHMDTracker::~BinaryFaceHMDTracker() {
    setEnabled(false);
    closeDevice();
    closeContext();
}

void BinaryFaceHMDTracker::init() {
    FaceTracker::init();
    openContext();
    setUser();
    _blendshapeCoefficients = QVector<float>(NUM_FACESHIFT_BLENDSHAPES, 0.f);
    setEnabled(Menu::getInstance()->isOptionChecked(MenuOption::BinaryFaceHMD) && !_isMuted);
}

void BinaryFaceHMDTracker::openContext() {
    QString model_file_path = PathUtils::resourcesPath() + "binaryfacehmd/model.bfh";
    QString cache_dir_path = PathUtils::getRootDataDirectory();
    //qCDebug(interfaceapp) << "BinaryFaceHMD Tracker: model_file_path: " << model_file_path;
    //qCDebug(interfaceapp) << "BinaryFaceHMD Tracker: cache_dir_path: " << cache_dir_path;
    binaryfacehmd_ret ret = binaryfacehmd_open_context(
    model_file_path.toStdString().c_str(),
    cache_dir_path.toStdString().c_str(),
    BINARYFACEHMD_SDK_DEFAULT_API_KEY,
    BINARYFACEHMD_SDK_VERSION,
    &_context, &_contextInfo);

    if (ret != BINARYFACEHMD_OK) {
        qCWarning(interfaceapp) << "BinaryFaceHMD Tracker: openContext: error occurred: " << ret;
        _isContextOpen = false;
        Menu::getInstance()->getActionForOption(MenuOption::BinaryFaceHMD)->setEnabled(false);
        Menu::getInstance()->setIsOptionChecked(MenuOption::NoFaceTracking, true);
        setSubMenuEnabled(false);
    }
    else {
        qCDebug(interfaceapp) << "BinaryFaceHMD Tracker: openContext: success";
        _isContextOpen = true;
    }
}

void BinaryFaceHMDTracker::closeContext() {
    if (!_isContextOpen) return;

    binaryfacehmd_ret ret = binaryfacehmd_close_context(_context);
    if (ret != BINARYFACEHMD_OK) {
        qCWarning(interfaceapp) << "bBinaryFaceHMD Tracker: closeContext: error occurred: " << ret;
    }
}

void BinaryFaceHMDTracker::openDevice() {
    // do nothing if device is already open
    if (!_isContextOpen || _isDeviceOpen) return;
  
    binaryfacehmd_ret ret = binaryfacehmd_open_device(_context, 0, &_imageInfo);
    if (ret != BINARYFACEHMD_OK) {
        qCWarning(interfaceapp) << "BinaryFaceHMD Tracker: openDevice: error occurred: " << ret;
        _isDeviceOpen = false;
    }
    else {
        qCDebug(interfaceapp) << "BinaryFaceHMD Tracker: openDevice: success";
        _isDeviceOpen = true;
    }
}

void BinaryFaceHMDTracker::closeDevice() {
    // do nothing if device is not open
    if (!_isContextOpen || !_isDeviceOpen) return;

    binaryfacehmd_ret ret = binaryfacehmd_close_device(_context);
    if (ret != BINARYFACEHMD_OK) {
        qCWarning(interfaceapp) << "BinaryFaceHMD Tracker: closeDevice: error occurred: " << ret;
        return;
    }
    else {
        qCDebug(interfaceapp) << "bBinaryFaceHMD Tracker: closeDevice: success";
        _isDeviceOpen = false;
    }
}

void BinaryFaceHMDTracker::setUser() {
    if (!_isContextOpen) return;

    string user_id = "binaryfacehmd" + std::to_string(BINARYFACEHMD_SDK_VERSION);
    binaryfacehmd_ret ret = binaryfacehmd_set_user(_context, user_id.c_str());
    if (ret == BINARYFACEHMD_USER_MODEL_LOADED) {
        _isUserModelLoaded = true;
        qCDebug(interfaceapp) << "BinaryFaceHMD Tracker: setUser: model found and loaded for this user";
    }
    else if (ret == BINARYFACEHMD_USER_MODEL_NOT_FOUND) {
        qCDebug(interfaceapp) << "BinaryFaceHMD Tracker: setUser: need calibration for this user";
    }
    else { // error
        qCWarning(interfaceapp) << "BinaryFaceHMD Tracker: setUser: error occurred: " << ret;
    }
}

void BinaryFaceHMDTracker::setSubMenuEnabled(bool enabled) {
    Menu::getInstance()->getActionForOption(MenuOption::ResetBinaryFaceHMD)->setEnabled(enabled);
    Menu::getInstance()->getActionForOption(MenuOption::CalibrateBinaryFaceHMD)->setEnabled(enabled);
}


void BinaryFaceHMDTracker::setEnabled(bool enabled) {
    // Don't enable until have explicitly initialized
    if (!_isInitialized || !_isContextOpen) return;

    if (enabled) {
        openDevice();
        if (_isUserModelLoaded)
            reset(); // reset tracker before use
        else // need to run calibration
            calibrate();

    }
    else {
        closeDevice();
    }

    setSubMenuEnabled(enabled);
}

void BinaryFaceHMDTracker::reset() {
    FaceTracker::reset();

    if (!checkDevice() || !isActive()) return;

    // show reset instruction message popup window
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    auto messagebox = offscreenUi->createMessageBox(
        OffscreenUi::ICON_INFORMATION, BINARYFACEHMD_MESSAGEBOX_TITLE,
        "Close your mouth and keep a neutral face. Ready?",
        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Yes);

    auto result = offscreenUi->waitForMessageBoxResult(messagebox);
    if (result != QMessageBox::Yes) {
        return;
    }

    binaryfacehmd_start_tracking(_context);
}

void BinaryFaceHMDTracker::resetTracker() {
    reset();
}

void BinaryFaceHMDTracker::update(float deltaTime) {
    FaceTracker::update(deltaTime);

    // update head translation/rotation when using HMD
    if (qApp->isHMDMode())
    {
        auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
        glm::mat4 hmdSensorMatrix = qApp->getActiveDisplayPlugin()->getHeadPose();
        _headTranslation = extractTranslation(hmdSensorMatrix);
        _headTranslation.x *= -1.0f;
        _headRotation = extractRotation(hmdSensorMatrix);
    }
    else
    {
        _headRotation = glm::vec3(0.f);
        _headTranslation = glm::vec3(0.f);
    }

    if (!isActive()) return;

    binaryfacehmd_ret ret = binaryfacehmd_get_processing_status(_context);
    // calibration in progress 
    if (ret == BINARYFACEHMD_ON_CALIBRATION) {
        addCalibrationDatum();
        return;
    }
    else if (ret == BINARYFACEHMD_ON_TRACKING) {
        // calibration is done
        if (_isCalibrating) {
            _calibrationStatusMessageBox->setProperty("text", "Calibration is done.");
            _calibrationStatusMessageBox->setProperty("buttons", QMessageBox::Ok);
            _calibrationStatusMessageBox->setProperty("defaultButton", QMessageBox::NoButton);
            _isCalibrating = false;
            _isUserModelLoaded = true;
        }

        binaryfacehmd_face_info_t face_info;
        vector<float> blendshape_weights(_contextInfo.num_blendshapes, 0);
        binaryfacehmd_ret ret = binaryfacehmd_get_face_info(_context, &face_info, blendshape_weights.data());

        for (int i = 0; i < _contextInfo.num_blendshapes; ++i) {
            int index = BINARYFACEHMD_TO_FACESHIFT_MAPPING[i];
            if (index == -1) continue;

            _blendshapeCoefficients[index] = glm::clamp(blendshape_weights[i], 0.0f, 1.0f);
        }
        //qCDebug(interfaceapp) << "time: " << deltaTime << "\n" << _blendshapeCoefficients;
    }
}

bool BinaryFaceHMDTracker::isActive() const {
    return (!_isMuted && _isContextOpen && _isDeviceOpen);
}

bool BinaryFaceHMDTracker::isTracking() const {
    return isActive() 
        && binaryfacehmd_get_processing_status(_context) == BINARYFACEHMD_ON_TRACKING;
}

bool BinaryFaceHMDTracker::checkDevice() {
    auto offscreenUi = DependencyManager::get<OffscreenUi>();

    if (!_isContextOpen) {
        auto messagebox = offscreenUi->createMessageBox(
            OffscreenUi::ICON_WARNING, BINARYFACEHMD_MESSAGEBOX_TITLE,
            "BinaryFaceHMD tracker is not installed correctly.",
            QMessageBox::Ok, QMessageBox::NoButton);
        return false;
    }

    // try to open device if it's not opened yet
    if (!_isDeviceOpen) openDevice();
    if (!_isDeviceOpen) {
        auto messagebox = offscreenUi->createMessageBox(
            OffscreenUi::ICON_WARNING, BINARYFACEHMD_MESSAGEBOX_TITLE,
            "Failed to open the camera. Please make sure the camera is connected.",
            QMessageBox::Ok, QMessageBox::NoButton);
        return false;
    }

    return true;
}

void BinaryFaceHMDTracker::calibrate() {
    if (!checkDevice() || !isActive()) return;
    if (binaryfacehmd_get_processing_status(_context) == BINARYFACEHMD_ON_CALIBRATION) return;

    if (!_isUserModelLoaded)
    {
        auto offscreenUi = DependencyManager::get<OffscreenUi>();
        auto messagebox = offscreenUi->createMessageBox(
            OffscreenUi::ICON_INFORMATION, BINARYFACEHMD_MESSAGEBOX_TITLE,
            "Please run one-time calibration to enable tracker.",
            QMessageBox::Ok, QMessageBox::NoButton);
        auto result = offscreenUi->waitForMessageBoxResult(messagebox);
    }

    // show calibration instruction message popup window
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    auto messagebox = offscreenUi->createMessageBox(
        OffscreenUi::ICON_INFORMATION, BINARYFACEHMD_MESSAGEBOX_TITLE,
        "Close your mouth and keep a neutral face during the calibration process. Ready?",
        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Yes);

    auto result = offscreenUi->waitForMessageBoxResult(messagebox);
    if (result != QMessageBox::Yes) {
        return;
    }

    // reset blendshape coefficients before calibration
    _blendshapeCoefficients = QVector<float>(NUM_FACESHIFT_BLENDSHAPES, 0.f);

    binaryfacehmd_start_calibration_and_tracking(_context);

    _isCalibrating = true;
    _calibrationCount = 0;
    _calibrationStatusMessage = "Calibration in progress\n";
    _calibrationStatusMessageBox = offscreenUi->createMessageBox(
        OffscreenUi::ICON_INFORMATION, BINARYFACEHMD_MESSAGEBOX_TITLE,
        _calibrationStatusMessage, QMessageBox::NoButton, QMessageBox::NoButton);

    qCDebug(interfaceapp) << "BinaryFaceHMD Tracker: calibration started";
}

void BinaryFaceHMDTracker::addCalibrationDatum() {
    const int SMALL_TICK_INTERVAL = 6;
    ++_calibrationCount;
    if (_calibrationCount % SMALL_TICK_INTERVAL == 0) {
        _calibrationStatusMessage += ".";
        _calibrationStatusMessageBox->setProperty("text", _calibrationStatusMessage);
    }
}

#else

BinaryFaceHMDTracker::BinaryFaceHMDTracker() {}
BinaryFaceHMDTracker::~BinaryFaceHMDTracker() {}
void BinaryFaceHMDTracker::setEnabled(bool enabled) {}

#endif

float BinaryFaceHMDTracker::getBlendshapeCoefficient(int index) const {
    return (index >= 0 && index < (int)_blendshapeCoefficients.size()) 
        ? _blendshapeCoefficients[index] : 0.0f;
}

