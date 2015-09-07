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

ZoneTracker::ZoneTracker() {
}

void ZoneTracker::trackZone(EntityItemPointer newZone) {
    _zones.insert(newZone->getID(), newZone);
}

void ZoneTracker::forgetZone(EntityItemPointer goingAwayZone) {
    _zones.remove(goingAwayZone->getID());
}


EntityItemPointer ZoneTracker::addEntity(const EntityItemID& entityID,
                                         const EntityItemProperties& properties,
                                         EntityTreePointer defaultTree) {
    return defaultTree->addEntity(entityID, properties);
}
