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
    virtual bool needsToCallUpdateInternal() const;

    void updateShapeType(ShapeType type);
    virtual ShapeType getShapeTypeInternal() const;

    // TODO: Move these to subclasses, or other appropriate abstraction
    // getters/setters applicable to models and particles

    const rgbColor& getColor() const { return _color; }
    xColor getXColor() const;
    xColor getXColorInternal() const;
    bool hasModel() const;
    bool hasModelInternal() const;
    virtual bool hasCompoundShapeURL() const;
    virtual bool hasCompoundShapeURLInternal() const;

    static const QString DEFAULT_MODEL_URL;
    QString getModelURL() const;
    QString getModelURLInternal() const;

    static const QString DEFAULT_COMPOUND_SHAPE_URL;
    QString getCompoundShapeURL() const;
    QString getCompoundShapeURLInternal() const;

    bool hasAnimation() const;
    bool hasAnimationInternal() const;
    static const QString DEFAULT_ANIMATION_URL;
    QString getAnimationURL() const;
    QString getAnimationURLInternal() const;

    void setColor(const rgbColor& value);
    void setColorInternal(const rgbColor& value);
    void setColor(const xColor& value);
    void setColorInternal(const xColor& value);

    // model related properties
    void setModelURL(const QString& url);
    void setModelURLInternal(const QString& url);

    virtual void setCompoundShapeURL(const QString& url);
    virtual void setCompoundShapeURLInternal(const QString& url);
    void setAnimationURL(const QString& url);
    void setAnimationURLInternal(const QString& url);
    static const float DEFAULT_ANIMATION_FRAME_INDEX;
    void setAnimationFrameIndex(float value);
    void setAnimationFrameIndexInternal(float value);
    void setAnimationSettings(const QString& value);
    void setAnimationSettingsInternal(const QString& value);

    static const bool DEFAULT_ANIMATION_IS_PLAYING;
    void setAnimationIsPlaying(bool value);
    void setAnimationIsPlayingInternal(bool value);

    static const float DEFAULT_ANIMATION_FPS;
    void setAnimationFPS(float value);
    void setAnimationFPSInternal(float value);

    void setAnimationLoop(bool loop);
    void setAnimationLoopInternal(bool loop);
    bool getAnimationLoop() const;
    bool getAnimationLoopInternal() const;

    void setAnimationHold(bool hold);
    void setAnimationHoldInternal(bool hold);
    bool getAnimationHold() const;
    bool getAnimationHoldInternal() const;

    void setAnimationStartAutomatically(bool startAutomatically);
    void setAnimationStartAutomaticallyInternal(bool startAutomatically);
    bool getAnimationStartAutomatically() const;
    bool getAnimationStartAutomaticallyInternal() const;

    void setAnimationFirstFrame(float firstFrame);
    void setAnimationFirstFrameInternal(float firstFrame);
    float getAnimationFirstFrame() const;
    float getAnimationFirstFrameInternal() const;

    void setAnimationLastFrame(float lastFrame);
    void setAnimationLastFrameInternal(float lastFrame);
    float getAnimationLastFrame() const;
    float getAnimationLastFrameInternal() const;

    void mapJoints(const QStringList& modelJointNames);
    QVector<glm::quat> getAnimationFrame();
    bool jointsMapped() const;

    bool getAnimationIsPlaying() const;
    bool getAnimationIsPlayingInternal() const;
    float getAnimationFrameIndex() const;
    float getAnimationFrameIndexInternal() const;
    float getAnimationFPS() const;
    float getAnimationFPSInternal() const;
    QString getAnimationSettings() const;
    QString getAnimationSettingsInternal() const;

    static const QString DEFAULT_TEXTURES;
    QString getTextures() const;
    QString getTexturesInternal() const;
    void setTextures(const QString& textures);
    void setTexturesInternal(const QString& textures);

    virtual bool shouldBePhysical() const;

    static void cleanupLoadedAnimations();

protected:

    virtual void debugDump() const;
    bool isAnimatingSomething() const;

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
