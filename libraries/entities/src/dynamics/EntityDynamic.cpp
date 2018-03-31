//
//  EntityDynamic.cpp
//  assignment-client/src/
//
//  Created by Seth Alves 2015-6-19
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "../EntitySimulation.h"

#include "EntityDynamic.h"

EntityDynamic::EntityDynamic(EntityDynamicType type, const QUuid& id, EntityItemPointer ownerEntity) :
    EntityDynamicInterface(type, id),
    _data(QByteArray()),
    _active(false),
    _ownerEntity(ownerEntity) {
}

EntityDynamic::~EntityDynamic() {
}

quint64 EntityDynamic::localTimeToServerTime(quint64 timeValue) const {
    // 0 indicates no expiration
    if (timeValue == 0) {
        return 0;
    }

    qint64 serverClockSkew = getEntityServerClockSkew();
    if (serverClockSkew < 0 && timeValue <= (quint64)(-serverClockSkew)) {
        return 1; // non-zero but long-expired value to avoid negative roll-over
    }

    return timeValue + serverClockSkew;
}

quint64 EntityDynamic::serverTimeToLocalTime(quint64 timeValue) const {
    // 0 indicates no expiration
    if (timeValue == 0) {
        return 0;
    }

    qint64 serverClockSkew = getEntityServerClockSkew();
    if (serverClockSkew > 0 && timeValue <= (quint64)serverClockSkew) {
        return 1; // non-zero but long-expired value to avoid negative roll-over
    }

    return timeValue - serverClockSkew;
}

qint64 EntityDynamic::getEntityServerClockSkew() const {
    auto nodeList = DependencyManager::get<NodeList>();

    auto ownerEntity = _ownerEntity.lock();
    if (!ownerEntity) {
        return 0;
    }

    const QUuid& entityServerNodeID = ownerEntity->getSourceUUID();
    auto entityServerNode = nodeList->nodeWithUUID(entityServerNodeID);
    if (entityServerNode) {
        return entityServerNode->getClockSkewUsec();
    }
    return 0;
}

void EntityDynamic::removeFromSimulation(EntitySimulationPointer simulation) const {
    withReadLock([&]{
        simulation->removeDynamic(_id);
        simulation->applyDynamicChanges();
    });
}

QByteArray EntityDynamic::serialize() const {
    QByteArray result;
    withReadLock([&]{
        result = _data;
    });
    return result;
}

void EntityDynamic::deserialize(QByteArray serializedArguments) {
    withWriteLock([&]{
        _data = serializedArguments;
    });
}

bool EntityDynamic::updateArguments(QVariantMap arguments) {
    bool somethingChanged = false;

    withWriteLock([&]{
        quint64 previousExpires = _expires;
        QString previousTag = _tag;

        bool ttlSet = true;
        float ttl = EntityDynamicInterface::extractFloatArgument("dynamic", arguments, "ttl", ttlSet, false);
        if (ttlSet) {
            quint64 now = usecTimestampNow();
            _expires = now + (quint64)(ttl * USECS_PER_SECOND);
        } else {
            _expires = 0;
        }

        bool tagSet = true;
        QString tag = EntityDynamicInterface::extractStringArgument("dynamic", arguments, "tag", tagSet, false);
        if (tagSet) {
            _tag = tag;
        } else {
            tag = "";
        }

        if (previousExpires != _expires || previousTag != _tag) {
            somethingChanged = true;
        }
    });

    return somethingChanged;
}

QVariantMap EntityDynamic::getArguments() {
    QVariantMap arguments;
    withReadLock([&]{
        if (_expires == 0) {
            arguments["ttl"] = 0.0f;
        } else {
            quint64 now = usecTimestampNow();
            arguments["ttl"] = (float)(_expires - now) / (float)USECS_PER_SECOND;
        }
        arguments["tag"] = _tag;

        // XXX
        // EntityItemPointer entity = _ownerEntity.lock();
        // if (entity) {
        //     ObjectMotionState* motionState = static_cast<ObjectMotionState*>(entity->getPhysicsInfo());
        //     if (motionState) {
        //         arguments["::active"] = motionState->isActive();
        //         arguments["::motion-type"] = motionTypeToString(motionState->getMotionType());
        //     } else {
        //         arguments["::no-motion-state"] = true;
        //     }
        // }
        arguments["isMine"] = isMine();
    });
    return arguments;
}
