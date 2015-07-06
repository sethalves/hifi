//
//  ObjectMotionState.h
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2014.11.05
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ObjectMotionState_h
#define hifi_ObjectMotionState_h

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>

#include <QSet>
#include <QVector>

#include <EntityItem.h>

#include "ContactInfo.h"
#include "ShapeManager.h"

enum MotionType {
    MOTION_TYPE_STATIC,     // no motion
    MOTION_TYPE_DYNAMIC,    // motion according to physical laws
    MOTION_TYPE_KINEMATIC   // keyframed motion
};

enum MotionStateType {
    MOTIONSTATE_TYPE_INVALID,
    MOTIONSTATE_TYPE_ENTITY,
    MOTIONSTATE_TYPE_AVATAR
};

// The update flags trigger two varieties of updates: "hard" which require the body to be pulled 
// and re-added to the physics engine and "easy" which just updates the body properties.
const uint32_t HARD_DIRTY_PHYSICS_FLAGS = (uint32_t)(EntityItem::DIRTY_MOTION_TYPE | EntityItem::DIRTY_SHAPE);
const uint32_t EASY_DIRTY_PHYSICS_FLAGS = (uint32_t)(EntityItem::DIRTY_TRANSFORM | EntityItem::DIRTY_VELOCITIES |
                                                     EntityItem::DIRTY_MASS | EntityItem::DIRTY_COLLISION_GROUP |
                                                     EntityItem::DIRTY_MATERIAL | EntityItem::DIRTY_SIMULATOR_ID | 
                                                     EntityItem::DIRTY_SIMULATOR_OWNERSHIP);

// These are the set of incoming flags that the PhysicsEngine needs to hear about:
const uint32_t DIRTY_PHYSICS_FLAGS = (uint32_t)(HARD_DIRTY_PHYSICS_FLAGS | EASY_DIRTY_PHYSICS_FLAGS |
                                                EntityItem::DIRTY_PHYSICS_ACTIVATION);

// These are the outgoing flags that the PhysicsEngine can affect:
const uint32_t OUTGOING_DIRTY_PHYSICS_FLAGS = EntityItem::DIRTY_TRANSFORM | EntityItem::DIRTY_VELOCITIES;


class OctreeEditPacketSender;
class PhysicsEngine;

class ObjectMotionState : public btMotionState {
public:
    // These poroperties of the PhysicsEngine are "global" within the context of all ObjectMotionStates
    // (assuming just one PhysicsEngine).  They are cached as statics for fast calculations in the 
    // ObjectMotionState context.
    static void setWorldOffset(const glm::vec3& offset);
    static const glm::vec3& getWorldOffset();

    static void setWorldSimulationStep(uint32_t step);
    static uint32_t getWorldSimulationStep();

    static void setShapeManager(ShapeManager* manager);
    static ShapeManager* getShapeManager();

    ObjectMotionState(btCollisionShape* shape);
    ~ObjectMotionState();

    virtual void handleEasyChanges(uint32_t flags, PhysicsEngine* engine);
    virtual void handleHardAndEasyChanges(uint32_t flags, PhysicsEngine* engine);

    void updateBodyMaterialProperties();
    void updateBodyVelocities();
    virtual void updateBodyMassProperties();

    MotionStateType getType() const { return _type; }
    virtual MotionType getMotionType() const { return _motionType; }

    void setMass(float mass) { _mass = fabsf(mass); }
    float getMass() { return _mass; }

    void setBodyLinearVelocity(const glm::vec3& velocity) const;
    void setBodyAngularVelocity(const glm::vec3& velocity) const;
    void setBodyGravity(const glm::vec3& gravity) const;

    glm::vec3 getBodyLinearVelocity() const;
    glm::vec3 getBodyAngularVelocity() const;
    virtual glm::vec3 getObjectLinearVelocityChange() const;

    virtual uint32_t getAndClearIncomingDirtyFlags() = 0;

    virtual MotionType computeObjectMotionType() const = 0;

    btCollisionShape* getShape() const { return _shape; }
    btRigidBody* getRigidBody() const { return _body; }

    void releaseShape();

    virtual bool isMoving() const = 0;

    // These pure virtual methods must be implemented for each MotionState type
    // and make it possible to implement more complicated methods in this base class.

    virtual float getObjectRestitution() const = 0;
    virtual float getObjectFriction() const = 0;
    virtual float getObjectLinearDamping() const = 0;
    virtual float getObjectAngularDamping() const = 0;

    virtual glm::vec3 getObjectPosition() const = 0;
    virtual glm::quat getObjectRotation() const = 0;
    virtual glm::vec3 getObjectLinearVelocity() const = 0;
    virtual glm::vec3 getObjectAngularVelocity() const = 0;
    virtual glm::vec3 getObjectGravity() const = 0;

    virtual const QUuid getObjectID() const = 0;

    virtual quint8 getSimulationPriority() const { return 0; }
    virtual QUuid getSimulatorID() const = 0;
    virtual void bump(quint8 priority) {}

    virtual QString getName() { return ""; }

    virtual int16_t computeCollisionGroup() = 0;

    bool isActive() const { return _body ? _body->isActive() : false; }

    friend class PhysicsEngine;

protected:
    virtual btCollisionShape* computeNewShape() = 0;
    void setMotionType(MotionType motionType);

    // clearObjectBackPointer() overrrides should call the base method, then actually clear the object back pointer.
    virtual void clearObjectBackPointer() { _type = MOTIONSTATE_TYPE_INVALID; }

    void setRigidBody(btRigidBody* body);

    MotionStateType _type = MOTIONSTATE_TYPE_INVALID; // type of MotionState
    MotionType _motionType; // type of motion: KINEMATIC, DYNAMIC, or STATIC

    btCollisionShape* _shape;
    btRigidBody* _body;
    float _mass;

    uint32_t _lastKinematicStep;
};

typedef QSet<ObjectMotionState*> SetOfMotionStates;
typedef QVector<ObjectMotionState*> VectorOfMotionStates;

#endif // hifi_ObjectMotionState_h
