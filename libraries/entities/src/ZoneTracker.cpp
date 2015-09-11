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


#define GET_TREE_NAME(x) (x ? x->getName() : "null")


void ZoneTracker::trackZone(EntityItemPointer newZone) {

    auto zone = std::dynamic_pointer_cast<ZoneEntityItem>(newZone);
    qDebug() << "*** *** TRACKING ZONE" << newZone->getID() << "tree is" << GET_TREE_NAME(zone->getSubTree())
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

        qDebug() << "BEFORE MOVE source =" << GET_TREE_NAME(sourceTree) << "dest =" << GET_TREE_NAME(destinationTree);
        qDebug() << "BEFORE MOVE" << entityID << "is in" << GET_TREE_NAME(findEntityByEntityItemID(entityID));

        // pull entity out of one tree
        sourceTree->deleteEntity(entityID, true, true, false);

        assert(! sourceTree->findEntityByEntityItemID(entityID));

        sourceTree->consistencyCheck();
        destinationTree->consistencyCheck();

        qDebug() << "AFTER DELETE" << entityID << "is in" << findEntityByEntityItemID(entityID).get();

        // insert it into another
        // destinationTree->addEntity(entityID, entity->getProperties());


        qDebug() << "MID-ADD- entity->getElement =" << entity->getElement().get();
        qDebug() << "MID-ADD- entity->getTree =" << GET_TREE_NAME(entity->getTree());
        if (entity->getTree()) {
            qDebug() << "MID-ADD- entity->getTree->count =" << entity->getTree()->getAllEntities().count();
        }
        qDebug() << "MID-ADD- destinationTree->count =" << destinationTree->getAllEntities().count();

        AddEntityOperator theOperator(destinationTree, entity);
        destinationTree->recurseTreeWithOperator(&theOperator);
        destinationTree->postAddEntity(entity);

        sourceTree->consistencyCheck();
        destinationTree->consistencyCheck();

        qDebug() << "MID-ADD entity->getElement =" << entity->getElement().get();
        qDebug() << "MID-ADD entity->getTree =" << GET_TREE_NAME(entity->getTree());
        if (entity->getTree()) {
            qDebug() << "MID-ADD entity->getTree->count =" << entity->getTree()->getAllEntities().count();
        }
        qDebug() << "MID-ADD destinationTree->count =" << destinationTree->getAllEntities().count();
        destinationTree->setContainingElement(entityID, entity->getElement());
        qDebug() << "MID-ADD+ entity->getElement =" << entity->getElement().get();
        qDebug() << "MID-ADD+ entity->getTree =" << GET_TREE_NAME(entity->getTree());
        if (entity->getTree()) {
            qDebug() << "MID-ADD+ entity->getTree->count =" << entity->getTree()->getAllEntities().count();
        }
        qDebug() << "MID-ADD+ destinationTree->count =" << destinationTree->getAllEntities().count();

        sourceTree->consistencyCheck();
        destinationTree->consistencyCheck();

        entity->requiresRecalcBoxes();
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
                         << GET_TREE_NAME(result->getTree())
                         << "zone child-count is" << zoneEntityItem->getTree()->getAllEntities().size();
                qDebug() << "child elt is" << result->getElement().get();
                qDebug() << "child elt's tree is" << GET_TREE_NAME(result->getElement()->getTree());
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
    EntityItemPointer result = nullptr;

    if (entityID == EntityItemID("{4a3544d6-a8f3-4717-a929-b48c01cf1d20}") ||
        entityID == EntityItemID("{2ff5305e-2b19-4d70-a5a7-0990aef18b98}"))
        qDebug() << "    ZoneTracker looking up" << entityID;

    if (_defaultTree) {
        result = _defaultTree->findEntityByEntityItemID(entityID);
        if (result) {
            if (entityID == EntityItemID("{4a3544d6-a8f3-4717-a929-b48c01cf1d20}") ||
                entityID == EntityItemID("{2ff5305e-2b19-4d70-a5a7-0990aef18b98}"))
                qDebug() << "    ZoneTracker::findEntityByEntityItemID found" << entityID << "in default tree";
            return result;
        }
    }

    for (QHash<EntityItemID, EntityItemPointer>::iterator it = _zones.begin(); it != _zones.end(); ) {
        // const EntityItemID entityID = it.key();
        const EntityItemPointer& zoneEntity = it.value();

        if (zoneEntity->getType() != EntityTypes::Zone) {
            qDebug() << "ZoneTracker found non-Zone entity in _zones list";
            it = _zones.erase(it);
            continue;
        }

        auto zone = std::dynamic_pointer_cast<ZoneEntityItem>(zoneEntity);
        ++it;

        result = zone->getSubTree()->findEntityByEntityItemID(entityID);
        if (result) {
            if (entityID == EntityItemID("{4a3544d6-a8f3-4717-a929-b48c01cf1d20}") ||
                entityID == EntityItemID("{2ff5305e-2b19-4d70-a5a7-0990aef18b98}"))
                qDebug() << "    found in subtree of" << zone->getName();
            return result;
        }
        if (entityID == EntityItemID("{4a3544d6-a8f3-4717-a929-b48c01cf1d20}") ||
            entityID == EntityItemID("{2ff5305e-2b19-4d70-a5a7-0990aef18b98}"))
            qDebug() << "    NOT found in subtree of" << zone->getID()
                     << "tree:" << GET_TREE_NAME(zone->getTree());
    }

    // result = _defaultTree->findEntityByEntityItemID(entityID);
    // if (result) {
    //     if (entityID == EntityItemID("{4a3544d6-a8f3-4717-a929-b48c01cf1d20}") ||
    //         entityID == EntityItemID("{2ff5305e-2b19-4d70-a5a7-0990aef18b98}"))
    //         qDebug() << "    found in default tree:" << _defaultTree->getName();
    //     return result;
    // }


    if (entityID == EntityItemID("{4a3544d6-a8f3-4717-a929-b48c01cf1d20}") ||
        entityID == EntityItemID("{2ff5305e-2b19-4d70-a5a7-0990aef18b98}"))
        qDebug() << "    NOT found at all.";
    return nullptr;
}
