//
//  EntityItem.cpp
//  libraries/models/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "EntityItem.h"

#include <QtCore/QObject>

#include <glm/gtx/transform.hpp>

#include <ByteCountCoding.h>
#include <GLMHelpers.h>
#include <Octree.h>
#include <PhysicsHelpers.h>
#include <RegisteredMetaTypes.h>
#include <SharedUtil.h> // usecTimestampNow()
#include <SoundCache.h>

#include "EntityScriptingInterface.h"
#include "EntitiesLogging.h"
#include "EntityTree.h"
#include "EntitySimulation.h"
#include "EntityActionFactoryInterface.h"


bool EntityItem::_sendPhysicsUpdates = true;
int EntityItem::_maxActionsDataSize = 800;

EntityItem::EntityItem(const EntityItemID& entityItemID) :
    _type(EntityTypes::Unknown),
    _id(entityItemID),
    _lastSimulated(0),
    _lastUpdated(0),
    _lastEdited(0),
    _lastEditedFromRemote(0),
    _lastEditedFromRemoteInRemoteTime(0),
    _created(UNKNOWN_CREATED_TIME),
    _changedOnServer(0),
    _transform(ENTITY_ITEM_DEFAULT_ROTATION,
               ENTITY_ITEM_DEFAULT_DIMENSIONS,
               ENTITY_ITEM_DEFAULT_POSITION),
    _glowLevel(ENTITY_ITEM_DEFAULT_GLOW_LEVEL),
    _localRenderAlpha(ENTITY_ITEM_DEFAULT_LOCAL_RENDER_ALPHA),
    _density(ENTITY_ITEM_DEFAULT_DENSITY),
    _volumeMultiplier(1.0f),
    _velocity(ENTITY_ITEM_DEFAULT_VELOCITY),
    _gravity(ENTITY_ITEM_DEFAULT_GRAVITY),
    _acceleration(ENTITY_ITEM_DEFAULT_ACCELERATION),
    _damping(ENTITY_ITEM_DEFAULT_DAMPING),
    _restitution(ENTITY_ITEM_DEFAULT_RESTITUTION),
    _friction(ENTITY_ITEM_DEFAULT_FRICTION),
    _lifetime(ENTITY_ITEM_DEFAULT_LIFETIME),
    _script(ENTITY_ITEM_DEFAULT_SCRIPT),
    _scriptTimestamp(ENTITY_ITEM_DEFAULT_SCRIPT_TIMESTAMP),
    _collisionSoundURL(ENTITY_ITEM_DEFAULT_COLLISION_SOUND_URL),
    _registrationPoint(ENTITY_ITEM_DEFAULT_REGISTRATION_POINT),
    _angularVelocity(ENTITY_ITEM_DEFAULT_ANGULAR_VELOCITY),
    _angularDamping(ENTITY_ITEM_DEFAULT_ANGULAR_DAMPING),
    _visible(ENTITY_ITEM_DEFAULT_VISIBLE),
    _ignoreForCollisions(ENTITY_ITEM_DEFAULT_IGNORE_FOR_COLLISIONS),
    _collisionsWillMove(ENTITY_ITEM_DEFAULT_COLLISIONS_WILL_MOVE),
    _locked(ENTITY_ITEM_DEFAULT_LOCKED),
    _userData(ENTITY_ITEM_DEFAULT_USER_DATA),
    _simulationOwner(),
    _marketplaceID(ENTITY_ITEM_DEFAULT_MARKETPLACE_ID),
    _name(ENTITY_ITEM_DEFAULT_NAME),
    _href(""),
    _description(""),
    _dirtyFlags(0),
    _element(nullptr),
    _physicsInfo(nullptr),
    _simulated(false)
{
    quint64 now = usecTimestampNow();
    _lastSimulated = now;
    _lastUpdated = now;
}

EntityItem::EntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) : EntityItem(entityItemID)
{
    setProperties(properties);
}

EntityItem::~EntityItem() {
    // clear out any left-over actions
    EntityTree* entityTree = _element ? _element->getTree() : nullptr;
    EntitySimulation* simulation = entityTree ? entityTree->getSimulation() : nullptr;
    if (simulation) {
        clearActions(simulation);
    }

    // these pointers MUST be correct at delete, else we probably have a dangling backpointer
    // to this EntityItem in the corresponding data structure.
    assert(!_simulated);
    assert(!_element);
    assert(!_physicsInfo);
}

EntityItemID EntityItem::getEntityItemID() const {
    assertUnlocked();
    lockForRead();
    auto result = getEntityItemIDInternal();
    unlock();
    return result;
}

EntityItemID EntityItem::getEntityItemIDInternal() const {
    return EntityItemID(_id);
}

EntityPropertyFlags EntityItem::getEntityProperties(EncodeBitstreamParams& params, bool doLocking) const {
    if (doLocking) {
        assertUnlocked();
        lockForRead();
    } else {
        assertLocked();
    }

    EntityPropertyFlags requestedProperties;

    requestedProperties += PROP_SIMULATION_OWNER;
    requestedProperties += PROP_POSITION;
    requestedProperties += PROP_ROTATION;
    requestedProperties += PROP_VELOCITY;
    requestedProperties += PROP_ANGULAR_VELOCITY;
    requestedProperties += PROP_ACCELERATION;

    requestedProperties += PROP_DIMENSIONS; // NOTE: PROP_RADIUS obsolete
    requestedProperties += PROP_DENSITY;
    requestedProperties += PROP_GRAVITY;
    requestedProperties += PROP_DAMPING;
    requestedProperties += PROP_RESTITUTION;
    requestedProperties += PROP_FRICTION;
    requestedProperties += PROP_LIFETIME;
    requestedProperties += PROP_SCRIPT;
    requestedProperties += PROP_SCRIPT_TIMESTAMP;
    requestedProperties += PROP_COLLISION_SOUND_URL;
    requestedProperties += PROP_REGISTRATION_POINT;
    requestedProperties += PROP_ANGULAR_DAMPING;
    requestedProperties += PROP_VISIBLE;
    requestedProperties += PROP_IGNORE_FOR_COLLISIONS;
    requestedProperties += PROP_COLLISIONS_WILL_MOVE;
    requestedProperties += PROP_LOCKED;
    requestedProperties += PROP_USER_DATA;
    requestedProperties += PROP_MARKETPLACE_ID;
    requestedProperties += PROP_NAME;
    requestedProperties += PROP_HREF;
    requestedProperties += PROP_DESCRIPTION;
    requestedProperties += PROP_ACTION_DATA;

    if (doLocking) {
        unlock();
    }

    return requestedProperties;
}

OctreeElement::AppendState EntityItem::appendEntityData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                            EntityTreeElementExtraEncodeData* entityTreeElementExtraEncodeData) const {
    assertUnlocked();
    lockForRead();
    // ALL this fits...
    //    object ID [16 bytes]
    //    ByteCountCoded(type code) [~1 byte]
    //    last edited [8 bytes]
    //    ByteCountCoded(last_edited to last_updated delta) [~1-8 bytes]
    //    PropertyFlags<>( everything ) [1-2 bytes]
    // ~27-35 bytes...

    OctreeElement::AppendState appendState = OctreeElement::COMPLETED; // assume the best

    // encode our ID as a byte count coded byte stream
    QByteArray encodedID = getIDInternal().toRfc4122();

    // encode our type as a byte count coded byte stream
    ByteCountCoded<quint32> typeCoder = getTypeInternal();
    QByteArray encodedType = typeCoder;

    // last updated (animations, non-physics changes)
    quint64 updateDelta =
        getLastUpdatedInternal() <= getLastEditedInternal() ? 0 : getLastUpdatedInternal() - getLastEditedInternal();
    ByteCountCoded<quint64> updateDeltaCoder = updateDelta;
    QByteArray encodedUpdateDelta = updateDeltaCoder;

    // last simulated (velocity, angular velocity, physics changes)
    quint64 simulatedDelta = getLastSimulatedInternal() <= getLastEditedInternal() ? 0 : getLastSimulatedInternal() - getLastEditedInternal();
    ByteCountCoded<quint64> simulatedDeltaCoder = simulatedDelta;
    QByteArray encodedSimulatedDelta = simulatedDeltaCoder;


    EntityPropertyFlags propertyFlags(PROP_LAST_ITEM);
    EntityPropertyFlags requestedProperties = getEntityProperties(params, false);
    EntityPropertyFlags propertiesDidntFit = requestedProperties;

    // If we are being called for a subsequent pass at appendEntityData() that failed to completely encode this item,
    // then our entityTreeElementExtraEncodeData should include data about which properties we need to append.
    if (entityTreeElementExtraEncodeData && entityTreeElementExtraEncodeData->entities.contains(getEntityItemIDInternal())) {
        requestedProperties = entityTreeElementExtraEncodeData->entities.value(getEntityItemIDInternal());
    }

    LevelDetails entityLevel = packetData->startLevel();

    quint64 lastEdited = getLastEditedInternal();

    #ifdef WANT_DEBUG
        float editedAgo = getEditedAgo();
        QString agoAsString = formatSecondsElapsed(editedAgo);
        qCDebug(entities) << "Writing entity " << getEntityItemID() << " to buffer, lastEdited =" << lastEdited
                        << " ago=" << editedAgo << "seconds - " << agoAsString;
    #endif

    bool successIDFits = false;
    bool successTypeFits = false;
    bool successCreatedFits = false;
    bool successLastEditedFits = false;
    bool successLastUpdatedFits = false;
    bool successLastSimulatedFits = false;
    bool successPropertyFlagsFits = false;
    int propertyFlagsOffset = 0;
    int oldPropertyFlagsLength = 0;
    QByteArray encodedPropertyFlags;
    int propertyCount = 0;

    successIDFits = packetData->appendRawData(encodedID);
    if (successIDFits) {
        successTypeFits = packetData->appendRawData(encodedType);
    }
    if (successTypeFits) {
        successCreatedFits = packetData->appendValue(_created);
    }
    if (successCreatedFits) {
        successLastEditedFits = packetData->appendValue(lastEdited);
    }
    if (successLastEditedFits) {
        successLastUpdatedFits = packetData->appendRawData(encodedUpdateDelta);
    }
    if (successLastUpdatedFits) {
        successLastSimulatedFits = packetData->appendRawData(encodedSimulatedDelta);
    }

    if (successLastSimulatedFits) {
        propertyFlagsOffset = packetData->getUncompressedByteOffset();
        encodedPropertyFlags = propertyFlags;
        oldPropertyFlagsLength = encodedPropertyFlags.length();
        successPropertyFlagsFits = packetData->appendRawData(encodedPropertyFlags);
    }

    bool headerFits = successIDFits && successTypeFits && successCreatedFits && successLastEditedFits
                              && successLastUpdatedFits && successPropertyFlagsFits;

    int startOfEntityItemData = packetData->getUncompressedByteOffset();

    if (headerFits) {
        bool successPropertyFits;

        propertyFlags -= PROP_LAST_ITEM; // clear the last item for now, we may or may not set it as the actual item

        // These items would go here once supported....
        //      PROP_PAGED_PROPERTY,
        //      PROP_CUSTOM_PROPERTIES_INCLUDED,

        APPEND_ENTITY_PROPERTY(PROP_SIMULATION_OWNER, _simulationOwner.toByteArray());
        APPEND_ENTITY_PROPERTY(PROP_POSITION, getPositionInternal());
        APPEND_ENTITY_PROPERTY(PROP_ROTATION, getRotationInternal());
        APPEND_ENTITY_PROPERTY(PROP_VELOCITY, getVelocity());
        APPEND_ENTITY_PROPERTY(PROP_ANGULAR_VELOCITY, getAngularVelocity());
        APPEND_ENTITY_PROPERTY(PROP_ACCELERATION, getAcceleration());

        APPEND_ENTITY_PROPERTY(PROP_DIMENSIONS, getDimensions()); // NOTE: PROP_RADIUS obsolete
        APPEND_ENTITY_PROPERTY(PROP_DENSITY, getDensity());
        APPEND_ENTITY_PROPERTY(PROP_GRAVITY, getGravity());
        APPEND_ENTITY_PROPERTY(PROP_DAMPING, getDamping());
        APPEND_ENTITY_PROPERTY(PROP_RESTITUTION, getRestitution());
        APPEND_ENTITY_PROPERTY(PROP_FRICTION, getFriction());
        APPEND_ENTITY_PROPERTY(PROP_LIFETIME, getLifetime());
        APPEND_ENTITY_PROPERTY(PROP_SCRIPT, getScript());
        APPEND_ENTITY_PROPERTY(PROP_SCRIPT_TIMESTAMP, getScriptTimestamp());
        APPEND_ENTITY_PROPERTY(PROP_REGISTRATION_POINT, getRegistrationPoint());
        APPEND_ENTITY_PROPERTY(PROP_ANGULAR_DAMPING, getAngularDamping());
        APPEND_ENTITY_PROPERTY(PROP_VISIBLE, getVisible());
        APPEND_ENTITY_PROPERTY(PROP_IGNORE_FOR_COLLISIONS, getIgnoreForCollisions());
        APPEND_ENTITY_PROPERTY(PROP_COLLISIONS_WILL_MOVE, getCollisionsWillMove());
        APPEND_ENTITY_PROPERTY(PROP_LOCKED, getLocked());
        APPEND_ENTITY_PROPERTY(PROP_USER_DATA, getUserData());
        APPEND_ENTITY_PROPERTY(PROP_MARKETPLACE_ID, getMarketplaceID());
        APPEND_ENTITY_PROPERTY(PROP_NAME, getName());
        APPEND_ENTITY_PROPERTY(PROP_COLLISION_SOUND_URL, getCollisionSoundURL());
        APPEND_ENTITY_PROPERTY(PROP_HREF, getHrefInternal());
        APPEND_ENTITY_PROPERTY(PROP_DESCRIPTION, getDescription());
        APPEND_ENTITY_PROPERTY(PROP_ACTION_DATA, getActionDataInternal());


        appendSubclassData(packetData, params, entityTreeElementExtraEncodeData,
                                requestedProperties,
                                propertyFlags,
                                propertiesDidntFit,
                                propertyCount,
                                appendState);
    }

    if (propertyCount > 0) {
        int endOfEntityItemData = packetData->getUncompressedByteOffset();
        encodedPropertyFlags = propertyFlags;
        int newPropertyFlagsLength = encodedPropertyFlags.length();
        packetData->updatePriorBytes(propertyFlagsOffset,
                (const unsigned char*)encodedPropertyFlags.constData(), encodedPropertyFlags.length());

        // if the size of the PropertyFlags shrunk, we need to shift everything down to front of packet.
        if (newPropertyFlagsLength < oldPropertyFlagsLength) {
            int oldSize = packetData->getUncompressedSize();
            const unsigned char* modelItemData = packetData->getUncompressedData(propertyFlagsOffset + oldPropertyFlagsLength);
            int modelItemDataLength = endOfEntityItemData - startOfEntityItemData;
            int newEntityItemDataStart = propertyFlagsOffset + newPropertyFlagsLength;
            packetData->updatePriorBytes(newEntityItemDataStart, modelItemData, modelItemDataLength);
            int newSize = oldSize - (oldPropertyFlagsLength - newPropertyFlagsLength);
            packetData->setUncompressedSize(newSize);

        } else {
            assert(newPropertyFlagsLength == oldPropertyFlagsLength); // should not have grown
        }

        packetData->endLevel(entityLevel);
    } else {
        packetData->discardLevel(entityLevel);
        appendState = OctreeElement::NONE; // if we got here, then we didn't include the item
    }

    // If any part of the model items didn't fit, then the element is considered partial
    if (appendState != OctreeElement::COMPLETED) {
        // add this item into our list for the next appendElementData() pass
        entityTreeElementExtraEncodeData->entities.insert(getEntityItemIDInternal(), propertiesDidntFit);
    }

    unlock();
    return appendState;
}

// TODO: My goal is to get rid of this concept completely. The old code (and some of the current code) used this
// result to calculate if a packet being sent to it was potentially bad or corrupt. I've adjusted this to now
// only consider the minimum header bytes as being required. But it would be preferable to completely eliminate
// this logic from the callers.
int EntityItem::expectedBytes() {
    // Header bytes
    //    object ID [16 bytes]
    //    ByteCountCoded(type code) [~1 byte]
    //    last edited [8 bytes]
    //    ByteCountCoded(last_edited to last_updated delta) [~1-8 bytes]
    //    PropertyFlags<>( everything ) [1-2 bytes]
    // ~27-35 bytes...
    const int MINIMUM_HEADER_BYTES = 27;
    return MINIMUM_HEADER_BYTES;
}


// clients use this method to unpack FULL updates from entity-server
int EntityItem::readEntityDataFromBuffer(const unsigned char* data, int bytesLeftToRead, ReadBitstreamToTreeParams& args) {
    assertUnlocked();
    lockForWrite();
    if (args.bitstreamVersion < VERSION_ENTITIES_SUPPORT_SPLIT_MTU) {

        // NOTE: This shouldn't happen. The only versions of the bit stream that didn't support split mtu buffers should
        // be handled by the model subclass and shouldn't call this routine.
        qCDebug(entities) << "EntityItem::readEntityDataFromBuffer()... "
                        "ERROR CASE...args.bitstreamVersion < VERSION_ENTITIES_SUPPORT_SPLIT_MTU";
        unlock();
        return 0;
    }

    args.entitiesPerPacket++;

    // Header bytes
    //    object ID [16 bytes]
    //    ByteCountCoded(type code) [~1 byte]
    //    last edited [8 bytes]
    //    ByteCountCoded(last_edited to last_updated delta) [~1-8 bytes]
    //    PropertyFlags<>( everything ) [1-2 bytes]
    // ~27-35 bytes...
    const int MINIMUM_HEADER_BYTES = 27;

    int bytesRead = 0;
    if (bytesLeftToRead < MINIMUM_HEADER_BYTES) {
        unlock();
        return 0;
    }

    int originalLength = bytesLeftToRead;
    // TODO: figure out a way to avoid the big deep copy below.
    QByteArray originalDataBuffer((const char*)data, originalLength); // big deep copy!

    int clockSkew = args.sourceNode ? args.sourceNode->getClockSkewUsec() : 0;

    const unsigned char* dataAt = data;

    // id
    QByteArray encodedID = originalDataBuffer.mid(bytesRead, NUM_BYTES_RFC4122_UUID); // maximum possible size
    _id = QUuid::fromRfc4122(encodedID);
    dataAt += encodedID.size();
    bytesRead += encodedID.size();

    // type
    QByteArray encodedType = originalDataBuffer.mid(bytesRead); // maximum possible size
    ByteCountCoded<quint32> typeCoder = encodedType;
    encodedType = typeCoder; // determine true length
    dataAt += encodedType.size();
    bytesRead += encodedType.size();
    quint32 type = typeCoder;
    _type = (EntityTypes::EntityType)type;

    bool overwriteLocalData = true; // assume the new content overwrites our local data

    // _created
    quint64 createdFromBuffer = 0;
    memcpy(&createdFromBuffer, dataAt, sizeof(createdFromBuffer));
    dataAt += sizeof(createdFromBuffer);
    bytesRead += sizeof(createdFromBuffer);

    quint64 now = usecTimestampNow();
    if (_created == UNKNOWN_CREATED_TIME) {
        // we don't yet have a _created timestamp, so we accept this one
        createdFromBuffer -= clockSkew;
        if (createdFromBuffer > now || createdFromBuffer == UNKNOWN_CREATED_TIME) {
            createdFromBuffer = now;
        }
        _created = createdFromBuffer;
    }

    #ifdef WANT_DEBUG
        quint64 lastEdited = getLastEditedInternal();
        float editedAgo = getEditedAgo();
        QString agoAsString = formatSecondsElapsed(editedAgo);
        QString ageAsString = formatSecondsElapsed(getAge());
        qCDebug(entities) << "------------------------------------------";
        qCDebug(entities) << "Loading entity " << getEntityItemID() << " from buffer...";
        qCDebug(entities) << "------------------------------------------";
        debugDump();
        qCDebug(entities) << "------------------------------------------";
        qCDebug(entities) << "    _created =" << _created;
        qCDebug(entities) << "    age=" << getAge() << "seconds - " << ageAsString;
        qCDebug(entities) << "    lastEdited =" << lastEdited;
        qCDebug(entities) << "    ago=" << editedAgo << "seconds - " << agoAsString;
    #endif

    quint64 lastEditedFromBuffer = 0;
    quint64 lastEditedFromBufferAdjusted = 0;

    // TODO: we could make this encoded as a delta from _created
    // _lastEdited
    memcpy(&lastEditedFromBuffer, dataAt, sizeof(lastEditedFromBuffer));
    dataAt += sizeof(lastEditedFromBuffer);
    bytesRead += sizeof(lastEditedFromBuffer);
    lastEditedFromBufferAdjusted = lastEditedFromBuffer - clockSkew;

    if (lastEditedFromBufferAdjusted > now) {
        lastEditedFromBufferAdjusted = now;
    }

    bool fromSameServerEdit = (lastEditedFromBuffer == _lastEditedFromRemoteInRemoteTime);

    #ifdef WANT_DEBUG
        qCDebug(entities) << "data from server **************** ";
        qCDebug(entities) << "                           entityItemID:" << getEntityItemID();
        qCDebug(entities) << "                                    now:" << now;
        qCDebug(entities) << "                          getLastEdited:" << debugTime(getLastEditedInternal(), now);
        qCDebug(entities) << "                   lastEditedFromBuffer:" << debugTime(lastEditedFromBuffer, now);
        qCDebug(entities) << "                              clockSkew:" << debugTimeOnly(clockSkew);
        qCDebug(entities) << "           lastEditedFromBufferAdjusted:" << debugTime(lastEditedFromBufferAdjusted, now);
        qCDebug(entities) << "                  _lastEditedFromRemote:" << debugTime(_lastEditedFromRemote, now);
        qCDebug(entities) << "      _lastEditedFromRemoteInRemoteTime:" << debugTime(_lastEditedFromRemoteInRemoteTime, now);
        qCDebug(entities) << "                     fromSameServerEdit:" << fromSameServerEdit;
    #endif

    bool ignoreServerPacket = false; // assume we'll use this server packet

    // If this packet is from the same server edit as the last packet we accepted from the server
    // we probably want to use it.
    if (fromSameServerEdit) {
        // If this is from the same sever packet, then check against any local changes since we got
        // the most recent packet from this server time
        if (_lastEdited > _lastEditedFromRemote) {
            ignoreServerPacket = true;
        }
    } else {
        // If this isn't from the same sever packet, then honor our skew adjusted times...
        // If we've changed our local tree more recently than the new data from this packet
        // then we will not be changing our values, instead we just read and skip the data
        if (_lastEdited > lastEditedFromBufferAdjusted) {
            ignoreServerPacket = true;
        }
    }

    if (ignoreServerPacket) {
        overwriteLocalData = false;
        #ifdef WANT_DEBUG
            qCDebug(entities) << "IGNORING old data from server!!! ****************";
            debugDump();
        #endif
    } else {
        #ifdef WANT_DEBUG
            qCDebug(entities) << "USING NEW data from server!!! ****************";
            debugDump();
        #endif

        // don't allow _lastEdited to be in the future
        _lastEdited = lastEditedFromBufferAdjusted;
        _lastEditedFromRemote = now;
        _lastEditedFromRemoteInRemoteTime = lastEditedFromBuffer;

        // TODO: only send this notification if something ACTUALLY changed (hint, we haven't yet parsed
        // the properties out of the bitstream (see below))
        somethingChangedNotification(); // notify derived classes that something has changed
    }

    // last updated is stored as ByteCountCoded delta from lastEdited
    QByteArray encodedUpdateDelta = originalDataBuffer.mid(bytesRead); // maximum possible size
    ByteCountCoded<quint64> updateDeltaCoder = encodedUpdateDelta;
    quint64 updateDelta = updateDeltaCoder;
    if (overwriteLocalData) {
        _lastUpdated = lastEditedFromBufferAdjusted + updateDelta; // don't adjust for clock skew since we already did that
        #ifdef WANT_DEBUG
            qCDebug(entities) << "                           _lastUpdated:" << debugTime(_lastUpdated, now);
            qCDebug(entities) << "                            _lastEdited:" << debugTime(_lastEdited, now);
            qCDebug(entities) << "           lastEditedFromBufferAdjusted:" << debugTime(lastEditedFromBufferAdjusted, now);
        #endif
    }

    encodedUpdateDelta = updateDeltaCoder; // determine true length
    dataAt += encodedUpdateDelta.size();
    bytesRead += encodedUpdateDelta.size();

    // Newer bitstreams will have a last simulated and a last updated value
    quint64 lastSimulatedFromBufferAdjusted = now;
    if (args.bitstreamVersion >= VERSION_ENTITIES_HAS_LAST_SIMULATED_TIME) {
        // last simulated is stored as ByteCountCoded delta from lastEdited
        QByteArray encodedSimulatedDelta = originalDataBuffer.mid(bytesRead); // maximum possible size
        ByteCountCoded<quint64> simulatedDeltaCoder = encodedSimulatedDelta;
        quint64 simulatedDelta = simulatedDeltaCoder;
        if (overwriteLocalData) {
            lastSimulatedFromBufferAdjusted = lastEditedFromBufferAdjusted + simulatedDelta; // don't adjust for clock skew since we already did that
            if (lastSimulatedFromBufferAdjusted > now) {
                lastSimulatedFromBufferAdjusted = now;
            }
            #ifdef WANT_DEBUG
                qCDebug(entities) << "                            _lastEdited:" << debugTime(_lastEdited, now);
                qCDebug(entities) << "           lastEditedFromBufferAdjusted:" << debugTime(lastEditedFromBufferAdjusted, now);
                qCDebug(entities) << "        lastSimulatedFromBufferAdjusted:" << debugTime(lastSimulatedFromBufferAdjusted, now);
            #endif
        }
        encodedSimulatedDelta = simulatedDeltaCoder; // determine true length
        dataAt += encodedSimulatedDelta.size();
        bytesRead += encodedSimulatedDelta.size();
    }

    #ifdef WANT_DEBUG
        if (overwriteLocalData) {
            qCDebug(entities) << "EntityItem::readEntityDataFromBuffer()... changed entity:" << getEntityItemID();
            qCDebug(entities) << "                          getLastEdited:" << debugTime(getLastEditedInternal(), now);
            qCDebug(entities) << "                       getLastSimulated:" << debugTime(getLastSimulated(), now);
            qCDebug(entities) << "                         getLastUpdated:" << debugTime(getLastUpdatedInternal(), now);
        }
    #endif


    // Property Flags
    QByteArray encodedPropertyFlags = originalDataBuffer.mid(bytesRead); // maximum possible size
    EntityPropertyFlags propertyFlags = encodedPropertyFlags;
    dataAt += propertyFlags.getEncodedLength();
    bytesRead += propertyFlags.getEncodedLength();

    if (args.bitstreamVersion >= VERSION_ENTITIES_HAVE_SIMULATION_OWNER_AND_ACTIONS_OVER_WIRE) {
        // pack SimulationOwner and terse update properties near each other

        // NOTE: the server is authoritative for changes to simOwnerID so we always unpack ownership data
        // even when we would otherwise ignore the rest of the packet.

        if (propertyFlags.getHasProperty(PROP_SIMULATION_OWNER)) {
            QByteArray simOwnerData;
            int bytes = OctreePacketData::unpackDataFromBytes(dataAt, simOwnerData);
            SimulationOwner newSimOwner;
            newSimOwner.fromByteArray(simOwnerData);
            dataAt += bytes;
            bytesRead += bytes;

            if (_simulationOwner.set(newSimOwner)) {
                _dirtyFlags |= EntityItem::DIRTY_SIMULATOR_ID;
            }
        }
        {   // When we own the simulation we don't accept updates to the entity's transform/velocities
            // but since we're using macros below we have to temporarily modify overwriteLocalData.
            auto nodeList = DependencyManager::get<NodeList>();
            bool weOwnIt = _simulationOwner.matchesValidID(nodeList->getSessionUUID());
            bool oldOverwrite = overwriteLocalData;
            overwriteLocalData = overwriteLocalData && !weOwnIt;
            READ_ENTITY_PROPERTY(PROP_POSITION, glm::vec3, updatePosition);
            READ_ENTITY_PROPERTY(PROP_ROTATION, glm::quat, updateRotation);
            READ_ENTITY_PROPERTY(PROP_VELOCITY, glm::vec3, updateVelocity);
            READ_ENTITY_PROPERTY(PROP_ANGULAR_VELOCITY, glm::vec3, updateAngularVelocity);
            READ_ENTITY_PROPERTY(PROP_ACCELERATION, glm::vec3, setAcceleration);
            overwriteLocalData = oldOverwrite;
        }

        READ_ENTITY_PROPERTY(PROP_DIMENSIONS, glm::vec3, updateDimensions);
        READ_ENTITY_PROPERTY(PROP_DENSITY, float, updateDensity);
        READ_ENTITY_PROPERTY(PROP_GRAVITY, glm::vec3, updateGravity);

        READ_ENTITY_PROPERTY(PROP_DAMPING, float, updateDamping);
        READ_ENTITY_PROPERTY(PROP_RESTITUTION, float, updateRestitution);
        READ_ENTITY_PROPERTY(PROP_FRICTION, float, updateFriction);
        READ_ENTITY_PROPERTY(PROP_LIFETIME, float, updateLifetime);
        READ_ENTITY_PROPERTY(PROP_SCRIPT, QString, setScript);
        READ_ENTITY_PROPERTY(PROP_SCRIPT_TIMESTAMP, quint64, setScriptTimestamp);
        READ_ENTITY_PROPERTY(PROP_REGISTRATION_POINT, glm::vec3, setRegistrationPoint);
    } else {
        // legacy order of packing here
        READ_ENTITY_PROPERTY(PROP_POSITION, glm::vec3, updatePosition);
        READ_ENTITY_PROPERTY(PROP_DIMENSIONS, glm::vec3, updateDimensions);
        READ_ENTITY_PROPERTY(PROP_ROTATION, glm::quat, updateRotation);
        READ_ENTITY_PROPERTY(PROP_DENSITY, float, updateDensity);
        READ_ENTITY_PROPERTY(PROP_VELOCITY, glm::vec3, updateVelocity);
        READ_ENTITY_PROPERTY(PROP_GRAVITY, glm::vec3, updateGravity);
        READ_ENTITY_PROPERTY(PROP_ACCELERATION, glm::vec3, setAcceleration);

        READ_ENTITY_PROPERTY(PROP_DAMPING, float, updateDamping);
        READ_ENTITY_PROPERTY(PROP_RESTITUTION, float, updateRestitution);
        READ_ENTITY_PROPERTY(PROP_FRICTION, float, updateFriction);
        READ_ENTITY_PROPERTY(PROP_LIFETIME, float, updateLifetime);
        READ_ENTITY_PROPERTY(PROP_SCRIPT, QString, setScript);
        READ_ENTITY_PROPERTY(PROP_SCRIPT_TIMESTAMP, quint64, setScriptTimestamp);
        READ_ENTITY_PROPERTY(PROP_REGISTRATION_POINT, glm::vec3, setRegistrationPoint);
        READ_ENTITY_PROPERTY(PROP_ANGULAR_VELOCITY, glm::vec3, updateAngularVelocity);
    }

    READ_ENTITY_PROPERTY(PROP_ANGULAR_DAMPING, float, updateAngularDamping);
    READ_ENTITY_PROPERTY(PROP_VISIBLE, bool, setVisible);
    READ_ENTITY_PROPERTY(PROP_IGNORE_FOR_COLLISIONS, bool, updateIgnoreForCollisions);
    READ_ENTITY_PROPERTY(PROP_COLLISIONS_WILL_MOVE, bool, updateCollisionsWillMove);
    READ_ENTITY_PROPERTY(PROP_LOCKED, bool, setLocked);
    READ_ENTITY_PROPERTY(PROP_USER_DATA, QString, setUserData);

    if (args.bitstreamVersion < VERSION_ENTITIES_HAVE_SIMULATION_OWNER_AND_ACTIONS_OVER_WIRE) {
        // this code for when there is only simulatorID and no simulation priority

        // we always accept the server's notion of simulatorID, so we fake overwriteLocalData as true
        // before we try to READ_ENTITY_PROPERTY it
        bool temp = overwriteLocalData;
        overwriteLocalData = true;
        READ_ENTITY_PROPERTY(PROP_SIMULATION_OWNER, QUuid, updateSimulatorID);
        overwriteLocalData = temp;
    }

    if (args.bitstreamVersion >= VERSION_ENTITIES_HAS_MARKETPLACE_ID) {
        READ_ENTITY_PROPERTY(PROP_MARKETPLACE_ID, QString, setMarketplaceID);
    }

    READ_ENTITY_PROPERTY(PROP_NAME, QString, setName);
    READ_ENTITY_PROPERTY(PROP_COLLISION_SOUND_URL, QString, setCollisionSoundURL);
    READ_ENTITY_PROPERTY(PROP_HREF, QString, setHrefInternal);
    READ_ENTITY_PROPERTY(PROP_DESCRIPTION, QString, setDescription);

    READ_ENTITY_PROPERTY(PROP_ACTION_DATA, QByteArray, setActionDataInternal);

    bytesRead += readEntitySubclassDataFromBuffer(dataAt, (bytesLeftToRead - bytesRead), args,
                                                  propertyFlags, overwriteLocalData);

    ////////////////////////////////////
    // WARNING: Do not add stream content here after the subclass. Always add it before the subclass
    //
    // NOTE: we had a bad version of the stream that we added stream data after the subclass. We can attempt to recover
    // by doing this parsing here... but it's not likely going to fully recover the content.
    //
    // TODO: Remove this conde once we've sufficiently migrated content past this damaged version
    if (args.bitstreamVersion == VERSION_ENTITIES_HAS_MARKETPLACE_ID_DAMAGED) {
        READ_ENTITY_PROPERTY(PROP_MARKETPLACE_ID, QString, setMarketplaceID);
    }

    if (overwriteLocalData && (getDirtyFlags() & (EntityItem::DIRTY_TRANSFORM | EntityItem::DIRTY_VELOCITIES))) {
        // NOTE: This code is attempting to "repair" the old data we just got from the server to make it more
        // closely match where the entities should be if they'd stepped forward in time to "now". The server
        // is sending us data with a known "last simulated" time. That time is likely in the past, and therefore
        // this "new" data is actually slightly out of date. We calculate the time we need to skip forward and
        // use our simulation helper routine to get a best estimate of where the entity should be.
        const float MIN_TIME_SKIP = 0.0f;
        const float MAX_TIME_SKIP = 1.0f; // in seconds
        float skipTimeForward = glm::clamp((float)(now - lastSimulatedFromBufferAdjusted) / (float)(USECS_PER_SECOND),
                MIN_TIME_SKIP, MAX_TIME_SKIP);
        if (skipTimeForward > 0.0f) {
            #ifdef WANT_DEBUG
                qCDebug(entities) << "skipTimeForward:" << skipTimeForward;
            #endif
            // we want to extrapolate the motion forward to compensate for packet travel time, but
            // we don't want the side effect of flag setting.
            simulateKinematicMotionInternal(skipTimeForward, false);
        }
    }

    auto nodeList = DependencyManager::get<NodeList>();
    const QUuid& myNodeID = nodeList->getSessionUUID();
    if (overwriteLocalData) {
        if (!_simulationOwner.matchesValidID(myNodeID)) {
            _lastSimulated = now;
        }
    }

    // Tracking for editing roundtrips here. We will tell our EntityTree that we just got incoming data about
    // and entity that was edited at some time in the past. The tree will determine how it wants to track this
    // information.
    if (_element && _element->getTree()) {
        _element->getTree()->trackIncomingEntityLastEdited(lastEditedFromBufferAdjusted, bytesRead);
    }

    unlock();
    return bytesRead;
}

void EntityItem::debugDump() const {
    assertLocked();
    auto position = getPositionInternal();
    qCDebug(entities) << "EntityItem id:" << getEntityItemID();
    qCDebug(entities, " edited ago:%f", (double)getEditedAgo());
    qCDebug(entities, " position:%f,%f,%f", (double)position.x, (double)position.y, (double)position.z);
    qCDebug(entities) << " dimensions:" << getDimensions();
}

// adjust any internal timestamps to fix clock skew for this server
void EntityItem::adjustEditPacketForClockSkew(unsigned char* editPacketBuffer, size_t length, int clockSkew) {
    unsigned char* dataAt = editPacketBuffer;
    int octets = numberOfThreeBitSectionsInCode(dataAt);
    int lengthOfOctcode = bytesRequiredForCodeLength(octets);
    dataAt += lengthOfOctcode;

    // lastEdited
    quint64 lastEditedInLocalTime;
    memcpy(&lastEditedInLocalTime, dataAt, sizeof(lastEditedInLocalTime));
    quint64 lastEditedInServerTime = lastEditedInLocalTime + clockSkew;
    memcpy(dataAt, &lastEditedInServerTime, sizeof(lastEditedInServerTime));
    #ifdef WANT_DEBUG
        qCDebug(entities, "EntityItem::adjustEditPacketForClockSkew()...");
        qCDebug(entities) << "     lastEditedInLocalTime: " << lastEditedInLocalTime;
        qCDebug(entities) << "                 clockSkew: " << clockSkew;
        qCDebug(entities) << "    lastEditedInServerTime: " << lastEditedInServerTime;
    #endif
    //assert(lastEditedInLocalTime > (quint64)0);
}

void EntityItem::update(const quint64& now) {
    assertUnlocked();
    lockForWrite();
    _lastUpdated = now;
    unlock();
}

quint64 EntityItem::getLastUpdated() const {
    assertUnlocked();
    lockForRead();
    auto result = getLastUpdatedInternal();
    unlock();
    return result;
}

quint64 EntityItem::getLastUpdatedInternal() const {
    return _lastUpdated;
}

float EntityItem::computeMass() const {
    return _density * _volumeMultiplier * getDimensions().x * getDimensions().y * getDimensions().z;
}

void EntityItem::setDensity(float density) {
    _density = glm::max(glm::min(density, ENTITY_ITEM_MAX_DENSITY), ENTITY_ITEM_MIN_DENSITY);
}

const float ACTIVATION_RELATIVE_DENSITY_DELTA = 0.01f; // 1 percent

void EntityItem::updateDensity(float density) {
    assertWriteLocked();
    float clampedDensity = glm::max(glm::min(density, ENTITY_ITEM_MAX_DENSITY), ENTITY_ITEM_MIN_DENSITY);
    if (_density != clampedDensity) {
        _density = clampedDensity;

        if (fabsf(_density - clampedDensity) / _density > ACTIVATION_RELATIVE_DENSITY_DELTA) {
            // the density has changed enough that we should update the physics simulation
            _dirtyFlags |= EntityItem::DIRTY_MASS;
        }
    }
}

void EntityItem::setMass(float mass) {
    // Setting the mass actually changes the _density (at fixed volume), however
    // we must protect the density range to help maintain stability of physics simulation
    // therefore this method might not accept the mass that is supplied.

    float volume = _volumeMultiplier * getDimensions().x * getDimensions().y * getDimensions().z;

    // compute new density
    const float MIN_VOLUME = 1.0e-6f; // 0.001mm^3
    if (volume < 1.0e-6f) {
        // avoid divide by zero
        _density = glm::min(mass / MIN_VOLUME, ENTITY_ITEM_MAX_DENSITY);
    } else {
        _density = glm::max(glm::min(mass / volume, ENTITY_ITEM_MAX_DENSITY), ENTITY_ITEM_MIN_DENSITY);
    }
}

void EntityItem::simulate(const quint64& now) {
    assertUnlocked();
    lockForWrite();
    if (_lastSimulated == 0) {
        _lastSimulated = now;
    }

    float timeElapsed = (float)(now - _lastSimulated) / (float)(USECS_PER_SECOND);

    #ifdef WANT_DEBUG
        qCDebug(entities) << "********** EntityItem::simulate()";
        qCDebug(entities) << "    entity ID=" << getEntityItemID();
        qCDebug(entities) << "    simulator ID=" << getSimulatorID();
        qCDebug(entities) << "    now=" << now;
        qCDebug(entities) << "    _lastSimulated=" << _lastSimulated;
        qCDebug(entities) << "    timeElapsed=" << timeElapsed;
        qCDebug(entities) << "    hasVelocity=" << hasVelocity();
        qCDebug(entities) << "    hasGravity=" << hasGravity();
        qCDebug(entities) << "    hasAcceleration=" << hasAcceleration();
        qCDebug(entities) << "    hasAngularVelocity=" << hasAngularVelocity();
        qCDebug(entities) << "    getAngularVelocity=" << getAngularVelocity();
        qCDebug(entities) << "    isMortal=" << isMortal();
        qCDebug(entities) << "    getAge()=" << getAge();
        qCDebug(entities) << "    getLifetime()=" << getLifetime();


        if (hasVelocity() || hasGravity()) {
            qCDebug(entities) << "    MOVING...=";
            qCDebug(entities) << "        hasVelocity=" << hasVelocity();
            qCDebug(entities) << "        hasGravity=" << hasGravity();
            qCDebug(entities) << "        hasAcceleration=" << hasAcceleration();
            qCDebug(entities) << "        hasAngularVelocity=" << hasAngularVelocity();
            qCDebug(entities) << "        getAngularVelocity=" << getAngularVelocity();
        }
        if (hasAngularVelocity()) {
            qCDebug(entities) << "    CHANGING...=";
            qCDebug(entities) << "        hasAngularVelocity=" << hasAngularVelocity();
            qCDebug(entities) << "        getAngularVelocity=" << getAngularVelocity();
        }
        if (isMortal()) {
            qCDebug(entities) << "    MORTAL...=";
            qCDebug(entities) << "        isMortal=" << isMortal();
            qCDebug(entities) << "        getAge()=" << getAge();
            qCDebug(entities) << "        getLifetime()=" << getLifetime();
        }
        qCDebug(entities) << "     ********** EntityItem::simulate() .... SETTING _lastSimulated=" << _lastSimulated;
    #endif

    simulateKinematicMotionInternal(timeElapsed);
    _lastSimulated = now;
    unlock();
}

void EntityItem::simulateKinematicMotion(float timeElapsed, bool setFlags) {
    assertUnlocked();
    lockForWrite();
    simulateKinematicMotionInternal(timeElapsed, setFlags);
    unlock();
}

void EntityItem::simulateKinematicMotionInternal(float timeElapsed, bool setFlags) {
    assertLocked();
    if (hasActions()) {
        return;
    }

    if (hasAngularVelocity()) {
        // angular damping
        if (_angularDamping > 0.0f) {
            _angularVelocity *= powf(1.0f - _angularDamping, timeElapsed);
            #ifdef WANT_DEBUG
                qCDebug(entities) << "    angularDamping :" << _angularDamping;
                qCDebug(entities) << "    newAngularVelocity:" << _angularVelocity;
            #endif
        }

        float angularSpeed = glm::length(_angularVelocity);

        const float EPSILON_ANGULAR_VELOCITY_LENGTH = 0.0017453f; // 0.0017453 rad/sec = 0.1f degrees/sec
        if (angularSpeed < EPSILON_ANGULAR_VELOCITY_LENGTH) {
            if (setFlags && angularSpeed > 0.0f) {
                _dirtyFlags |= EntityItem::DIRTY_MOTION_TYPE;
            }
            _angularVelocity = ENTITY_ITEM_ZERO_VEC3;
        } else {
            // for improved agreement with the way Bullet integrates rotations we use an approximation
            // and break the integration into bullet-sized substeps
            glm::quat rotation = getRotationInternal();
            float dt = timeElapsed;
            while (dt > PHYSICS_ENGINE_FIXED_SUBSTEP) {
                glm::quat  dQ = computeBulletRotationStep(_angularVelocity, PHYSICS_ENGINE_FIXED_SUBSTEP);
                rotation = glm::normalize(dQ * rotation);
                dt -= PHYSICS_ENGINE_FIXED_SUBSTEP;
            }
            // NOTE: this final partial substep can drift away from a real Bullet simulation however
            // it only becomes significant for rapidly rotating objects
            // (e.g. around PI/4 radians per substep, or 7.5 rotations/sec at 60 substeps/sec).
            glm::quat  dQ = computeBulletRotationStep(_angularVelocity, dt);
            rotation = glm::normalize(dQ * rotation);

            setRotationInternal(rotation);
        }
    }

    if (hasVelocity()) {
        // linear damping
        glm::vec3 velocity = getVelocity();
        if (_damping > 0.0f) {
            velocity *= powf(1.0f - _damping, timeElapsed);
            #ifdef WANT_DEBUG
                qCDebug(entities) << "    damping:" << _damping;
                qCDebug(entities) << "    velocity AFTER dampingResistance:" << velocity;
                qCDebug(entities) << "    glm::length(velocity):" << glm::length(velocity);
            #endif
        }

        // integrate position forward
        glm::vec3 position = getPositionInternal();
        glm::vec3 newPosition = position + (velocity * timeElapsed);

        #ifdef WANT_DEBUG
            qCDebug(entities) << "  EntityItem::simulate()....";
            qCDebug(entities) << "    timeElapsed:" << timeElapsed;
            qCDebug(entities) << "    old AACube:" << getMaximumAACube();
            qCDebug(entities) << "    old position:" << position;
            qCDebug(entities) << "    old velocity:" << velocity;
            qCDebug(entities) << "    old getAABox:" << getAABox();
            qCDebug(entities) << "    newPosition:" << newPosition;
            qCDebug(entities) << "    glm::distance(newPosition, position):" << glm::distance(newPosition, position);
        #endif

        position = newPosition;

        // apply effective acceleration, which will be the same as gravity if the Entity isn't at rest.
        if (hasAcceleration()) {
            velocity += getAcceleration() * timeElapsed;
        }

        float speed = glm::length(velocity);
        const float EPSILON_LINEAR_VELOCITY_LENGTH = 0.001f; // 1mm/sec
        if (speed < EPSILON_LINEAR_VELOCITY_LENGTH) {
            setVelocity(ENTITY_ITEM_ZERO_VEC3);
            if (setFlags && speed > 0.0f) {
                _dirtyFlags |= EntityItem::DIRTY_MOTION_TYPE;
            }
        } else {
            setPositionInternal(position);
            setVelocity(velocity);
        }

        #ifdef WANT_DEBUG
            qCDebug(entities) << "    new position:" << position;
            qCDebug(entities) << "    new velocity:" << velocity;
            qCDebug(entities) << "    new AACube:" << getMaximumAACube();
            qCDebug(entities) << "    old getAABox:" << getAABox();
        #endif
    }
}

bool EntityItem::isMoving() const {
    return hasVelocity() || hasAngularVelocity();
}

glm::mat4 EntityItem::getEntityToWorldMatrix() const {
    assertUnlocked();
    lockForRead();
    glm::mat4 translation = glm::translate(getPositionInternal());
    glm::mat4 rotation = glm::mat4_cast(getRotationInternal());
    glm::mat4 scale = glm::scale(getDimensions());
    glm::mat4 registration = glm::translate(ENTITY_ITEM_DEFAULT_REGISTRATION_POINT - getRegistrationPoint());
    unlock();
    return translation * rotation * scale * registration;
}

glm::mat4 EntityItem::getWorldToEntityMatrix() const {
    return glm::inverse(getEntityToWorldMatrix());
}

glm::vec3 EntityItem::entityToWorld(const glm::vec3& point) const {
    return glm::vec3(getEntityToWorldMatrix() * glm::vec4(point, 1.0f));
}

glm::vec3 EntityItem::worldToEntity(const glm::vec3& point) const {
    return glm::vec3(getWorldToEntityMatrix() * glm::vec4(point, 1.0f));
}

bool EntityItem::lifetimeHasExpired() const {
    return isMortal() && (getAge() > getLifetime());
}

quint64 EntityItem::getExpiry() const {
    return _created + (quint64)(_lifetime * (float)USECS_PER_SECOND);
}

EntityItemProperties EntityItem::getProperties() const {
    assertUnlocked();
    lockForRead();
    EntityItemProperties properties;
    properties._id = getIDInternal();
    properties._idSet = true;
    properties._created = _created;

    properties._type = getTypeInternal();
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(simulationOwner, getSimulationOwner);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(position, getPositionInternal);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(dimensions, getDimensions); // NOTE: radius is obsolete
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(rotation, getRotationInternal);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(density, getDensity);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(velocity, getVelocity);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(gravity, getGravity);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(acceleration, getAcceleration);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(damping, getDamping);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(restitution, getRestitution);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(friction, getFriction);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(created, getCreated);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(lifetime, getLifetime);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(script, getScript);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(scriptTimestamp, getScriptTimestamp);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(collisionSoundURL, getCollisionSoundURL);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(registrationPoint, getRegistrationPoint);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(angularVelocity, getAngularVelocity);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(angularDamping, getAngularDamping);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(glowLevel, getGlowLevel);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(localRenderAlpha, getLocalRenderAlpha);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(visible, getVisible);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(ignoreForCollisions, getIgnoreForCollisions);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(collisionsWillMove, getCollisionsWillMove);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(locked, getLocked);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(userData, getUserData);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(marketplaceID, getMarketplaceID);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(name, getName);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(href, getHrefInternal);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(description, getDescription);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(actionData, getActionDataInternal);

    properties._defaultSettings = false;

    unlock();
    return properties;
}

void EntityItem::getAllTerseUpdateProperties(EntityItemProperties& properties) const {
    assertUnlocked();
    lockForRead();
    // a TerseUpdate includes the transform and its derivatives
    properties._position = getPositionInternal();
    properties._velocity = _velocity;
    properties._rotation = getRotationInternal();
    properties._angularVelocity = _angularVelocity;
    properties._acceleration = _acceleration;

    properties._positionChanged = true;
    properties._velocityChanged = true;
    properties._rotationChanged = true;
    properties._angularVelocityChanged = true;
    properties._accelerationChanged = true;
    unlock();
}

bool EntityItem::setProperties(const EntityItemProperties& properties) {
    assertUnlocked();
    lockForWrite();
    bool somethingChanged = false;

    // these affect TerseUpdate properties
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(simulationOwner, setSimulationOwner);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(position, updatePosition);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(rotation, updateRotation);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(velocity, updateVelocity);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(angularVelocity, updateAngularVelocity);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(acceleration, setAcceleration);

    // these (along with "position" above) affect tree structure
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(dimensions, updateDimensions);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(registrationPoint, setRegistrationPoint);

    // these (along with all properties above) affect the simulation
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(density, updateDensity);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(gravity, updateGravity);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(damping, updateDamping);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(angularDamping, updateAngularDamping);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(restitution, updateRestitution);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(friction, updateFriction);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(ignoreForCollisions, updateIgnoreForCollisions);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(collisionsWillMove, updateCollisionsWillMove);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(created, updateCreated);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(lifetime, updateLifetime);

    // non-simulation properties below
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(script, setScript);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(scriptTimestamp, setScriptTimestamp);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(collisionSoundURL, setCollisionSoundURL);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(glowLevel, setGlowLevel);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(localRenderAlpha, setLocalRenderAlpha);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(visible, setVisible);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(locked, setLocked);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(userData, setUserData);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(marketplaceID, setMarketplaceID);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(name, setName);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(href, setHrefInternal);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(description, setDescription);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(actionData, setActionDataInternal);

    if (somethingChanged) {
        uint64_t now = usecTimestampNow();
        #ifdef WANT_DEBUG
            int elapsed = now - getLastEditedInternal();
            qCDebug(entities) << "EntityItem::setProperties() AFTER update... edited AGO=" << elapsed <<
                    "now=" << now << " getLastEdited()=" << getLastEditedInternal();
        #endif
        setLastEditedInternal(now);
        somethingChangedNotification(); // notify derived classes that something has changed
        if (getDirtyFlags() & (EntityItem::DIRTY_TRANSFORM | EntityItem::DIRTY_VELOCITIES)) {
            // anything that sets the transform or velocity must update _lastSimulated which is used
            // for kinematic extrapolation (e.g. we want to extrapolate forward from this moment
            // when position and/or velocity was changed).
            _lastSimulated = now;
        }
    }

    // timestamps
    quint64 timestamp = properties.getCreated();
    if (_created == UNKNOWN_CREATED_TIME && timestamp != UNKNOWN_CREATED_TIME) {
        quint64 now = usecTimestampNow();
        if (timestamp > now) {
            timestamp = now;
        }
        _created = timestamp;
    }

    unlock();
    return somethingChanged;
}

void EntityItem::recordCreationTime() {
    assertUnlocked();
    lockForWrite();
    if (_created == UNKNOWN_CREATED_TIME) {
        _created = usecTimestampNow();
    }
    auto now = usecTimestampNow();
    _lastEdited = _created;
    _lastUpdated = now;
    _lastSimulated = now;
    unlock();
}

quint64 EntityItem::getLastSimulated() const {
    assertUnlocked();
    lockForRead();
    auto result = getLastSimulatedInternal();
    unlock();
    return result;
}

quint64 EntityItem::getLastSimulatedInternal() const {
    assertLocked();
    return _lastSimulated;
}

void EntityItem::setLastSimulated(quint64 now) {
    assertUnlocked();
    lockForWrite();
    _lastSimulated = now;
    unlock();
}

quint64 EntityItem::getLastEdited() const {
    assertUnlocked();
    lockForRead();
    auto result = _lastEdited;
    unlock();
    return result;
}

quint64 EntityItem::getLastEditedInternal() const {
    assertLocked();
    return _lastEdited;
}

void EntityItem::setLastEdited(quint64 lastEdited) {
    assertUnlocked();
    lockForWrite();
    setLastEditedInternal(lastEdited);
    unlock();
}

void EntityItem::setLastEditedInternal(quint64 lastEdited) {
    assertWriteLocked();
    _lastEdited = _lastUpdated = lastEdited;
    _changedOnServer = glm::max(lastEdited, _changedOnServer);
}

float EntityItem::getEditedAgo() const {
    return (float)(usecTimestampNow() - getLastEdited()) / (float)USECS_PER_SECOND;
}

quint64 EntityItem::getLastBroadcast() const {
    assertUnlocked();
    lockForRead();
    auto result = _lastBroadcast;
    unlock();
    return result;
}

void EntityItem::setLastBroadcast(quint64 lastBroadcast) {
    assertUnlocked();
    lockForWrite();
    _lastBroadcast = lastBroadcast;
    unlock();
}

void EntityItem::markAsChangedOnServer() {
    assertUnlocked();
    lockForWrite();
    _changedOnServer = usecTimestampNow();
    unlock();
}

quint64 EntityItem::getLastChangedOnServer() const {
    assertUnlocked();
    lockForRead();
    auto result = _changedOnServer;
    unlock();
    return result;
}

EntityTypes::EntityType EntityItem::getType() const {
    assertUnlocked();
    lockForRead();
    auto result = getTypeInternal();
    unlock();
    return result;
}

EntityTypes::EntityType EntityItem::getTypeInternal() const {
    assertLocked();
    return _type;
}

void EntityItem::setCenterPosition(const glm::vec3& position) {
    assertUnlocked();
    Transform transformToCenter = getTransformToCenter();
    transformToCenter.setTranslation(position);
    setTranformToCenter(transformToCenter);
}

const Transform EntityItem::getTransformToCenter() const {
    assertUnlocked();
    Transform result = getTransform();
    if (getRegistrationPoint() != ENTITY_ITEM_HALF_VEC3) { // If it is not already centered, translate to center
        result.postTranslate(ENTITY_ITEM_HALF_VEC3 - getRegistrationPoint()); // Position to center
    }
    return result;
}

void EntityItem::setTranformToCenter(const Transform& transform) {
    assertUnlocked();
    if (getRegistrationPoint() == ENTITY_ITEM_HALF_VEC3) {
        // If it is already centered, just call setTransform
        setTransform(transform);
        return;
    }

    Transform copy = transform;
    copy.postTranslate(getRegistrationPoint() - ENTITY_ITEM_HALF_VEC3); // Center to position
    setTransform(copy);
}

const Transform EntityItem::getTransform() const {
    assertUnlocked();
    lockForRead();
    auto result = _transform;
    unlock();
    return result;
}

void EntityItem::setTransform(const Transform& transform) {
    assertUnlocked();
    lockForWrite();
    _transform = transform;
    unlock();
}

glm::vec3 EntityItem::getPosition() const {
    assertUnlocked();
    lockForRead();
    auto result = getPositionInternal();
    unlock();
    return result;
}

glm::vec3 EntityItem::getPositionInternal() const {
    assertLocked();
    return _transform.getTranslation();
}

void EntityItem::setPosition(const glm::vec3& value) {
    assertUnlocked();
    lockForWrite();
    setPositionInternal(value);
    unlock();
}

void EntityItem::setPositionInternal(const glm::vec3& value) {
    assertWriteLocked();
    _transform.setTranslation(value);
}

glm::quat EntityItem::getRotation() const {
    assertUnlocked();
    lockForRead();
    auto result = getRotationInternal();
    unlock();
    return result;
}

glm::quat EntityItem::getRotationInternal() const {
    assertLocked();
    return _transform.getRotation();
}

void EntityItem::setRotation(const glm::quat& rotation) {
    assertUnlocked();
    lockForWrite();
    setRotationInternal(rotation);
    unlock();
}

void EntityItem::setRotationInternal(const glm::quat& rotation) {
    assertWriteLocked();
    _transform.setRotation(rotation);
}

QString EntityItem::getHref() const {
    assertUnlocked();
    lockForRead();
    auto result = getHrefInternal();
    unlock();
    return result;
}

QString EntityItem::getHrefInternal() const {
    assertLocked();
    return _href;
}

void EntityItem::setHref(QString value) {
    assertUnlocked();
    lockForWrite();
    setHrefInternal(value);
    unlock();
}

void EntityItem::setHrefInternal(QString value) {
    assertWriteLocked();
    _href = value;
}

void EntityItem::setDimensions(const glm::vec3& value) {
    if (value.x <= 0.0f || value.y <= 0.0f || value.z <= 0.0f) {
        return;
    }
    _transform.setScale(value);
}

/// The maximum bounding cube for the entity, independent of it's rotation.
/// This accounts for the registration point (upon which rotation occurs around).
///
AACube EntityItem::getMaximumAACube() const {
    // * we know that the position is the center of rotation
    glm::vec3 centerOfRotation = getPosition(); // also where _registration point is

    // * we know that the registration point is the center of rotation
    // * we can calculate the length of the furthest extent from the registration point
    //   as the dimensions * max (registrationPoint, (1.0,1.0,1.0) - registrationPoint)
    glm::vec3 registrationPoint = (getDimensions() * getRegistrationPoint());
    glm::vec3 registrationRemainder = (getDimensions() * (glm::vec3(1.0f, 1.0f, 1.0f) - getRegistrationPoint()));
    glm::vec3 furthestExtentFromRegistration = glm::max(registrationPoint, registrationRemainder);

    // * we know that if you rotate in any direction you would create a sphere
    //   that has a radius of the length of furthest extent from registration point
    float radius = glm::length(furthestExtentFromRegistration);

    // * we know that the minimum bounding cube of this maximum possible sphere is
    //   (center - radius) to (center + radius)
    glm::vec3 minimumCorner = centerOfRotation - glm::vec3(radius, radius, radius);

    AACube boundingCube(minimumCorner, radius * 2.0f);
    return boundingCube;
}

/// The minimum bounding cube for the entity accounting for it's rotation.
/// This accounts for the registration point (upon which rotation occurs around).
///
AACube EntityItem::getMinimumAACube() const {
    // _position represents the position of the registration point.
    glm::vec3 registrationRemainder = glm::vec3(1.0f, 1.0f, 1.0f) - _registrationPoint;

    glm::vec3 unrotatedMinRelativeToEntity = - (getDimensions() * getRegistrationPoint());
    glm::vec3 unrotatedMaxRelativeToEntity = getDimensions() * registrationRemainder;
    Extents unrotatedExtentsRelativeToRegistrationPoint = { unrotatedMinRelativeToEntity, unrotatedMaxRelativeToEntity };
    Extents rotatedExtentsRelativeToRegistrationPoint = unrotatedExtentsRelativeToRegistrationPoint.getRotated(getRotation());

    // shift the extents to be relative to the position/registration point
    rotatedExtentsRelativeToRegistrationPoint.shiftBy(getPosition());

    // the cube that best encompasses extents is...
    AABox box(rotatedExtentsRelativeToRegistrationPoint);
    glm::vec3 centerOfBox = box.calcCenter();
    float longestSide = box.getLargestDimension();
    float halfLongestSide = longestSide / 2.0f;
    glm::vec3 cornerOfCube = centerOfBox - glm::vec3(halfLongestSide, halfLongestSide, halfLongestSide);


    // old implementation... not correct!!!
    return AACube(cornerOfCube, longestSide);
}

AABox EntityItem::getAABox() const {
    // _position represents the position of the registration point.
    glm::vec3 registrationRemainder = glm::vec3(1.0f, 1.0f, 1.0f) - _registrationPoint;

    glm::vec3 unrotatedMinRelativeToEntity = - (getDimensions() * _registrationPoint);
    glm::vec3 unrotatedMaxRelativeToEntity = getDimensions() * registrationRemainder;
    Extents unrotatedExtentsRelativeToRegistrationPoint = { unrotatedMinRelativeToEntity, unrotatedMaxRelativeToEntity };
    Extents rotatedExtentsRelativeToRegistrationPoint = unrotatedExtentsRelativeToRegistrationPoint.getRotated(getRotation());

    // shift the extents to be relative to the position/registration point
    rotatedExtentsRelativeToRegistrationPoint.shiftBy(getPosition());

    return AABox(rotatedExtentsRelativeToRegistrationPoint);
}

// NOTE: This should only be used in cases of old bitstreams which only contain radius data
//    0,0,0 --> maxDimension,maxDimension,maxDimension
//    ... has a corner to corner distance of glm::length(maxDimension,maxDimension,maxDimension)
//    ... radius = cornerToCornerLength / 2.0f
//    ... radius * 2.0f = cornerToCornerLength
//    ... cornerToCornerLength = sqrt(3 x maxDimension ^ 2)
//    ... cornerToCornerLength = sqrt(3 x maxDimension ^ 2)
//    ... radius * 2.0f = sqrt(3 x maxDimension ^ 2)
//    ... (radius * 2.0f) ^2 = 3 x maxDimension ^ 2
//    ... ((radius * 2.0f) ^2) / 3 = maxDimension ^ 2
//    ... sqrt(((radius * 2.0f) ^2) / 3) = maxDimension
//    ... sqrt((diameter ^2) / 3) = maxDimension
//
void EntityItem::setRadius(float value) {
    float diameter = value * 2.0f;
    float maxDimension = sqrt((diameter * diameter) / 3.0f);
    setDimensions(glm::vec3(maxDimension, maxDimension, maxDimension));
}

// TODO: get rid of all users of this function...
//    ... radius = cornerToCornerLength / 2.0f
//    ... cornerToCornerLength = sqrt(3 x maxDimension ^ 2)
//    ... radius = sqrt(3 x maxDimension ^ 2) / 2.0f;
float EntityItem::getRadius() const {
    return 0.5f * glm::length(getDimensions());
}

bool EntityItem::contains(const glm::vec3& point) const {
    if (getShapeType() == SHAPE_TYPE_COMPOUND) {
        return getAABox().contains(point);
    } else {
        ShapeInfo info;
        info.setParams(getShapeType(), glm::vec3(0.5f));
        return info.contains(worldToEntity(point));
    }
}

void EntityItem::computeShapeInfo(ShapeInfo& info) {
    info.setParams(getShapeType(), 0.5f * getDimensions());
}

void EntityItem::updatePosition(const glm::vec3& value) {
    assertWriteLocked();
    auto delta = glm::distance(getPositionInternal(), value);
    if (delta > IGNORE_POSITION_DELTA) {
        _dirtyFlags |= EntityItem::DIRTY_POSITION;
        setPositionInternal(value);
        if (delta > ACTIVATION_POSITION_DELTA) {
            _dirtyFlags |= EntityItem::DIRTY_PHYSICS_ACTIVATION;
        }
    }
}

void EntityItem::updateDimensions(const glm::vec3& value) {
    assertWriteLocked();
    auto delta = glm::distance(getDimensions(), value);
    if (delta > IGNORE_DIMENSIONS_DELTA) {
        setDimensions(value);
        if (delta > ACTIVATION_DIMENSIONS_DELTA) {
            // rebuilding the shape will always activate
            _dirtyFlags |= (EntityItem::DIRTY_SHAPE | EntityItem::DIRTY_MASS);
        }
    }
}

void EntityItem::updateRotation(const glm::quat& rotation) {
    assertWriteLocked();
    if (getRotationInternal() != rotation) {
        setRotationInternal(rotation);

        auto alignmentDot = glm::abs(glm::dot(getRotationInternal(), rotation));
        if (alignmentDot < IGNORE_ALIGNMENT_DOT) {
            _dirtyFlags |= EntityItem::DIRTY_ROTATION;
        }
        if (alignmentDot < ACTIVATION_ALIGNMENT_DOT) {
            _dirtyFlags |= EntityItem::DIRTY_PHYSICS_ACTIVATION;
        }
    }
}

void EntityItem::updateMass(float mass) {
    assertWriteLocked();
    // Setting the mass actually changes the _density (at fixed volume), however
    // we must protect the density range to help maintain stability of physics simulation
    // therefore this method might not accept the mass that is supplied.

    float volume = _volumeMultiplier * getDimensions().x * getDimensions().y * getDimensions().z;

    // compute new density
    float newDensity = _density;
    const float MIN_VOLUME = 1.0e-6f; // 0.001mm^3
    if (volume < 1.0e-6f) {
        // avoid divide by zero
        newDensity = glm::min(mass / MIN_VOLUME, ENTITY_ITEM_MAX_DENSITY);
    } else {
        newDensity = glm::max(glm::min(mass / volume, ENTITY_ITEM_MAX_DENSITY), ENTITY_ITEM_MIN_DENSITY);
    }

    float oldDensity = _density;
    _density = newDensity;

    if (fabsf(_density - oldDensity) / _density > ACTIVATION_RELATIVE_DENSITY_DELTA) {
        // the density has changed enough that we should update the physics simulation
        _dirtyFlags |= EntityItem::DIRTY_MASS;
    }
}

void EntityItem::updateVelocity(const glm::vec3& value) {
    assertWriteLocked();
    auto delta = glm::distance(_velocity, value);
    if (delta > IGNORE_LINEAR_VELOCITY_DELTA) {
        _dirtyFlags |= EntityItem::DIRTY_LINEAR_VELOCITY;
        const float MIN_LINEAR_SPEED = 0.001f;
        if (glm::length(value) < MIN_LINEAR_SPEED) {
            _velocity = ENTITY_ITEM_ZERO_VEC3;
        } else {
            _velocity = value;
            // only activate when setting non-zero velocity
            if (delta > ACTIVATION_LINEAR_VELOCITY_DELTA) {
                _dirtyFlags |= EntityItem::DIRTY_PHYSICS_ACTIVATION;
            }
        }
    }
}

void EntityItem::updateDamping(float value) {
    assertWriteLocked();
    auto clampedDamping = glm::clamp(value, 0.0f, 1.0f);
    if (fabsf(_damping - clampedDamping) > IGNORE_DAMPING_DELTA) {
        _damping = clampedDamping;
        _dirtyFlags |= EntityItem::DIRTY_MATERIAL;
    }
}

void EntityItem::updateGravity(const glm::vec3& value) {
    assertWriteLocked();
    auto delta = glm::distance(_gravity, value);
    if (delta > IGNORE_GRAVITY_DELTA) {
        _gravity = value;
        _dirtyFlags |= EntityItem::DIRTY_LINEAR_VELOCITY;
        if (delta > ACTIVATION_GRAVITY_DELTA) {
            _dirtyFlags |= EntityItem::DIRTY_PHYSICS_ACTIVATION;
        }
    }
}

void EntityItem::updateAngularVelocity(const glm::vec3& value) {
    assertWriteLocked();
    auto delta = glm::distance(_angularVelocity, value);
    if (delta > IGNORE_ANGULAR_VELOCITY_DELTA) {
        _dirtyFlags |= EntityItem::DIRTY_ANGULAR_VELOCITY;
        const float MIN_ANGULAR_SPEED = 0.0002f;
        if (glm::length(value) < MIN_ANGULAR_SPEED) {
            _angularVelocity = ENTITY_ITEM_ZERO_VEC3;
        } else {
            _angularVelocity = value;
            // only activate when setting non-zero velocity
            if (delta > ACTIVATION_ANGULAR_VELOCITY_DELTA) {
                _dirtyFlags |= EntityItem::DIRTY_PHYSICS_ACTIVATION;
            }
        }
    }
}

void EntityItem::updateAngularDamping(float value) {
    assertWriteLocked();
    auto clampedDamping = glm::clamp(value, 0.0f, 1.0f);
    if (fabsf(_angularDamping - clampedDamping) > IGNORE_DAMPING_DELTA) {
        _angularDamping = clampedDamping;
        _dirtyFlags |= EntityItem::DIRTY_MATERIAL;
    }
}

void EntityItem::updateIgnoreForCollisions(bool value) {
    assertWriteLocked();
    if (_ignoreForCollisions != value) {
        _ignoreForCollisions = value;
        _dirtyFlags |= EntityItem::DIRTY_COLLISION_GROUP;
    }
}

void EntityItem::updateCollisionsWillMove(bool value) {
    assertWriteLocked();
    if (_collisionsWillMove != value) {
        _collisionsWillMove = value;
        _dirtyFlags |= EntityItem::DIRTY_MOTION_TYPE;
    }
}

void EntityItem::updateRestitution(float value) {
    assertWriteLocked();
    float clampedValue = glm::max(glm::min(ENTITY_ITEM_MAX_RESTITUTION, value), ENTITY_ITEM_MIN_RESTITUTION);
    if (_restitution != clampedValue) {
        _restitution = clampedValue;
        _dirtyFlags |= EntityItem::DIRTY_MATERIAL;
    }
}

void EntityItem::updateFriction(float value) {
    assertWriteLocked();
    float clampedValue = glm::max(glm::min(ENTITY_ITEM_MAX_FRICTION, value), ENTITY_ITEM_MIN_FRICTION);
    if (_friction != clampedValue) {
        _friction = clampedValue;
        _dirtyFlags |= EntityItem::DIRTY_MATERIAL;
    }
}

void EntityItem::setRestitution(float value) {
    float clampedValue = glm::max(glm::min(ENTITY_ITEM_MAX_RESTITUTION, value), ENTITY_ITEM_MIN_RESTITUTION);
    _restitution = clampedValue;
}

void EntityItem::setFriction(float value) {
    float clampedValue = glm::max(glm::min(ENTITY_ITEM_MAX_FRICTION, value), ENTITY_ITEM_MIN_FRICTION);
    _friction = clampedValue;
}

void EntityItem::updateLifetime(float value) {
    assertWriteLocked();
    if (_lifetime != value) {
        _lifetime = value;
        _dirtyFlags |= EntityItem::DIRTY_LIFETIME;
    }
}

void EntityItem::updateCreated(uint64_t value) {
    assertWriteLocked();
    if (_created != value) {
        _created = value;
        _dirtyFlags |= EntityItem::DIRTY_LIFETIME;
    }
}

void EntityItem::setSimulationOwner(const QUuid& id, quint8 priority) {
    _simulationOwner.set(id, priority);
}

void EntityItem::setSimulationOwner(const SimulationOwner& owner) {
    _simulationOwner.set(owner);
}

void EntityItem::updateSimulatorID(const QUuid& value) {
    assertWriteLocked();
    if (_simulationOwner.setID(value)) {
        _dirtyFlags |= EntityItem::DIRTY_SIMULATOR_ID;
    }
}

void EntityItem::clearSimulationOwnership() {
    _simulationOwner.clear();
    // don't bother setting the DIRTY_SIMULATOR_ID flag because clearSimulationOwnership()
    // is only ever called entity-server-side and the flags are only used client-side
    //_dirtyFlags |= EntityItem::DIRTY_SIMULATOR_ID;

}

bool EntityItem::addAction(EntitySimulation* simulation, EntityActionPointer action) {
    lockForWrite();
    checkWaitingToRemove(simulation);

    bool result = addActionInternal(simulation, action);
    if (!result) {
        removeAction(simulation, action->getID());
    }

    unlock();
    return result;
}

bool EntityItem::addActionInternal(EntitySimulation* simulation, EntityActionPointer action) {
    assertLocked();
    assert(action);
    assert(simulation);
    auto actionOwnerEntity = action->getOwnerEntity().lock();
    assert(actionOwnerEntity);
    assert(actionOwnerEntity.get() == this);

    const QUuid& actionID = action->getID();
    assert(!_objectActions.contains(actionID) || _objectActions[actionID] == action);
    _objectActions[actionID] = action;
    simulation->addAction(action);

    bool success;
    QByteArray newDataCache = serializeActions(success);
    if (success) {
        _allActionsDataCache = newDataCache;
    }
    return success;
}

bool EntityItem::updateAction(EntitySimulation* simulation, const QUuid& actionID, const QVariantMap& arguments) {
    lockForWrite();
    checkWaitingToRemove(simulation);

    if (!_objectActions.contains(actionID)) {
        unlock();
        return false;
    }
    EntityActionPointer action = _objectActions[actionID];

    bool success = action->updateArguments(arguments);
    if (success) {
        _allActionsDataCache = serializeActions(success);
    } else {
        qDebug() << "EntityItem::updateAction failed";
    }

    unlock();
    return success;
}

bool EntityItem::removeAction(EntitySimulation* simulation, const QUuid& actionID) {
    lockForWrite();
    checkWaitingToRemove(simulation);

    bool success = removeActionInternal(actionID);
    unlock();
    return success;
}

bool EntityItem::removeActionInternal(const QUuid& actionID, EntitySimulation* simulation) {
    assertWriteLocked();
    if (_objectActions.contains(actionID)) {
        if (!simulation) {
            EntityTree* entityTree = _element ? _element->getTree() : nullptr;
            simulation = entityTree ? entityTree->getSimulation() : nullptr;
        }

        EntityActionPointer action = _objectActions[actionID];
        action->setOwnerEntity(nullptr);
        _objectActions.remove(actionID);

        if (simulation) {
            action->removeFromSimulation(simulation);
        }

        bool success = true;
        _allActionsDataCache = serializeActions(success);
        return success;
    }
    return false;
}

bool EntityItem::clearActions(EntitySimulation* simulation) {
    lockForWrite();
    QHash<QUuid, EntityActionPointer>::iterator i = _objectActions.begin();
    while (i != _objectActions.end()) {
        const QUuid id = i.key();
        EntityActionPointer action = _objectActions[id];
        i = _objectActions.erase(i);
        action->setOwnerEntity(nullptr);
        action->removeFromSimulation(simulation);
    }
    // empty _serializedActions means no actions for the EntityItem
    _actionsToRemove.clear();
    _allActionsDataCache.clear();
    unlock();
    return true;
}


void EntityItem::deserializeActions() {
    assertUnlocked();
    lockForWrite();
    deserializeActionsInternal();
    unlock();
}


void EntityItem::deserializeActionsInternal() {
    assertWriteLocked();

    if (!_element) {
        return;
    }

    // Keep track of which actions got added or updated by the new actionData

    EntityTree* entityTree = _element ? _element->getTree() : nullptr;
    assert(entityTree);
    EntitySimulation* simulation = entityTree ? entityTree->getSimulation() : nullptr;
    assert(simulation);

    QVector<QByteArray> serializedActions;
    if (_allActionsDataCache.size() > 0) {
        QDataStream serializedActionsStream(_allActionsDataCache);
        serializedActionsStream >> serializedActions;
    }

    QSet<QUuid> updated;

    foreach(QByteArray serializedAction, serializedActions) {
        QDataStream serializedActionStream(serializedAction);
        EntityActionType actionType;
        QUuid actionID;
        serializedActionStream >> actionType;
        serializedActionStream >> actionID;
        updated << actionID;

        if (_objectActions.contains(actionID)) {
            EntityActionPointer action = _objectActions[actionID];
            // TODO: make sure types match?  there isn't currently a way to
            // change the type of an existing action.
            action->deserialize(serializedAction);
        } else {
            auto actionFactory = DependencyManager::get<EntityActionFactoryInterface>();

            // EntityItemPointer entity = entityTree->findEntityByEntityItemID(_id, false);
            EntityItemPointer entity = shared_from_this();
            EntityActionPointer action = actionFactory->factoryBA(entity, serializedAction);
            if (action) {
                entity->addActionInternal(simulation, action);
            }
        }
    }

    // remove any actions that weren't included in the new data.
    QHash<QUuid, EntityActionPointer>::const_iterator i = _objectActions.begin();
    while (i != _objectActions.end()) {
        const QUuid id = i.key();
        if (!updated.contains(id)) {
            _actionsToRemove << id;
        }
        i++;
    }

    return;
}

void EntityItem::checkWaitingToRemove(EntitySimulation* simulation) {
    assertLocked();
    foreach(QUuid actionID, _actionsToRemove) {
        removeActionInternal(actionID, simulation);
    }
    _actionsToRemove.clear();
}

void EntityItem::setActionData(QByteArray actionData) {
    assertUnlocked();
    lockForWrite();
    setActionDataInternal(actionData);
    unlock();
}

void EntityItem::setActionDataInternal(QByteArray actionData) {
    assertWriteLocked();
    checkWaitingToRemove();
    _allActionsDataCache = actionData;
    deserializeActionsInternal();
}

QByteArray EntityItem::serializeActions(bool& success) const {
    assertLocked();
    QByteArray result;

    if (_objectActions.size() == 0) {
        success = true;
        return QByteArray();
    }

    QVector<QByteArray> serializedActions;
    QHash<QUuid, EntityActionPointer>::const_iterator i = _objectActions.begin();
    while (i != _objectActions.end()) {
        const QUuid id = i.key();
        EntityActionPointer action = _objectActions[id];
        QByteArray bytesForAction = action->serialize();
        serializedActions << bytesForAction;
        i++;
    }

    QDataStream serializedActionsStream(&result, QIODevice::WriteOnly);
    serializedActionsStream << serializedActions;

    if (result.size() >= _maxActionsDataSize) {
        success = false;
        return result;
    }

    success = true;
    return result;
}

const QByteArray EntityItem::getActionDataInternal() const {
    return _allActionsDataCache;
}

const QByteArray EntityItem::getActionData() const {
    assertUnlocked();
    lockForRead();
    auto result = getActionDataInternal();
    unlock();
    return result;
}

QVariantMap EntityItem::getActionArguments(const QUuid& actionID) const {
    QVariantMap result;
    lockForRead();

    if (_objectActions.contains(actionID)) {
        EntityActionPointer action = _objectActions[actionID];
        result = action->getArguments();
        result["type"] = EntityActionInterface::actionTypeToString(action->getType());
    }
    unlock();
    return result;
}



#define ENABLE_LOCKING 1

#ifdef ENABLE_LOCKING
void EntityItem::lockForRead() const {
    _lock.lockForRead();
}

bool EntityItem::tryLockForRead() const {
    return _lock.tryLockForRead();
}

void EntityItem::lockForWrite() const {
    _lock.lockForWrite();
}

bool EntityItem::tryLockForWrite() const {
    return _lock.tryLockForWrite();
}

void EntityItem::unlock() const {
    _lock.unlock();
}

bool EntityItem::isLocked() const {
    bool readSuccess = tryLockForRead();
    if (readSuccess) {
        unlock();
    }
    bool writeSuccess = tryLockForWrite();
    if (writeSuccess) {
        unlock();
    }
    if (readSuccess && writeSuccess) {
        return false;  // if we can take both kinds of lock, there was no previous lock
    }
    return true; // either read or write failed, so there is some lock in place.
}


bool EntityItem::isWriteLocked() const {
    bool readSuccess = tryLockForRead();
    if (readSuccess) {
        unlock();
        return false;
    }
    bool writeSuccess = tryLockForWrite();
    if (writeSuccess) {
        unlock();
        return false;
    }
    return true; // either read or write failed, so there is some lock in place.
}


bool EntityItem::isUnlocked() const {
    // this can't be sure -- this may get unlucky and hit locks from other threads.  what we're actually trying
    // to discover is if *this* thread hasn't locked the EntityItem.  Try repeatedly to take both kinds of lock.
    bool readSuccess = false;
    for (int i=0; i<30; i++) {
        readSuccess = tryLockForRead();
        if (readSuccess) {
            unlock();
            break;
        }
        QThread::usleep(200);
    }

    bool writeSuccess = false;
    if (readSuccess) {
        for (int i=0; i<30; i++) {
            writeSuccess = tryLockForWrite();
            if (writeSuccess) {
                unlock();
                break;
            }
            QThread::usleep(200);
        }
    }

    if (readSuccess && writeSuccess) {
        return true;  // if we can take both kinds of lock, there was no previous lock
    }
    return false;
}
#else
void EntityItem::lockForRead() const { }
bool EntityItem::tryLockForRead() const { return true; }
void EntityItem::lockForWrite() const { }
bool EntityItem::tryLockForWrite() const { return true; }
void EntityItem::unlock() const { }
bool EntityItem::isLocked() const { return true; }
bool EntityItem::isWriteLocked() const { return true; }
bool EntityItem::isUnlocked() const { return true; }
#endif
