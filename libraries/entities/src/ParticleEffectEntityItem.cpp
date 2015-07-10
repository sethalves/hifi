//
//  ParticleEffectEntityItem.cpp
//  libraries/entities/src
//
//  Some starter code for a particle simulation entity, which could ideally be used for a variety of effects.
//  This is some really early and rough stuff here.  It was enough for now to just get it up and running in the interface.
//
//  Todo's and other notes:
//  - The simulation should restart when the AnimationLoop's max frame is reached (or passed), but there doesn't seem
//    to be a good way to set that max frame to something reasonable right now.
//  - There seems to be a bug whereby entities on the edge of screen will just pop off or on.  This is probably due
//    to my lack of understanding of how entities in the octree are picked for rendering.  I am updating the entity
//    dimensions based on the bounds of the sim, but maybe I need to update a dirty flag or something.
//  - This should support some kind of pre-roll of the simulation.
//  - Just to get this out the door, I just did forward Euler integration.  There are better ways.
//  - Gravity always points along the Y axis.  Support an actual gravity vector.
//  - Add the ability to add arbitrary forces to the simulation.
//  - Add controls for spread (which is currently hard-coded) and varying emission strength (not currently implemented).
//  - Add drag.
//  - Add some kind of support for collisions.
//  - There's no synchronization of the simulation across clients at all.  In fact, it's using rand() under the hood, so
//    there's no gaurantee that different clients will see simulations that look anything like the other.
//
//  Created by Jason Rickwald on 3/2/15.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include <glm/gtx/transform.hpp>
#include <QtCore/QJsonDocument>

#include <QDebug>

#include <ByteCountCoding.h>
#include <GeometryUtil.h>

#include "EntityTree.h"
#include "EntityTreeElement.h"
#include "EntitiesLogging.h"
#include "ParticleEffectEntityItem.h"

const xColor ParticleEffectEntityItem::DEFAULT_COLOR = { 255, 255, 255 };
const float ParticleEffectEntityItem::DEFAULT_ANIMATION_FRAME_INDEX = 0.0f;
const bool ParticleEffectEntityItem::DEFAULT_ANIMATION_IS_PLAYING = false;
const float ParticleEffectEntityItem::DEFAULT_ANIMATION_FPS = 30.0f;
const quint32 ParticleEffectEntityItem::DEFAULT_MAX_PARTICLES = 1000;
const float ParticleEffectEntityItem::DEFAULT_LIFESPAN = 3.0f;
const float ParticleEffectEntityItem::DEFAULT_EMIT_RATE = 15.0f;
const glm::vec3 ParticleEffectEntityItem::DEFAULT_EMIT_DIRECTION(0.0f, 1.0f, 0.0f);
const float ParticleEffectEntityItem::DEFAULT_EMIT_STRENGTH = 25.0f;
const float ParticleEffectEntityItem::DEFAULT_LOCAL_GRAVITY = -9.8f;
const float ParticleEffectEntityItem::DEFAULT_PARTICLE_RADIUS = 0.025f;
const QString ParticleEffectEntityItem::DEFAULT_TEXTURES = "";


EntityItemPointer ParticleEffectEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return EntityItemPointer(new ParticleEffectEntityItem(entityID, properties));
}

// our non-pure virtual subclass for now...
ParticleEffectEntityItem::ParticleEffectEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
    EntityItem(entityItemID, properties),
    _maxParticles(DEFAULT_MAX_PARTICLES),
    _lifespan(DEFAULT_LIFESPAN),
    _emitRate(DEFAULT_EMIT_RATE),
    _emitDirection(DEFAULT_EMIT_DIRECTION),
    _emitStrength(DEFAULT_EMIT_STRENGTH),
    _localGravity(DEFAULT_LOCAL_GRAVITY),
    _particleRadius(DEFAULT_PARTICLE_RADIUS),
    _lastAnimated(usecTimestampNow()),
    _animationLoop(),
    _animationSettings(),
    _textures(DEFAULT_TEXTURES),
    _texturesChangedFlag(false),
    _shapeType(SHAPE_TYPE_NONE),
    _particleLifetimes(DEFAULT_MAX_PARTICLES, 0.0f),
    _particlePositions(DEFAULT_MAX_PARTICLES, glm::vec3(0.0f, 0.0f, 0.0f)),
    _particleVelocities(DEFAULT_MAX_PARTICLES, glm::vec3(0.0f, 0.0f, 0.0f)),
    _timeUntilNextEmit(0.0f),
    _particleHeadIndex(0),
    _particleTailIndex(0),
    _particleMaxBound(glm::vec3(1.0f, 1.0f, 1.0f)),
    _particleMinBound(glm::vec3(-1.0f, -1.0f, -1.0f)) {

    _type = EntityTypes::ParticleEffect;
    setColor(DEFAULT_COLOR);
    setProperties(properties);
}

ParticleEffectEntityItem::~ParticleEffectEntityItem() {
}

EntityItemProperties ParticleEffectEntityItem::getProperties(bool doLocking) const {
    if (doLocking) {
        assertUnlocked();
        lockForRead();
    } else {
        assertLocked();
    }
    EntityItemProperties properties = EntityItem::getProperties(false); // get the properties from our base class

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(color, getXColorInternal);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(animationIsPlaying, getAnimationIsPlayingInternal);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(animationFrameIndex, getAnimationFrameIndexInternal);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(animationFPS, getAnimationFPSInternal);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(glowLevel, getGlowLevelInternal);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(animationSettings, getAnimationSettingsInternal);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(shapeType, getShapeTypeInternal);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(maxParticles, getMaxParticlesInternal);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(lifespan, getLifespanInternal);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(emitRate, getEmitRateInternal);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(emitDirection, getEmitDirectionInternal);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(emitStrength, getEmitStrengthInternal);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(localGravity, getLocalGravityInternal);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(particleRadius, getParticleRadiusInternal);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(textures, getTexturesInternal);

    if (doLocking) {
        unlock();
    }

    return properties;
}

bool ParticleEffectEntityItem::setProperties(const EntityItemProperties& properties, bool doLocking) {
    if (doLocking) {
        assertUnlocked();
        lockForWrite();
    } else {
        assertWriteLocked();
    }

    bool somethingChanged = EntityItem::setProperties(properties, false); // set the properties in our base class

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(color, setColorInternal);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(animationIsPlaying, setAnimationIsPlayingInternal);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(animationFrameIndex, setAnimationFrameIndexInternal);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(animationFPS, setAnimationFPSInternal);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(glowLevel, setGlowLevelInternal);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(animationSettings, setAnimationSettingsInternal);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(shapeType, updateShapeType);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(maxParticles, setMaxParticlesInternal);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(lifespan, setLifespanInternal);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(emitRate, setEmitRateInternal);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(emitDirection, setEmitDirectionInternal);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(emitStrength, setEmitStrengthInternal);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(localGravity, setLocalGravityInternal);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(particleRadius, setParticleRadiusInternal);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(textures, setTexturesInternal);

    if (somethingChanged) {
        bool wantDebug = false;
        if (wantDebug) {
            uint64_t now = usecTimestampNow();
            int elapsed = now - getLastEditedInternal();
            qCDebug(entities) << "ParticleEffectEntityItem::setProperties() AFTER update... edited AGO=" << elapsed <<
                "now=" << now << " getLastEdited()=" << getLastEditedInternal();
        }
        setLastEditedInternal(properties.getLastEdited());
    }

    if (doLocking) {
        unlock();
    }
    return somethingChanged;
}

int ParticleEffectEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                               ReadBitstreamToTreeParams& args,
                                                               EntityPropertyFlags& propertyFlags, bool overwriteLocalData) {
    assertWriteLocked();
    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_COLOR, rgbColor, setColorInternal);

    // Because we're using AnimationLoop which will reset the frame index if you change it's running state
    // we want to read these values in the order they appear in the buffer, but call our setters in an
    // order that allows AnimationLoop to preserve the correct frame rate.
    float animationFPS = getAnimationFPSInternal();
    float animationFrameIndex = getAnimationFrameIndexInternal();
    bool animationIsPlaying = getAnimationIsPlayingInternal();
    READ_ENTITY_PROPERTY(PROP_ANIMATION_FPS, float, setAnimationFPSInternal);
    READ_ENTITY_PROPERTY(PROP_ANIMATION_FRAME_INDEX, float, setAnimationFrameIndexInternal);
    READ_ENTITY_PROPERTY(PROP_ANIMATION_PLAYING, bool, setAnimationIsPlayingInternal);

    if (propertyFlags.getHasProperty(PROP_ANIMATION_PLAYING)) {
        if (animationIsPlaying != getAnimationIsPlayingInternal()) {
            setAnimationIsPlayingInternal(animationIsPlaying);
        }
    }
    if (propertyFlags.getHasProperty(PROP_ANIMATION_FPS)) {
        setAnimationFPSInternal(animationFPS);
    }
    if (propertyFlags.getHasProperty(PROP_ANIMATION_FRAME_INDEX)) {
        setAnimationFrameIndexInternal(animationFrameIndex);
    }

    READ_ENTITY_PROPERTY(PROP_ANIMATION_SETTINGS, QString, setAnimationSettingsInternal);
    READ_ENTITY_PROPERTY(PROP_SHAPE_TYPE, ShapeType, updateShapeType);
    READ_ENTITY_PROPERTY(PROP_MAX_PARTICLES, quint32, setMaxParticlesInternal);
    READ_ENTITY_PROPERTY(PROP_LIFESPAN, float, setLifespanInternal);
    READ_ENTITY_PROPERTY(PROP_EMIT_RATE, float, setEmitRateInternal);
    READ_ENTITY_PROPERTY(PROP_EMIT_DIRECTION, glm::vec3, setEmitDirectionInternal);
    READ_ENTITY_PROPERTY(PROP_EMIT_STRENGTH, float, setEmitStrengthInternal);
    READ_ENTITY_PROPERTY(PROP_LOCAL_GRAVITY, float, setLocalGravityInternal);
    READ_ENTITY_PROPERTY(PROP_PARTICLE_RADIUS, float, setParticleRadiusInternal);
    READ_ENTITY_PROPERTY(PROP_TEXTURES, QString, setTexturesInternal);

    unlock();
    return bytesRead;
}


// TODO: eventually only include properties changed since the params.lastViewFrustumSent time
EntityPropertyFlags ParticleEffectEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);

    requestedProperties += PROP_COLOR;
    requestedProperties += PROP_ANIMATION_FPS;
    requestedProperties += PROP_ANIMATION_FRAME_INDEX;
    requestedProperties += PROP_ANIMATION_PLAYING;
    requestedProperties += PROP_ANIMATION_SETTINGS;
    requestedProperties += PROP_SHAPE_TYPE;
    requestedProperties += PROP_MAX_PARTICLES;
    requestedProperties += PROP_LIFESPAN;
    requestedProperties += PROP_EMIT_RATE;
    requestedProperties += PROP_EMIT_DIRECTION;
    requestedProperties += PROP_EMIT_STRENGTH;
    requestedProperties += PROP_LOCAL_GRAVITY;
    requestedProperties += PROP_PARTICLE_RADIUS;
    requestedProperties += PROP_TEXTURES;

    return requestedProperties;
}

void ParticleEffectEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                                  EntityTreeElementExtraEncodeData* modelTreeElementExtraEncodeData,
                                                  EntityPropertyFlags& requestedProperties,
                                                  EntityPropertyFlags& propertyFlags,
                                                  EntityPropertyFlags& propertiesDidntFit,
                                                  int& propertyCount,
                                                  OctreeElement::AppendState& appendState) const {
    assertLocked();
    bool successPropertyFits = true;
    APPEND_ENTITY_PROPERTY(PROP_COLOR, getColorInternal());
    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FPS, getAnimationFPSInternal());
    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FRAME_INDEX, getAnimationFrameIndexInternal());
    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_PLAYING, getAnimationIsPlayingInternal());
    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_SETTINGS, getAnimationSettingsInternal());
    APPEND_ENTITY_PROPERTY(PROP_SHAPE_TYPE, (uint32_t)getShapeTypeInternal());
    APPEND_ENTITY_PROPERTY(PROP_MAX_PARTICLES, getMaxParticlesInternal());
    APPEND_ENTITY_PROPERTY(PROP_LIFESPAN, getLifespanInternal());
    APPEND_ENTITY_PROPERTY(PROP_EMIT_RATE, getEmitRateInternal());
    APPEND_ENTITY_PROPERTY(PROP_EMIT_DIRECTION, getEmitDirectionInternal());
    APPEND_ENTITY_PROPERTY(PROP_EMIT_STRENGTH, getEmitStrengthInternal());
    APPEND_ENTITY_PROPERTY(PROP_LOCAL_GRAVITY, getLocalGravityInternal());
    APPEND_ENTITY_PROPERTY(PROP_PARTICLE_RADIUS, getParticleRadiusInternal());
    APPEND_ENTITY_PROPERTY(PROP_TEXTURES, getTexturesInternal());
}

bool ParticleEffectEntityItem::isAnimatingSomething() const {
    assertUnlocked();
    lockForRead();
    auto result = isAnimatingSomethingInternal();
    unlock();
    return result;
}

bool ParticleEffectEntityItem::isAnimatingSomethingInternal() const {
    assertLocked();
    // keep animating if there are particles still alive.
    return (getAnimationIsPlayingInternal() || getLivingParticleCountInternal() > 0) && getAnimationFPSInternal() != 0.0f;
}

bool ParticleEffectEntityItem::needsToCallUpdate() const {
    assertUnlocked();
    lockForRead();
    auto result = needsToCallUpdateInternal();
    unlock();
    return result;
}

bool ParticleEffectEntityItem::needsToCallUpdateInternal() const {
    assertLocked();
    return isAnimatingSomethingInternal() ? true : EntityItem::needsToCallUpdateInternal();
}

void ParticleEffectEntityItem::update(const quint64& now, bool doLocking) {
    if (doLocking) {
        assertUnlocked();
        lockForRead();
    } else {
        assertLocked();
    }

    float deltaTime = (float)(now - _lastAnimated) / (float)USECS_PER_SECOND;
     _lastAnimated = now;

    // only advance the frame index if we're playing
    if (getAnimationIsPlayingInternal()) {
        _animationLoop.simulate(deltaTime);
    }

    if (isAnimatingSomethingInternal()) {
        stepSimulation(deltaTime);

        // update the dimensions
        glm::vec3 dims;
        dims.x = glm::max(glm::abs(_particleMinBound.x), glm::abs(_particleMaxBound.x)) * 2.0f;
        dims.y = glm::max(glm::abs(_particleMinBound.y), glm::abs(_particleMaxBound.y)) * 2.0f;
        dims.z = glm::max(glm::abs(_particleMinBound.z), glm::abs(_particleMaxBound.z)) * 2.0f;
        setDimensionsInternal(dims);
    }

    EntityItem::update(now, false); // let our base class handle it's updates...

    if (doLocking) {
        unlock();
    }
}

void ParticleEffectEntityItem::debugDump() const {
    assertLocked();
    quint64 now = usecTimestampNow();
    qCDebug(entities) << "PA EFFECT EntityItem id:" << getEntityItemID() << "---------------------------------------------";
    qCDebug(entities) << "                  color:" << _color[0] << "," << _color[1] << "," << _color[2];
    qCDebug(entities) << "               position:" << debugTreeVector(getPosition());
    qCDebug(entities) << "             dimensions:" << debugTreeVector(getDimensions());
    qCDebug(entities) << "          getLastEdited:" << debugTime(getLastEdited(), now);
}

void ParticleEffectEntityItem::updateShapeType(ShapeType type) {
    assertWriteLocked();
    if (type != _shapeType) {
        _shapeType = type;
        _dirtyFlags |= EntityItem::DIRTY_SHAPE | EntityItem::DIRTY_MASS;
    }
}

void ParticleEffectEntityItem::setAnimationFrameIndex(float value) {
    assertUnlocked();
    lockForWrite();
    setAnimationFrameIndexInternal(value);
    unlock();
}

void ParticleEffectEntityItem::setAnimationFrameIndexInternal(float value) {
    assertWriteLocked();
#ifdef WANT_DEBUG
    if (isAnimatingSomething()) {
        qCDebug(entities) << "ParticleEffectEntityItem::setAnimationFrameIndex()";
        qCDebug(entities) << "    value:" << value;
        qCDebug(entities) << "    was:" << _animationLoop.getFrameIndex();
    }
#endif
    _animationLoop.setFrameIndex(value);
}

void ParticleEffectEntityItem::setAnimationSettings(const QString& value) {
    assertUnlocked();
    lockForWrite();
    setAnimationSettingsInternal(value);
    unlock();
}

void ParticleEffectEntityItem::setAnimationSettingsInternal(const QString& value) {
    assertWriteLocked();
    // the animations setting is a JSON string that may contain various animation settings.
    // if it includes fps, frameIndex, or running, those values will be parsed out and
    // will over ride the regular animation settings

    QJsonDocument settingsAsJson = QJsonDocument::fromJson(value.toUtf8());
    QJsonObject settingsAsJsonObject = settingsAsJson.object();
    QVariantMap settingsMap = settingsAsJsonObject.toVariantMap();
    if (settingsMap.contains("fps")) {
        float fps = settingsMap["fps"].toFloat();
        setAnimationFPSInternal(fps);
    }

    if (settingsMap.contains("frameIndex")) {
        float frameIndex = settingsMap["frameIndex"].toFloat();
#ifdef WANT_DEBUG
        if (isAnimatingSomething()) {
            qCDebug(entities) << "ParticleEffectEntityItem::setAnimationSettings() calling setAnimationFrameIndex()...";
            qCDebug(entities) << "    settings:" << value;
            qCDebug(entities) << "    settingsMap[frameIndex]:" << settingsMap["frameIndex"];
            qCDebug(entities"    frameIndex: %20.5f", frameIndex);
        }
#endif

        setAnimationFrameIndexInternal(frameIndex);
    }

    if (settingsMap.contains("running")) {
        bool running = settingsMap["running"].toBool();
        if (running != getAnimationIsPlayingInternal()) {
            setAnimationIsPlayingInternal(running);
        }
    }

    if (settingsMap.contains("firstFrame")) {
        float firstFrame = settingsMap["firstFrame"].toFloat();
        setAnimationFirstFrameInternal(firstFrame);
    }

    if (settingsMap.contains("lastFrame")) {
        float lastFrame = settingsMap["lastFrame"].toFloat();
        setAnimationLastFrameInternal(lastFrame);
    }

    if (settingsMap.contains("loop")) {
        bool loop = settingsMap["loop"].toBool();
        setAnimationLoopInternal(loop);
    }

    if (settingsMap.contains("hold")) {
        bool hold = settingsMap["hold"].toBool();
        setAnimationHoldInternal(hold);
    }

    if (settingsMap.contains("startAutomatically")) {
        bool startAutomatically = settingsMap["startAutomatically"].toBool();
        setAnimationStartAutomaticallyInternal(startAutomatically);
    }

    _animationSettings = value;
    _dirtyFlags |= EntityItem::DIRTY_UPDATEABLE;
    unlock();
}

void ParticleEffectEntityItem::setAnimationIsPlaying(bool value) {
    assertUnlocked();
    lockForWrite();
    setAnimationIsPlayingInternal(value);
    unlock();
}

void ParticleEffectEntityItem::setAnimationIsPlayingInternal(bool value) {
    assertWriteLocked();
    _dirtyFlags |= EntityItem::DIRTY_UPDATEABLE;
    _animationLoop.setRunning(value);
}

void ParticleEffectEntityItem::setAnimationFPS(float value) {
    assertUnlocked();
    lockForWrite();
    setAnimationFPSInternal(value);
    unlock();
}

void ParticleEffectEntityItem::setAnimationFPSInternal(float value) {
    assertWriteLocked();
    _dirtyFlags |= EntityItem::DIRTY_UPDATEABLE;
    _animationLoop.setFPS(value);
}

QString ParticleEffectEntityItem::getAnimationSettings() const {
    assertUnlocked();
    lockForRead();
    auto result = getAnimationSettingsInternal();
    unlock();
    return result;
}

QString ParticleEffectEntityItem::getAnimationSettingsInternal() const {
    assertLocked();
    // the animations setting is a JSON string that may contain various animation settings.
    // if it includes fps, frameIndex, or running, those values will be parsed out and
    // will over ride the regular animation settings
    QString value = _animationSettings;

    QJsonDocument settingsAsJson = QJsonDocument::fromJson(value.toUtf8());
    QJsonObject settingsAsJsonObject = settingsAsJson.object();
    QVariantMap settingsMap = settingsAsJsonObject.toVariantMap();

    QVariant fpsValue(getAnimationFPSInternal());
    settingsMap["fps"] = fpsValue;

    QVariant frameIndexValue(getAnimationFrameIndexInternal());
    settingsMap["frameIndex"] = frameIndexValue;

    QVariant runningValue(getAnimationIsPlayingInternal());
    settingsMap["running"] = runningValue;

    QVariant firstFrameValue(getAnimationFirstFrameInternal());
    settingsMap["firstFrame"] = firstFrameValue;

    QVariant lastFrameValue(getAnimationLastFrameInternal());
    settingsMap["lastFrame"] = lastFrameValue;

    QVariant loopValue(getAnimationLoopInternal());
    settingsMap["loop"] = loopValue;

    QVariant holdValue(getAnimationHoldInternal());
    settingsMap["hold"] = holdValue;

    QVariant startAutomaticallyValue(getAnimationStartAutomaticallyInternal());
    settingsMap["startAutomatically"] = startAutomaticallyValue;

    settingsAsJsonObject = QJsonObject::fromVariantMap(settingsMap);
    QJsonDocument newDocument(settingsAsJsonObject);
    QByteArray jsonByteArray = newDocument.toJson(QJsonDocument::Compact);
    QString jsonByteString(jsonByteArray);
    return jsonByteString;
}

void ParticleEffectEntityItem::extendBounds(const glm::vec3& point) {
    assertUnlocked();
    lockForWrite();
    extendBoundsInternal(point);
    unlock();
}

void ParticleEffectEntityItem::extendBoundsInternal(const glm::vec3& point) {
    assertWriteLocked();
    _particleMinBound.x = glm::min(_particleMinBound.x, point.x);
    _particleMinBound.y = glm::min(_particleMinBound.y, point.y);
    _particleMinBound.z = glm::min(_particleMinBound.z, point.z);
    _particleMaxBound.x = glm::max(_particleMaxBound.x, point.x);
    _particleMaxBound.y = glm::max(_particleMaxBound.y, point.y);
    _particleMaxBound.z = glm::max(_particleMaxBound.z, point.z);
}

void ParticleEffectEntityItem::integrateParticle(quint32 index, float deltaTime) {
    assertWriteLocked();
    glm::vec3 atSquared(0.0f, 0.5f * _localGravity * deltaTime * deltaTime, 0.0f);
    glm::vec3 at(0.0f, _localGravity * deltaTime, 0.0f);
    _particlePositions[index] += _particleVelocities[index] * deltaTime + atSquared;
    _particleVelocities[index] += at;
}

void ParticleEffectEntityItem::stepSimulation(float deltaTime) {
    assertWriteLocked();
    _particleMinBound = glm::vec3(-1.0f, -1.0f, -1.0f);
    _particleMaxBound = glm::vec3(1.0f, 1.0f, 1.0f);

    // update particles between head and tail
    for (quint32 i = _particleHeadIndex; i != _particleTailIndex; i = (i + 1) % _maxParticles) {
        _particleLifetimes[i] -= deltaTime;

        // if particle has died.
        if (_particleLifetimes[i] <= 0.0f) {
            // move head forward
            _particleHeadIndex = (_particleHeadIndex + 1) % _maxParticles;
        }
        else {
            integrateParticle(i, deltaTime);
            extendBoundsInternal(_particlePositions[i]);
        }
    }

    // emit new particles, but only if animaiton is playing
    if (getAnimationIsPlayingInternal()) {

        float timeLeftInFrame = deltaTime;
        while (_timeUntilNextEmit < timeLeftInFrame) {

            timeLeftInFrame -= _timeUntilNextEmit;
            _timeUntilNextEmit = 1.0f / _emitRate;

            // emit a new particle at tail index.
            quint32 i = _particleTailIndex;
            _particleLifetimes[i] = _lifespan;

            // jitter the _emitDirection by a random offset
            glm::vec3 randOffset;
            randOffset.x = (randFloat() - 0.5f) * 0.25f * _emitStrength;
            randOffset.y = (randFloat() - 0.5f) * 0.25f * _emitStrength;
            randOffset.z = (randFloat() - 0.5f) * 0.25f * _emitStrength;

            // set initial conditions
            _particlePositions[i] = glm::vec3(0.0f, 0.0f, 0.0f);
            _particleVelocities[i] = _emitDirection * _emitStrength + randOffset;

            integrateParticle(i, timeLeftInFrame);
            extendBoundsInternal(_particlePositions[i]);

            _particleTailIndex = (_particleTailIndex + 1) % _maxParticles;

            // overflow! move head forward by one.
            // because the case of head == tail indicates an empty array, not a full one.
            // This can drop an existing older particle, but this is by design, newer particles are a higher priority.
            if (_particleTailIndex == _particleHeadIndex) {
                _particleHeadIndex = (_particleHeadIndex + 1) % _maxParticles;
            }
        }

        _timeUntilNextEmit -= timeLeftInFrame;
    }
}

void ParticleEffectEntityItem::setMaxParticles(quint32 maxParticles) {
    assertUnlocked();
    lockForWrite();
    setMaxParticlesInternal(maxParticles);
    unlock();
}

void ParticleEffectEntityItem::setMaxParticlesInternal(quint32 maxParticles) {
    assertWriteLocked();
    if (_maxParticles != maxParticles) {
        _maxParticles = maxParticles;

        // TODO: try to do something smart here and preserve the state of existing particles.

        // resize vectors
        _particleLifetimes.resize(_maxParticles);
        _particlePositions.resize(_maxParticles);
        _particleVelocities.resize(_maxParticles);

        // effectivly clear all particles and start emitting new ones from scratch.
        _particleHeadIndex = 0;
        _particleTailIndex = 0;
        _timeUntilNextEmit = 0.0f;
    }
}



quint32 ParticleEffectEntityItem::getMaxParticles() const {
    assertUnlocked();
    lockForRead();
    auto result = getMaxParticlesInternal();
    unlock();
    return result;
}

quint32 ParticleEffectEntityItem::getMaxParticlesInternal() const {
    assertLocked();
    return _maxParticles;
}

// because particles are in a ring buffer, this isn't trivial
quint32 ParticleEffectEntityItem::getLivingParticleCount() const {
    assertUnlocked();
    lockForRead();
    auto result = getLivingParticleCountInternal();
    unlock();
    return result;
}

quint32 ParticleEffectEntityItem::getLivingParticleCountInternal() const {
    if (_particleTailIndex >= _particleHeadIndex) {
        return _particleTailIndex - _particleHeadIndex;
    } else {
        return (_maxParticles - _particleHeadIndex) + _particleTailIndex;
    }
}

const rgbColor& ParticleEffectEntityItem::getColor() const {
    assertUnlocked();
    lockForRead();
    auto& result = getColorInternal();
    unlock();
    return result;
}

const rgbColor& ParticleEffectEntityItem::getColorInternal() const {
    assertLocked();
    return _color;
}

xColor ParticleEffectEntityItem::getXColor() const {
    assertUnlocked();
    lockForRead();
    auto result = getXColorInternal();
    unlock();
    return result;
}

xColor ParticleEffectEntityItem::getXColorInternal() const {
    assertLocked();
    xColor color = { _color[RED_INDEX], _color[GREEN_INDEX], _color[BLUE_INDEX] };
    return color;
}

void ParticleEffectEntityItem::setColor(const rgbColor& value) {
    assertUnlocked();
    lockForWrite();
    setColorInternal(value);
    unlock();
}

void ParticleEffectEntityItem::setColorInternal(const rgbColor& value) {
    assertWriteLocked();
    memcpy(_color, value, sizeof(_color));
}


void ParticleEffectEntityItem::setColor(const xColor& value) {
    assertUnlocked();
    lockForWrite();
    setColorInternal(value);
    unlock();
}

void ParticleEffectEntityItem::setColorInternal(const xColor& value) {
    assertWriteLocked();
    _color[RED_INDEX] = value.red;
    _color[GREEN_INDEX] = value.green;
    _color[BLUE_INDEX] = value.blue;
}

void ParticleEffectEntityItem::setAnimationLoop(bool loop) {
    assertUnlocked();
    lockForWrite();
    setAnimationLoopInternal(loop);
    unlock();
}

void ParticleEffectEntityItem::setAnimationLoopInternal(bool loop) {
    assertWriteLocked();
    _animationLoop.setLoop(loop);
}

bool ParticleEffectEntityItem::getAnimationLoop() const {
    assertUnlocked();
    lockForRead();
    auto result = getAnimationLoopInternal();
    unlock();
    return result;
}

bool ParticleEffectEntityItem::getAnimationLoopInternal() const {
    assertLocked();
    return _animationLoop.getLoop();
}

void ParticleEffectEntityItem::setAnimationHold(bool hold) {
    assertUnlocked();
    lockForWrite();
    setAnimationHoldInternal(hold);
    unlock();
}

void ParticleEffectEntityItem::setAnimationHoldInternal(bool hold) {
    assertWriteLocked();
    _animationLoop.setHold(hold);
}

bool ParticleEffectEntityItem::getAnimationHold() const {
    assertUnlocked();
    lockForRead();
    auto result = getAnimationHoldInternal();
    unlock();
    return result;
}

bool ParticleEffectEntityItem::getAnimationHoldInternal() const {
    assertLocked();
    return _animationLoop.getHold();
}

void ParticleEffectEntityItem::setAnimationStartAutomatically(bool startAutomatically) {
    assertUnlocked();
    lockForWrite();
    setAnimationStartAutomaticallyInternal(startAutomatically);
    unlock();
}

void ParticleEffectEntityItem::setAnimationStartAutomaticallyInternal(bool startAutomatically) {
    assertWriteLocked();
    _animationLoop.setStartAutomatically(startAutomatically);
}

bool ParticleEffectEntityItem::getAnimationStartAutomatically() const {
    assertUnlocked();
    lockForRead();
    auto result = getAnimationStartAutomaticallyInternal();
    unlock();
    return result;
}

bool ParticleEffectEntityItem::getAnimationStartAutomaticallyInternal() const {
    assertLocked();
    return _animationLoop.getStartAutomatically();
}

void ParticleEffectEntityItem::setAnimationFirstFrame(float firstFrame) {
    assertUnlocked();
    lockForWrite();
    setAnimationFirstFrameInternal(firstFrame);
    unlock();
}

void ParticleEffectEntityItem::setAnimationFirstFrameInternal(float firstFrame) {
    assertWriteLocked();
    _animationLoop.setFirstFrame(firstFrame);
}

float ParticleEffectEntityItem::getAnimationFirstFrame() const {
    assertUnlocked();
    lockForRead();
    auto result = getAnimationFirstFrameInternal();
    unlock();
    return result;
}

float ParticleEffectEntityItem::getAnimationFirstFrameInternal() const {
    assertLocked();
    return _animationLoop.getFirstFrame();
}

void ParticleEffectEntityItem::setAnimationLastFrame(float lastFrame) {
    assertUnlocked();
    lockForWrite();
    setAnimationLastFrameInternal(lastFrame);
    unlock();
}

void ParticleEffectEntityItem::setAnimationLastFrameInternal(float lastFrame) {
    assertWriteLocked();
    _animationLoop.setLastFrame(lastFrame);
}

float ParticleEffectEntityItem::getAnimationLastFrame() const {
    assertUnlocked();
    lockForRead();
    auto result = getAnimationLastFrameInternal();
    unlock();
    return result;
}

float ParticleEffectEntityItem::getAnimationLastFrameInternal() const {
    assertLocked();
    return _animationLoop.getLastFrame();
}

void ParticleEffectEntityItem::setLifespan(float lifespan) {
    assertUnlocked();
    lockForWrite();
    setLifespanInternal(lifespan);
    unlock();
}

void ParticleEffectEntityItem::setLifespanInternal(float lifespan) {
    assertWriteLocked();
    _lifespan = lifespan;
}

float ParticleEffectEntityItem::getLifespan() const {
    assertUnlocked();
    lockForRead();
    auto result = getLifespanInternal();
    unlock();
    return result;
}

float ParticleEffectEntityItem::getLifespanInternal() const {
    assertLocked();
    return _lifespan;
}

void ParticleEffectEntityItem::setEmitRate(float emitRate) {
    assertUnlocked();
    lockForWrite();
    setEmitRateInternal(emitRate);
    unlock();
}

void ParticleEffectEntityItem::setEmitRateInternal(float emitRate) {
    assertWriteLocked();
    _emitRate = emitRate;
}

float ParticleEffectEntityItem::getEmitRate() const {
    assertUnlocked();
    lockForRead();
    auto result = getEmitRateInternal();
    unlock();
    return result;
}

float ParticleEffectEntityItem::getEmitRateInternal() const {
    assertLocked();
    return _emitRate;
}

void ParticleEffectEntityItem::setEmitDirection(glm::vec3 emitDirection) {
    assertUnlocked();
    lockForWrite();
    setEmitDirectionInternal(emitDirection);
    unlock();
}

void ParticleEffectEntityItem::setEmitDirectionInternal(glm::vec3 emitDirection) {
    assertWriteLocked();
    _emitDirection = glm::normalize(emitDirection);
}

glm::vec3 ParticleEffectEntityItem::getEmitDirection() const {
    assertUnlocked();
    lockForRead();
    auto result = getEmitDirectionInternal();
    unlock();
    return result;
}

glm::vec3 ParticleEffectEntityItem::getEmitDirectionInternal() const {
    assertLocked();
    return _emitDirection;
}

void ParticleEffectEntityItem::setEmitStrength(float emitStrength) {
    assertUnlocked();
    lockForWrite();
    setEmitStrengthInternal(emitStrength);
    unlock();
}

void ParticleEffectEntityItem::setEmitStrengthInternal(float emitStrength) {
    assertWriteLocked();
    _emitStrength = emitStrength;
}

float ParticleEffectEntityItem::getEmitStrength() const {
    assertUnlocked();
    lockForRead();
    auto result = getEmitStrengthInternal();
    unlock();
    return result;
}

float ParticleEffectEntityItem::getEmitStrengthInternal() const {
    assertLocked();
    return _emitStrength;
}

void ParticleEffectEntityItem::setLocalGravity(float localGravity) {
    assertUnlocked();
    lockForWrite();
    setLocalGravityInternal(localGravity);
    unlock();
}

void ParticleEffectEntityItem::setLocalGravityInternal(float localGravity) {
    assertWriteLocked();
    _localGravity = localGravity;
}

float ParticleEffectEntityItem::getLocalGravity() const {
    assertUnlocked();
    lockForRead();
    auto result = getLocalGravityInternal();
    unlock();
    return result;
}

float ParticleEffectEntityItem::getLocalGravityInternal() const {
    assertLocked();
    return _localGravity;
}

void ParticleEffectEntityItem::setParticleRadius(float particleRadius) {
    assertUnlocked();
    lockForWrite();
    setParticleRadiusInternal(particleRadius);
    unlock();
}

void ParticleEffectEntityItem::setParticleRadiusInternal(float particleRadius) {
    assertWriteLocked();
    _particleRadius = particleRadius;
}

float ParticleEffectEntityItem::getParticleRadius() const {
    assertUnlocked();
    lockForRead();
    auto result = getParticleRadiusInternal();
    unlock();
    return result;
}

float ParticleEffectEntityItem::getParticleRadiusInternal() const {
    assertLocked();
    return _particleRadius;
}

bool ParticleEffectEntityItem::getAnimationIsPlaying() const {
    assertUnlocked();
    lockForRead();
    auto result = getAnimationIsPlayingInternal();
    unlock();
    return result;
}

bool ParticleEffectEntityItem::getAnimationIsPlayingInternal() const {
    assertLocked();
    return _animationLoop.isRunning();
}

float ParticleEffectEntityItem::getAnimationFrameIndex() const {
    assertUnlocked();
    lockForRead();
    auto result = getAnimationFrameIndexInternal();
    unlock();
    return result;
}

float ParticleEffectEntityItem::getAnimationFrameIndexInternal() const {
    assertLocked();
    return _animationLoop.getFrameIndex();
}

float ParticleEffectEntityItem::getAnimationFPS() const {
    assertUnlocked();
    lockForRead();
    auto result = getAnimationFPSInternal();
    unlock();
    return result;
}

float ParticleEffectEntityItem::getAnimationFPSInternal() const {
    assertLocked();
    return _animationLoop.getFPS();
}

QString ParticleEffectEntityItem::getTextures() const {
    assertUnlocked();
    lockForRead();
    auto result = getTexturesInternal();
    unlock();
    return result;
}

QString ParticleEffectEntityItem::getTexturesInternal() const {
    assertLocked();
    return _textures;
}

void ParticleEffectEntityItem::setTextures(const QString& textures) {
    assertUnlocked();
    lockForWrite();
    setTexturesInternal(textures);
    unlock();
}

void ParticleEffectEntityItem::setTexturesInternal(const QString& textures) {
    assertWriteLocked();
    if (_textures != textures) {
        _textures = textures;
        _texturesChangedFlag = true;
    }
}
