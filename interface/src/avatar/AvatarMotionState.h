//
//  AvatarMotionState.h
//  interface/src/avatar/
//
//  Created by Andrew Meadows 2015.05.14
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AvatarMotionState_h
#define hifi_AvatarMotionState_h

#include <QSet>

#include <ObjectMotionState.h>

class Avatar;

class PhysicsEngine;
using PhysicsEnginePointer = std::shared_ptr<PhysicsEngine>;
using PhysicsEngineWeakPointer = std::weak_ptr<PhysicsEngine>;


class AvatarMotionState : public ObjectMotionState {
public:
    AvatarMotionState(AvatarSharedPointer avatar, const btCollisionShape* shape,
                      EntitySimulationPointer simulation, PhysicsEnginePointer physicsEngine);

    virtual void setPhysicsEngine(PhysicsEnginePointer physicsEngine) override { _physicsEngine = physicsEngine; }

    virtual QString getName() const override { return "avatar"; }

    virtual PhysicsMotionType getMotionType() const override { return _motionType; }

    virtual uint32_t getIncomingDirtyFlags() override;
    virtual void clearIncomingDirtyFlags() override;

    virtual PhysicsMotionType computePhysicsMotionType() const override;

    virtual bool isMoving() const override;

    // this relays incoming position/rotation to the RigidBody
    virtual void getWorldTransform(btTransform& worldTrans) const override;

    // this relays outgoing position/rotation to the EntityItem
    virtual void setWorldTransform(const btTransform& worldTrans) override;


    // These pure virtual methods must be implemented for each MotionState type
    // and make it possible to implement more complicated methods in this base class.

    // pure virtual overrides from ObjectMotionState
    virtual float getObjectRestitution() const override;
    virtual float getObjectFriction() const override;
    virtual float getObjectLinearDamping() const override;
    virtual float getObjectAngularDamping() const override;

    virtual glm::vec3 getObjectPosition() const override;
    virtual glm::quat getObjectRotation() const override;
    virtual glm::vec3 getObjectLinearVelocity() const override;
    virtual glm::vec3 getObjectAngularVelocity() const override;
    virtual glm::vec3 getObjectGravity() const override;

    virtual const QUuid getObjectID() const override;

    virtual QUuid getSimulatorID() const override;

    void setBoundingBox(const glm::vec3& corner, const glm::vec3& diagonal);

    void addDirtyFlags(uint32_t flags) { _dirtyFlags |= flags; }

    virtual void computeCollisionGroupAndMask(int16_t& group, int16_t& mask) const override;

    friend class AvatarManager;
    friend class Avatar;

    Avatar* getAvatar() const { return _avatar; }
    virtual PhysicsEnginePointer getPhysicsEngine() const override;
    virtual PhysicsEnginePointer getShouldBeInPhysicsEngine() const override { return getAvatar()->getPhysicsEngine(); }
    virtual void maybeSwitchPhysicsEngines() override;

protected:
    // the dtor had been made protected to force the compiler to verify that it is only
    // ever called by the Avatar class dtor.
    ~AvatarMotionState();

    virtual bool isReadyToComputeShape() const override { return true; }
    virtual const btCollisionShape* computeNewShape() override;

    AvatarSharedPointer _avatar;

    PhysicsEngineWeakPointer _physicsEngine;

    uint32_t _dirtyFlags;
};

#endif // hifi_AvatarMotionState_h
