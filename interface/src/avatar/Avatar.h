//
//  Avatar.h
//  interface/src/avatar
//
//  Copyright 2012 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Avatar_h
#define hifi_Avatar_h

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <QtCore/QMutex>
#include <QtCore/QScopedPointer>
#include <QtCore/QUuid>

#include <AvatarData.h>
#include <ShapeInfo.h>

#include <render/Scene.h>

#include "Hand.h"
#include "Head.h"
#include "SkeletonModel.h"
#include "world.h"
#include "ZoneEntityItem.h"

namespace render {
    template <> const ItemKey payloadGetKey(const AvatarSharedPointer& avatar);
    template <> const Item::Bound payloadGetBound(const AvatarSharedPointer& avatar);
    template <> void payloadRender(const AvatarSharedPointer& avatar, RenderArgs* args);
}

static const float SCALING_RATIO = .05f;
static const float SMOOTHING_RATIO = .05f; // 0 < ratio < 1
static const float RESCALING_TOLERANCE = .02f;

extern const float CHAT_MESSAGE_SCALE;
extern const float CHAT_MESSAGE_HEIGHT;

enum DriveKeys {
    FWD = 0,
    BACK,
    LEFT,
    RIGHT,
    UP,
    DOWN,
    ROT_LEFT,
    ROT_RIGHT,
    ROT_UP,
    ROT_DOWN,
    BOOM_IN,
    BOOM_OUT,
    MAX_DRIVE_KEYS
};

enum ScreenTintLayer {
    SCREEN_TINT_BEFORE_LANDSCAPE = 0,
    SCREEN_TINT_BEFORE_AVATARS,
    SCREEN_TINT_BEFORE_MY_AVATAR,
    SCREEN_TINT_AFTER_AVATARS,
    NUM_SCREEN_TINT_LAYERS
};

class AvatarMotionState;
class Texture;

class Avatar : public AvatarData {
    Q_OBJECT
    Q_PROPERTY(glm::vec3 skeletonOffset READ getSkeletonOffset WRITE setSkeletonOffset)

public:
    Avatar(RigPointer rig = nullptr);
    ~Avatar();

    typedef render::Payload<AvatarData> Payload;
    typedef std::shared_ptr<render::Item::PayloadInterface> PayloadPointer;

    void init();
    void simulate(float deltaTime);

    virtual void render(RenderArgs* renderArgs, const glm::vec3& cameraPosition);

    bool addToScene(AvatarSharedPointer self, std::shared_ptr<render::Scene> scene,
                            render::PendingChanges& pendingChanges);

    void removeFromScene(AvatarSharedPointer self, std::shared_ptr<render::Scene> scene,
                                render::PendingChanges& pendingChanges);

    //setters
    void setDisplayingLookatVectors(bool displayingLookatVectors) { getHead()->setRenderLookatVectors(displayingLookatVectors); }
    void setDisplayingLookatTarget(bool displayingLookatTarget) { getHead()->setRenderLookatTarget(displayingLookatTarget); }
    void setIsLookAtTarget(const bool isLookAtTarget) { _isLookAtTarget = isLookAtTarget; }
    bool getIsLookAtTarget() const { return _isLookAtTarget; }
    //getters
    bool isInitialized() const { return _initialized; }
    SkeletonModel& getSkeletonModel() { return _skeletonModel; }
    const SkeletonModel& getSkeletonModel() const { return _skeletonModel; }
    const QVector<Model*>& getAttachmentModels() const { return _attachmentModels; }
    glm::vec3 getChestPosition() const;
    float getScale() const { return _scale; }
    const Head* getHead() const { return static_cast<const Head*>(_headData); }
    Head* getHead() { return static_cast<Head*>(_headData); }
    Hand* getHand() { return static_cast<Hand*>(_handData); }
    glm::quat getWorldAlignedOrientation() const;

    AABox getBounds() const;

    /// Returns the distance to use as a LOD parameter.
    float getLODDistance() const;

    virtual bool isMyAvatar() const { return false; }
    
    virtual QVector<glm::quat> getJointRotations() const;
    virtual glm::quat getJointRotation(int index) const;
    virtual int getJointIndex(const QString& name) const;
    virtual QStringList getJointNames() const;
    
    virtual void setFaceModelURL(const QUrl& faceModelURL);
    virtual void setSkeletonModelURL(const QUrl& skeletonModelURL);
    virtual void setAttachmentData(const QVector<AttachmentData>& attachmentData);
    virtual void setBillboard(const QByteArray& billboard);

    void setShowDisplayName(bool showDisplayName);
    
    virtual int parseDataFromBuffer(const QByteArray& buffer);

    static void renderJointConnectingCone( gpu::Batch& batch, glm::vec3 position1, glm::vec3 position2,
                                                float radius1, float radius2, const glm::vec4& color);

    virtual void applyCollision(const glm::vec3& contactPoint, const glm::vec3& penetration) { }

    virtual const glm::vec3& getAbsolutePosition() const;
    virtual const glm::quat& getAbsoluteOrientation() const;

    Q_INVOKABLE void setSkeletonOffset(const glm::vec3& offset);
    Q_INVOKABLE glm::vec3 getSkeletonOffset() { return _skeletonOffset; }
    virtual glm::vec3 getSkeletonPosition() const;
    virtual const glm::vec3& getAbsoluteSkeletonPosition() const;
    
    Q_INVOKABLE glm::vec3 getJointPosition(int index) const;
    Q_INVOKABLE glm::vec3 getJointPosition(const QString& name) const;
    Q_INVOKABLE glm::quat getJointCombinedRotation(int index) const;
    Q_INVOKABLE glm::quat getJointCombinedRotation(const QString& name) const;
    
    Q_INVOKABLE void setJointModelPositionAndOrientation(int index, const glm::vec3 position, const glm::quat& rotation);
    Q_INVOKABLE void setJointModelPositionAndOrientation(const QString& name, const glm::vec3 position,
        const glm::quat& rotation);
    
    Q_INVOKABLE glm::vec3 getNeckPosition() const;

    Q_INVOKABLE glm::vec3 getAcceleration() const { return _acceleration; }
    Q_INVOKABLE glm::vec3 getAngularVelocity() const { return _angularVelocity; }
    Q_INVOKABLE glm::vec3 getAngularAcceleration() const { return _angularAcceleration; }

    /// Scales a world space position vector relative to the avatar position and scale
    /// \param vector position to be scaled. Will store the result
    void scaleVectorRelativeToPosition(glm::vec3 &positionToScale) const;

    void slamPosition(const glm::vec3& position);
    virtual void updateAttitude() { _skeletonModel.updateAttitude(); }

    // Call this when updating Avatar position with a delta.  This will allow us to
    // _accurately_ measure position changes and compute the resulting velocity
    // (otherwise floating point error will cause problems at large positions).
    void applyPositionDelta(const glm::vec3& delta);

    virtual void rebuildSkeletonBody();

    virtual void computeShapeInfo(ShapeInfo& shapeInfo);

    void setMotionState(AvatarMotionState* motionState) { _motionState = motionState; }
    AvatarMotionState* getMotionState() { return _motionState; }

protected:
    SkeletonModel _skeletonModel;
    glm::vec3 _skeletonOffset;
    QVector<Model*> _attachmentModels;
    QVector<Model*> _attachmentsToRemove;
    QVector<Model*> _unusedAttachments;
    float _bodyYawDelta;

    // These position histories and derivatives are in the local physics-frame.
    // The derivatives are the MEASURED results of all external and internal forces
    // and are therefore READ-ONLY --> motion control of the Avatar is NOT obtained
    // by setting these values.
    // Floating point error prevents us from accurately measuring velocity using a naive approach
    // (e.g. vel = (pos - lastPos)/dt) so instead we use _positionDeltaAccumulator.
    glm::vec3 _positionDeltaAccumulator;
    glm::vec3 _lastVelocity;
    glm::vec3 _acceleration;
    glm::vec3 _angularVelocity;
    glm::vec3 _lastAngularVelocity;
    glm::vec3 _angularAcceleration;
    glm::quat _lastOrientation;

    float _leanScale;
    float _scale;
    glm::vec3 _worldUpDirection;
    float _stringLength;
    bool _moving; ///< set when position is changing

    // protected methods...
    glm::vec3 getBodyRightDirection() const { return getLocalOrientation() * IDENTITY_RIGHT; }
    glm::vec3 getBodyUpDirection() const { return getLocalOrientation() * IDENTITY_UP; }
    glm::vec3 getBodyFrontDirection() const { return getLocalOrientation() * IDENTITY_FRONT; }
    glm::quat computeRotationFromBodyToWorldUp(float proportion = 1.0f) const;
    void setScale(float scale);
    void measureMotionDerivatives(float deltaTime);

    float getSkeletonHeight() const;
    float getHeadHeight() const;
    float getPelvisFloatingHeight() const;
    glm::vec3 getDisplayNamePosition() const;

    Transform calculateDisplayNameTransform(const ViewFrustum& frustum, float fontSize, const glm::ivec4& viewport) const;
    void renderDisplayName(gpu::Batch& batch, const ViewFrustum& frustum, const glm::ivec4& viewport) const;
    virtual void renderBody(RenderArgs* renderArgs, ViewFrustum* renderFrustum, float glowLevel = 0.0f);
    virtual bool shouldRenderHead(const RenderArgs* renderArgs) const;
    virtual void fixupModelsInScene();

    void simulateAttachments(float deltaTime);

    virtual void updateJointMappings();

    render::ItemID _renderItemID;

    mutable glm::vec3 _absoluteSkeletonPosition; // this is so Avatar::getAbsoluteSkeletonPosition can return by reference
    mutable glm::vec3 _absolutePosition; // this is so Avatar::getAbsolutePosition can return by reference
    mutable glm::quat _absoluteRotation; // this is so Avatar::getAbsoluteRotation can return by reference

private:
    bool _initialized;
    NetworkTexturePointer _billboardTexture;
    bool _shouldRenderBillboard;
    bool _isLookAtTarget;

    void renderBillboard(RenderArgs* renderArgs);

    float getBillboardSize() const;

    static int _jointConesID;

    int _voiceSphereID;

    AvatarMotionState* _motionState = nullptr;
};

#endif // hifi_Avatar_h
