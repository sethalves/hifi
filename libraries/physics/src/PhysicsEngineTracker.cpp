//
//  PhysicsEngineTracker.cpp
//  libraries/physics/src/
//
//  Created by Seth Alves on 2016-5-14
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PhysicsEngine.h"
#include "PhysicsEngineTracker.h"


PhysicsEnginePointer PhysicsEngineTracker::newPhysicsEngine(QUuid key, const glm::vec3& offset, EntityTreePointer tree) {
    PhysicsEnginePointer physicsEngine { new PhysicsEngine(key, offset) };
    _physicsEnginesLock.withWriteLock([&] {
        _physicsEngines[key] = physicsEngine;
    });
    physicsEngine->init();

    return physicsEngine;
}
