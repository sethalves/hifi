//
//  PhysicsEngineTracker.h
//  libraries/physics/src
//
//  Created by Seth Alves on 2016-5-14
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef hifi_PhysicsEngineTracker_h
#define hifi_PhysicsEngineTracker_h

#include <memory>
#include <QUuid>
#include <DependencyManager.h>
#include "shared/ReadWriteLockable.h"
#include "PhysicsEngineTrackerInterface.h"


class EntityPhysicsEngine;
using EntityPhysicsEnginePointer = std::shared_ptr<EntityPhysicsEngine>;
class EntityTree;
typedef std::shared_ptr<EntityTree> EntityTreePointer;

class PhysicsEngineTracker : public PhysicsEngineTrackerInterface {

public:
    virtual PhysicsEnginePointer newPhysicsEngine(QUuid key, const glm::vec3& offset, EntityTreePointer tree) override;
};


#endif // hifi_PhysicsEngineTracker_h
