//
//  BinaryFaceHMDTracker.h
//
//
//  Created by Jungwoon Park on 11/11/16.
//  Copyright 2016 High Fidelity, Inc. ???
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_BinaryFaceHMDTracker_h
#define hifi_BinaryFaceHMDTracker_h

#include <DependencyManager.h>
#include <ui/overlays/TextOverlay.h>

#include "FaceTracker.h"
#ifdef HAVE_BINARYFACEHMD
#include <binaryfacehmd.h>
#endif

class BinaryFaceHMDTracker : public FaceTracker, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

#ifdef HAVE_BINARYFACEHMD
public:
    virtual void init() override;
    virtual void reset() override;
    virtual void update(float deltaTime) override;

    virtual bool isActive() const override;
    virtual bool isTracking() const override;

public slots:
    void resetTracker();
    void calibrate();

private:
    void openContext();
    void closeContext();
    void openDevice();
    void closeDevice();
    void setUser();
    bool checkDevice();

    // sub menu: reset, calibrate
    void setSubMenuEnabled(bool enabled);

    // status
    bool _isContextOpen;
    bool _isDeviceOpen;
    bool _isCalibrating;
    bool _isCalibrated;
    bool _isUserModelLoaded;

    int _calibrationCount;
    QQuickItem* _calibrationStatusMessageBox;
    QString _calibrationStatusMessage;
    void addCalibrationDatum();
  
    // binaryfacehmd sdk
    binaryfacehmd_context_t _context;
    binaryfacehmd_context_info_t _contextInfo;
    binaryfacehmd_image_info_t _imageInfo;

#endif

public slots:
    void setEnabled(bool enabled) override;

private:
    BinaryFaceHMDTracker();
    virtual ~BinaryFaceHMDTracker();

    float getBlendshapeCoefficient(int index) const;
};

#endif // hifi_BinaryFaceHMD_h
