//
//  ObjectAction.h
//  libraries/physcis/src
//
//  Created by Seth Alves 2015-6-2
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  http://bulletphysics.org/Bullet/BulletFull/classbtActionInterface.html

#ifndef hifi_ObjectAction_h
#define hifi_ObjectAction_h

#include <QUuid>

#include <btBulletDynamicsCommon.h>

#include <shared/ReadWriteLockable.h>

#include "ObjectMotionState.h"
#include "BulletUtil.h"
#include "EntityActionInterface.h"


class ObjectAction : public btActionInterface, public EntityActionInterface, public ReadWriteLockable {
public:
    ObjectAction(EntityActionType type, const QUuid& id, EntityItemPointer ownerEntity);
    virtual ~ObjectAction();

    virtual void removeFromSimulation(EntitySimulationPointer simulation) const;
    virtual EntityItemWeakPointer getOwnerEntity() const { return _ownerEntity; }
    virtual void setOwnerEntity(const EntityItemPointer ownerEntity) { _ownerEntity = ownerEntity; }

    virtual bool updateArguments(QVariantMap arguments);
    virtual QVariantMap getArguments();

    // this is called from updateAction and should be overridden by subclasses
    virtual void updateActionWorker(float deltaTimeStep) = 0;

    // these are from btActionInterface
    virtual void updateAction(btCollisionWorld* collisionWorld, btScalar deltaTimeStep);
    virtual void debugDraw(btIDebugDraw* debugDrawer);

    virtual QByteArray serialize() const = 0;
    virtual void deserialize(QByteArray serializedArguments) = 0;

    virtual bool lifetimeIsOver();

protected:

    virtual btRigidBody* getRigidBody();
    virtual glm::vec3 getPosition();
    virtual void setPosition(glm::vec3 position);
    virtual glm::quat getRotation();
    virtual void setRotation(glm::quat rotation);
    virtual glm::vec3 getLinearVelocity();
    virtual void setLinearVelocity(glm::vec3 linearVelocity);
    virtual glm::vec3 getAngularVelocity();
    virtual void setAngularVelocity(glm::vec3 angularVelocity);
    virtual void activateBody();

    bool _active;
    EntityItemWeakPointer _ownerEntity;

    quint64 _expires; // in seconds since epoch
    QString _tag;
};

#endif // hifi_ObjectAction_h
