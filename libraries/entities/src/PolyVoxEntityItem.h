//
//  PolyVoxEntityItem.h
//  libraries/entities/src
//
//  Created by Seth Alves on 5/11/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PolyVoxEntityItem_h
#define hifi_PolyVoxEntityItem_h

#include "EntityItem.h"

class PolyVoxEntityItem : public EntityItem {
 public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    PolyVoxEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties);

    ALLOW_INSTANTIATION // This class can be instantiated

    // methods for getting/setting all properties of an entity
    virtual EntityItemProperties getProperties() const;
    virtual bool setProperties(const EntityItemProperties& properties);

    // TODO: eventually only include properties changed since the params.lastViewFrustumSent time
    virtual EntityPropertyFlags getEntityProperties(EncodeBitstreamParams& params, bool doLocking = true) const;

    virtual void appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                    EntityTreeElementExtraEncodeData* modelTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount,
                                    OctreeElement::AppendState& appendState) const;

    virtual int readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                 ReadBitstreamToTreeParams& args,
                                                 EntityPropertyFlags& propertyFlags, bool overwriteLocalData);

    // never have a ray intersection pick a PolyVoxEntityItem.
    virtual bool supportsDetailedRayIntersection() const { return true; }
    virtual bool findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                         bool& keepSearching, OctreeElement*& element, float& distance, BoxFace& face,
                         void** intersectedObject, bool precisionPicking) const { return false; }

    virtual void debugDump() const;

    virtual void setVoxelVolumeSize(glm::vec3 voxelVolumeSize);
    virtual const glm::vec3& getVoxelVolumeSize() const { return _voxelVolumeSize; }

    virtual void setVoxelData(QByteArray voxelData) { _voxelData = voxelData; }
    virtual const QByteArray& getVoxelData() const { return _voxelData; }

    enum PolyVoxSurfaceStyle {
        SURFACE_MARCHING_CUBES,
        SURFACE_CUBIC,
        SURFACE_EDGED_CUBIC
    };

    void setVoxelSurfaceStyle(PolyVoxSurfaceStyle voxelSurfaceStyle);
    // this other version of setVoxelSurfaceStyle is needed for SET_ENTITY_PROPERTY_FROM_PROPERTIES
    void setVoxelSurfaceStyle(uint16_t voxelSurfaceStyle) { setVoxelSurfaceStyle((PolyVoxSurfaceStyle) voxelSurfaceStyle); }
    virtual PolyVoxSurfaceStyle getVoxelSurfaceStyle() const { return _voxelSurfaceStyle; }

    static const glm::vec3 DEFAULT_VOXEL_VOLUME_SIZE;
    static const float MAX_VOXEL_DIMENSION;

    static const QByteArray DEFAULT_VOXEL_DATA;
    static const PolyVoxSurfaceStyle DEFAULT_VOXEL_SURFACE_STYLE;

    // coords are in voxel-volume space
    virtual void setSphereInVolume(glm::vec3 center, float radius, uint8_t toValue) {}

    // coords are in world-space
    virtual void setSphere(glm::vec3 center, float radius, uint8_t toValue) {}

    virtual void setAll(uint8_t toValue) {}

    virtual void setVoxelInVolume(glm::vec3 position, uint8_t toValue) {}

    virtual uint8_t getVoxel(int x, int y, int z) { return 0; }
    virtual void setVoxel(int x, int y, int z, uint8_t toValue) {}

    static QByteArray makeEmptyVoxelData(quint16 voxelXSize = 16, quint16 voxelYSize = 16, quint16 voxelZSize = 16);

 protected:
    virtual void updateVoxelSurfaceStyle(PolyVoxSurfaceStyle voxelSurfaceStyle) {
        _voxelSurfaceStyle = voxelSurfaceStyle;
    }

    glm::vec3 _voxelVolumeSize; // this is always 3 bytes
    QByteArray _voxelData;
    PolyVoxSurfaceStyle _voxelSurfaceStyle;
};

#endif // hifi_PolyVoxEntityItem_h
