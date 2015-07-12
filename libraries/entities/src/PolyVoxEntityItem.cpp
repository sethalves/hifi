//
//  PolyVoxEntityItem.cpp
//  libraries/entities/src
//
//  Created by Seth Alves on 5/11/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include <QByteArray>
#include <QDebug>

#include <ByteCountCoding.h>

#include "PolyVoxEntityItem.h"
#include "EntityTree.h"
#include "EntitiesLogging.h"
#include "EntityTreeElement.h"


const glm::vec3 PolyVoxEntityItem::DEFAULT_VOXEL_VOLUME_SIZE = glm::vec3(32, 32, 32);
const float PolyVoxEntityItem::MAX_VOXEL_DIMENSION = 32.0f;
const QByteArray PolyVoxEntityItem::DEFAULT_VOXEL_DATA(PolyVoxEntityItem::makeEmptyVoxelData());
const PolyVoxEntityItem::PolyVoxSurfaceStyle PolyVoxEntityItem::DEFAULT_VOXEL_SURFACE_STYLE =
    PolyVoxEntityItem::SURFACE_MARCHING_CUBES;

EntityItemPointer PolyVoxEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return EntityItemPointer(new PolyVoxEntityItem(entityID, properties));
}

QByteArray PolyVoxEntityItem::makeEmptyVoxelData(quint16 voxelXSize, quint16 voxelYSize, quint16 voxelZSize) {
    int rawSize = voxelXSize * voxelYSize * voxelZSize;

    QByteArray uncompressedData = QByteArray(rawSize, '\0');
    QByteArray newVoxelData;
    QDataStream writer(&newVoxelData, QIODevice::WriteOnly | QIODevice::Truncate);
    writer << voxelXSize << voxelYSize << voxelZSize;

    QByteArray compressedData = qCompress(uncompressedData, 9);
    writer << compressedData;

    return newVoxelData;
}

PolyVoxEntityItem::PolyVoxEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
    EntityItem(entityItemID),
    _voxelVolumeSize(PolyVoxEntityItem::DEFAULT_VOXEL_VOLUME_SIZE),
    _voxelData(PolyVoxEntityItem::DEFAULT_VOXEL_DATA),
    _voxelSurfaceStyle(PolyVoxEntityItem::DEFAULT_VOXEL_SURFACE_STYLE)
{
    _type = EntityTypes::PolyVox;
    _created = properties.getCreated();
    setProperties(properties);
}

void PolyVoxEntityItem::setVoxelVolumeSize(glm::vec3 voxelVolumeSize) {
    assertUnlocked();
    lockForWrite();
    setVoxelVolumeSizeInternal(voxelVolumeSize);
    unlock();
}

void PolyVoxEntityItem::setVoxelVolumeSizeInternal(glm::vec3 voxelVolumeSize) {
    assertWriteLocked();
    assert((int)_voxelVolumeSize.x == _voxelVolumeSize.x);
    assert((int)_voxelVolumeSize.y == _voxelVolumeSize.y);
    assert((int)_voxelVolumeSize.z == _voxelVolumeSize.z);

    _voxelVolumeSize = voxelVolumeSize;
    if (_voxelVolumeSize.x < 1) {
        qDebug() << "PolyVoxEntityItem::setVoxelVolumeSize clamping x of" << _voxelVolumeSize.x << "to 1";
        _voxelVolumeSize.x = 1;
    }
    if (_voxelVolumeSize.x > MAX_VOXEL_DIMENSION) {
        qDebug() << "PolyVoxEntityItem::setVoxelVolumeSize clamping x of" << _voxelVolumeSize.x << "to max";
        _voxelVolumeSize.x = MAX_VOXEL_DIMENSION;
    }

    if (_voxelVolumeSize.y < 1) {
        qDebug() << "PolyVoxEntityItem::setVoxelVolumeSize clamping y of" << _voxelVolumeSize.y << "to 1";
        _voxelVolumeSize.y = 1;
    }
    if (_voxelVolumeSize.y > MAX_VOXEL_DIMENSION) {
        qDebug() << "PolyVoxEntityItem::setVoxelVolumeSize clamping y of" << _voxelVolumeSize.y << "to max";
        _voxelVolumeSize.y = MAX_VOXEL_DIMENSION;
    }

    if (_voxelVolumeSize.z < 1) {
        qDebug() << "PolyVoxEntityItem::setVoxelVolumeSize clamping z of" << _voxelVolumeSize.z << "to 1";
        _voxelVolumeSize.z = 1;
    }
    if (_voxelVolumeSize.z > MAX_VOXEL_DIMENSION) {
        qDebug() << "PolyVoxEntityItem::setVoxelVolumeSize clamping z of" << _voxelVolumeSize.z << "to max";
        _voxelVolumeSize.z = MAX_VOXEL_DIMENSION;
    }
}


glm::vec3 PolyVoxEntityItem::getVoxelVolumeSize() const {
    assertUnlocked();
    lockForRead();
    auto result = getVoxelVolumeSizeInternal();
    unlock();
    return result;
}

glm::vec3 PolyVoxEntityItem::getVoxelVolumeSizeInternal() const {
    assertLocked();
    return _voxelVolumeSize;
}

void PolyVoxEntityItem::setVoxelData(QByteArray voxelData) {
    assertUnlocked();
    lockForWrite();
    setVoxelDataInternal(voxelData);
    unlock();
}

void PolyVoxEntityItem::setVoxelDataInternal(QByteArray voxelData) {
    assertWriteLocked();
    _voxelData = voxelData;
}


QByteArray PolyVoxEntityItem::getVoxelData() const {
    assertUnlocked();
    lockForRead();
    auto result = getVoxelDataInternal();
    unlock();
    return result;
}

QByteArray PolyVoxEntityItem::getVoxelDataInternal() const {
    assertLocked();
    return _voxelData;
}


EntityItemProperties PolyVoxEntityItem::getProperties(bool doLocking) const {
    if (doLocking) {
        assertUnlocked();
        lockForRead();
    } else {
        assertLocked();
    }
    EntityItemProperties properties = EntityItem::getProperties(false); // get the properties from our base class
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(voxelVolumeSize, getVoxelVolumeSizeInternal);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(voxelData, getVoxelDataInternal);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(voxelSurfaceStyle, getVoxelSurfaceStyleInternal);

    if (doLocking) {
        unlock();
    }

    return properties;
}

bool PolyVoxEntityItem::setProperties(const EntityItemProperties& properties, bool doLocking) {
    if (doLocking) {
        assertUnlocked();
        lockForWrite();
    } else {
        assertWriteLocked();
    }

    bool somethingChanged = EntityItem::setProperties(properties, false); // set the properties in our base class
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(voxelVolumeSize, setVoxelVolumeSizeInternal);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(voxelData, setVoxelDataInternal);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(voxelSurfaceStyle, setVoxelSurfaceStyleInternal);

    if (somethingChanged) {
        bool wantDebug = false;
        if (wantDebug) {
            uint64_t now = usecTimestampNow();
            int elapsed = now - getLastEditedInternal();
            qCDebug(entities) << "PolyVoxEntityItem::setProperties() AFTER update... edited AGO=" << elapsed <<
                "now=" << now << " getLastEdited()=" << getLastEditedInternal();
        }
        setLastEditedInternal(properties._lastEdited);
    }

    if (doLocking) {
        unlock();
    }
    return somethingChanged;
}

int PolyVoxEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                                        ReadBitstreamToTreeParams& args,
                                                        EntityPropertyFlags& propertyFlags, bool overwriteLocalData) {
    assertWriteLocked();
    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_VOXEL_VOLUME_SIZE, glm::vec3, setVoxelVolumeSizeInternal);
    READ_ENTITY_PROPERTY(PROP_VOXEL_DATA, QByteArray, setVoxelDataInternal);
    READ_ENTITY_PROPERTY(PROP_VOXEL_SURFACE_STYLE, uint16_t, setVoxelSurfaceStyleInternal);

    return bytesRead;
}


// TODO: eventually only include properties changed since the params.lastViewFrustumSent time
EntityPropertyFlags PolyVoxEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);
    requestedProperties += PROP_VOXEL_VOLUME_SIZE;
    requestedProperties += PROP_VOXEL_DATA;
    requestedProperties += PROP_VOXEL_SURFACE_STYLE;
    return requestedProperties;
}

void PolyVoxEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                           EntityTreeElementExtraEncodeData* modelTreeElementExtraEncodeData,
                                           EntityPropertyFlags& requestedProperties,
                                           EntityPropertyFlags& propertyFlags,
                                           EntityPropertyFlags& propertiesDidntFit,
                                           int& propertyCount,
                                           OctreeElement::AppendState& appendState) const {
    assertLocked();
    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_VOXEL_VOLUME_SIZE, getVoxelVolumeSizeInternal());
    APPEND_ENTITY_PROPERTY(PROP_VOXEL_DATA, getVoxelDataInternal());
    APPEND_ENTITY_PROPERTY(PROP_VOXEL_SURFACE_STYLE, (uint16_t) getVoxelSurfaceStyleInternal());
}

void PolyVoxEntityItem::debugDump() const {
    assertLocked();
    quint64 now = usecTimestampNow();
    qCDebug(entities) << "   POLYVOX EntityItem id:" << getEntityItemID() << "---------------------------------------------";
    qCDebug(entities) << "            position:" << debugTreeVector(getPosition());
    qCDebug(entities) << "          dimensions:" << debugTreeVector(getDimensions());
    qCDebug(entities) << "       getLastEdited:" << debugTime(getLastEdited(), now);
}

void PolyVoxEntityItem::setVoxelSurfaceStyle(PolyVoxSurfaceStyle voxelSurfaceStyle) {
    assertUnlocked();
    lockForWrite();
    setVoxelSurfaceStyleInternal(voxelSurfaceStyle);
    unlock();
}

void PolyVoxEntityItem::setVoxelSurfaceStyleInternal(PolyVoxSurfaceStyle voxelSurfaceStyle) {
    assertWriteLocked();
    if (voxelSurfaceStyle == _voxelSurfaceStyle) {
        return;
    }
    updateVoxelSurfaceStyle(voxelSurfaceStyle);
}

void PolyVoxEntityItem::setVoxelSurfaceStyle(uint16_t voxelSurfaceStyle) {
    assertUnlocked();
    lockForWrite();
    setVoxelSurfaceStyleInternal(voxelSurfaceStyle);
    unlock();
}

void PolyVoxEntityItem::setVoxelSurfaceStyleInternal(uint16_t voxelSurfaceStyle) {
    assertWriteLocked();
    setVoxelSurfaceStyleInternal((PolyVoxSurfaceStyle) voxelSurfaceStyle);
}

PolyVoxEntityItem::PolyVoxSurfaceStyle PolyVoxEntityItem::getVoxelSurfaceStyle() const {
    assertUnlocked();
    lockForRead();
    auto result = getVoxelSurfaceStyleInternal();
    unlock();
    return result;
}

PolyVoxEntityItem::PolyVoxSurfaceStyle PolyVoxEntityItem::getVoxelSurfaceStyleInternal() const {
    assertLocked();
    return _voxelSurfaceStyle;
}

void PolyVoxEntityItem::setSphereInVolume(glm::vec3 center, float radius, uint8_t toValue) {
    assertUnlocked();
    lockForWrite();
    setSphereInVolumeInternal(center, radius, toValue);
    unlock();
}

void PolyVoxEntityItem::setSphere(glm::vec3 center, float radius, uint8_t toValue) {
    assertUnlocked();
    lockForWrite();
    setSphereInternal(center, radius, toValue);
    unlock();
}

void PolyVoxEntityItem::setAll(uint8_t toValue) {
    assertUnlocked();
    lockForWrite();
    setAllInternal(toValue);
    unlock();
}

void PolyVoxEntityItem::setVoxelInVolume(glm::vec3 position, uint8_t toValue) {
    assertUnlocked();
    lockForWrite();
    setVoxelInVolumeInternal(position, toValue);
    unlock();
}

uint8_t PolyVoxEntityItem::getVoxel(int x, int y, int z) {
    assertUnlocked();
    lockForRead();
    auto result = getVoxelInternal(x, y, z);
    unlock();
    return result;
}

void PolyVoxEntityItem::setVoxel(int x, int y, int z, uint8_t toValue) {
    assertUnlocked();
    lockForWrite();
    setVoxelInternal(x, y, z, toValue);
    unlock();
}
