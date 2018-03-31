//
//  AssignmentDynamcFactory.cpp
//  assignment-client/src/
//
//  Created by Seth Alves on 2015-6-19
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AssignmentDynamicFactory.h"
#include "AssignmentActionOffset.h"


EntityDynamicPointer assignmentDynamicFactory(EntityDynamicType type, const QUuid& id, EntityItemPointer ownerEntity) {
    switch (type) {
        case DYNAMIC_TYPE_NONE:
            return EntityDynamicPointer();
        case DYNAMIC_TYPE_OFFSET:
            return std::make_shared<AssignmentActionOffset>(id, ownerEntity);
        case DYNAMIC_TYPE_SPRING:
            qDebug() << "The 'spring' Action is deprecated.  Replacing with 'tractor' Action.";
        case DYNAMIC_TYPE_TRACTOR:
            // return std::make_shared<ObjectActionTractor>(id, ownerEntity);
            return EntityDynamicPointer();
        case DYNAMIC_TYPE_HOLD:
            // return std::make_shared<AvatarActionHold>(id, ownerEntity);
            return EntityDynamicPointer();
        case DYNAMIC_TYPE_TRAVEL_ORIENTED:
            // return std::make_shared<ObjectActionTravelOriented>(id, ownerEntity);
            return EntityDynamicPointer();
        case DYNAMIC_TYPE_HINGE:
            // return std::make_shared<ObjectConstraintHinge>(id, ownerEntity);
            return EntityDynamicPointer();
        case DYNAMIC_TYPE_FAR_GRAB:
            // return std::make_shared<AvatarActionFarGrab>(id, ownerEntity);
            return EntityDynamicPointer();
        case DYNAMIC_TYPE_SLIDER:
            // return std::make_shared<ObjectConstraintSlider>(id, ownerEntity);
            return EntityDynamicPointer();
        case DYNAMIC_TYPE_BALL_SOCKET:
            // return std::make_shared<ObjectConstraintBallSocket>(id, ownerEntity);
            return EntityDynamicPointer();
        case DYNAMIC_TYPE_CONE_TWIST:
            // return std::make_shared<ObjectConstraintConeTwist>(id, ownerEntity);
            return EntityDynamicPointer();
    }

    qDebug() << "upsupported entity dynamic type";
    return EntityDynamicPointer();
}

EntityDynamicPointer AssignmentDynamicFactory::factory(EntityDynamicType type,
                                                     const QUuid& id,
                                                     EntityItemPointer ownerEntity,
                                                     QVariantMap arguments) {
    EntityDynamicPointer dynamic = assignmentDynamicFactory(type, id, ownerEntity);
    if (dynamic) {
        bool ok = dynamic->updateArguments(arguments);
        if (ok) {
            return dynamic;
        }
    }
    return nullptr;
}


EntityDynamicPointer AssignmentDynamicFactory::factoryBA(EntityItemPointer ownerEntity, QByteArray data) {
    QDataStream serializedDynamicDataStream(data);
    EntityDynamicType type;
    QUuid id;

    serializedDynamicDataStream >> type;
    serializedDynamicDataStream >> id;

    EntityDynamicPointer dynamic = assignmentDynamicFactory(type, id, ownerEntity);

    if (dynamic) {
        dynamic->deserialize(data);
    }
    return dynamic;
}
