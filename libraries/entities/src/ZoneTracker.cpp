//
//  ZoneTracker.cpp
//  libraries/entities/src
//
//  Created by Seth Alves on 2015-9-6.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <SharedUtil.h>

#include "ZoneTracker.h"
#include "ZoneEntityItem.h"
#include "AddEntityOperator.h"
#include "EntitySimulation.h"

ZoneTracker::ZoneTracker() {
}

void ZoneTracker::trackZone(EntityItemPointer newZone) {

    auto zone = std::dynamic_pointer_cast<ZoneEntityItem>(newZone);
    qDebug() << "*** *** TRACKING ZONE" << newZone->getID();
    _zones.insert(newZone->getID(), std::static_pointer_cast<ZoneEntityItem>(newZone));

    foreach (EntityItemPointer child, getChildrenOf(newZone->getID())) {
        child->refreshParentEntityItemPointer();
    }
}

void ZoneTracker::forgetZone(EntityItemPointer goingAwayZone) {
    _zones.remove(goingAwayZone->getID());
}

void ZoneTracker::setParent(EntityItemID childID, EntityItemID parent) {
    if (_childrenMap.contains(parent)) {
        ZoneChildren& zoneChildren = _childrenMap[parent];
        if (parent == UNKNOWN_ENTITY_ID) {
            zoneChildren.remove(childID);
        } else {
            zoneChildren.insert(childID);
        }
    } else {
        ZoneChildren zoneChildren;
        if (parent != UNKNOWN_ENTITY_ID) {
            zoneChildren.insert(childID);
        }
        _childrenMap[parent] = zoneChildren;
    }

    EntityItemPointer child = _defaultTree->findEntityByEntityItemID(childID);
    if (child) {
        child->refreshParentEntityItemPointer();
    }
}


QList<EntityItemPointer> ZoneTracker::getChildrenOf(EntityItemID parent) {
    QList<EntityItemPointer> children;
    if (_childrenMap.contains(parent)) {
        foreach (EntityItemID childID, _childrenMap[parent]) {
            EntityItemPointer child = _defaultTree->findEntityByEntityItemID(childID);
            if (child && child->getParentID() == parent) {
                children << child;
            }
        }
    }
    return children;
}


EntitySimulationPointer ZoneTracker::getSimulationByReferential(QUuid referential) {
    EntitySimulationPointer result;
    _defaultTree->withReadLock([&] {
            EntityItemPointer parentEntityItem = _defaultTree->findEntityByEntityItemID(referential);
            auto zone = std::dynamic_pointer_cast<ZoneEntityItem>(parentEntityItem);
            if (zone) {
                result = zone->getSubEntitySimulation();
            } else {
                result = _defaultTree->getSimulation();
            }
        });
    return result;
}

QList<EntitySimulationPointer> ZoneTracker::getAllSimulations() {
    QList<EntitySimulationPointer> allSimulations;
    allSimulations << _defaultTree->getSimulation();
    foreach (ZoneEntityItemPointer parentEntityItem, getAllZones()) {
        auto zone = std::dynamic_pointer_cast<ZoneEntityItem>(parentEntityItem);
        if (zone) {
            EntitySimulationPointer zoneSimulation = zone->getSubEntitySimulation();
            if (zoneSimulation) {
                allSimulations << zoneSimulation;
            }
        }
    }
    return allSimulations;
}
