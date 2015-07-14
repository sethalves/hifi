//
//  ParticleEffectEntityItem.h
//  libraries/entities/src
//
//  Created by Jason Rickwald on 3/2/15.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ParticleEffectEntityItem_h
#define hifi_ParticleEffectEntityItem_h

#include <AnimationLoop.h>
#include "EntityItem.h"

class ParticleEffectEntityItem : public EntityItem {
public:

    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    ParticleEffectEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties);
    virtual ~ParticleEffectEntityItem();

    ALLOW_INSTANTIATION // This class can be instantiated

    // methods for getting/setting all properties of this entity
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

    virtual void update(const quint64& now, bool doLocking = true);
    virtual bool needsToCallUpdate() const;

    const rgbColor& getColor() const;
    xColor getXColor() const;

    static const xColor DEFAULT_COLOR;
    void setColor(const rgbColor& value);
    void setColor(const xColor& value);

    void updateShapeType(ShapeType type);

    static const float DEFAULT_ANIMATION_FRAME_INDEX;
    void setAnimationFrameIndex(float value);
    void setAnimationSettings(const QString& value);

    static const bool DEFAULT_ANIMATION_IS_PLAYING;
    void setAnimationIsPlaying(bool value);

    static const float DEFAULT_ANIMATION_FPS;
    void setAnimationFPS(float value);

    void setAnimationLoop(bool loop);
    bool getAnimationLoop() const;

    void setAnimationHold(bool hold);
    bool getAnimationHold() const;

    void setAnimationStartAutomatically(bool startAutomatically);
    bool getAnimationStartAutomatically() const;

    void setAnimationFirstFrame(float firstFrame);
    float getAnimationFirstFrame() const;

    void setAnimationLastFrame(float lastFrame);
    float getAnimationLastFrame() const;

    static const quint32 DEFAULT_MAX_PARTICLES;
    void setMaxParticles(quint32 maxParticles);
    quint32 getMaxParticles() const;

    static const float DEFAULT_LIFESPAN;
    void setLifespan(float lifespan);
    float getLifespan() const;

    static const float DEFAULT_EMIT_RATE;
    void setEmitRate(float emitRate);
    float getEmitRate() const;

    static const glm::vec3 DEFAULT_EMIT_DIRECTION;
    void setEmitDirection(glm::vec3 emitDirection);
    glm::vec3 getEmitDirection() const;

    static const float DEFAULT_EMIT_STRENGTH;
    void setEmitStrength(float emitStrength);
    float getEmitStrength() const;

    static const float DEFAULT_LOCAL_GRAVITY;
    void setLocalGravity(float localGravity);
    float getLocalGravity() const;

    static const float DEFAULT_PARTICLE_RADIUS;
    void setParticleRadius(float particleRadius);
    float getParticleRadius() const;

    bool getAnimationIsPlaying() const;
    float getAnimationFrameIndex() const;
    float getAnimationFPS() const;
    QString getAnimationSettings() const;

    static const QString DEFAULT_TEXTURES;
    QString getTextures() const;
    void setTextures(const QString& textures);

protected:
    void setAnimationSettingsInternal(const QString& value);
    quint32 getLivingParticleCountInternal() const;
    void setMaxParticlesInternal(quint32 maxParticles);
    void extendBoundsInternal(const vec3& point);
    QString getAnimationSettingsInternal() const;
    void setAnimationFPSInternal(float value);
    void setAnimationIsPlayingInternal(bool value);
    bool isAnimatingSomethingInternal() const;
    void setAnimationFrameIndexInternal(float value);
    virtual bool needsToCallUpdateInternal() const;
    const rgbColor& getColorInternal() const;
    xColor getXColorInternal() const;
    void setColorInternal(const rgbColor& value);
    void setColorInternal(const xColor& value);
    virtual ShapeType getShapeTypeInternal() const { assertLocked(); return _shapeType; }
    void setAnimationLoopInternal(bool loop);
    bool getAnimationLoopInternal() const;
    void setAnimationHoldInternal(bool hold);
    bool getAnimationHoldInternal() const;
    void setAnimationStartAutomaticallyInternal(bool startAutomatically);
    bool getAnimationStartAutomaticallyInternal() const;
    void setAnimationFirstFrameInternal(float firstFrame);
    float getAnimationFirstFrameInternal() const;
    void setAnimationLastFrameInternal(float lastFrame);
    float getAnimationLastFrameInternal() const;
    quint32 getMaxParticlesInternal() const;
    void setLifespanInternal(float lifespan);
    float getLifespanInternal() const;
    void setEmitRateInternal(float emitRate);
    float getEmitRateInternal() const;
    void setEmitDirectionInternal(glm::vec3 emitDirection);
    glm::vec3 getEmitDirectionInternal() const;
    void setEmitStrengthInternal(float emitStrength);
    float getEmitStrengthInternal() const;
    void setLocalGravityInternal(float localGravity);
    float getLocalGravityInternal() const;
    void setParticleRadiusInternal(float particleRadius);
    float getParticleRadiusInternal() const;
    bool getAnimationIsPlayingInternal() const;
    float getAnimationFrameIndexInternal() const;
    float getAnimationFPSInternal() const;
    QString getTexturesInternal() const;
    void setTexturesInternal(const QString& textures);

    virtual void debugDump() const;
    bool isAnimatingSomething() const;
    void stepSimulation(float deltaTime);
    void extendBounds(const glm::vec3& point);
    void integrateParticle(quint32 index, float deltaTime);
    quint32 getLivingParticleCount() const;

    // the properties of this entity
    rgbColor _color;
    quint32 _maxParticles;
    float _lifespan;
    float _emitRate;
    glm::vec3 _emitDirection;
    float _emitStrength;
    float _localGravity;
    float _particleRadius;
    quint64 _lastAnimated;
    AnimationLoop _animationLoop;
    QString _animationSettings;
    QString _textures;
    bool _texturesChangedFlag;
    ShapeType _shapeType = SHAPE_TYPE_NONE;

    // all the internals of running the particle sim
    QVector<float> _particleLifetimes;
    QVector<glm::vec3> _particlePositions;
    QVector<glm::vec3> _particleVelocities;
    float _timeUntilNextEmit;

    // particle arrays are a ring buffer, use these indicies
    // to keep track of the living particles.
    quint32 _particleHeadIndex;
    quint32 _particleTailIndex;

    // bounding volume
    glm::vec3 _particleMaxBound;
    glm::vec3 _particleMinBound;
};

#endif // hifi_ParticleEffectEntityItem_h
