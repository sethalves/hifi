//
//  ObjectActionOffset.cpp
//  libraries/physics/src
//
//  Created by Seth Alves 2015-6-17
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "QVariantGLM.h"

#include "ObjectActionOffset.h"

#include "PhysicsLogging.h"


const uint16_t ObjectActionOffset::offsetVersion = 1;

ObjectActionOffset::ObjectActionOffset(const QUuid& id, EntityItemPointer ownerEntity) :
    ObjectAction(DYNAMIC_TYPE_OFFSET, id, ownerEntity),
    _pointToOffsetFrom(0.0f),
    _linearDistance(0.0f),
    _linearTimeScale(FLT_MAX),
    _positionalTargetSet(false) {
    #if WANT_DEBUG
    qCDebug(physics) << "ObjectActionOffset::ObjectActionOffset";
    #endif
}

ObjectActionOffset::~ObjectActionOffset() {
    #if WANT_DEBUG
    qCDebug(physics) << "ObjectActionOffset::~ObjectActionOffset";
    #endif
}

void ObjectActionOffset::updateActionWorker(btScalar deltaTimeStep) {
    withTryReadLock([&]{
        auto ownerEntity = _ownerEntity.lock();
        if (!ownerEntity) {
            return;
        }

        void* physicsInfo = ownerEntity->getPhysicsInfo();
        if (!physicsInfo) {
            return;
        }

        ObjectMotionState* motionState = static_cast<ObjectMotionState*>(physicsInfo);
        btRigidBody* rigidBody = motionState->getRigidBody();
        if (!rigidBody) {
            qCDebug(physics) << "ObjectActionOffset::updateActionWorker no rigidBody";
            return;
        }

        const float MAX_LINEAR_TIMESCALE = 600.0f;  // 10 minutes is a long time
        if (_positionalTargetSet && _linearTimeScale < MAX_LINEAR_TIMESCALE) {
            glm::vec3 objectPosition = bulletToGLM(rigidBody->getCenterOfMassPosition());
            glm::vec3 springAxis = objectPosition - _pointToOffsetFrom; // from anchor to object
            float distance = glm::length(springAxis);
            if (distance > FLT_EPSILON) {
                springAxis /= distance;  // normalize springAxis

                // compute (critically damped) target velocity of spring relaxation
                glm::vec3 offset = (distance - _linearDistance) * springAxis;
                glm::vec3 targetVelocity = (-1.0f / _linearTimeScale) * offset;

                // compute current velocity and its parallel component
                glm::vec3 currentVelocity = bulletToGLM(rigidBody->getLinearVelocity());
                glm::vec3 parallelVelocity = glm::dot(currentVelocity, springAxis) * springAxis;

                // we blend the parallel component with the spring's target velocity to get the new velocity
                float blend = deltaTimeStep / _linearTimeScale;
                if (blend > 1.0f) {
                    blend = 1.0f;
                }
                rigidBody->setLinearVelocity(glmToBullet(currentVelocity + blend * (targetVelocity - parallelVelocity)));
            }
        }
    });
}

bool EntityActionOffset::updateArguments(QVariantMap arguments) {
    bool needUpdate = EntityActionOffset::updateArguments(arguments);

    if (needUpdate) {
        _active = true;
        auto ownerEntity = _ownerEntity.lock();
        if (ownerEntity) {
            ownerEntity->setDynamicDataDirty(true);
            ownerEntity->setDynamicDataNeedsTransmit(true);
        }
        activateBody();
    }

    return needUpdate;
}
