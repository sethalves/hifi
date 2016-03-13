//
//  ParentMissingOperator.cpp
//  libraries/entities/src
//
//  Created by Seth Alves on 2016-3-1.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "EntityTree.h"
#include "ParentMissingOperator.h"

ParentMissingOperator::~ParentMissingOperator() {
    if (_tree) {
        _tree->deleteEntities(_toDelete, true, true);
    }
}

bool ParentMissingOperator::postRecursion(OctreeElementPointer element) {
    EntityTreeElementPointer entityTreeElement = std::static_pointer_cast<EntityTreeElement>(element);
    EntityTreePointer tree = entityTreeElement->getTree();
    _tree = tree;

    entityTreeElement->forEachEntity([&](EntityItemPointer entityItem) {
        if (_goneParentIDs.indexOf(entityItem->getParentID()) >= 0) {
            _toDelete += entityItem->getEntityItemID();
        }
    });
    return true;
}
