//
//  SettingsScriptingInterface.cpp
//  interface/src/scripting
//
//  Created by Brad Hefta-Gaub on 2/25/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QApplication>
#include "SettingsScriptingInterface.h"

#include <SettingHandle.h>

SettingsScriptingInterface* SettingsScriptingInterface::getInstance() {
    static SettingsScriptingInterface sharedInstance;
    return &sharedInstance;
}

QVariant SettingsScriptingInterface::getValue(const QString& setting) {
    QVariant value = Setting::Handle<QVariant>(setting).get();
    if (!value.isValid()) {
        value = "";
    }
    return value;
}

QVariant SettingsScriptingInterface::getValue(const QString& setting, const QVariant& defaultValue) {
    QVariant value = Setting::Handle<QVariant>(setting, defaultValue).get();
    if (!value.isValid()) {
        value = "";
    }
    return value;
}

void SettingsScriptingInterface::setValue(const QString& setting, const QVariant& value) {
    Setting::Handle<QVariant>(setting).set(value);
}

QVariant SettingsScriptingInterface::getProperty(const QString& propertyName) {
    QByteArray propertyNameBA = propertyName.toLatin1();
    const char *propertyNameC = propertyNameBA.data();
    return qApp->property(propertyNameC);
}
