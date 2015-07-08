//
//  ModelEntityItem.cpp
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QJsonDocument>

#include <ByteCountCoding.h>
#include <GLMHelpers.h>

#include "EntityTree.h"
#include "EntityTreeElement.h"
#include "EntitiesLogging.h"
#include "ResourceCache.h"
#include "ModelEntityItem.h"

const QString ModelEntityItem::DEFAULT_MODEL_URL = QString("");
const QString ModelEntityItem::DEFAULT_COMPOUND_SHAPE_URL = QString("");
const QString ModelEntityItem::DEFAULT_ANIMATION_URL = QString("");
const float ModelEntityItem::DEFAULT_ANIMATION_FRAME_INDEX = 0.0f;
const bool ModelEntityItem::DEFAULT_ANIMATION_IS_PLAYING = false;
const float ModelEntityItem::DEFAULT_ANIMATION_FPS = 30.0f;


EntityItemPointer ModelEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return EntityItemPointer(new ModelEntityItem(entityID, properties));
}

ModelEntityItem::ModelEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        EntityItem(entityItemID, properties)
{
    _type = EntityTypes::Model;
    setProperties(properties);
    _lastAnimated = usecTimestampNow();
    _jointMappingCompleted = false;
    _color[0] = _color[1] = _color[2] = 0;
}

EntityItemProperties ModelEntityItem::getProperties(bool doLocking) const {
    if (doLocking) {
        assertUnlocked();
        lockForRead();
    } else {
        assertLocked();
    }
    EntityItemProperties properties = EntityItem::getProperties(false); // get the properties from our base class

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(color, getXColorInternal);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(modelURL, getModelURLInternal);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(compoundShapeURL, getCompoundShapeURLInternal);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(animationURL, getAnimationURLInternal);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(animationIsPlaying, getAnimationIsPlaying);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(animationFrameIndex, getAnimationFrameIndex);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(animationFPS, getAnimationFPS);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(glowLevel, getGlowLevelInternal);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(textures, getTextures);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(animationSettings, getAnimationSettings);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(shapeType, getShapeTypeInternal);

    if (doLocking) {
        unlock();
    }

    return properties;
}

bool ModelEntityItem::setProperties(const EntityItemProperties& properties, bool doLocking) {
    assertUnlocked();
    lockForWrite();
    bool somethingChanged = false;
    somethingChanged = EntityItem::setProperties(properties, false); // set the properties in our base class

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(color, setColorInternal);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(modelURL, setModelURLInternal);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(compoundShapeURL, setCompoundShapeURL);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(animationURL, setAnimationURL);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(animationIsPlaying, setAnimationIsPlaying);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(animationFrameIndex, setAnimationFrameIndex);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(animationFPS, setAnimationFPS);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(textures, setTextures);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(animationSettings, setAnimationSettings);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(shapeType, updateShapeType);

    if (somethingChanged) {
        bool wantDebug = false;
        if (wantDebug) {
            uint64_t now = usecTimestampNow();
            int elapsed = now - getLastEdited();
            qCDebug(entities) << "ModelEntityItem::setProperties() AFTER update... edited AGO=" << elapsed <<
                    "now=" << now << " getLastEdited()=" << getLastEdited();
        }
        setLastEditedInternal(properties._lastEdited);
    }

    unlock();
    return somethingChanged;
}

int ModelEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                      ReadBitstreamToTreeParams& args,
                                                      EntityPropertyFlags& propertyFlags, bool overwriteLocalData) {
    assertWriteLocked();
    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_COLOR, rgbColor, setColor);
    READ_ENTITY_PROPERTY(PROP_MODEL_URL, QString, setModelURLInternal);
    if (args.bitstreamVersion < VERSION_ENTITIES_HAS_COLLISION_MODEL) {
        setCompoundShapeURL("");
    } else if (args.bitstreamVersion == VERSION_ENTITIES_HAS_COLLISION_MODEL) {
        READ_ENTITY_PROPERTY(PROP_COMPOUND_SHAPE_URL, QString, setCompoundShapeURL);
    } else {
        READ_ENTITY_PROPERTY(PROP_COMPOUND_SHAPE_URL, QString, setCompoundShapeURL);
    }
    READ_ENTITY_PROPERTY(PROP_ANIMATION_URL, QString, setAnimationURL);

    // Because we're using AnimationLoop which will reset the frame index if you change it's running state
    // we want to read these values in the order they appear in the buffer, but call our setters in an
    // order that allows AnimationLoop to preserve the correct frame rate.
    float animationFPS = getAnimationFPS();
    float animationFrameIndex = getAnimationFrameIndex();
    bool animationIsPlaying = getAnimationIsPlaying();
    READ_ENTITY_PROPERTY(PROP_ANIMATION_FPS, float, setAnimationFPS);
    READ_ENTITY_PROPERTY(PROP_ANIMATION_FRAME_INDEX, float, setAnimationFrameIndex);
    READ_ENTITY_PROPERTY(PROP_ANIMATION_PLAYING, bool, setAnimationIsPlaying);

    if (propertyFlags.getHasProperty(PROP_ANIMATION_PLAYING)) {
        if (animationIsPlaying != getAnimationIsPlaying()) {
            setAnimationIsPlaying(animationIsPlaying);
        }
    }
    if (propertyFlags.getHasProperty(PROP_ANIMATION_FPS)) {
        setAnimationFPS(animationFPS);
    }
    if (propertyFlags.getHasProperty(PROP_ANIMATION_FRAME_INDEX)) {
        setAnimationFrameIndex(animationFrameIndex);
    }

    READ_ENTITY_PROPERTY(PROP_TEXTURES, QString, setTextures);
    READ_ENTITY_PROPERTY(PROP_ANIMATION_SETTINGS, QString, setAnimationSettings);
    READ_ENTITY_PROPERTY(PROP_SHAPE_TYPE, ShapeType, updateShapeType);

    return bytesRead;
}

// TODO: eventually only include properties changed since the params.lastViewFrustumSent time
EntityPropertyFlags ModelEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);

    requestedProperties += PROP_MODEL_URL;
    requestedProperties += PROP_COMPOUND_SHAPE_URL;
    requestedProperties += PROP_ANIMATION_URL;
    requestedProperties += PROP_ANIMATION_FPS;
    requestedProperties += PROP_ANIMATION_FRAME_INDEX;
    requestedProperties += PROP_ANIMATION_PLAYING;
    requestedProperties += PROP_ANIMATION_SETTINGS;
    requestedProperties += PROP_TEXTURES;
    requestedProperties += PROP_SHAPE_TYPE;

    return requestedProperties;
}


void ModelEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                EntityTreeElementExtraEncodeData* entityTreeElementExtraEncodeData,
                                EntityPropertyFlags& requestedProperties,
                                EntityPropertyFlags& propertyFlags,
                                EntityPropertyFlags& propertiesDidntFit,
                                int& propertyCount, OctreeElement::AppendState& appendState) const {
    assertLocked();
    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_COLOR, getColor());
    APPEND_ENTITY_PROPERTY(PROP_MODEL_URL, getModelURLInternal());
    APPEND_ENTITY_PROPERTY(PROP_COMPOUND_SHAPE_URL, getCompoundShapeURLInternal());
    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_URL, getAnimationURLInternal());
    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FPS, getAnimationFPS());
    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FRAME_INDEX, getAnimationFrameIndex());
    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_PLAYING, getAnimationIsPlaying());
    APPEND_ENTITY_PROPERTY(PROP_TEXTURES, getTextures());
    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_SETTINGS, getAnimationSettings());
    APPEND_ENTITY_PROPERTY(PROP_SHAPE_TYPE, (uint32_t)getShapeTypeInternal());
}


// TODO: improve cleanup by leveraging the AnimationPointer(s)
QMap<QString, AnimationPointer> ModelEntityItem::_loadedAnimations;

void ModelEntityItem::cleanupLoadedAnimations() {
    foreach(AnimationPointer animation, _loadedAnimations) {
        animation.clear();
    }
    _loadedAnimations.clear();
}

Animation* ModelEntityItem::getAnimation(const QString& url) {
    AnimationPointer animation;

    // if we don't already have this model then create it and initialize it
    if (_loadedAnimations.find(url) == _loadedAnimations.end()) {
        animation = DependencyManager::get<AnimationCache>()->getAnimation(url);
        _loadedAnimations[url] = animation;
    } else {
        animation = _loadedAnimations[url];
    }
    return animation.data();
}

void ModelEntityItem::mapJoints(const QStringList& modelJointNames) {
    // if we don't have animation, or we're already joint mapped then bail early
    if (!hasAnimation() || _jointMappingCompleted) {
        return;
    }

    Animation* myAnimation = getAnimation(_animationURL);

    if (!_jointMappingCompleted) {
        QStringList animationJointNames = myAnimation->getJointNames();

        if (modelJointNames.size() > 0 && animationJointNames.size() > 0) {
            _jointMapping.resize(modelJointNames.size());
            for (int i = 0; i < modelJointNames.size(); i++) {
                _jointMapping[i] = animationJointNames.indexOf(modelJointNames[i]);
            }
            _jointMappingCompleted = true;
        }
    }
}

QVector<glm::quat> ModelEntityItem::getAnimationFrame() {
    QVector<glm::quat> frameData;
    if (hasAnimation() && _jointMappingCompleted) {
        Animation* myAnimation = getAnimation(_animationURL);
        QVector<FBXAnimationFrame> frames = myAnimation->getFrames();
        int frameCount = frames.size();
        if (frameCount > 0) {
            int animationFrameIndex = (int)(glm::floor(getAnimationFrameIndex())) % frameCount;
            if (animationFrameIndex < 0 || animationFrameIndex > frameCount) {
                animationFrameIndex = 0;
            }

            QVector<glm::quat> rotations = frames[animationFrameIndex].rotations;

            frameData.resize(_jointMapping.size());
            for (int j = 0; j < _jointMapping.size(); j++) {
                int rotationIndex = _jointMapping[j];
                if (rotationIndex != -1 && rotationIndex < rotations.size()) {
                    frameData[j] = rotations[rotationIndex];
                }
            }
        }
    }
    return frameData;
}

bool ModelEntityItem::isAnimatingSomething() const {
    return getAnimationIsPlaying() &&
            getAnimationFPS() != 0.0f &&
            !getAnimationURL().isEmpty();
}

bool ModelEntityItem::needsToCallUpdate() const {
    return isAnimatingSomething() ?  true : EntityItem::needsToCallUpdate();
}

void ModelEntityItem::update(const quint64& now, bool doLocking) {
    if (doLocking) {
        assertUnlocked();
        lockForRead();
    } else {
        assertLocked();
    }

    // only advance the frame index if we're playing
    if (getAnimationIsPlaying()) {
        float deltaTime = (float)(now - _lastAnimated) / (float)USECS_PER_SECOND;
        _lastAnimated = now;
        _animationLoop.simulate(deltaTime);
    } else {
        _lastAnimated = now;
    }

    EntityItem::update(now, false); // let our base class handle it's updates...

    if (doLocking) {
        unlock();
    }
}

void ModelEntityItem::debugDump() const {
    qCDebug(entities) << "ModelEntityItem id:" << getEntityItemID();
    qCDebug(entities) << "    edited ago:" << getEditedAgo();
    qCDebug(entities) << "    position:" << getPosition();
    qCDebug(entities) << "    dimensions:" << getDimensions();
    qCDebug(entities) << "    model URL:" << getModelURL();
    qCDebug(entities) << "    compound shape URL:" << getCompoundShapeURL();
}

void ModelEntityItem::updateShapeType(ShapeType type) {
    assertWriteLocked();
    // BEGIN_TEMPORARY_WORKAROUND
    // we have allowed inconsistent ShapeType's to be stored in SVO files in the past (this was a bug)
    // but we are now enforcing the entity properties to be consistent.  To make the possible we're
    // introducing a temporary workaround: we will ignore ShapeType updates that conflict with the
    // _compoundShapeURL.
    if (hasCompoundShapeURLInternal()) {
        type = SHAPE_TYPE_COMPOUND;
    }
    // END_TEMPORARY_WORKAROUND

    if (type != _shapeType) {
        _shapeType = type;
        _dirtyFlags |= EntityItem::DIRTY_SHAPE | EntityItem::DIRTY_MASS;
    }
}

// virtual
ShapeType ModelEntityItem::getShapeTypeInternal() const {
    assertLocked();
    if (_shapeType == SHAPE_TYPE_COMPOUND) {
        return hasCompoundShapeURLInternal() ? SHAPE_TYPE_COMPOUND : SHAPE_TYPE_NONE;
    } else {
        auto result = _shapeType;
        return result;
    }
}


xColor ModelEntityItem::getXColor() const {
    assertUnlocked();
    lockForRead();
    auto result = getXColorInternal();
    unlock();
    return result;
}

xColor ModelEntityItem::getXColorInternal() const {
    assertLocked();
    xColor color = { _color[RED_INDEX], _color[GREEN_INDEX], _color[BLUE_INDEX] };
    return color;
}

bool ModelEntityItem::hasModel() const {
    assertUnlocked();
    lockForRead();
    auto result = hasModelInternal();
    unlock();
    return result;
}

bool ModelEntityItem::hasModelInternal() const {
    assertLocked();
    return !_modelURL.isEmpty();
}


bool ModelEntityItem::hasCompoundShapeURL() const {
    assertUnlocked();
    lockForRead();
    auto result = hasCompoundShapeURLInternal();
    unlock();
    return result;
}

bool ModelEntityItem::hasCompoundShapeURLInternal() const {
    assertLocked();
    return !_compoundShapeURL.isEmpty();
}

QString ModelEntityItem::getModelURL() const {
    assertUnlocked();
    lockForRead();
    auto result = getModelURLInternal();
    unlock();
    return result;
}

QString ModelEntityItem::getModelURLInternal() const {
    assertLocked();
    return _modelURL;
}

QString ModelEntityItem::getCompoundShapeURL() const {
    assertUnlocked();
    lockForRead();
    auto result = getCompoundShapeURLInternal();
    unlock();
    return result;
}

QString ModelEntityItem::getCompoundShapeURLInternal() const {
    assertLocked();
    return _compoundShapeURL;
}

bool ModelEntityItem::hasAnimation() const {
    assertUnlocked();
    lockForRead();
    auto result = hasAnimationInternal();
    unlock();
    return result;
}

bool ModelEntityItem::hasAnimationInternal() const {
    assertLocked();
    return !_animationURL.isEmpty();
}

QString ModelEntityItem::getAnimationURL() const {
    assertUnlocked();
    lockForRead();
    auto result = getAnimationURLInternal();
    unlock();
    return result;
}

QString ModelEntityItem::getAnimationURLInternal() const {
    assertLocked();
    return _animationURL;
}

void ModelEntityItem::setColor(const rgbColor& value) {
    assertUnlocked();
    lockForWrite();
    setColorInternal(value);
    unlock();
}

void ModelEntityItem::setColorInternal(const rgbColor& value) {
    assertWriteLocked();
    memcpy(_color, value, sizeof(_color));
}

void ModelEntityItem::setColor(const xColor& value) {
    assertUnlocked();
    lockForWrite();
    setColorInternal(value);
    unlock();
}

void ModelEntityItem::setColorInternal(const xColor& value) {
    assertWriteLocked();
    _color[RED_INDEX] = value.red;
    _color[GREEN_INDEX] = value.green;
    _color[BLUE_INDEX] = value.blue;
}

void ModelEntityItem::setModelURL(const QString& url) {
    assertUnlocked();
    lockForWrite();
    setModelURLInternal(url);
    unlock();
}

void ModelEntityItem::setModelURLInternal(const QString& url) {
    assertWriteLocked();
    _modelURL = url;
}

void ModelEntityItem::setCompoundShapeURL(const QString& url) {
    if (_compoundShapeURL != url) {
        _compoundShapeURL = url;
        _dirtyFlags |= EntityItem::DIRTY_SHAPE | EntityItem::DIRTY_MASS;
        _shapeType = _compoundShapeURL.isEmpty() ? SHAPE_TYPE_NONE : SHAPE_TYPE_COMPOUND;
    }
}

void ModelEntityItem::setAnimationURL(const QString& url) {
    _dirtyFlags |= EntityItem::DIRTY_UPDATEABLE;
    _animationURL = url;
}

void ModelEntityItem::setAnimationFrameIndex(float value) {
#ifdef WANT_DEBUG
    if (isAnimatingSomething()) {
        qCDebug(entities) << "ModelEntityItem::setAnimationFrameIndex()";
        qCDebug(entities) << "    value:" << value;
        qCDebug(entities) << "    was:" << _animationLoop.getFrameIndex();
        qCDebug(entities) << "    model URL:" << getModelURL();
        qCDebug(entities) << "    animation URL:" << getAnimationURL();
    }
#endif
    _animationLoop.setFrameIndex(value);
}

void ModelEntityItem::setAnimationSettings(const QString& value) {
    // the animations setting is a JSON string that may contain various animation settings.
    // if it includes fps, frameIndex, or running, those values will be parsed out and
    // will over ride the regular animation settings

    QJsonDocument settingsAsJson = QJsonDocument::fromJson(value.toUtf8());
    QJsonObject settingsAsJsonObject = settingsAsJson.object();
    QVariantMap settingsMap = settingsAsJsonObject.toVariantMap();
    if (settingsMap.contains("fps")) {
        float fps = settingsMap["fps"].toFloat();
        setAnimationFPS(fps);
    }

    if (settingsMap.contains("frameIndex")) {
        float frameIndex = settingsMap["frameIndex"].toFloat();
#ifdef WANT_DEBUG
        if (isAnimatingSomething()) {
            qCDebug(entities) << "ModelEntityItem::setAnimationSettings() calling setAnimationFrameIndex()...";
            qCDebug(entities) << "    model URL:" << getModelURL();
            qCDebug(entities) << "    animation URL:" << getAnimationURL();
            qCDebug(entities) << "    settings:" << value;
            qCDebug(entities) << "    settingsMap[frameIndex]:" << settingsMap["frameIndex"];
            qCDebug(entities"    frameIndex: %20.5f", frameIndex);
        }
#endif

        setAnimationFrameIndex(frameIndex);
    }

    if (settingsMap.contains("running")) {
        bool running = settingsMap["running"].toBool();
        if (running != getAnimationIsPlaying()) {
            setAnimationIsPlaying(running);
        }
    }

    if (settingsMap.contains("firstFrame")) {
        float firstFrame = settingsMap["firstFrame"].toFloat();
        setAnimationFirstFrame(firstFrame);
    }

    if (settingsMap.contains("lastFrame")) {
        float lastFrame = settingsMap["lastFrame"].toFloat();
        setAnimationLastFrame(lastFrame);
    }

    if (settingsMap.contains("loop")) {
        bool loop = settingsMap["loop"].toBool();
        setAnimationLoop(loop);
    }

    if (settingsMap.contains("hold")) {
        bool hold = settingsMap["hold"].toBool();
        setAnimationHold(hold);
    }

    if (settingsMap.contains("startAutomatically")) {
        bool startAutomatically = settingsMap["startAutomatically"].toBool();
        setAnimationStartAutomatically(startAutomatically);
    }

    _animationSettings = value;
    _dirtyFlags |= EntityItem::DIRTY_UPDATEABLE;
}

void ModelEntityItem::setAnimationIsPlaying(bool value) {
    _dirtyFlags |= EntityItem::DIRTY_UPDATEABLE;
    _animationLoop.setRunning(value);
}

void ModelEntityItem::setAnimationFPS(float value) {
    _dirtyFlags |= EntityItem::DIRTY_UPDATEABLE;
    _animationLoop.setFPS(value);
}

QString ModelEntityItem::getAnimationSettings() const {
    // the animations setting is a JSON string that may contain various animation settings.
    // if it includes fps, frameIndex, or running, those values will be parsed out and
    // will over ride the regular animation settings
    QString value = _animationSettings;

    QJsonDocument settingsAsJson = QJsonDocument::fromJson(value.toUtf8());
    QJsonObject settingsAsJsonObject = settingsAsJson.object();
    QVariantMap settingsMap = settingsAsJsonObject.toVariantMap();

    QVariant fpsValue(getAnimationFPS());
    settingsMap["fps"] = fpsValue;

    QVariant frameIndexValue(getAnimationFrameIndex());
    settingsMap["frameIndex"] = frameIndexValue;

    QVariant runningValue(getAnimationIsPlaying());
    settingsMap["running"] = runningValue;

    QVariant firstFrameValue(getAnimationFirstFrame());
    settingsMap["firstFrame"] = firstFrameValue;

    QVariant lastFrameValue(getAnimationLastFrame());
    settingsMap["lastFrame"] = lastFrameValue;

    QVariant loopValue(getAnimationLoop());
    settingsMap["loop"] = loopValue;

    QVariant holdValue(getAnimationHold());
    settingsMap["hold"] = holdValue;

    QVariant startAutomaticallyValue(getAnimationStartAutomatically());
    settingsMap["startAutomatically"] = startAutomaticallyValue;

    settingsAsJsonObject = QJsonObject::fromVariantMap(settingsMap);
    QJsonDocument newDocument(settingsAsJsonObject);
    QByteArray jsonByteArray = newDocument.toJson(QJsonDocument::Compact);
    QString jsonByteString(jsonByteArray);
    return jsonByteString;
}

// virtual
bool ModelEntityItem::shouldBePhysical() const {
    return EntityItem::shouldBePhysical() && getShapeType() != SHAPE_TYPE_NONE;
}


