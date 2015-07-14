//
//  ModelEntityItem.h
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ModelEntityItem_h
#define hifi_ModelEntityItem_h

#include <AnimationLoop.h>

#include "EntityItem.h" 

class ModelEntityItem : public EntityItem {
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    ModelEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties);

    ALLOW_INSTANTIATION // This class can be instantiated

    // methods for getting/setting all properties of an entity
    virtual EntityItemProperties getProperties(bool doLocking = true) const;
    virtual bool setProperties(const EntityItemProperties& properties, bool doLocking = true);

    // TODO: eventually only include properties changed since the params.lastViewFrustumSent time
    virtual EntityPropertyFlags getEntityProperties(EncodeBitstreamParams& params) const;

    virtual void appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                    EntityTreeElementExtraEncodeData* entityTreeElementExtraEncodeData,
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

    void updateShapeType(ShapeType type);

    // TODO: Move these to subclasses, or other appropriate abstraction
    // getters/setters applicable to models and particles

    const rgbColor& getColor() const { return _color; }
    xColor getXColor() const;
    bool hasModel() const;
    virtual bool hasCompoundShapeURL() const;

    static const QString DEFAULT_MODEL_URL;
    QString getModelURL() const;

    static const QString DEFAULT_COMPOUND_SHAPE_URL;
    QString getCompoundShapeURL() const;

    bool hasAnimation() const;
    static const QString DEFAULT_ANIMATION_URL;
    QString getAnimationURL() const;

    void setColor(const rgbColor& value);
    void setColor(const xColor& value);

    // model related properties
    void setModelURL(const QString& url);

    virtual void setCompoundShapeURL(const QString& url);
    void setAnimationURL(const QString& url);
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

    void mapJoints(const QStringList& modelJointNames);
    QVector<glm::quat> getAnimationFrame();
    bool jointsMapped() const;

    bool getAnimationIsPlaying() const;
    float getAnimationFrameIndex() const;
    float getAnimationFPS() const;
    QString getAnimationSettings() const;

    static const QString DEFAULT_TEXTURES;
    QString getTextures() const;
    void setTextures(const QString& textures);

    virtual bool shouldBePhysical() const;

    static void cleanupLoadedAnimations();

protected:

    virtual void debugDump() const;
    bool isAnimatingSomething() const;
    virtual bool needsToCallUpdateInternal() const;
    virtual ShapeType getShapeTypeInternal() const;
    xColor getXColorInternal() const;
    bool hasModelInternal() const;
    virtual bool hasCompoundShapeURLInternal() const;
    QString getModelURLInternal() const;
    QString getCompoundShapeURLInternal() const;
    bool hasAnimationInternal() const;
    QString getAnimationURLInternal() const;
    void setColorInternal(const rgbColor& value);
    void setColorInternal(const xColor& value);
    void setModelURLInternal(const QString& url);
    virtual void setCompoundShapeURLInternal(const QString& url);
    void setAnimationURLInternal(const QString& url);
    void setAnimationFrameIndexInternal(float value);
    void setAnimationSettingsInternal(const QString& value);
    void setAnimationIsPlayingInternal(bool value);
    void setAnimationFPSInternal(float value);
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
    bool getAnimationIsPlayingInternal() const;
    float getAnimationFrameIndexInternal() const;
    float getAnimationFPSInternal() const;
    QString getAnimationSettingsInternal() const;
    QString getTexturesInternal() const;
    void setTexturesInternal(const QString& textures);

    rgbColor _color;
    QString _modelURL;
    QString _compoundShapeURL;

    quint64 _lastAnimated;
    QString _animationURL;
    AnimationLoop _animationLoop;
    QString _animationSettings;
    QString _textures;
    ShapeType _shapeType = SHAPE_TYPE_NONE;

    // used on client side
    bool _jointMappingCompleted;
    QVector<int> _jointMapping;

    static Animation* getAnimation(const QString& url);
    static QMap<QString, AnimationPointer> _loadedAnimations;
    static AnimationCache _animationCache;

};

#endif // hifi_ModelEntityItem_h
