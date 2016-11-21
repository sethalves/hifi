//
//  PacketHeaders.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 4/8/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PacketHeaders_h
#define hifi_PacketHeaders_h

#pragma once

#include <cstdint>
#include <map>

#include <QtCore/QCryptographicHash>
#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtCore/QUuid>

// The enums are inside this PacketTypeEnum for run-time conversion of enum value to string via
// Q_ENUMS, without requiring a macro that is called for each enum value.
class PacketTypeEnum {
    Q_GADGET
    Q_ENUMS(Value)
public:
    // If adding a new packet packetType, you can replace one marked usable or add at the end.
    // This enum must hold 256 or fewer packet types (so the value is <= 255) since it is statically typed as a uint8_t
    enum class Value : uint8_t {
        Unknown,
        StunResponse,
        DomainList,
        Ping,
        PingReply,
        KillAvatar,
        AvatarData,
        InjectAudio,
        MixedAudio,
        MicrophoneAudioNoEcho,
        MicrophoneAudioWithEcho,
        BulkAvatarData,
        SilentAudioFrame,
        DomainListRequest,
        RequestAssignment,
        CreateAssignment,
        DomainConnectionDenied,
        MuteEnvironment,
        AudioStreamStats,
        DomainServerPathQuery,
        DomainServerPathResponse,
        DomainServerAddedNode,
        ICEServerPeerInformation,
        ICEServerQuery,
        OctreeStats,
        Jurisdiction,
        JurisdictionRequest,
        AssignmentClientStatus,
        NoisyMute,
        AvatarIdentity,
        NodeIgnoreRequest,
        DomainConnectRequest,
        DomainServerRequireDTLS,
        NodeJsonStats,
        OctreeDataNack,
        StopNode,
        AudioEnvironment,
        EntityEditNack,
        ICEServerHeartbeat,
        ICEPing,
        ICEPingReply,
        EntityData,
        EntityQuery,
        EntityAdd,
        EntityErase,
        EntityEdit,
        DomainServerConnectionToken,
        DomainSettingsRequest,
        DomainSettings,
        AssetGet,
        AssetGetReply,
        AssetUpload,
        AssetUploadReply,
        AssetGetInfo,
        AssetGetInfoReply,
        DomainDisconnectRequest,
        DomainServerRemovedNode,
        MessagesData,
        MessagesSubscribe,
        MessagesUnsubscribe,
        ICEServerHeartbeatDenied,
        AssetMappingOperation,
        AssetMappingOperationReply,
        ICEServerHeartbeatACK,
        NegotiateAudioFormat,
        SelectedAudioFormat,
        MoreEntityShapes,
        NodeKickRequest,
        NodeMuteRequest,
        RadiusIgnoreRequest,
        EntityEditCAS,
        EntityEditRejected,
        LAST_PACKET_TYPE = EntityEditRejected
    };
};

using PacketType = PacketTypeEnum::Value;

const int NUM_BYTES_MD5_HASH = 16;

typedef char PacketVersion;

extern const QSet<PacketType> NON_VERIFIED_PACKETS;
extern const QSet<PacketType> NON_SOURCED_PACKETS;

PacketVersion versionForPacketType(PacketType packetType);
QByteArray protocolVersionsSignature(); /// returns a unqiue signature for all the current protocols
QString protocolVersionsSignatureBase64();

#if (PR_BUILD || DEV_BUILD)
void sendWrongProtocolVersionsSignature(bool sendWrongVersion); /// for debugging version negotiation
#endif

uint qHash(const PacketType& key, uint seed);
QDebug operator<<(QDebug debug, const PacketType& type);

enum class EntitiesPacketVersion : PacketVersion {
    LastEditedBy = 65,
    CASAndEditRejected = 66,
};

enum class AssetServerPacketVersion: PacketVersion {
    VegasCongestionControl = 19
};

enum class AvatarMixerPacketVersion : PacketVersion {
    TranslationSupport = 17,
    SoftAttachmentSupport,
    AvatarEntities,
    AbsoluteSixByteRotations,
    SensorToWorldMat,
    HandControllerJoints
};

enum class DomainConnectRequestVersion : PacketVersion {
    NoHostname = 17,
    HasHostname,
    HasProtocolVersions,
    HasMACAddress
};

enum class DomainConnectionDeniedVersion : PacketVersion {
    ReasonMessageOnly = 17,
    IncludesReasonCode,
    IncludesExtraInfo
};

enum class DomainServerAddedNodeVersion : PacketVersion {
    PrePermissionsGrid = 17,
    PermissionsGrid
};

enum class DomainListVersion : PacketVersion {
    PrePermissionsGrid = 18,
    PermissionsGrid
};

enum class AudioVersion : PacketVersion {
    HasCompressedAudio = 17,
    CodecNameInAudioPackets,
    Exactly10msAudioPackets,
    TerminatingStreamStats,
};

#endif // hifi_PacketHeaders_h
