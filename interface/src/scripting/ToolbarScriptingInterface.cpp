//
//  Created by Bradley Austin Davis on 2016-06-16
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ToolbarScriptingInterface.h"

#include <QtCore/QThread>
#include <QUuid>

#include <OffscreenUi.h>

class QmlWrapper : public QObject {
    Q_OBJECT
public:
    QmlWrapper(QObject* qmlObject, QObject* parent = nullptr)
        : QObject(parent), _qmlObject(qmlObject) {
    }

    Q_INVOKABLE void writeProperty(QString propertyName, QVariant propertyValue) {
        auto offscreenUi = DependencyManager::get<OffscreenUi>();
        offscreenUi->executeOnUiThread([=] {
            _qmlObject->setProperty(propertyName.toStdString().c_str(), propertyValue);
        });
    }

    Q_INVOKABLE void writeProperties(QVariant propertyMap) {
        auto offscreenUi = DependencyManager::get<OffscreenUi>();
        offscreenUi->executeOnUiThread([=] {
            QVariantMap map = propertyMap.toMap();
            for (const QString& key : map.keys()) {
                _qmlObject->setProperty(key.toStdString().c_str(), map[key]);
            }
        });
    }

    Q_INVOKABLE QVariant readProperty(const QString& propertyName) {
        auto offscreenUi = DependencyManager::get<OffscreenUi>();
        return offscreenUi->returnFromUiThread([&]()->QVariant {
            return _qmlObject->property(propertyName.toStdString().c_str());
        });
    }

    Q_INVOKABLE QVariant readProperties(const QVariant& propertyList) {
        auto offscreenUi = DependencyManager::get<OffscreenUi>();
        return offscreenUi->returnFromUiThread([&]()->QVariant {
            QVariantMap result;
            for (const QVariant& property : propertyList.toList()) {
                QString propertyString = property.toString();
                result.insert(propertyString, _qmlObject->property(propertyString.toStdString().c_str()));
            }
            return result;
        });
    }


protected:
    QObject* _qmlObject{ nullptr };
};


class ToolbarButtonProxy : public QmlWrapper {
    Q_OBJECT

public:
    ToolbarButtonProxy(QString name, QObject* qmlObject, QObject* parent = nullptr) :
        QmlWrapper(qmlObject, parent),
        _name(name)
    {
        connect(qmlObject, SIGNAL(clicked()), this, SLOT(qmlClicked()));
    }

public slots:
    void qmlClicked();

signals:
    void clicked();

protected:
    QString _name;
};

void ToolbarButtonProxy::qmlClicked() {
    qDebug() << "CLICKED on" << _name;
    emit clicked();
}



class ToolbarProxy : public QmlWrapper {
    Q_OBJECT

public:
    ToolbarProxy(const QString& toolbarId, QObject* qmlObject, QObject* parent = nullptr) :
        QmlWrapper(qmlObject, parent),
        _toolbarID(toolbarId) {
    }

    Q_INVOKABLE QObject* addButton(const QVariant& properties) {
        QVariant resultVar;
        Qt::ConnectionType connectionType = Qt::AutoConnection;
        if (QThread::currentThread() != _qmlObject->thread()) {
            connectionType = Qt::BlockingQueuedConnection;
        }
        bool invokeResult = QMetaObject::invokeMethod(_qmlObject, "addButton", connectionType,
                                                      Q_RETURN_ARG(QVariant, resultVar), Q_ARG(QVariant, properties));
        if (!invokeResult) {
            return nullptr;
        }

        QObject* rawButton = qvariant_cast<QObject *>(resultVar);
        if (!rawButton) {
            return nullptr;
        }

        QMap<QString, QVariant> propertiesMap = properties.toMap();
        QString objectName;
        if (propertiesMap.contains("objectName")) {
            objectName = propertiesMap["objectName"].toString();
        } else {
            objectName = QUuid::createUuid().toString();
            propertiesMap["objectName"] = QVariant(objectName);
        }

        auto toolbarScriptingInterface = DependencyManager::get<ToolbarScriptingInterface>();
        ToolbarButtonProxy* result = new ToolbarButtonProxy(objectName, rawButton, this);

        toolbarScriptingInterface->setToolbarButton(_toolbarID, objectName, propertiesMap);
        toolbarScriptingInterface->rememberButtonProxy(_toolbarID, objectName, result);

        return result;
    }

    Q_INVOKABLE void removeButton(const QVariant& name) {
        QMetaObject::invokeMethod(_qmlObject, "removeButton", Qt::AutoConnection, Q_ARG(QVariant, name));
    }

    Q_INVOKABLE QObject* findButton(const QVariant& name) {
        QVariant resultVar;
        QMetaObject::invokeMethod(_qmlObject, "findButton", Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(QVariant, resultVar), Q_ARG(QVariant, name));
        return qvariant_cast<QObject *>(resultVar);
    }


protected:
    QString _toolbarID;
};


QObject* ToolbarScriptingInterface::getToolbar(const QString& toolbarID) {
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    auto desktop = offscreenUi->getDesktop();
    Qt::ConnectionType connectionType = Qt::AutoConnection;
    if (QThread::currentThread() != desktop->thread()) {
        connectionType = Qt::BlockingQueuedConnection;
    }
    QVariant resultVar;
    bool invokeResult = QMetaObject::invokeMethod(desktop, "getToolbar", connectionType,
                                                  Q_RETURN_ARG(QVariant, resultVar), Q_ARG(QVariant, toolbarID));
    if (!invokeResult) {
        return nullptr;
    }

    QObject* rawToolbar = qvariant_cast<QObject *>(resultVar);
    if (!rawToolbar) {
        return nullptr;
    }

    return new ToolbarProxy(toolbarID, rawToolbar);
}

QObject* ToolbarScriptingInterface::hookUpButtonClone(const QString& toolbarID, QVariant rootVar, QVariant properties) {
    QObject* root = qvariant_cast<QObject *>(rootVar);
    if (!root) {
        qDebug() << "HERE cast of root failed 0";
    }

    QMap<QString, QVariant> propertiesMap = properties.toMap();
    QString buttonName = propertiesMap["objectName"].toString();

    QObject* toolbar = getToolbar(toolbarID);
    if (!toolbar) {
        qDebug() << "HERE can't find toolbar with ID:" << toolbarID;
        return nullptr;
    }

    ToolbarProxy* toolbarProxy = dynamic_cast<ToolbarProxy*>(toolbar);
    if (!toolbarProxy) {
        qDebug() << "HERE cast of toolbar failed." << toolbar;
        return nullptr;
    }

    // QObject* originalButton = toolbarProxy->findButton(buttonName);
    // if (!originalButton) {
    //     qDebug() << "can't find button named:" << buttonName << "in toolbar with ID:" << toolbarID;
    //     return nullptr;
    // }
    // ToolbarButtonProxy* originalButtonProxy = new ToolbarButtonProxy(buttonName, originalButton, nullptr);

    ToolbarButtonProxy* originalButtonProxy = getButtonProxy(toolbarID, buttonName);
    if (!originalButtonProxy) {
        qDebug() << "can't find button named:" << buttonName << " in toolbar with ID:" << toolbarID;
        return nullptr;
    }
    
    QQuickItem* rootQ = dynamic_cast<QQuickItem*>(root);
    if (!rootQ) {
        qDebug() << "HERE cast of root failed 1";
        return nullptr;
    }

    QQuickItem *tabletUIBase = root->findChild<QQuickItem*>("tabletUIBase");
    if (!tabletUIBase) {
        qDebug() << "HERE didn't find tabletUIBase";
        return nullptr;
    }

    qDebug() << "HERE calling addCloneButton on " << tabletUIBase->objectName();

    QVariant resultVar;
    bool invokeResult = QMetaObject::invokeMethod(tabletUIBase, "addCloneButton", Qt::BlockingQueuedConnection,
                                                  Q_RETURN_ARG(QVariant, resultVar), Q_ARG(QVariant, properties));
    if (!invokeResult) {
        qDebug() << "HERE addCloneButton failed";
        return nullptr;
    }

    QObject* cloneButton = qvariant_cast<QObject *>(resultVar);
    if (!cloneButton) {
        qDebug() << "HERE didn't get raw button" << resultVar;
        return nullptr;
    }

    qDebug() << "HERE c++ hooking up button: " << buttonName;
    connect(cloneButton, SIGNAL(clicked()), originalButtonProxy, SLOT(qmlClicked()));

    return new ToolbarButtonProxy(buttonName, cloneButton, nullptr);
}


void ToolbarScriptingInterface::setToolbarButton(QString toolbarID, QString objectName, QVariant properties) {
    _buttonsLock.withWriteLock([&] {
        _toolbarButtons[toolbarID][objectName] = properties;
    });
}

QList<QVariant> ToolbarScriptingInterface::getToolbarButtons(QString toolbarID) {
    QList<QVariant> result;
    _buttonsLock.withReadLock([&] {
        result = _toolbarButtons[toolbarID].values();
    });
    return result;
}

void ToolbarScriptingInterface::rememberButtonProxy(QString toolbarID, QString buttonName, ToolbarButtonProxy* proxy) {
    _buttonsLock.withWriteLock([&] {
        if (!_toolbarButtonProxies.contains(buttonName)) {
            _toolbarButtonProxies[toolbarID][buttonName] = proxy;
        }
    });
}

ToolbarButtonProxy* ToolbarScriptingInterface::getButtonProxy(QString toolbarID, QString buttonName) {
    ToolbarButtonProxy* result = nullptr;
    _buttonsLock.withReadLock([&] {
        result = _toolbarButtonProxies[toolbarID][buttonName];
    });
    return result;
}

#include "ToolbarScriptingInterface.moc"
