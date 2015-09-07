//
//  ZoneTracker.h
//  libraries/entities/src
//
//  Created by Seth Alves on 2015-9-6.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ZoneTracker_h
#define hifi_ZoneTracker_h

#include <QtCore/QObject>
#include <QtCore/QHash>

#include <DependencyManager.h>

#include "EntityTree.h"
#include "EntityItem.h"

class ZoneTracker : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    ZoneTracker();
    void trackZone(EntityItemPointer newZone);
    void forgetZone(EntityItemPointer goingAwayZone);
    EntityItemPointer addEntity(const EntityItemID& entityID,
                                const EntityItemProperties& properties,
                                EntityTreePointer defaultTree);

private:
    QHash<EntityItemID, EntityItemPointer> _zones;
};


#endif // hifi_ZoneTracker_h
