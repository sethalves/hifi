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

#include "SimpleEntitySimulation.h"
#include "SimulationTracker.h"


const QUuid SimulationTracker::DEFAULT_SIMULATOR_ID = QUuid();


EntitySimulationPointer SimulationTracker::newSimulation(QUuid key, EntityTreePointer tree) {
    SimpleEntitySimulationPointer simulation { new SimpleEntitySimulation() };
    simulation->setEntityTree(tree);

    _simulationsLock.withWriteLock([&] {
        _simulations[key] = simulation;
    });

    return simulation;
}

void SimulationTracker::removeSimulation(QUuid key) {
    _simulationsLock.withWriteLock([&] {
        _simulations.erase(key);
    });
}

EntitySimulationPointer SimulationTracker::getSimulationByKey(QUuid key) {
    EntitySimulationPointer result;
    _simulationsLock.withReadLock([&] {
        if (_simulations.count(key)) {
            result = _simulations[key];
        }
    });
    return result;
}

void SimulationTracker::forEachSimulation(std::function<void(EntitySimulationPointer)> actor) {
    _simulationsLock.withReadLock([&] {
        for (auto keyAndSim : _simulations) {
            actor(keyAndSim.second);
        }
    });
}
