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

ZoneTracker::ZoneTracker() {
}

void ZoneTracker::trackZone(EntityItemPointer newZone) {

    auto zone = std::dynamic_pointer_cast<ZoneEntityItem>(newZone);
    qDebug() << "*** *** TRACKING ZONE" << newZone->getID();
    _zones.insert(newZone->getID(), newZone);
}

void ZoneTracker::forgetZone(EntityItemPointer goingAwayZone) {
    _zones.remove(goingAwayZone->getID());
}

void ZoneTracker::setParent(EntityItemID child, EntityItemID parent) {
    if (_childrenMap.contains(parent)) {
        ZoneChildren& zoneChildren = _childrenMap[parent];
        if (parent == UNKNOWN_ENTITY_ID) {
            zoneChildren.remove(child);
        } else {
            zoneChildren.insert(child);
        }
    } else {
        ZoneChildren zoneChildren;
        if (parent != UNKNOWN_ENTITY_ID) {
            zoneChildren.insert(child);
        }
        _childrenMap[parent] = zoneChildren;
    }
}


QList<EntityItemPointer> ZoneTracker::getChildrenOf(EntityItemID parent) {
    QList<EntityItemPointer> children;
    if (_childrenMap.contains(parent)) {
        foreach (EntityItemID childID, _childrenMap[parent]) {
            EntityItemPointer child = _defaultTree->findEntityByEntityItemID(childID);
            children << child;
        }
    }
    return children;
}
