//
//  Created by Bradley Austin Davis on 2015-12-15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "QmlWebWindowClass.h"

#include <QtCore/QThread>
#include <QJsonDocument>
#include <QJsonObject>

#include <QtScript/QScriptContext>
#include <QtScript/QScriptEngine>

#include "OffscreenUi.h"

static const char* const URL_PROPERTY = "source";

// Method called by Qt scripts to create a new web window in the overlay
QScriptValue QmlWebWindowClass::constructor(QScriptContext* context, QScriptEngine* engine) {
    auto properties = parseArguments(context);
    QmlWebWindowClass* retVal { nullptr };
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    offscreenUi->executeOnUiThread([&] {
        retVal = new QmlWebWindowClass();
        retVal->initQml(properties);
    }, true);
    Q_ASSERT(retVal);
    connect(engine, &QScriptEngine::destroyed, retVal, &QmlWindowClass::deleteLater);
    return engine->newQObject(retVal);
}

void QmlWebWindowClass::emitScriptEvent(const QVariant& scriptMessage) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "emitScriptEvent", Qt::QueuedConnection, Q_ARG(QVariant, scriptMessage));
    } else {
        emit scriptEventReceived(scriptMessage);
    }
}

// QQuickItem* findActiveFocus(QQuickItem* item) {
//     if (!item) {
//         return nullptr;
//     }
//     if (!item->isVisible()) {
//         return nullptr;
//     }
//     if (!item->hasActiveFocus() && !item->hasFocus()) {
//         return nullptr;
//     }
//     // if (item->hasActiveFocus() || item->hasFocus()) {
//     //     return item;
//     // }
//     const QObjectList& children = item->children();
//     for (int i = 0; i < children.length(); i++) {
//         QQuickItem* hasFocus = findActiveFocus(dynamic_cast<QQuickItem*>(children[i]));
//         if (hasFocus) {
//             return hasFocus;
//         }
//     }
//     return item;
// }

QString itemToNameAgain(QQuickItem* item) {
    QString result;
    while (item) {
        if (result != "") {
            result += "-->";
        }
        result += "'" + item->objectName() + "'";
        item = dynamic_cast<QQuickItem*>(item->parentItem());
    }
    return result;
}

void dumpTree(QQuickItem* item, int depth) {
    if (!item) {
        return;
    }
    QString indent;
    for (int i = 0; i < depth; i++) {
        indent += "  ";
    }
    qDebug() << "|" << indent << item->objectName();
    QList<QQuickItem*> children = item->childItems();
    for (int i = 0; i < children.length(); i++) {
        dumpTree(children[i], depth + 1);
    }
}


void QmlWebWindowClass::emitWebEvent(const QVariant& webMessage) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "emitWebEvent", Qt::QueuedConnection, Q_ARG(QVariant, webMessage));
    } else {
        qDebug() << "HERE QmlWebWindowClass::emitWebEvent: " << webMessage.toString()
                 << (void*) this << "_toolWindow =" << _toolWindow;
        // emit webEventReceived(webMessage);

        if (webMessage.type() != QVariant::String) {
            qDebug() << "not a string";
            emit webEventReceived(webMessage);
            return;
        }
        QString messageJSON = webMessage.toString();
        QJsonDocument doc(QJsonDocument::fromJson(messageJSON.toUtf8()));
        QJsonObject messageObject = doc.object();

        // special case to handle raising and lowering the virtual keyboard
        if (messageObject["type"].toString() != "showKeyboard") {
            qDebug() << "not showKeyboard";
            emit webEventReceived(webMessage);
            return;
        }

        if (_toolWindow) {
            QQuickItem* self = asQuickItem();
            if (self) {
                QQuickItem* tabView = self->findChild<QQuickItem*>("ToolWindowTabView");
                if (tabView) {
                    QList<QQuickItem*> children = tabView->childItems();
                    qDebug() << "tabView is" << itemToNameAgain(tabView)
                             << "and has" << children.length() << "children.";
                    for (int i = 0; i < children.length(); i++) {
                        setKeyboardRaised(children[i], messageObject["value"].toBool());
                    }
                } else {
                    qDebug() << "no tabView";
                    dumpTree(asQuickItem(), 0);
                }
            } else {
                qDebug() << "no self";
            }
        } else {
            setKeyboardRaised(asQuickItem(), messageObject["value"].toBool());
        }

        // setKeyboardRaised(asQuickItem(), messageObject["value"].toBool());
        // setKeyboardRaised(findActiveFocus(asQuickItem()), messageObject["value"].toBool());
        // setKeyboardRaised(dynamic_cast<QQuickItem*>(_qmlWindow.data()), messageObject["value"].toBool());

        // const QObjectList& children = asQuickItem()->children();
        // for (int i = 0; i < children.length(); i++) {
        //     QQuickItem* child = dynamic_cast<QQuickItem*>(children[i]);
        //     if (child && child->isVisible()) {
        //         setKeyboardRaised(child, messageObject["value"].toBool());
        //     }
        // }

        // const QObjectList& children = asQuickItem()->children();
        // for (int i = 0; i < children.length(); i++) {
        //     QQuickItem* child = dynamic_cast<QQuickItem*>(children[i]);
        //     if (child && child->isVisible()) {
        //         QObject* root = child->findChild<QObject*>("root");
        //         if (root) {
        //             setKeyboardRaised(root, messageObject["value"].toBool());
        //         }
        //     }
        // }

        // QObject* root = asQuickItem()->findChild<QObject*>("root");
        // if (root) {
        //     setKeyboardRaised(root, messageObject["value"].toBool());
        // } else {
        //     // const QObjectList& children = asQuickItem()->children();
        //     QList<QQuickItem*> children = asQuickItem()->childItems();
        //     qDebug() << "didn't find root.  asQuickItem is" << itemToNameAgain(asQuickItem())
        //              << "and has" << children.length() << "children";
        //     // for (int i = 0; i < children.length(); i++) {
        //     //     qDebug() << "|    " << itemToNameAgain(children[i]);
        //     // }

        //     // dumpTree(asQuickItem(), 0);

        //     QQuickItem* tabView = asQuickItem()->findChild<QQuickItem*>("ToolWindowTabView");
        //     qDebug() << "tabView is" << itemToNameAgain(tabView);

        //     // qDebug() << "_qmlWindow is" << itemToNameAgain(dynamic_cast<QQuickItem*>(&*_qmlWindow));
        // }
    }
}

void QmlWebWindowClass::setKeyboardRaised(QObject* object, bool raised) {

    // raise the keyboard only while in HMD mode and it's being requested.
    // XXX
    // bool value = AbstractViewStateInterface::instance()->isHMDMode() && raised;
    // getRootItem()->setProperty("keyboardRaised", QVariant(value));

    if (!object) {
        return;
    }

    QQuickItem* item = dynamic_cast<QQuickItem*>(object);
    if (!item) {
        qDebug() << "cast failed";
    }

    while (item) {
        if (item->property("keyboardRaised").isValid()) {
            item->setProperty("keyboardRaised", QVariant(raised));
            qDebug() << "yes";
            return;
        } else {
            qDebug() << "no";
        }
        item = dynamic_cast<QQuickItem*>(item->parentItem());
    }

    qDebug() << "failed";
}

QString QmlWebWindowClass::getURL() const {
    QVariant result = DependencyManager::get<OffscreenUi>()->returnFromUiThread([&]()->QVariant {
        if (_qmlWindow.isNull()) {
            return QVariant();
        }
        return _qmlWindow->property(URL_PROPERTY);
    });
    return result.toString();
}

// HACK find a good place to declare and store this
extern QString fixupHifiUrl(const QString& urlString);

void QmlWebWindowClass::setURL(const QString& urlString) {
    DependencyManager::get<OffscreenUi>()->executeOnUiThread([=] {
        if (!_qmlWindow.isNull()) {
            _qmlWindow->setProperty(URL_PROPERTY, fixupHifiUrl(urlString));
        }
    });
}
