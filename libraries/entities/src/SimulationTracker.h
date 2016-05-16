//
//  SimulationTracker.h
//  libraries/entities/src
//
//  Created by Seth Alves on 2016-5-14
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef hifi_SimulationTracker_h
#define hifi_SimulationTracker_h

#include <memory>
#include <QUuid>
#include <DependencyManager.h>
#include "shared/ReadWriteLockable.h"

class EntitySimulation;
using EntitySimulationPointer = std::shared_ptr<EntitySimulation>;

class SimulationTracker: public Dependency {

public:
    void forEachSimulation(std::function<void(EntitySimulationPointer)> actor);

    void addSimulation(QUuid key, EntitySimulationPointer simulation);
    void removeSimulation(QUuid key);
    EntitySimulationPointer getSimulationByKey(QUuid key);

    static const QUuid DEFAULT_SIMULATOR_ID;

private:
    mutable ReadWriteLockable _simulationsLock;
    std::map<QUuid, EntitySimulationPointer> _simulations;
};


#endif // hifi_SimulationTracker_h
