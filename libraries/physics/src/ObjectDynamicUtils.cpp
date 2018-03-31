//
//  ObjectDynamicUtils.cpp
//  libraries/physcis/src
//
//  Created by Seth Alves 2015-6-2
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <btBulletDynamicsCommon.h>

#include <dynamics/EntityDynamic.h>

#include "EntitySimulation.h"
#include "ObjectMotionState.h"
#include "PhysicsLogging.h"
#include "ObjectDynamicUtils.h"

btRigidBody* getDynamicRigidBody(EntityDynamicPointer dynamic) {
    ObjectMotionState* motionState = nullptr;
    auto ownerEntityWP = dynamic->getOwnerEntity();
    auto ownerEntity = ownerEntityWP.lock();
    if (!ownerEntity) {
        return nullptr;
    }
    void* physicsInfo = ownerEntity->getPhysicsInfo();
    if (!physicsInfo) {
        return nullptr;
    }
    motionState = static_cast<ObjectMotionState*>(physicsInfo);
    if (motionState) {
        return motionState->getRigidBody();
    }
    return nullptr;
}

void activateDynamicBody(EntityDynamicPointer dynamic, bool forceActivation) {
    auto rigidBody = getRigidBody(dynamic);
    if (rigidBody) {
        rigidBody->activate(forceActivation);
    } else {
        qCDebug(physics) << "activateBody -- no rigid body" << (void*)rigidBody;
    }
}

void forceBodyNonStatic(EntityDynamicPointer dynamic) {
    auto ownerEntityWP = dynamic->getOwnerEntity();
    auto ownerEntity = ownerEntityWP.lock();
    if (!ownerEntity) {
        return;
    }
    void* physicsInfo = ownerEntity->getPhysicsInfo();
    ObjectMotionState* motionState = static_cast<ObjectMotionState*>(physicsInfo);
    if (motionState && motionState->getMotionType() == MOTION_TYPE_STATIC) {
        ownerEntity->flagForMotionStateChange();
    }
}

EntityItemPointer getEntityByID(EntityDynamicPointer dynamic, EntityItemID entityID) {
    QSharedPointer<SpatialParentFinder> parentFinder = DependencyManager::get<SpatialParentFinder>();
    if (!parentFinder) {
        return nullptr;
    }

    auto ownerEntityWP = dynamic->getOwnerEntity();
    auto ownerEntity = ownerEntityWP.lock();
    if (!ownerEntity) {
        return nullptr;
    }

    bool success;
    SpatiallyNestableWeakPointer entityWP = parentFinder->find(entityID, success, ownerEntity->getParentTree());
    if (success) {
        return std::dynamic_pointer_cast<EntityItem>(entityWP.lock());
    }
    return nullptr;
}

btRigidBody* getRigidBody(EntityDynamicPointer dynamic) {
    ObjectMotionState* motionState = nullptr;
    auto ownerEntityWP = dynamic->getOwnerEntity();
    auto ownerEntity = ownerEntityWP.lock();
    if (!ownerEntity) {
        return nullptr;
    }
    void* physicsInfo = ownerEntity->getPhysicsInfo();
    if (!physicsInfo) {
        return nullptr;
    }
    motionState = static_cast<ObjectMotionState*>(physicsInfo);
    if (motionState) {
        return motionState->getRigidBody();
    }
    return nullptr;
}

btRigidBody* getOtherRigidBody(EntityDynamicPointer dynamic) {

    EntityItemPointer otherEntity = dynamic->getOther();
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

QList<btRigidBody*> getRigidBodies(EntityDynamicPointer dynamic) {
    QList<btRigidBody*> result;
    result += getRigidBody(dynamic);

    btRigidBody* other = getOtherRigidBody(dynamic);
    if (other) {
        result += other;
    }

    return result;
}
