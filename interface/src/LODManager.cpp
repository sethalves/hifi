//
//  LODManager.cpp
//  interface/src/LODManager.h
//
//  Created by Clement on 1/16/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <SettingHandle.h>
#include <OctreeUtils.h>
#include <Util.h>

#include "Application.h"
#include "ui/DialogsManager.h"
#include "InterfaceLogging.h"

#include "LODManager.h"


Setting::Handle<float> desktopLODDecreaseFPS("desktopLODDecreaseFPS", DEFAULT_DESKTOP_LOD_DOWN_FPS);
Setting::Handle<float> hmdLODDecreaseFPS("hmdLODDecreaseFPS", DEFAULT_HMD_LOD_DOWN_FPS);

LODManager::LODManager() {
}

float LODManager::getLODDecreaseFPS() const {
    if (qApp->isHMDMode()) {
        return getHMDLODDecreaseFPS();
    }
    return getDesktopLODDecreaseFPS();
}

float LODManager::getLODIncreaseFPS() const {
    if (qApp->isHMDMode()) {
        return getHMDLODIncreaseFPS();
    }
    return getDesktopLODIncreaseFPS();
}

// We use a "time-weighted running average" of the maxRenderTime and compare it against min/max thresholds
// to determine if we should adjust the level of detail (LOD).
//
// A time-weighted running average has a timescale which determines how fast the average tracks the measured
// value in real-time.  Given a step-function in the mesured value, and assuming measurements happen
// faster than the runningAverage is computed, the error between the value and its runningAverage will be
// reduced by 1/e every timescale of real-time that passes.
const float LOD_ADJUST_RUNNING_AVG_TIMESCALE = 0.08f; // sec
//
// Assuming the measured value is affected by logic invoked by the runningAverage bumping up against its
// thresholds, we expect the adjustment to introduce a step-function.  We want the runningAverage to settle
// to the new value BEFORE we test it aginst its thresholds again.  Hence we test on a period that is a few
// multiples of the running average timescale:
const uint64_t LOD_AUTO_ADJUST_PERIOD = 4 * (uint64_t)(LOD_ADJUST_RUNNING_AVG_TIMESCALE * (float)USECS_PER_MSEC); // usec

const float LOD_AUTO_ADJUST_DECREMENT_FACTOR = 0.8f;
const float LOD_AUTO_ADJUST_INCREMENT_FACTOR = 1.2f;

void LODManager::setRenderTimes(float presentTime, float engineRunTime, float gpuTime) {
    _presentTime = presentTime;
    _engineRunTime = engineRunTime;
    _gpuTime = gpuTime;
}

void LODManager::autoAdjustLOD(float realTimeDelta) {
    float maxRenderTime = glm::max(glm::max(_presentTime, _engineRunTime), _gpuTime);
    // compute time-weighted running average maxRenderTime
    // Note: we MUST clamp the blend to 1.0 for stability
    float blend = (realTimeDelta < LOD_ADJUST_RUNNING_AVG_TIMESCALE) ? realTimeDelta / LOD_ADJUST_RUNNING_AVG_TIMESCALE : 1.0f;
    _avgRenderTime = (1.0f - blend) * _avgRenderTime + blend * maxRenderTime; // msec
    if (!_automaticLODAdjust) {
        // early exit
        return;
    }

    float oldOctreeSizeScale = _octreeSizeScale;
    float currentFPS = (float)MSECS_PER_SECOND / _avgRenderTime;
    uint64_t now = usecTimestampNow();
    if (currentFPS < getLODDecreaseFPS()) {
        if (now > _decreaseFPSExpiry) {
            _decreaseFPSExpiry = now + LOD_AUTO_ADJUST_PERIOD;
            if (_octreeSizeScale > ADJUST_LOD_MIN_SIZE_SCALE) {
                _octreeSizeScale *= LOD_AUTO_ADJUST_DECREMENT_FACTOR;
                if (_octreeSizeScale < ADJUST_LOD_MIN_SIZE_SCALE) {
                    _octreeSizeScale = ADJUST_LOD_MIN_SIZE_SCALE;
                }
                emit LODDecreased();
                // Assuming the LOD adjustment will work: we optimistically reset _avgRenderTime
                // to provide an FPS just above the decrease threshold.  It will drift close to its
                // true value after a few LOD_ADJUST_TIMESCALEs and we'll adjust again as necessary.
                _avgRenderTime = (float)MSECS_PER_SECOND / (getLODDecreaseFPS() + 1.0f);
            }
            _decreaseFPSExpiry = now + LOD_AUTO_ADJUST_PERIOD;
        }
        _increaseFPSExpiry = now + LOD_AUTO_ADJUST_PERIOD;
    } else if (currentFPS > getLODIncreaseFPS()) {
        if (now > _increaseFPSExpiry) {
            _increaseFPSExpiry = now + LOD_AUTO_ADJUST_PERIOD;
            if (_octreeSizeScale < ADJUST_LOD_MAX_SIZE_SCALE) {
                if (_octreeSizeScale < ADJUST_LOD_MIN_SIZE_SCALE) {
                    _octreeSizeScale = ADJUST_LOD_MIN_SIZE_SCALE;
                } else {
                    _octreeSizeScale *= LOD_AUTO_ADJUST_INCREMENT_FACTOR;
                }
                if (_octreeSizeScale > ADJUST_LOD_MAX_SIZE_SCALE) {
                    _octreeSizeScale = ADJUST_LOD_MAX_SIZE_SCALE;
                }
                emit LODIncreased();
                // Assuming the LOD adjustment will work: we optimistically reset _avgRenderTime
                // to provide an FPS just below the increase threshold.  It will drift close to its
                // true value after a few LOD_ADJUST_TIMESCALEs and we'll adjust again as necessary.
                _avgRenderTime = (float)MSECS_PER_SECOND / (getLODIncreaseFPS() - 1.0f);
            }
            _increaseFPSExpiry = now + LOD_AUTO_ADJUST_PERIOD;
        }
        _decreaseFPSExpiry = now + LOD_AUTO_ADJUST_PERIOD;
    } else {
        _increaseFPSExpiry = now + LOD_AUTO_ADJUST_PERIOD;
        _decreaseFPSExpiry = _increaseFPSExpiry;
    }

    if (oldOctreeSizeScale != _octreeSizeScale) {
        auto lodToolsDialog = DependencyManager::get<DialogsManager>()->getLodToolsDialog();
        if (lodToolsDialog) {
            lodToolsDialog->reloadSliders();
        }
    }
}

void LODManager::resetLODAdjust() {
    _decreaseFPSExpiry = _increaseFPSExpiry = usecTimestampNow() + LOD_AUTO_ADJUST_PERIOD;
}

float LODManager::getLODLevel() const {
    // simpleLOD is a linearized and normalized number that represents how much LOD is being applied.
    // It ranges from:
    //    1.0 = normal (max) level of detail
    //    0.0 = min level of detail
    // In other words: as LOD "drops" the value of simpleLOD will also "drop", and it cannot go lower than 0.0.
    const float LOG_MIN_LOD_RATIO = logf(ADJUST_LOD_MIN_SIZE_SCALE / ADJUST_LOD_MAX_SIZE_SCALE);
    float power = logf(_octreeSizeScale / ADJUST_LOD_MAX_SIZE_SCALE);
    float simpleLOD = (LOG_MIN_LOD_RATIO - power) / LOG_MIN_LOD_RATIO;
    return simpleLOD;
}

const float MIN_DECREASE_FPS = 0.5f;

void LODManager::setDesktopLODDecreaseFPS(float fps) {
    if (fps < MIN_DECREASE_FPS) {
        // avoid divide by zero
        fps = MIN_DECREASE_FPS;
    }
    _desktopMaxRenderTime = (float)MSECS_PER_SECOND / fps;
}

float LODManager::getDesktopLODDecreaseFPS() const {
    return (float)MSECS_PER_SECOND / _desktopMaxRenderTime;
}

float LODManager::getDesktopLODIncreaseFPS() const {
    return glm::min(((float)MSECS_PER_SECOND / _desktopMaxRenderTime) + INCREASE_LOD_GAP_FPS, MAX_LIKELY_DESKTOP_FPS);
}

void LODManager::setHMDLODDecreaseFPS(float fps) {
    if (fps < MIN_DECREASE_FPS) {
        // avoid divide by zero
        fps = MIN_DECREASE_FPS;
    }
    _hmdMaxRenderTime = (float)MSECS_PER_SECOND / fps;
}

float LODManager::getHMDLODDecreaseFPS() const {
    return (float)MSECS_PER_SECOND / _hmdMaxRenderTime;
}

float LODManager::getHMDLODIncreaseFPS() const {
    return glm::min(((float)MSECS_PER_SECOND / _hmdMaxRenderTime) + INCREASE_LOD_GAP_FPS, MAX_LIKELY_HMD_FPS);
}

QString LODManager::getLODFeedbackText() {
    // determine granularity feedback
    int boundaryLevelAdjust = getBoundaryLevelAdjust();
    QString granularityFeedback;
    switch (boundaryLevelAdjust) {
        case 0: {
            granularityFeedback = QString(".");
        } break;
        case 1: {
            granularityFeedback = QString(" at half of standard granularity.");
        } break;
        case 2: {
            granularityFeedback = QString(" at a third of standard granularity.");
        } break;
        default: {
            granularityFeedback = QString(" at 1/%1th of standard granularity.").arg(boundaryLevelAdjust + 1);
        } break;
    }
    // distance feedback
    float octreeSizeScale = getOctreeSizeScale();
    float relativeToDefault = octreeSizeScale / DEFAULT_OCTREE_SIZE_SCALE;
    int relativeToTwentyTwenty = 20 / relativeToDefault;

    QString result;
    if (relativeToDefault > 1.01f) {
        result = QString("20:%1 or %2 times further than average vision%3").arg(relativeToTwentyTwenty).arg(relativeToDefault,0,'f',2).arg(granularityFeedback);
    } else if (relativeToDefault > 0.99f) {
        result = QString("20:20 or the default distance for average vision%1").arg(granularityFeedback);
    } else if (relativeToDefault > 0.01f) {
        result = QString("20:%1 or %2 of default distance for average vision%3").arg(relativeToTwentyTwenty).arg(relativeToDefault,0,'f',3).arg(granularityFeedback);
    } else {
        result = QString("%2 of default distance for average vision%3").arg(relativeToDefault,0,'f',3).arg(granularityFeedback);
    }
    return result;
}

bool LODManager::shouldRender(const RenderArgs* args, const AABox& bounds) {
    // FIXME - eventually we want to use the render accuracy as an indicator for the level of detail
    // to use in rendering.
    float renderAccuracy = calculateRenderAccuracy(args->getViewFrustum().getPosition(), bounds, args->_sizeScale, args->_boundaryLevelAdjust);
    return (renderAccuracy > 0.0f);
};

void LODManager::setOctreeSizeScale(float sizeScale) {
    _octreeSizeScale = sizeScale;
}

void LODManager::setBoundaryLevelAdjust(int boundaryLevelAdjust) {
    _boundaryLevelAdjust = boundaryLevelAdjust;
}

void LODManager::loadSettings() {
    setDesktopLODDecreaseFPS(desktopLODDecreaseFPS.get());
    setHMDLODDecreaseFPS(hmdLODDecreaseFPS.get());
}

void LODManager::saveSettings() {
    desktopLODDecreaseFPS.set(getDesktopLODDecreaseFPS());
    hmdLODDecreaseFPS.set(getHMDLODDecreaseFPS());
}

