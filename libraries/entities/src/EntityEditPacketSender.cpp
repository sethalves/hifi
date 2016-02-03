//
//  EntityEditPacketSender.cpp
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QJsonDocument>

#include <assert.h>
#include <PerfStat.h>
#include <OctalCode.h>
#include <udt/PacketHeaders.h>
#include "EntityEditPacketSender.h"
#include "EntitiesLogging.h"
#include "EntityItem.h"
#include "EntityItemProperties.h"

EntityEditPacketSender::EntityEditPacketSender() {
    auto& packetReceiver = DependencyManager::get<NodeList>()->getPacketReceiver();
    packetReceiver.registerDirectListener(PacketType::EntityEditNack, this, "processEntityEditNackPacket");
}

void EntityEditPacketSender::processEntityEditNackPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode) {
    if (_shouldProcessNack) {
        processNackPacket(*message, sendingNode);
    }
}

void EntityEditPacketSender::adjustEditPacketForClockSkew(PacketType type, QByteArray& buffer, int clockSkew) {
    if (type == PacketType::EntityAdd || type == PacketType::EntityEdit) {
        EntityItem::adjustEditPacketForClockSkew(buffer, clockSkew);
    }
}

void EntityEditPacketSender::queueEditEntityMessage(PacketType type, EntityItemID entityItemID,
                                                    const EntityItemProperties& properties) {
    if (!_shouldSend) {
        return; // bail early
    }
    if (properties.getClientOnly()) {
        // this is an avatar-based entity.  update our avatar-data rather than sending to the entity-server
        assert(_myAvatar);
        QScriptValue scriptProperties = EntityItemNonDefaultPropertiesToScriptValue(&_scriptEngine, properties);
        QVariant variantProperties = scriptProperties.toVariant();
        QByteArray jsonProperties = QJsonDocument::fromVariant(variantProperties).toJson();
        _myAvatar->updateAvatarEntity(entityItemID, jsonProperties);
        return;
    }

    QByteArray bufferOut(NLPacket::maxPayloadSize(type), 0);

    if (EntityItemProperties::encodeEntityEditPacket(type, entityItemID, properties, bufferOut)) {
        #ifdef WANT_DEBUG
            qCDebug(entities) << "calling queueOctreeEditMessage()...";
            qCDebug(entities) << "    id:" << entityItemID;
            qCDebug(entities) << "    properties:" << properties;
        #endif
        queueOctreeEditMessage(type, bufferOut);
    }
}

void EntityEditPacketSender::queueEraseEntityMessage(const EntityItemID& entityItemID) {
    if (!_shouldSend) {
        return; // bail early
    }

    QByteArray bufferOut(NLPacket::maxPayloadSize(PacketType::EntityErase), 0);

    if (EntityItemProperties::encodeEraseEntityMessage(entityItemID, bufferOut)) {
        queueOctreeEditMessage(PacketType::EntityErase, bufferOut);
    }
}
