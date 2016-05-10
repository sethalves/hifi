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

#include <assert.h>
#include <QJsonDocument>
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

void EntityEditPacketSender::adjustEditPacketForClockSkew(PacketType type, QByteArray& buffer, qint64 clockSkew) {
    if (type == PacketType::EntityAdd || type == PacketType::EntityEdit) {
        EntityItem::adjustEditPacketForClockSkew(buffer, clockSkew);
    }
}

void EntityEditPacketSender::queueEditEntityMessage(PacketType type,
                                                    EntityTreePointer entityTree,
                                                    EntityItemID entityItemID,
                                                    const EntityItemProperties& properties) {
    if (!_shouldSend) {
        return; // bail early
    }
    if (properties.getClientOnly()) {
        // this is an avatar-based entity.  update our avatar-data rather than sending to the entity-server
        assert(_myAvatar);

        if (!entityTree) {
            qDebug() << "EntityEditPacketSender::queueEditEntityMessage null entityTree.";
            return;
        }
        EntityItemPointer entity = entityTree->findEntityByEntityItemID(entityItemID);
        if (!entity) {
            qDebug() << "EntityEditPacketSender::queueEditEntityMessage can't find entity.";
            return;
        }

        qDebug() << "sending edit message for client-only entity" << entity->getName() << entity->getID();

        // the properties that get serialized into the avatar identity packet should be the entire set
        // rather than just the ones being edited.
        entity->setProperties(properties);
        EntityItemProperties entityProperties = entity->getProperties();

        QScriptValue scriptProperties = EntityItemNonDefaultPropertiesToScriptValue(&_scriptEngine, entityProperties);
        QVariant variantProperties = scriptProperties.toVariant();
        QJsonDocument jsonProperties = QJsonDocument::fromVariant(variantProperties);

        // the ID of the parent/avatar changes from session to session.  use a special UUID to indicate the avatar
        QJsonObject jsonObject = jsonProperties.object();
        if (QUuid(jsonObject["parentID"].toString()) == _myAvatar->getID()) {
            jsonObject["parentID"] = AVATAR_SELF_ID.toString();
        }
        jsonProperties = QJsonDocument(jsonObject);

        QByteArray binaryProperties = jsonProperties.toBinaryData();

        qDebug() << "encoded entity data is: " << QString(binaryProperties);

        _myAvatar->updateAvatarEntity(entityItemID, binaryProperties);
        return;
    }

    qDebug() << "sending edit message for server entity" << properties.getName() << entityItemID;

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
