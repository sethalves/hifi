//
//  RecurseOctreeToMapOperator.cpp
//  libraries/entities/src
//
//  Created by Seth Alves on 3/16/15.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RecurseOctreeToMapOperator.h"
#include "ZoneEntityItem.h"


RecurseOctreeToMapOperator::RecurseOctreeToMapOperator(QVariantMap& map,
                                                       OctreeElementPointer top,
                                                       QScriptEngine* engine,
                                                       bool skipDefaultValues) :
        RecurseOctreeOperator(),
        _map(map),
        _top(top),
        _engine(engine),
        _skipDefaultValues(skipDefaultValues)
{
    // if some element "top" was given, only save information for that element and its children.
    if (_top) {
        _withinTop = false;
    } else {
        // top was NULL, export entire tree.
        _withinTop = true;
    }
};

bool RecurseOctreeToMapOperator::preRecursion(OctreeElementPointer element) {
    if (element == _top) {
        _withinTop = true;
    }
    return true;
}

bool RecurseOctreeToMapOperator::postRecursion(OctreeElementPointer element) {

    EntityItemProperties defaultProperties;

    EntityTreeElementPointer entityTreeElement = std::static_pointer_cast<EntityTreeElement>(element);
    QVariantList entitiesQList = qvariant_cast<QVariantList>(_map["Entities"]);

    entityTreeElement->forEachEntity([&](EntityItemPointer entityItem) {
        EntityItemProperties properties = entityItem->getProperties();
        QScriptValue qScriptValues;
        if (_skipDefaultValues) {
            qScriptValues = EntityItemNonDefaultPropertiesToScriptValue(_engine, properties);
        } else {
            qScriptValues = EntityItemPropertiesToScriptValue(_engine, properties);
        }
        entitiesQList << qScriptValues.toVariant();

        // if (entityItem->getType() == EntityTypes::Zone) {
        //     // Zones have their own sub-EntityTrees
        //     auto zoneEntityItem = std::dynamic_pointer_cast<ZoneEntityItem>(entityItem);
        //     EntityTreePointer subTree = zoneEntityItem->getSubTree();
        //     EntityTreeElementPointer subRoot = subTree->getRoot();
        //     QVariantMap entityDescription;
        //     entityDescription["Entities"] = QVariantList();
        //     RecurseOctreeToMapOperator theOperator(entityDescription, subRoot, _engine, _skipDefaultValues);
        //     subTree->recurseTreeWithOperator(&theOperator);
        //     QVariantList subEntitiesQList = qvariant_cast<QVariantList>(entityDescription["Entities"]);
        //     entitiesQList << subEntitiesQList;
        // }
    });

    _map["Entities"] = entitiesQList;
    if (element == _top) {
        _withinTop = false;
    }
    return true;
}
