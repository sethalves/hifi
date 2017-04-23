//
//  PhysicsEngineTrackerInterface.h
//  libraries/entities/src
//
//  Created by Seth Alves on 2016-5-14
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef hifi_PhysicsEngineTrackerInterface_h
#define hifi_PhysicsEngineTrackerInterface_h

#include <memory>
#include <QUuid>
#include <glm/glm.hpp>
#include <DependencyManager.h>
#include "shared/ReadWriteLockable.h"


class PhysicsEngine;
using PhysicsEnginePointer = std::shared_ptr<PhysicsEngine>;
class EntityTree;
typedef std::shared_ptr<EntityTree> EntityTreePointer;

class PhysicsEngineTrackerInterface : public Dependency {

public:
    virtual void forEachPhysicsEngine(std::function<void(PhysicsEnginePointer)> actor);

    virtual PhysicsEnginePointer newPhysicsEngine(QUuid key, const glm::vec3& offset, EntityTreePointer tree) = 0;
    virtual void removePhysicsEngine(QUuid key);
    virtual PhysicsEnginePointer getPhysicsEngineByID(QUuid id);

    static const QUuid DEFAULT_PHYSICS_ENGINE_ID;

protected:
    mutable ReadWriteLockable _physicsEnginesLock;
    std::map<QUuid, PhysicsEnginePointer> _physicsEngines;
};


#endif // hifi_PhysicsEngineTrackerInterface_h
