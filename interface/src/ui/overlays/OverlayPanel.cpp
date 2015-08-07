//
//  OverlayPanel.cpp
//  interface/src/ui/overlays
//
//  Created by Zander Otavka on 7/2/15.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "OverlayPanel.h"

#include <QVariant>
#include <RegisteredMetaTypes.h>
#include <DependencyManager.h>
#include <EntityScriptingInterface.h>

#include "avatar/AvatarManager.h"
#include "avatar/MyAvatar.h"
#include "Application.h"
#include "Base3DOverlay.h"

void OverlayPanel::addChild(unsigned int childId) {
    if (!_children.contains(childId)) {
        _children.append(childId);
    }
}

void OverlayPanel::removeChild(unsigned int childId) {
    if (_children.contains(childId)) {
        _children.removeOne(childId);
    }
}

QScriptValue OverlayPanel::getProperty(const QString &property) {
    if (property == "position") {
        return vec3toScriptValue(_scriptEngine, getPosition());
    }
    if (property == "positionBinding") {
        QScriptValue obj = _scriptEngine->newObject();

        if (_positionBindMyAvatar) {
            obj.setProperty("avatar", "MyAvatar");
        } else if (!_positionBindEntity.isNull()) {
            obj.setProperty("entity", _scriptEngine->newVariant(_positionBindEntity));
        }
        
        return obj;
    }
    if (property == "rotation") {
        return quatToScriptValue(_scriptEngine, getRotation());
    }
    if (property == "rotationBinding") {
        QScriptValue obj = _scriptEngine->newObject();

        if (_rotationBindMyAvatar) {
            obj.setProperty("avatar", "MyAvatar");
        } else if (!_rotationBindEntity.isNull()) {
            obj.setProperty("entity", _scriptEngine->newVariant(_rotationBindEntity));
        }

        return obj;
    }
    if (property == "visible") {
        return getVisible();
    }
    if (property == "children") {
        QScriptValue array = _scriptEngine->newArray(_children.length());
        for (int i = 0; i < _children.length(); i++) {
            array.setProperty(i, _children[i]);
        }
        return array;
    }

    return PanelAttachable::getProperty(_scriptEngine, property);
}

void OverlayPanel::setProperties(const QScriptValue &properties) {
    PanelAttachable::setProperties(properties);

    QScriptValue position = properties.property("position");
    if (position.isValid()) {
        QScriptValue x = position.property("x");
        QScriptValue y = position.property("y");
        QScriptValue z = position.property("z");
        if (x.isValid() && y.isValid() && z.isValid()) {
            glm::vec3 newPosition;
            newPosition.x = x.toVariant().toFloat();
            newPosition.y = y.toVariant().toFloat();
            newPosition.z = z.toVariant().toFloat();
            setPosition(newPosition);
        }
    }

    QScriptValue positionBinding = properties.property("positionBinding");
    if (positionBinding.isValid()) {
        _positionBindMyAvatar = false;
        _positionBindEntity = QUuid();

        QScriptValue avatar = positionBinding.property("avatar");
        QScriptValue entity = positionBinding.property("entity");

        if (avatar.isValid()) {
            _positionBindMyAvatar = (avatar.toVariant().toString() == "MyAvatar");
        } else if (entity.isValid() && !entity.isNull()) {
            _positionBindEntity = entity.toVariant().toUuid();
        }
    }

    QScriptValue rotation = properties.property("rotation");
    if (rotation.isValid()) {
        QScriptValue x = rotation.property("x");
        QScriptValue y = rotation.property("y");
        QScriptValue z = rotation.property("z");
        QScriptValue w = rotation.property("w");

        if (x.isValid() && y.isValid() && z.isValid() && w.isValid()) {
            glm::quat newRotation;
            newRotation.x = x.toVariant().toFloat();
            newRotation.y = y.toVariant().toFloat();
            newRotation.z = z.toVariant().toFloat();
            newRotation.w = w.toVariant().toFloat();
            setRotation(newRotation);
        }
    }

    QScriptValue rotationBinding = properties.property("rotationBinding");
    if (rotationBinding.isValid()) {
        _rotationBindMyAvatar = false;
        _rotationBindEntity = QUuid();

        QScriptValue avatar = rotationBinding.property("avatar");
        QScriptValue entity = positionBinding.property("entity");

        if (avatar.isValid()) {
            _rotationBindMyAvatar = (avatar.toVariant().toString() == "MyAvatar");
        } else if (entity.isValid() && !entity.isNull()) {
            _rotationBindEntity = entity.toVariant().toUuid();
        }
    }

    QScriptValue visible = properties.property("visible");
    if (visible.isValid()) {
        setVisible(visible.toVariant().toBool());
    }
}

void OverlayPanel::applyTransformTo(Transform& transform) {
    if (getParentPanel()) {
        getParentPanel()->applyTransformTo(transform);
    } else {
        transform.setTranslation(getComputedPosition());
        transform.setRotation(getComputedRotation());
    }
    _position = transform.getTranslation();
    _rotation = transform.getRotation();
    transform.postTranslate(getOffsetPosition());
    transform.postRotate(getOffsetRotation());
}

glm::vec3 OverlayPanel::getComputedPosition() const {
    if (_positionBindMyAvatar) {
        return DependencyManager::get<AvatarManager>()->getMyAvatar()->getPosition();
    } else if (!_positionBindEntity.isNull()) {
        return DependencyManager::get<EntityScriptingInterface>()->getEntityTree()->
        findEntityByID(_positionBindEntity)->getPosition();
    }
    return getPosition();
}

glm::quat OverlayPanel::getComputedRotation() const {
    if (_rotationBindMyAvatar) {
        return DependencyManager::get<AvatarManager>()->getMyAvatar()->getOrientation() *
        glm::angleAxis(glm::pi<float>(), IDENTITY_UP);
    } else if (!_rotationBindEntity.isNull()) {
        return DependencyManager::get<EntityScriptingInterface>()->getEntityTree()->
        findEntityByID(_rotationBindEntity)->getRotation();
    }
    return getRotation();
}
