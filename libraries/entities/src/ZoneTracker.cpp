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
    qDebug() << "*** *** TRACKING ZONE" << newZone->getID() << "tree is" << zone->getSubTree().get()
             << "root element is" << zone->getSubTree()->getRoot().get();

    _zones.insert(newZone->getID(), newZone);
}


void ZoneTracker::forgetZone(EntityItemPointer goingAwayZone) {
    _zones.remove(goingAwayZone->getID());
}

void ZoneTracker::reparent(EntityItemID needsReparentID) {
    _needsReparent.insert(needsReparentID);
}


void ZoneTracker::doReparentings() {
    if (!_defaultTree) {
        return;
    }
    QMutableSetIterator<EntityItemID> iter(_needsReparent);
    while (iter.hasNext()) {
        EntityItemID entityID = iter.next();
        EntityItemPointer entity = findEntityByEntityItemID(entityID);
        if (!entity) {
            qDebug() << "ZoneTracker cannot reparent unknown entity:" << entityID;
            iter.remove();
            continue;
        }

        EntityItemID parentID = entity->getParentID();
        EntityTreePointer destinationTree;
        if (parentID == UNKNOWN_ENTITY_ID) {
            destinationTree = _defaultTree;
        } else {
            EntityItemPointer parent = findEntityByEntityItemID(parentID);
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

        sourceTree->consistencyCheck();
        destinationTree->consistencyCheck();


        if (sourceTree == destinationTree) {
            qDebug() << "ZoneTracker cannot move" << entityID << "from zone" << parentID << "to itself";
            iter.remove();
            continue;
        }

        sourceTree->consistencyCheck();
        destinationTree->consistencyCheck();

        qDebug() << "BEFORE MOVE source =" << sourceTree.get() << "dest =" << destinationTree.get();
        qDebug() << "BEFORE MOVE" << entityID << "is in" << findEntityByEntityItemID(entityID).get();

        // pull entity out of one tree
        sourceTree->deleteEntity(entityID, true, true, false);

        sourceTree->consistencyCheck();
        destinationTree->consistencyCheck();

        qDebug() << "AFTER DELETE" << entityID << "is in" << findEntityByEntityItemID(entityID).get();

        // insert it into another
        // destinationTree->addEntity(entityID, entity->getProperties());


        qDebug() << "MID-ADD- entity->getElement =" << entity->getElement().get();
        qDebug() << "MID-ADD- entity->getTree =" << entity->getTree().get();
        if (entity->getTree()) {
            qDebug() << "MID-ADD- entity->getTree->count =" << entity->getTree()->getAllEntities().count();
        }

        AddEntityOperator theOperator(destinationTree, entity);
        destinationTree->recurseTreeWithOperator(&theOperator);
        destinationTree->postAddEntity(entity);

        sourceTree->consistencyCheck();
        destinationTree->consistencyCheck();

        qDebug() << "MID-ADD entity->getElement =" << entity->getElement().get();
        qDebug() << "MID-ADD entity->getTree =" << entity->getTree().get();
        if (entity->getTree()) {
            qDebug() << "MID-ADD entity->getTree->count =" << entity->getTree()->getAllEntities().count();
        }
        destinationTree->setContainingElement(entityID, entity->getElement());
        qDebug() << "MID-ADD+ entity->getElement =" << entity->getElement().get();
        qDebug() << "MID-ADD+ entity->getTree =" << entity->getTree().get();
        if (entity->getTree()) {
            qDebug() << "MID-ADD+ entity->getTree->count =" << entity->getTree()->getAllEntities().count();
        }

        sourceTree->consistencyCheck();
        destinationTree->consistencyCheck();

        iter.remove();

        qDebug() << "AFTER ADD" << entityID << "is in" << findEntityByEntityItemID(entityID).get();

        qDebug() << "XXXXXXX XXXXXX XXXXXXXX XXXXXX moved" << entityID << "to" << parentID;
    }
}


EntityItemPointer ZoneTracker::addEntity(const EntityItemID& entityID,
                                         const EntityItemProperties& properties) {
    assert(_defaultTree);
    EntityItemID parentID = properties.getParentID();
    if (parentID != UNKNOWN_ENTITY_ID) {
        EntityItemPointer parentEntity = _defaultTree->findEntityByEntityItemID(parentID);
        if (parentEntity) {
            if (parentEntity->getType() == EntityTypes::Zone) {
                auto zoneEntityItem = std::dynamic_pointer_cast<ZoneEntityItem>(parentEntity);
                qDebug() << "\n\nAdding" << entityID << "to parent zone" << parentEntity->getID();
                EntityItemPointer result = zoneEntityItem->getSubTree()->addEntity(entityID, properties);
                // zoneEntityItem->getSubTree()->setContainingElement(entityID, result->getElement());
                qDebug() << "result is" << result->getID() << "tree is"
                         << result->getTree().get() << "default is" << _defaultTree.get()
                         << "zone child-count is" << zoneEntityItem->getTree()->getAllEntities().size();
                qDebug() << "child elt is" << result->getElement().get();
                qDebug() << "child elt's tree is" << result->getElement()->getTree().get();
                qDebug() << "_entityToElementMap.contains ="
                         << zoneEntityItem->getSubTree()->_entityToElementMap.contains(result->getID());
                qDebug() << "_entityToElementMap[id] ="
                         << zoneEntityItem->getSubTree()->_entityToElementMap[result->getID()].get();
                qDebug() << "_entityToElementMap[id]->size() ="
                         << (zoneEntityItem->getSubTree()->_entityToElementMap[result->getID()]->_entityItems)->size();
                return result;
            } else {
                qDebug() << "Cannot add entity" << entityID << "to Parent" << parentID << "of type" << parentEntity->getType();
            }
        } else {
            // entity will be placed in top-level Tree, until its parent is loaded.
            qDebug() << "Entity" << entityID << "has unknown parent:" << parentID;
            reparent(entityID);

        }
    }

    auto result = _defaultTree->addEntity(entityID, properties);
    doReparentings();
    return result;
}


EntityItemPointer ZoneTracker::findEntityByEntityItemID(const EntityItemID& entityID) {
    if (entityID == EntityItemID("{4a3544d6-a8f3-4717-a929-b48c01cf1d20}") ||
        entityID == EntityItemID("{2ff5305e-2b19-4d70-a5a7-0990aef18b98}"))
        qDebug() << "    ZoneTracker looking up" << entityID;

    assert(_defaultTree);
    EntityItemPointer result = _defaultTree->findEntityByEntityItemID(entityID);
    if (result) {
        if (entityID == EntityItemID("{4a3544d6-a8f3-4717-a929-b48c01cf1d20}") ||
            entityID == EntityItemID("{2ff5305e-2b19-4d70-a5a7-0990aef18b98}"))
            qDebug() << "    found in default tree:" << _defaultTree.get();
        return result;
    }

    for (QHash<EntityItemID, EntityItemPointer>::iterator it = _zones.begin(); it != _zones.end(); ) {
        const EntityItemID entityID = it.key();
        const EntityItemPointer& entity = it.value();

        if (entity->getType() != EntityTypes::Zone) {
            if (entityID == EntityItemID("{4a3544d6-a8f3-4717-a929-b48c01cf1d20}") ||
                entityID == EntityItemID("{2ff5305e-2b19-4d70-a5a7-0990aef18b98}"))
                qDebug() << "ZoneTracker found non-Zone entity in _zones list";
            it = _zones.erase(it);
            continue;
        }

        auto zone = std::dynamic_pointer_cast<ZoneEntityItem>(entity);
        ++it;

        result = zone->getSubTree()->findEntityByEntityItemID(entityID);
        if (result) {
            if (entityID == EntityItemID("{4a3544d6-a8f3-4717-a929-b48c01cf1d20}") ||
                entityID == EntityItemID("{2ff5305e-2b19-4d70-a5a7-0990aef18b98}"))
                qDebug() << "    found in subtree of" << zone->getID() << "tree:" << zone->getSubTree().get();
            return result;
        }
        if (entityID == EntityItemID("{4a3544d6-a8f3-4717-a929-b48c01cf1d20}") ||
            entityID == EntityItemID("{2ff5305e-2b19-4d70-a5a7-0990aef18b98}"))
            qDebug() << "    NOT found in subtree of" << zone->getID() << "tree:" << zone->getSubTree().get();
    }

    if (entityID == EntityItemID("{4a3544d6-a8f3-4717-a929-b48c01cf1d20}") ||
        entityID == EntityItemID("{2ff5305e-2b19-4d70-a5a7-0990aef18b98}"))
        qDebug() << "    NOT found at all.";
    return nullptr;
}
