//
//  SpatiallyNestable.cpp
//  libraries/entities/src/
//
//  Created by Seth Alves on 2016-5-14
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SimulationTracker.h"


const QUuid SimulationTracker::DEFAULT_SIMULATOR_ID = QUuid();


void SimulationTracker::addSimulation(QUuid key, EntitySimulationPointer simulation) {
    _simulationsLock.withWriteLock([&] {
        _simulations[key] = simulation;
    });
}

void SimulationTracker::removeSimulation(QUuid key) {
    _simulationsLock.withWriteLock([&] {
        _simulations.erase(key);
    });
}

EntitySimulationPointer SimulationTracker::getSimulationByKey(QUuid key) {
    if (_simulations.count(key)) {
        return _simulations[key];
    }
    return nullptr;
}

void SimulationTracker::forEachSimulation(std::function<void(EntitySimulationPointer)> actor) {
    _simulationsLock.withReadLock([&] {
        for (auto keyAndSim : _simulations) {
            actor(keyAndSim.second);
        }
    });
}
