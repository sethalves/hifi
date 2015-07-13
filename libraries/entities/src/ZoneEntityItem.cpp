//
//  ZoneEntityItem.cpp
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

#include "ZoneEntityItem.h"
#include "EntityTree.h"
#include "EntitiesLogging.h"
#include "EntityTreeElement.h"

bool ZoneEntityItem::_zonesArePickable = false;
bool ZoneEntityItem::_drawZoneBoundaries = false;

const xColor ZoneEntityItem::DEFAULT_KEYLIGHT_COLOR = { 255, 255, 255 };
const float ZoneEntityItem::DEFAULT_KEYLIGHT_INTENSITY = 1.0f;
const float ZoneEntityItem::DEFAULT_KEYLIGHT_AMBIENT_INTENSITY = 0.5f;
const glm::vec3 ZoneEntityItem::DEFAULT_KEYLIGHT_DIRECTION = { 0.0f, -1.0f, 0.0f };
const ShapeType ZoneEntityItem::DEFAULT_SHAPE_TYPE = SHAPE_TYPE_BOX;
const QString ZoneEntityItem::DEFAULT_COMPOUND_SHAPE_URL = "";

EntityItemPointer ZoneEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return EntityItemPointer(new ZoneEntityItem(entityID, properties));
}

ZoneEntityItem::ZoneEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
    EntityItem(entityItemID)
{
    _type = EntityTypes::Zone;
    _created = properties.getCreated();

    _keyLightColor[RED_INDEX] = DEFAULT_KEYLIGHT_COLOR.red;
    _keyLightColor[GREEN_INDEX] = DEFAULT_KEYLIGHT_COLOR.green;
    _keyLightColor[BLUE_INDEX] = DEFAULT_KEYLIGHT_COLOR.blue;

    _keyLightIntensity = DEFAULT_KEYLIGHT_INTENSITY;
    _keyLightAmbientIntensity = DEFAULT_KEYLIGHT_AMBIENT_INTENSITY;
    _keyLightDirection = DEFAULT_KEYLIGHT_DIRECTION;
    _shapeType = DEFAULT_SHAPE_TYPE;
    _compoundShapeURL = DEFAULT_COMPOUND_SHAPE_URL;

    _backgroundMode = BACKGROUND_MODE_INHERIT;

    setProperties(properties);
}


EnvironmentData ZoneEntityItem::getEnvironmentData() const {
    assertUnlocked();
    lockForRead();
    EnvironmentData result;

    result.setAtmosphereCenter(_atmosphereProperties.getCenter());
    result.setAtmosphereInnerRadius(_atmosphereProperties.getInnerRadius());
    result.setAtmosphereOuterRadius(_atmosphereProperties.getOuterRadius());
    result.setRayleighScattering(_atmosphereProperties.getRayleighScattering());
    result.setMieScattering(_atmosphereProperties.getMieScattering());
    result.setScatteringWavelengths(_atmosphereProperties.getScatteringWavelengths());
    result.setHasStars(_atmosphereProperties.getHasStars());

    // NOTE: The sunLocation and SunBrightness will be overwritten in the EntityTreeRenderer to use the
    // keyLight details from the scene interface
    //result.setSunLocation(1000, 900, 1000));
    //result.setSunBrightness(20.0f);

    unlock();
    return result;
}

EntityItemProperties ZoneEntityItem::getProperties(bool doLocking) const {
    if (doLocking) {
        assertUnlocked();
        lockForRead();
    } else {
        assertLocked();
    }
    EntityItemProperties properties = EntityItem::getProperties(false); // get the properties from our base class

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(keyLightColor, getKeyLightColorInternal);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(keyLightIntensity, getKeyLightIntensityInternal);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(keyLightAmbientIntensity, getKeyLightAmbientIntensityInternal);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(keyLightDirection, getKeyLightDirectionInternal);

    _stageProperties.getProperties(properties);

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(shapeType, getShapeTypeInternal);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(compoundShapeURL, getCompoundShapeURLInternal);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(backgroundMode, getBackgroundModeInternal);

    _atmosphereProperties.getProperties(properties);
    _skyboxProperties.getProperties(properties);

    if (doLocking) {
        unlock();
    }

    return properties;
}

bool ZoneEntityItem::setProperties(const EntityItemProperties& properties, bool doLocking) {
    if (doLocking) {
        assertUnlocked();
        lockForWrite();
    } else {
        assertWriteLocked();
    }

    bool somethingChanged = false;
    somethingChanged = EntityItem::setProperties(properties, false); // set the properties in our base class

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(keyLightColor, setKeyLightColorInternal);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(keyLightIntensity, setKeyLightIntensityInternal);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(keyLightAmbientIntensity, setKeyLightAmbientIntensityInternal);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(keyLightDirection, setKeyLightDirectionInternal);

    bool somethingChangedInStage = _stageProperties.setProperties(properties);

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(shapeType, updateShapeType);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(compoundShapeURL, setCompoundShapeURLInternal);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(backgroundMode, setBackgroundModeInternal);

    bool somethingChangedInAtmosphere = _atmosphereProperties.setProperties(properties);
    bool somethingChangedInSkybox = _skyboxProperties.setProperties(properties);

    somethingChanged = somethingChanged || somethingChangedInStage || somethingChangedInAtmosphere || somethingChangedInSkybox;

    if (somethingChanged) {
        bool wantDebug = false;
        if (wantDebug) {
            uint64_t now = usecTimestampNow();
            int elapsed = now - getLastEditedInternal();
            qCDebug(entities) << "ZoneEntityItem::setProperties() AFTER update... edited AGO=" << elapsed <<
                    "now=" << now << " getLastEdited()=" << getLastEditedInternal();
        }
        setLastEditedInternal(properties._lastEdited);
    }

    if (doLocking) {
        unlock();
    }

    return somethingChanged;
}

int ZoneEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                     ReadBitstreamToTreeParams& args,
                                                     EntityPropertyFlags& propertyFlags, bool overwriteLocalData) {
    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_KEYLIGHT_COLOR, rgbColor, setKeyLightColorInternal);
    READ_ENTITY_PROPERTY(PROP_KEYLIGHT_INTENSITY, float, setKeyLightIntensityInternal);
    READ_ENTITY_PROPERTY(PROP_KEYLIGHT_AMBIENT_INTENSITY, float, setKeyLightAmbientIntensityInternal);
    READ_ENTITY_PROPERTY(PROP_KEYLIGHT_DIRECTION, glm::vec3, setKeyLightDirectionInternal);

    int bytesFromStage = _stageProperties.readEntitySubclassDataFromBuffer(dataAt, (bytesLeftToRead - bytesRead), args,
                                                                           propertyFlags, overwriteLocalData);

    bytesRead += bytesFromStage;
    dataAt += bytesFromStage;

    READ_ENTITY_PROPERTY(PROP_SHAPE_TYPE, ShapeType, updateShapeType);
    READ_ENTITY_PROPERTY(PROP_COMPOUND_SHAPE_URL, QString, setCompoundShapeURLInternal);
    READ_ENTITY_PROPERTY(PROP_BACKGROUND_MODE, BackgroundMode, setBackgroundModeInternal);

    int bytesFromAtmosphere = _atmosphereProperties.readEntitySubclassDataFromBuffer(dataAt, (bytesLeftToRead - bytesRead),
                                                                                     args, propertyFlags, overwriteLocalData);

    bytesRead += bytesFromAtmosphere;
    dataAt += bytesFromAtmosphere;

    int bytesFromSkybox = _skyboxProperties.readEntitySubclassDataFromBuffer(dataAt, (bytesLeftToRead - bytesRead), args,
                                                                             propertyFlags, overwriteLocalData);
    bytesRead += bytesFromSkybox;
    dataAt += bytesFromSkybox;

    return bytesRead;
}


// TODO: eventually only include properties changed since the params.lastViewFrustumSent time
EntityPropertyFlags ZoneEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);

    requestedProperties += PROP_KEYLIGHT_COLOR;
    requestedProperties += PROP_KEYLIGHT_INTENSITY;
    requestedProperties += PROP_KEYLIGHT_AMBIENT_INTENSITY;
    requestedProperties += PROP_KEYLIGHT_DIRECTION;
    requestedProperties += PROP_SHAPE_TYPE;
    requestedProperties += PROP_COMPOUND_SHAPE_URL;
    requestedProperties += PROP_BACKGROUND_MODE;
    requestedProperties += _stageProperties.getEntityProperties(params);
    requestedProperties += _atmosphereProperties.getEntityProperties(params);
    requestedProperties += _skyboxProperties.getEntityProperties(params);

    return requestedProperties;
}

void ZoneEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                        EntityTreeElementExtraEncodeData* modelTreeElementExtraEncodeData,
                                        EntityPropertyFlags& requestedProperties,
                                        EntityPropertyFlags& propertyFlags,
                                        EntityPropertyFlags& propertiesDidntFit,
                                        int& propertyCount,
                                        OctreeElement::AppendState& appendState) const {

    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_KEYLIGHT_COLOR, _keyLightColor);
    APPEND_ENTITY_PROPERTY(PROP_KEYLIGHT_INTENSITY, getKeyLightIntensityInternal());
    APPEND_ENTITY_PROPERTY(PROP_KEYLIGHT_AMBIENT_INTENSITY, getKeyLightAmbientIntensityInternal());
    APPEND_ENTITY_PROPERTY(PROP_KEYLIGHT_DIRECTION, getKeyLightDirectionInternal());

    _stageProperties.appendSubclassData(packetData, params, modelTreeElementExtraEncodeData, requestedProperties,
                                        propertyFlags, propertiesDidntFit, propertyCount, appendState);

    APPEND_ENTITY_PROPERTY(PROP_SHAPE_TYPE, (uint32_t)getShapeTypeInternal());
    APPEND_ENTITY_PROPERTY(PROP_COMPOUND_SHAPE_URL, getCompoundShapeURLInternal());
    APPEND_ENTITY_PROPERTY(PROP_BACKGROUND_MODE, (uint32_t)getBackgroundModeInternal()); // could this be a uint16??

    _atmosphereProperties.appendSubclassData(packetData, params, modelTreeElementExtraEncodeData, requestedProperties,
                                             propertyFlags, propertiesDidntFit, propertyCount, appendState);

    _skyboxProperties.appendSubclassData(packetData, params, modelTreeElementExtraEncodeData, requestedProperties,
                                         propertyFlags, propertiesDidntFit, propertyCount, appendState);
}

void ZoneEntityItem::debugDump() const {
    quint64 now = usecTimestampNow();
    qCDebug(entities) << "   ZoneEntityItem id:" << getEntityItemID() << "---------------------------------------------";
    qCDebug(entities) << "             keyLightColor:"
                      << _keyLightColor[0] << "," << _keyLightColor[1] << "," << _keyLightColor[2];
    qCDebug(entities) << "                  position:" << debugTreeVector(getPosition());
    qCDebug(entities) << "                dimensions:" << debugTreeVector(getDimensions());
    qCDebug(entities) << "             getLastEdited:" << debugTime(getLastEdited(), now);
    qCDebug(entities) << "        _keyLightIntensity:" << _keyLightIntensity;
    qCDebug(entities) << " _keyLightAmbientIntensity:" << _keyLightAmbientIntensity;
    qCDebug(entities) << "        _keyLightDirection:" << _keyLightDirection;
    qCDebug(entities) << "               _backgroundMode:" << EntityItemProperties::getBackgroundModeString(_backgroundMode);

    _stageProperties.debugDump();
    _atmosphereProperties.debugDump();
    _skyboxProperties.debugDump();
}

ShapeType ZoneEntityItem::getShapeTypeInternal() const {
    assertLocked();
    // Zones are not allowed to have a SHAPE_TYPE_NONE... they are always at least a SHAPE_TYPE_BOX
    if (_shapeType == SHAPE_TYPE_COMPOUND) {
        return hasCompoundShapeURL() ? SHAPE_TYPE_COMPOUND : DEFAULT_SHAPE_TYPE;
    } else {
        return _shapeType == SHAPE_TYPE_NONE ? DEFAULT_SHAPE_TYPE : _shapeType;
    }
}

bool ZoneEntityItem::findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                         bool& keepSearching, OctreeElement*& element, float& distance, BoxFace& face,
                         void** intersectedObject, bool precisionPicking) const {
    assertUnlocked();
    lockForRead();
    auto result = _zonesArePickable;
    unlock();
    return result;
}

xColor ZoneEntityItem::getKeyLightColor() const {
    assertUnlocked();
    lockForRead();
    auto result = getKeyLightColorInternal();
    unlock();
    return result;
}

xColor ZoneEntityItem::getKeyLightColorInternal() const {
    assertLocked();
    xColor color = { _keyLightColor[RED_INDEX], _keyLightColor[GREEN_INDEX], _keyLightColor[BLUE_INDEX] };
    return color;
}

void ZoneEntityItem::setKeyLightColor(const xColor& value) {
    assertUnlocked();
    lockForWrite();
    setKeyLightColorInternal(value);
    unlock();
}

void ZoneEntityItem::setKeyLightColorInternal(const xColor& value) {
    assertWriteLocked();
    _keyLightColor[RED_INDEX] = value.red;
    _keyLightColor[GREEN_INDEX] = value.green;
    _keyLightColor[BLUE_INDEX] = value.blue;
}

void ZoneEntityItem::setKeyLightColor(const rgbColor& value) {
    assertUnlocked();
    lockForWrite();
    setKeyLightColorInternal(value);
    unlock();
}

void ZoneEntityItem::setKeyLightColorInternal(const rgbColor& value) {
    assertWriteLocked();
    _keyLightColor[RED_INDEX] = value[RED_INDEX];
    _keyLightColor[GREEN_INDEX] = value[GREEN_INDEX];
    _keyLightColor[BLUE_INDEX] = value[BLUE_INDEX];
}

glm::vec3 ZoneEntityItem::getKeyLightColorVec3() const {
    assertUnlocked();
    lockForRead();
    auto result = getKeyLightColorVec3Internal();
    unlock();
    return result;
}

glm::vec3 ZoneEntityItem::getKeyLightColorVec3Internal() const {
    assertLocked();
    const quint8 MAX_COLOR = 255;
    glm::vec3 color = { (float)_keyLightColor[RED_INDEX] / (float)MAX_COLOR,
                        (float)_keyLightColor[GREEN_INDEX] / (float)MAX_COLOR,
                        (float)_keyLightColor[BLUE_INDEX] / (float)MAX_COLOR };
    return color;
}

float ZoneEntityItem::getKeyLightIntensity() const {
    assertUnlocked();
    lockForRead();
    auto result = getKeyLightIntensityInternal();
    unlock();
    return result;
}

float ZoneEntityItem::getKeyLightIntensityInternal() const {
    assertLocked();
    return _keyLightIntensity;
}

void ZoneEntityItem::setKeyLightIntensity(float value) {
    assertUnlocked();
    lockForWrite();
    setKeyLightIntensityInternal(value);
    unlock();
}

void ZoneEntityItem::setKeyLightIntensityInternal(float value) {
    assertWriteLocked();
    _keyLightIntensity = value;
}

float ZoneEntityItem::getKeyLightAmbientIntensity() const {
    assertUnlocked();
    lockForRead();
    auto result = getKeyLightAmbientIntensityInternal();
    unlock();
    return result;
}

float ZoneEntityItem::getKeyLightAmbientIntensityInternal() const {
    assertLocked();
    return _keyLightAmbientIntensity;
}

void ZoneEntityItem::setKeyLightAmbientIntensity(float value) {
    assertUnlocked();
    lockForWrite();
    setKeyLightAmbientIntensityInternal(value);
    unlock();
}

void ZoneEntityItem::setKeyLightAmbientIntensityInternal(float value) {
    assertWriteLocked();
    _keyLightAmbientIntensity = value;
}

glm::vec3 ZoneEntityItem::getKeyLightDirection() const {
    assertUnlocked();
    lockForRead();
    auto result = getKeyLightDirectionInternal();
    unlock();
    return result;
}

glm::vec3 ZoneEntityItem::getKeyLightDirectionInternal() const {
    assertLocked();
    return _keyLightDirection;
}


void ZoneEntityItem::setKeyLightDirection(const glm::vec3& value) {
    assertUnlocked();
    lockForWrite();
    setKeyLightDirectionInternal(value);
    unlock();
}

void ZoneEntityItem::setKeyLightDirectionInternal(const glm::vec3& value) {
    assertWriteLocked();
    _keyLightDirection = value;
}

bool ZoneEntityItem::hasCompoundShapeURL() const {
    assertUnlocked();
    lockForRead();
    auto result = hasCompoundShapeURLInternal();
    unlock();
    return result;
}

bool ZoneEntityItem::hasCompoundShapeURLInternal() const {
    assertLocked();
    return !_compoundShapeURL.isEmpty();
}

QString ZoneEntityItem::getCompoundShapeURL() const {
    assertUnlocked();
    lockForRead();
    auto result = getCompoundShapeURLInternal();
    unlock();
    return result;
}

QString ZoneEntityItem::getCompoundShapeURLInternal() const {
    assertLocked();
    return _compoundShapeURL;
}

void ZoneEntityItem::setCompoundShapeURL(const QString& url) {
    assertUnlocked();
    lockForWrite();
    setCompoundShapeURLInternal(url);
    unlock();
}

void ZoneEntityItem::setCompoundShapeURLInternal(const QString& url) {
    assertWriteLocked();
    _compoundShapeURL = url;
    if (_compoundShapeURL.isEmpty() && _shapeType == SHAPE_TYPE_COMPOUND) {
        _shapeType = DEFAULT_SHAPE_TYPE;
    }
}

void ZoneEntityItem::setBackgroundMode(BackgroundMode value) {
    assertUnlocked();
    lockForWrite();
    setBackgroundModeInternal(value);
    unlock();
}

void ZoneEntityItem::setBackgroundModeInternal(BackgroundMode value) {
    assertWriteLocked();
    _backgroundMode = value;
}

BackgroundMode ZoneEntityItem::getBackgroundMode() const {
    assertUnlocked();
    lockForRead();
    auto result = getBackgroundModeInternal();
    unlock();
    return result;
}

BackgroundMode ZoneEntityItem::getBackgroundModeInternal() const {
    assertLocked();
    return _backgroundMode;
}

AtmospherePropertyGroup ZoneEntityItem::getAtmosphereProperties() const {
    assertUnlocked();
    lockForRead();
    auto result = getAtmospherePropertiesInternal();
    unlock();
    return result;
}

AtmospherePropertyGroup ZoneEntityItem::getAtmospherePropertiesInternal() const {
    assertLocked();
    return _atmosphereProperties;
}

SkyboxPropertyGroup ZoneEntityItem::getSkyboxProperties() const {
    assertUnlocked();
    lockForRead();
    auto result = getSkyboxPropertiesInternal();
    unlock();
    return result;
}

SkyboxPropertyGroup ZoneEntityItem::getSkyboxPropertiesInternal() const {
    assertLocked();
    return _skyboxProperties;
}

StagePropertyGroup ZoneEntityItem::getStageProperties() const {
    assertUnlocked();
    lockForRead();
    auto result = getStagePropertiesInternal();
    unlock();
    return result;
}

StagePropertyGroup ZoneEntityItem::getStagePropertiesInternal() const {
    assertLocked();
    return _stageProperties;
}
