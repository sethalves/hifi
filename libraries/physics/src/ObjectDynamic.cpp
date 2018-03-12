//
//  ObjectDynamic.cpp
//  libraries/physcis/src
//
//  Created by Seth Alves 2015-6-2
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "EntitySimulation.h"

#include "ObjectDynamic.h"

#include "PhysicsLogging.h"


ObjectDynamic::ObjectDynamic(EntityDynamicType type, const QUuid& id, EntityItemPointer ownerEntity) :
    EntityDynamicInterface(type, id, ownerEntity) {
}

ObjectDynamic::~ObjectDynamic() {
}

void ObjectDynamic::remapIDs(QHash<EntityItemID, EntityItemID>& map) {
    withWriteLock([&]{
        if (!_id.isNull()) {
            // just force our ID to something new -- action IDs don't go into the map
            _id = QUuid::createUuid();
        }

        if (!_otherID.isNull()) {
            QHash<EntityItemID, EntityItemID>::iterator iter = map.find(_otherID);
            if (iter == map.end()) {
                // not found, add it
                QUuid oldOtherID = _otherID;
                _otherID = QUuid::createUuid();
                map.insert(oldOtherID, _otherID);
            } else {
                _otherID = iter.value();
            }
        }
    });
}

qint64 ObjectDynamic::getEntityServerClockSkew() const {
    auto nodeList = DependencyManager::get<NodeList>();

    auto ownerEntity = _ownerEntity.lock();
    if (!ownerEntity) {
        return 0;
    }

    const QUuid& entityServerNodeID = ownerEntity->getSourceUUID();
    auto entityServerNode = nodeList->nodeWithUUID(entityServerNodeID);
    if (entityServerNode) {
        return entityServerNode->getClockSkewUsec();
    }
    return 0;
}

void ObjectDynamic::removeFromSimulation(EntitySimulationPointer simulation) const {
    QUuid myID;
    withReadLock([&]{
        myID = _id;
    });
    simulation->removeDynamic(myID);
}

EntityItemPointer ObjectDynamic::getEntityByID(EntityItemID entityID) const {
    EntityItemPointer ownerEntity;
    withReadLock([&]{
        ownerEntity = _ownerEntity.lock();
    });
    EntityTreeElementPointer element = ownerEntity ? ownerEntity->getElement() : nullptr;
    EntityTreePointer tree = element ? element->getTree() : nullptr;
    if (!tree) {
        return nullptr;
    }
    return tree->findEntityByID(entityID);
}


btRigidBody* ObjectDynamic::getRigidBody() {
    ObjectMotionState* motionState = nullptr;
    withReadLock([&]{
        auto ownerEntity = _ownerEntity.lock();
        if (!ownerEntity) {
            return;
        }
        void* physicsInfo = ownerEntity->getPhysicsInfo();
        if (!physicsInfo) {
            return;
        }
        motionState = static_cast<ObjectMotionState*>(physicsInfo);
    });
    if (motionState) {
        return motionState->getRigidBody();
    }
    return nullptr;
}

void ObjectDynamic::activateBody(bool forceActivation) {
    auto rigidBody = getRigidBody();
    if (rigidBody) {
        rigidBody->activate(forceActivation);
    } else {
        qCDebug(physics) << "ObjectDynamic::activateBody -- no rigid body" << (void*)rigidBody;
    }
}

void ObjectDynamic::forceBodyNonStatic() {
    auto ownerEntity = _ownerEntity.lock();
    if (!ownerEntity) {
        return;
    }
    void* physicsInfo = ownerEntity->getPhysicsInfo();
    ObjectMotionState* motionState = static_cast<ObjectMotionState*>(physicsInfo);
    if (motionState && motionState->getMotionType() == MOTION_TYPE_STATIC) {
        ownerEntity->flagForMotionStateChange();
    }
}

quint64 ObjectDynamic::localTimeToServerTime(quint64 timeValue) const {
    // 0 indicates no expiration
    if (timeValue == 0) {
        return 0;
    }

    qint64 serverClockSkew = getEntityServerClockSkew();
    if (serverClockSkew < 0 && timeValue <= (quint64)(-serverClockSkew)) {
        return 1; // non-zero but long-expired value to avoid negative roll-over
    }

    return timeValue + serverClockSkew;
}

quint64 ObjectDynamic::serverTimeToLocalTime(quint64 timeValue) const {
    // 0 indicates no expiration
    if (timeValue == 0) {
        return 0;
    }

    qint64 serverClockSkew = getEntityServerClockSkew();
    if (serverClockSkew > 0 && timeValue <= (quint64)serverClockSkew) {
        return 1; // non-zero but long-expired value to avoid negative roll-over
    }

    return timeValue - serverClockSkew;
}

btRigidBody* ObjectDynamic::getOtherRigidBody(EntityItemID otherEntityID) {
    EntityItemPointer otherEntity = getEntityByID(otherEntityID);
    if (!otherEntity) {
        return nullptr;
    }

    void* otherPhysicsInfo = otherEntity->getPhysicsInfo();
    if (!otherPhysicsInfo) {
        return nullptr;
    }

    ObjectMotionState* otherMotionState = static_cast<ObjectMotionState*>(otherPhysicsInfo);
    if (!otherMotionState) {
        return nullptr;
    }

    return otherMotionState->getRigidBody();
}

QList<btRigidBody*> ObjectDynamic::getRigidBodies() {
    QList<btRigidBody*> result;
    result += getRigidBody();
    return result;
}

SpatiallyNestablePointer ObjectDynamic::getOther() {
    SpatiallyNestablePointer other;
    withWriteLock([&]{
        if (_otherID == QUuid()) {
            // no other
            return;
        }
        other = _other.lock();
        if (other && other->getID() == _otherID) {
            // other is already up-to-date
            return;
        }
        if (other) {
            // we have a pointer to other, but it's wrong
            other.reset();
            _other.reset();
        }
        // we have an other-id but no pointer to other cached
        QSharedPointer<SpatialParentFinder> parentFinder = DependencyManager::get<SpatialParentFinder>();
        if (!parentFinder) {
            return;
        }
        EntityItemPointer ownerEntity = _ownerEntity.lock();
        if (!ownerEntity) {
            return;
        }
        bool success;
        _other = parentFinder->find(_otherID, success, ownerEntity->getParentTree());
        if (success) {
            other = _other.lock();
        }
    });
    return other;
}
