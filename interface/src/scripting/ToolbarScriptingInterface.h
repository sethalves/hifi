//
//  Created by Bradley Austin Davis on 2016-06-16
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ToolbarScriptingInterface_h
#define hifi_ToolbarScriptingInterface_h

#include <mutex>

#include <QtCore/QObject>

#include <DependencyManager.h>

class ToolbarProxy;

class ToolbarScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
public:
    Q_INVOKABLE QObject* getToolbar(const QString& toolbarID);
    Q_INVOKABLE QObject* hookUpButtonClone(const QString& toolbarID, QVariant rootVar, QVariant properties);

    void setToolbarButton(QString toolbarID, QString objectName, QVariant properties);
    Q_INVOKABLE QList<QVariant> getToolbarButtons(QString toolbarID);

protected:
    QHash<QString, QHash<QString, QVariant>> _toolbarButtons; // QHash<toolbar-name, QHash<button-name, properties>>
};

#endif // hifi_ToolbarScriptingInterface_h
