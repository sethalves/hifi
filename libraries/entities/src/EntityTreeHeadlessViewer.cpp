//
//  EntityTreeHeadlessViewer.cpp
//  libraries/models/src
//
//  Created by Brad Hefta-Gaub on 2/26/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "EntityTreeHeadlessViewer.h"
#include "SimpleEntitySimulation.h"

EntityTreeHeadlessViewer::EntityTreeHeadlessViewer()
    :   OctreeHeadlessViewer(), _simulation(NULL) {
}

EntityTreeHeadlessViewer::~EntityTreeHeadlessViewer() {
}

void EntityTreeHeadlessViewer::init() {
    OctreeHeadlessViewer::init();
    if (!_simulation) {
        EntitySimulationPointer simpleSimulation = EntitySimulationPointer(new SimpleEntitySimulation());
        EntityTreePointer entityTree = std::static_pointer_cast<EntityTree>(_tree);
        entityTree->setSimulation(simpleSimulation);
        _simulation = simpleSimulation;
    }
}

void EntityTreeHeadlessViewer::update() {
    if (_tree) {
        EntityTreePointer tree = std::static_pointer_cast<EntityTree>(_tree);
        tree->withTryWriteLock([&] {
            tree->update();
        });
    }
}

void EntityTreeHeadlessViewer::processEraseMessage(NLPacket& packet, const SharedNodePointer& sourceNode) {
    std::static_pointer_cast<EntityTree>(_tree)->processEraseMessage(packet, sourceNode);
}
