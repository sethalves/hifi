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


typedef QSet<EntityItemID> ZoneChildren;
typedef QHash<EntityItemID, ZoneChildren> ZoneChildMap;

class ZoneTracker : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    ZoneTracker();
    void setDefaultTree(EntityTreePointer defaultTree) { assert(defaultTree); _defaultTree = defaultTree; }
    EntityTreePointer getDefaultTree() { return _defaultTree; }
    void trackZone(EntityItemPointer newZone);
    void forgetZone(EntityItemPointer goingAwayZone);
    void setParent(EntityItemID child, EntityItemID parent);
    QList<EntityItemPointer> getChildrenOf(EntityItemID parent);
    QList<EntityItemPointer> getChildrenOf(EntityItemPointer parent) { return getChildrenOf(parent->getID()); }
private:
    QHash<EntityItemID, EntityItemPointer> _zones;
    QSet<EntityItemID> _needsReparent;
    EntityTreePointer _defaultTree;
    ZoneChildMap _childrenMap;
};


#endif // hifi_ZoneTracker_h
