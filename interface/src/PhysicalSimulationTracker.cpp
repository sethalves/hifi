//
//  SpatiallyNestable.cpp
//  interface/src/
//
//  Created by Seth Alves on 2016-5-14
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QObject>

#include <EntityTreeRenderer.h>
#include "PhysicalEntitySimulation.h"
#include "PhysicsEngine.h"

#include "PhysicalSimulationTracker.h"

EntitySimulationPointer PhysicalSimulationTracker::newSimulation(QUuid key, EntityTreePointer tree) {

    PhysicalEntitySimulationPointer entitySimulation { new PhysicalEntitySimulation(key) };
    PhysicsEnginePointer physicsEngine { new PhysicsEngine(Vectors::ZERO) };
    physicsEngine->init();
    entitySimulation->init(tree, physicsEngine, _entityEditSender);

    _simulationsLock.withWriteLock([&] {
        _simulations[key] = entitySimulation;
    });

    // connect the _entityCollisionSystem to our EntityTreeRenderer since that's what handles running entity scripts
    QObject::connect(entitySimulation.get(), SIGNAL(&EntitySimulation::entityCollisionWithEntity),
                     tree.get(), SLOT(&EntityTreeRenderer::entityCollisionWithEntity));

    return entitySimulation;
}
