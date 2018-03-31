//
//  AssignmentActionOffset.cpp
//  assignment-client/src/dynamics/
//
//  Created by Seth Alves 2018-3-31
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "QVariantGLM.h"

#include "AssignmentActionOffset.h"


const uint16_t AssignmentActionOffset::offsetVersion = 1;

AssignmentActionOffset::AssignmentActionOffset(const QUuid& id, EntityItemPointer ownerEntity) :
    EntityDynamic(DYNAMIC_TYPE_OFFSET, id, ownerEntity),
    _pointToOffsetFrom(0.0f),
    _linearDistance(0.0f),
    _linearTimeScale(FLT_MAX),
    _positionalTargetSet(false) {
    #if WANT_DEBUG
    qCDebug(physics) << "AssignmentActionOffset::AssignmentActionOffset";
    #endif
}

AssignmentActionOffset::~AssignmentActionOffset() {
    #if WANT_DEBUG
    qCDebug(physics) << "AssignmentActionOffset::~AssignmentActionOffset";
    #endif
}

bool AssignmentActionOffset::updateArguments(QVariantMap arguments) {
    glm::vec3 pointToOffsetFrom;
    float linearTimeScale;
    float linearDistance;

    bool needUpdate = false;
    bool somethingChanged = EntityDynamic::updateArguments(arguments);

    withReadLock([&]{
        bool ok = true;
        pointToOffsetFrom =
            EntityDynamicInterface::extractVec3Argument("offset action", arguments, "pointToOffsetFrom", ok, true);
        if (!ok) {
            pointToOffsetFrom = _pointToOffsetFrom;
        }

        ok = true;
        linearTimeScale =
            EntityDynamicInterface::extractFloatArgument("offset action", arguments, "linearTimeScale", ok, false);
        if (!ok) {
            linearTimeScale = _linearTimeScale;
        }

        ok = true;
        linearDistance =
            EntityDynamicInterface::extractFloatArgument("offset action", arguments, "linearDistance", ok, false);
        if (!ok) {
            linearDistance = _linearDistance;
        }

        // only change stuff if something actually changed
        if (somethingChanged ||
            _pointToOffsetFrom != pointToOffsetFrom ||
            _linearTimeScale != linearTimeScale ||
            _linearDistance != linearDistance) {
            needUpdate = true;
        }
    });


    if (needUpdate) {
        withWriteLock([&] {
            _pointToOffsetFrom = pointToOffsetFrom;
            _linearTimeScale = linearTimeScale;
            _linearDistance = linearDistance;
            _positionalTargetSet = true;
            _active = true;

            auto ownerEntity = _ownerEntity.lock();
            if (ownerEntity) {
                ownerEntity->setDynamicDataDirty(true);
                ownerEntity->setDynamicDataNeedsTransmit(true);
            }
        });
        // activateBody(); // XXX
    }

    return true;
}

/**jsdoc
 * The <code>"offset"</code> {@link Entities.ActionType|ActionType} moves an entity so that it is a set distance away from a
 * target point.
 * It has arguments in addition to the common {@link Entities.ActionArguments|ActionArguments}.
 *
 * @typedef {object} Entities.ActionArguments-Offset
 * @property {Vec3} pointToOffsetFrom=0,0,0 - The target point to offset the entity from.
 * @property {number} linearDistance=0 - The distance away from the target point to position the entity.
 * @property {number} linearTimeScale=34e+38 - Controls how long it takes for the entity's position to catch up with the
 *     target offset. The value is the time for the action to catch up to 1/e = 0.368 of the target value, where the action
 *     is applied using an exponential decay.
 */
QVariantMap AssignmentActionOffset::getArguments() {
    QVariantMap arguments = EntityDynamic::getArguments();
    withReadLock([&] {
        arguments["pointToOffsetFrom"] = glmToQMap(_pointToOffsetFrom);
        arguments["linearTimeScale"] = _linearTimeScale;
        arguments["linearDistance"] = _linearDistance;
    });
    return arguments;
}

QByteArray AssignmentActionOffset::serialize() const {
    QByteArray ba;
    QDataStream dataStream(&ba, QIODevice::WriteOnly);
    dataStream << DYNAMIC_TYPE_OFFSET;
    dataStream << getID();
    dataStream << AssignmentActionOffset::offsetVersion;

    withReadLock([&] {
        dataStream << _pointToOffsetFrom;
        dataStream << _linearDistance;
        dataStream << _linearTimeScale;
        dataStream << _positionalTargetSet;
        dataStream << localTimeToServerTime(_expires);
        dataStream << _tag;
    });

    return ba;
}

void AssignmentActionOffset::deserialize(QByteArray serializedArguments) {
    QDataStream dataStream(serializedArguments);

    EntityDynamicType type;
    dataStream >> type;
    assert(type == getType());

    QUuid id;
    dataStream >> id;
    assert(id == getID());

    uint16_t serializationVersion;
    dataStream >> serializationVersion;
    if (serializationVersion != AssignmentActionOffset::offsetVersion) {
        return;
    }

    withWriteLock([&] {
        dataStream >> _pointToOffsetFrom;
        dataStream >> _linearDistance;
        dataStream >> _linearTimeScale;
        dataStream >> _positionalTargetSet;

        quint64 serverExpires;
        dataStream >> serverExpires;
        _expires = serverTimeToLocalTime(serverExpires);

        dataStream >> _tag;
        _active = true;
    });
}
