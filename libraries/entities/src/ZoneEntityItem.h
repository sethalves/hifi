//
//  ZoneEntityItem.h
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ZoneEntityItem_h
#define hifi_ZoneEntityItem_h

#include <EnvironmentData.h>

#include "AtmospherePropertyGroup.h"
#include "EntityItem.h"

class ZoneEntityItem : public EntityItem {
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    ZoneEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties);

    ALLOW_INSTANTIATION // This class can be instantiated

    // methods for getting/setting all properties of an entity
    virtual EntityItemProperties getProperties(bool doLocking = true) const;
    virtual bool setProperties(const EntityItemProperties& properties, bool doLocking = true);

    // TODO: eventually only include properties changed since the params.lastViewFrustumSent time
    virtual EntityPropertyFlags getEntityProperties(EncodeBitstreamParams& params) const;

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

    xColor getKeyLightColor() const;
    void setKeyLightColor(const xColor& value);

    void setKeyLightColor(const rgbColor& value);

    glm::vec3 getKeyLightColorVec3() const;

    float getKeyLightIntensity() const;
    void setKeyLightIntensity(float value);

    float getKeyLightAmbientIntensity() const;
    void setKeyLightAmbientIntensity(float value);

    glm::vec3 getKeyLightDirection() const;
    void setKeyLightDirection(const glm::vec3& value);

    static bool getZonesArePickable() { return _zonesArePickable; }
    static void setZonesArePickable(bool value) { _zonesArePickable = value; }

    static bool getDrawZoneBoundaries() { return _drawZoneBoundaries; }
    static void setDrawZoneBoundaries(bool value) { _drawZoneBoundaries = value; }

    virtual bool isReadyToComputeShape() { return false; }
    void updateShapeType(ShapeType type) { _shapeType = type; }

    virtual bool hasCompoundShapeURL() const;
    QString getCompoundShapeURL() const;
    virtual void setCompoundShapeURL(const QString& url);

    void setBackgroundMode(BackgroundMode value);
    BackgroundMode getBackgroundMode() const;

    EnvironmentData getEnvironmentData() const;
    AtmospherePropertyGroup getAtmosphereProperties() const;
    SkyboxPropertyGroup getSkyboxProperties() const;
    StagePropertyGroup getStageProperties() const;

    virtual bool supportsDetailedRayIntersection() const { return true; }
    virtual bool findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                         bool& keepSearching, OctreeElement*& element, float& distance, BoxFace& face,
                         void** intersectedObject, bool precisionPicking) const;

    static const xColor DEFAULT_KEYLIGHT_COLOR;
    static const float DEFAULT_KEYLIGHT_INTENSITY;
    static const float DEFAULT_KEYLIGHT_AMBIENT_INTENSITY;
    static const glm::vec3 DEFAULT_KEYLIGHT_DIRECTION;
    static const ShapeType DEFAULT_SHAPE_TYPE;
    static const QString DEFAULT_COMPOUND_SHAPE_URL;

protected:

    void setKeyLightColorInternal(const xColor& value);
    void setKeyLightColorInternal(const rgbColor& value);
    glm::vec3 getKeyLightColorVec3Internal() const;
    float getKeyLightIntensityInternal() const;
    void setKeyLightIntensityInternal(float value);
    float getKeyLightAmbientIntensityInternal() const;
    void setKeyLightAmbientIntensityInternal(float value);
    glm::vec3 getKeyLightDirectionInternal() const;
    void setKeyLightDirectionInternal(const glm::vec3& value);
    virtual ShapeType getShapeTypeInternal() const;
    virtual bool hasCompoundShapeURLInternal() const;
    QString getCompoundShapeURLInternal() const;
    virtual void setCompoundShapeURLInternal(const QString& url);
    void setBackgroundModeInternal(BackgroundMode value);
    BackgroundMode getBackgroundModeInternal() const;
    AtmospherePropertyGroup getAtmospherePropertiesInternal() const;
    SkyboxPropertyGroup getSkyboxPropertiesInternal() const;
    StagePropertyGroup getStagePropertiesInternal() const;
    xColor getKeyLightColorInternal() const;

    
    virtual void debugDump() const;

    // properties of the "sun" in the zone
    rgbColor _keyLightColor;
    float _keyLightIntensity;
    float _keyLightAmbientIntensity;
    glm::vec3 _keyLightDirection;

    ShapeType _shapeType = DEFAULT_SHAPE_TYPE;
    QString _compoundShapeURL;

    BackgroundMode _backgroundMode = BACKGROUND_MODE_INHERIT;

    StagePropertyGroup _stageProperties;
    AtmospherePropertyGroup _atmosphereProperties;
    SkyboxPropertyGroup _skyboxProperties;

    static bool _drawZoneBoundaries;
    static bool _zonesArePickable;
};

#endif // hifi_ZoneEntityItem_h
