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
#include "shared/ReadWriteLockable.h"


class ToolbarButtonProxy;
class ToolbarProxy;

class ToolbarScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
public:
    Q_INVOKABLE QObject* getToolbar(const QString& toolbarID);
    Q_INVOKABLE QObject* hookUpButtonClone(const QString& toolbarID, QVariant rootVar, QVariant properties);
    Q_INVOKABLE void destroyButtonClones(const QString& toolbarID);

    void setToolbarButton(QString toolbarID, QString objectName, QVariant properties);
    Q_INVOKABLE QList<QVariant> getToolbarButtons(QString toolbarID);

    void rememberButtonProxy(QString toolbarID, QString buttonName, ToolbarButtonProxy* proxy);
    Q_INVOKABLE QObject* getButtonProxy(QString toolbarID, QString buttonName);

protected:
    // QHash<toolbar-name, QHash<button-name, properties>>
    QHash<QString, QHash<QString, QVariant>> _toolbarButtons;
    // QHash<toolbar-name, QHash<button-name, proxy>>
    QHash<QString, QHash<QString, ToolbarButtonProxy*>> _toolbarButtonProxies;
    mutable ReadWriteLockable _buttonsLock;

    QHash<QString, ToolbarProxy*> _toolbarProxies;
};

#endif // hifi_ToolbarScriptingInterface_h
