//
//  PhysicsEngineTrackerInterface.cpp
//  libraries/entities/src/
//
//  Created by Seth Alves on 2016-5-14
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PhysicsEngineTrackerInterface.h"

const QUuid PhysicsEngineTrackerInterface::DEFAULT_PHYSICS_ENGINE_ID = QUuid();

void PhysicsEngineTrackerInterface::removePhysicsEngine(QUuid key) {
    _physicsEnginesLock.withWriteLock([&] {
        _physicsEngines.erase(key);
    });
}

PhysicsEnginePointer PhysicsEngineTrackerInterface::getPhysicsEngineByID(QUuid id) {
    PhysicsEnginePointer result;
    _physicsEnginesLock.withReadLock([&] {
        if (_physicsEngines.count(id)) {
            result = _physicsEngines[id];
        }
    });
    return result;
}

void PhysicsEngineTrackerInterface::forEachPhysicsEngine(std::function<void(PhysicsEnginePointer)> actor) {
    _physicsEnginesLock.withReadLock([&] {
        for (auto keyAndSim : _physicsEngines) {
            actor(keyAndSim.second);
        }
    });
}
