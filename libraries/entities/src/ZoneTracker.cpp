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
    _zones.insert(newZone->getID(), newZone);
}


void ZoneTracker::forgetZone(EntityItemPointer goingAwayZone) {
    _zones.remove(goingAwayZone->getID());
}

void ZoneTracker::reparent(EntityItemID needsReparentID) {
    _needsReparent.insert(needsReparentID);
}


void ZoneTracker::doReparentings(EntityTreePointer defaultTree) {
    QMutableSetIterator<EntityItemID> iter(_needsReparent);
    while (iter.hasNext()) {
        EntityItemID entityID = iter.next();
        EntityItemPointer entity = findEntityByEntityItemID(defaultTree, entityID);
        if (!entity) {
            qDebug() << "ZoneTracker cannot reparent unknown entity:" << entityID;
            iter.remove();
            continue;
        }

        EntityItemID parentID = entity->getParentID();
        EntityTreePointer destinationTree;
        if (parentID == UNKNOWN_ENTITY_ID) {
            destinationTree = defaultTree;
        } else {
            EntityItemPointer parent = findEntityByEntityItemID(defaultTree, parentID);
            if (!parent) {
                // parent may not yet be loaded
                continue;
            }

            if (parent->getType() != EntityTypes::Zone) {
                qDebug() << "ZoneTracker cannot reparent entity" << entityID << "to Parent"
                         << parentID << "of type" << parent->getType();
                iter.remove();
                continue;
            }
            auto parentAsZone = std::dynamic_pointer_cast<ZoneEntityItem>(parent);
            destinationTree = parentAsZone->getSubTree();
        }

        EntityTreePointer sourceTree = entity->getTree();
        if (sourceTree == destinationTree) {
            qDebug() << "ZoneTracker cannot move" << entityID << "from zone" << parentID << "to itself";
            iter.remove();
            continue;
        }

        // pull entity out of one tree
        sourceTree->deleteEntity(entityID, true, true);

        // insert it into another
        AddEntityOperator theOperator(destinationTree, entity);
        destinationTree->recurseTreeWithOperator(&theOperator);
        destinationTree->postAddEntity(entity);
        iter.remove();

        qDebug() << "XXXXXXX XXXXXX XXXXXXXX XXXXXX moved" << entityID << "to" << parentID;
    }
}


EntityItemPointer ZoneTracker::addEntity(const EntityItemID& entityID,
                                         const EntityItemProperties& properties,
                                         EntityTreePointer defaultTree) {

    EntityItemID parentID = properties.getParentID();
    if (parentID != UNKNOWN_ENTITY_ID) {
        EntityItemPointer parentEntity = defaultTree->findEntityByEntityItemID(parentID);
        if (parentEntity) {
            if (parentEntity->getType() == EntityTypes::Zone) {
                auto zoneEntityItem = std::dynamic_pointer_cast<ZoneEntityItem>(parentEntity);
                qDebug() << "\n\nAdding" << entityID << "to parent zone" << parentEntity->getID();
                return zoneEntityItem->getSubTree()->addEntity(entityID, properties);
            } else {
                qDebug() << "Cannot add entity" << entityID << "to Parent" << parentID << "of type" << parentEntity->getType();
            }
        } else {
            // entity will be placed in top-level Tree, until its parent is loaded.
            qDebug() << "Entity" << entityID << "has unknown parent:" << parentID;
            reparent(entityID);

        }
    }

    auto result = defaultTree->addEntity(entityID, properties);
    doReparentings(defaultTree);
    return result;
}


EntityItemPointer ZoneTracker::findEntityByEntityItemID(EntityTreePointer defaultTree, const EntityItemID& entityID) {
    EntityItemPointer result = defaultTree->findEntityByEntityItemID(entityID);
    if (result) {
        return result;
    }

    for (QHash<EntityItemID, EntityItemPointer>::iterator it = _zones.begin(); it != _zones.end(); ) {
        const EntityItemID entityID = it.key();
        const EntityItemPointer& entity = it.value();

        if (entity->getType() != EntityTypes::Zone) {
            qDebug() << "ZoneTracker found non-Zone entity in _zones list";
            it = _zones.erase(it);
            continue;
        }

        auto zone = std::dynamic_pointer_cast<ZoneEntityItem>(entity);
        ++it;

        result = zone->getSubTree()->findEntityByEntityItemID(entityID);
        if (result) {
            return result;
        }
    }

    return nullptr;
}
