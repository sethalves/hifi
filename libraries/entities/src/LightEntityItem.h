//
//  LightEntityItem.h
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_LightEntityItem_h
#define hifi_LightEntityItem_h

#include "EntityItem.h"

class LightEntityItem : public EntityItem {
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    LightEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties);

    ALLOW_INSTANTIATION // This class can be instantiated

    /// set dimensions in domain scale units (0.0 - 1.0) this will also reset radius appropriately
    virtual void setDimensions(const glm::vec3& value);
    void setDimensionsInternal(const glm::vec3& value);

    // methods for getting/setting all properties of an entity
    virtual EntityItemProperties getProperties(bool doLocking = true) const;
    virtual bool setProperties(const EntityItemProperties& properties, bool doLocking = true);

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

    const rgbColor& getColor() const;
    const rgbColor& getColorInternal() const;
    xColor getXColor() const;
    xColor getXColorInternal() const;

    void setColor(const rgbColor& value);
    void setColorInternal(const rgbColor& value);
    void setColor(const xColor& value);
    void setColorInternal(const xColor& value);

    bool getIsSpotlight() const;
    bool getIsSpotlightInternal() const;
    void setIsSpotlight(bool value);
    void setIsSpotlightInternal(bool value);

    void setIgnoredColor(const rgbColor& value) { }
    void setIgnoredAttenuation(float value) { }

    float getIntensity() const;
    float getIntensityInternal() const;
    void setIntensity(float value);
    void setIntensityInternal(float value);

    float getExponent() const;
    float getExponentInternal() const;
    void setExponent(float value);
    void setExponentInternal(float value);

    float getCutoff() const;
    float getCutoffInternal() const;
    void setCutoff(float value);
    void setCutoffInternal(float value);

    static bool getLightsArePickable() { return _lightsArePickable; }
    static void setLightsArePickable(bool value) { _lightsArePickable = value; }

protected:

    // properties of a light
    rgbColor _color;
    bool _isSpotlight;
    float _intensity;
    float _exponent;
    float _cutoff;

    static bool _lightsArePickable;
};

#endif // hifi_LightEntityItem_h
