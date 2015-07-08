//
//  LightEntityItem.cpp
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include <QDebug>

#include <ByteCountCoding.h>

#include "EntityItemID.h"
#include "EntityTree.h"
#include "EntityTreeElement.h"
#include "EntitiesLogging.h"
#include "LightEntityItem.h"

bool LightEntityItem::_lightsArePickable = false;

EntityItemPointer LightEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItemPointer result { new LightEntityItem(entityID, properties) };
    return result;
}

// our non-pure virtual subclass for now...
LightEntityItem::LightEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        EntityItem(entityItemID, properties) 
{
    _type = EntityTypes::Light;

    // default property values
    _color[RED_INDEX] = _color[GREEN_INDEX] = _color[BLUE_INDEX] = 0;
    _intensity = 1.0f;
    _exponent = 0.0f;
    _cutoff = PI;

    setProperties(properties, false);
}

void LightEntityItem::setDimensions(const glm::vec3& value) {
    assertUnlocked();
    lockForWrite();
    setDimensionsInternal(value);
    unlock();
}

void LightEntityItem::setDimensionsInternal(const glm::vec3& value) {
    assertWriteLocked();
    if (_isSpotlight) {
        // If we are a spotlight, treat the z value as our radius or length, and
        // recalculate the x/y dimensions to properly encapsulate the spotlight.
        const float length = value.z;
        const float width = length * glm::sin(glm::radians(_cutoff));
        EntityItem::setDimensionsInternal(glm::vec3(width, width, length));
    } else {
        float maxDimension = glm::max(value.x, value.y, value.z);
        EntityItem::setDimensionsInternal(glm::vec3(maxDimension, maxDimension, maxDimension));
    }
}


EntityItemProperties LightEntityItem::getProperties(bool doLocking) const {
    if (doLocking) {
        assertUnlocked();
        lockForRead();
    } else {
        assertLocked();
    }
    EntityItemProperties properties = EntityItem::getProperties(false); // get the properties from our base class

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(isSpotlight, getIsSpotlight);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(color, getXColor);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(intensity, getIntensity);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(exponent, getExponent);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(cutoff, getCutoff);

    if (doLocking) {
        unlock();
    }

    return properties;
}

bool LightEntityItem::setProperties(const EntityItemProperties& properties, bool doLocking) {
    if (doLocking) {
        assertUnlocked();
        lockForWrite();
    } else {
        assertWriteLocked();
    }

    bool somethingChanged = EntityItem::setProperties(properties, false); // set the properties in our base class

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(isSpotlight, setIsSpotlightInternal);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(color, setColorInternal);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(intensity, setIntensityInternal);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(exponent, setExponentInternal);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(cutoff, setCutoffInternal);

    if (somethingChanged) {
        bool wantDebug = false;
        if (wantDebug) {
            uint64_t now = usecTimestampNow();
            int elapsed = now - getLastEditedInternal();
            qCDebug(entities) << "LightEntityItem::setProperties() AFTER update... edited AGO=" << elapsed <<
                    "now=" << now << " getLastEdited()=" << getLastEditedInternal();
        }
        setLastEditedInternal(properties.getLastEdited());
    }

    if (doLocking) {
        unlock();
    }
    return somethingChanged;
}

int LightEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData) {
    assertWriteLocked();
    int bytesRead = 0;
    const unsigned char* dataAt = data;

    if (args.bitstreamVersion < VERSION_ENTITIES_LIGHT_HAS_INTENSITY_AND_COLOR_PROPERTIES) {
        READ_ENTITY_PROPERTY(PROP_IS_SPOTLIGHT, bool, setIsSpotlightInternal);

        // _diffuseColor has been renamed to _color
        READ_ENTITY_PROPERTY(PROP_DIFFUSE_COLOR, rgbColor, setColorInternal);

        // Ambient and specular color are from an older format and are no longer supported.
        // Their values will be ignored.
        READ_ENTITY_PROPERTY(PROP_AMBIENT_COLOR_UNUSED, rgbColor, setIgnoredColor);
        READ_ENTITY_PROPERTY(PROP_SPECULAR_COLOR_UNUSED, rgbColor, setIgnoredColor);

        // _constantAttenuation has been renamed to _intensity
        READ_ENTITY_PROPERTY(PROP_INTENSITY, float, setIntensityInternal);

        // Linear and quadratic attenuation are from an older format and are no longer supported.
        // Their values will be ignored.
        READ_ENTITY_PROPERTY(PROP_LINEAR_ATTENUATION_UNUSED, float, setIgnoredAttenuation);
        READ_ENTITY_PROPERTY(PROP_QUADRATIC_ATTENUATION_UNUSED, float, setIgnoredAttenuation);

        READ_ENTITY_PROPERTY(PROP_EXPONENT, float, setExponentInternal);
        READ_ENTITY_PROPERTY(PROP_CUTOFF, float, setCutoffInternal);
    } else {
        READ_ENTITY_PROPERTY(PROP_IS_SPOTLIGHT, bool, setIsSpotlightInternal);
        READ_ENTITY_PROPERTY(PROP_COLOR, rgbColor, setColorInternal);
        READ_ENTITY_PROPERTY(PROP_INTENSITY, float, setIntensityInternal);
        READ_ENTITY_PROPERTY(PROP_EXPONENT, float, setExponentInternal);
        READ_ENTITY_PROPERTY(PROP_CUTOFF, float, setCutoffInternal);
    }

    return bytesRead;
}


// TODO: eventually only include properties changed since the params.lastViewFrustumSent time
EntityPropertyFlags LightEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);
    requestedProperties += PROP_IS_SPOTLIGHT;
    requestedProperties += PROP_COLOR;
    requestedProperties += PROP_INTENSITY;
    requestedProperties += PROP_EXPONENT;
    requestedProperties += PROP_CUTOFF;
    return requestedProperties;
}

void LightEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                    EntityTreeElementExtraEncodeData* modelTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount,
                                    OctreeElement::AppendState& appendState) const {
    assertLocked();
    bool successPropertyFits = true;
    APPEND_ENTITY_PROPERTY(PROP_IS_SPOTLIGHT, getIsSpotlightInternal());
    APPEND_ENTITY_PROPERTY(PROP_COLOR, getColorInternal());
    APPEND_ENTITY_PROPERTY(PROP_INTENSITY, getIntensityInternal());
    APPEND_ENTITY_PROPERTY(PROP_EXPONENT, getExponentInternal());
    APPEND_ENTITY_PROPERTY(PROP_CUTOFF, getCutoffInternal());
}

const rgbColor& LightEntityItem::getColor() const {
    assertUnlocked();
    lockForRead();
    auto& result = getColorInternal();
    unlock();
    return result;
}

const rgbColor& LightEntityItem::getColorInternal() const {
    assertLocked();
    return _color;
}

xColor LightEntityItem::getXColor() const {
    assertUnlocked();
    lockForRead();
    auto result = getXColorInternal();
    unlock();
    return result;
}

xColor LightEntityItem::getXColorInternal() const {
    assertLocked();
    xColor color = { _color[RED_INDEX], _color[GREEN_INDEX], _color[BLUE_INDEX] };
    return color;
}

void LightEntityItem::setColor(const rgbColor& value) {
    assertUnlocked();
    lockForWrite();
    setColorInternal(value);
    unlock();
}

void LightEntityItem::setColorInternal(const rgbColor& value) {
    assertWriteLocked();
    memcpy(_color, value, sizeof(_color));
}

void LightEntityItem::setColor(const xColor& value) {
    assertUnlocked();
    lockForWrite();
    setColorInternal(value);
    unlock();
}

void LightEntityItem::setColorInternal(const xColor& value) {
    assertWriteLocked();
    _color[RED_INDEX] = value.red;
    _color[GREEN_INDEX] = value.green;
    _color[BLUE_INDEX] = value.blue;
}

bool LightEntityItem::getIsSpotlight() const {
    assertUnlocked();
    lockForRead();
    auto result = getIsSpotlightInternal();
    unlock();
    return result;
}

bool LightEntityItem::getIsSpotlightInternal() const {
    assertLocked();
    return _isSpotlight;
}

void LightEntityItem::setIsSpotlight(bool value) {
    assertUnlocked();
    lockForWrite();
    setIsSpotlightInternal(value);
    unlock();
}

void LightEntityItem::setIsSpotlightInternal(bool value) {
    assertWriteLocked();
    if (value != _isSpotlight) {
        _isSpotlight = value;

        if (_isSpotlight) {
            const float length = getDimensions().z;
            const float width = length * glm::sin(glm::radians(_cutoff));
            setDimensions(glm::vec3(width, width, length));
        } else {
            float maxDimension = glm::max(getDimensions().x, getDimensions().y, getDimensions().z);
            setDimensions(glm::vec3(maxDimension, maxDimension, maxDimension));
        }
    }
}

float LightEntityItem::getIntensity() const {
    assertUnlocked();
    lockForRead();
    auto result = getIntensityInternal();
    unlock();
    return result;
}

float LightEntityItem::getIntensityInternal() const {
    assertLocked();
    return _intensity;
}

void LightEntityItem::setIntensity(float value) {
    assertUnlocked();
    lockForWrite();
    setIntensityInternal(value);
    unlock();
}

void LightEntityItem::setIntensityInternal(float value) {
    assertWriteLocked();
    _intensity = value;
}

float LightEntityItem::getExponent() const {
    assertUnlocked();
    lockForRead();
    auto result = getExponentInternal();
    unlock();
    return result;
}

float LightEntityItem::getExponentInternal() const {
    assertLocked();
    return _exponent;
}

void LightEntityItem::setExponent(float value) {
    assertUnlocked();
    lockForWrite();
    setExponentInternal(value);
    unlock();
}

void LightEntityItem::setExponentInternal(float value) {
    assertWriteLocked();
    _exponent = value;
}

float LightEntityItem::getCutoff() const {
    assertUnlocked();
    lockForRead();
    auto result = getCutoffInternal();
    unlock();
    return result;
}

float LightEntityItem::getCutoffInternal() const {
    assertLocked();
    return _cutoff;
}

void LightEntityItem::setCutoff(float value) {
    assertUnlocked();
    lockForWrite();
    setCutoffInternal(value);
    unlock();
}

void LightEntityItem::setCutoffInternal(float value) {
    assertWriteLocked();
    _cutoff = glm::clamp(value, 0.0f, 90.0f);

    if (_isSpotlight) {
        // If we are a spotlight, adjusting the cutoff will affect the area we encapsulate,
        // so update the dimensions to reflect this.
        const float length = getDimensions().z;
        const float width = length * glm::sin(glm::radians(_cutoff));
        setDimensions(glm::vec3(width, width, length));
    }
}
