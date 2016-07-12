//
//  SpatiallyNestable.h
//  libraries/shared/src/
//
//  Created by Seth Alves on 2015-10-18
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SpatiallyNestable_h
#define hifi_SpatiallyNestable_h

#include <atomic>

#include <QUuid>

#include "shared/WriteLocklessRead.h"

#include "Transform.h"
#include "AACube.h"
#include "SpatialParentFinder.h"
#include "shared/ReadWriteLockable.h"


class SpatiallyNestable;
using SpatiallyNestableWeakPointer = std::weak_ptr<SpatiallyNestable>;
using SpatiallyNestableWeakConstPointer = std::weak_ptr<const SpatiallyNestable>;
using SpatiallyNestablePointer = std::shared_ptr<SpatiallyNestable>;
using SpatiallyNestableConstPointer = std::shared_ptr<const SpatiallyNestable>;

enum class SpatiallyNestableFlagBits {
    Entity = 0x01,
    Avatar = 0x02,
};

template <typename BitType, typename MaskType = uint32_t>
class Flags {
public:
    Flags() {}
    Flags(BitType bit) : _mask(static_cast<MaskType>(bit)) {}
    Flags(const Flags<BitType>& flags) : _mask(flags._mask) {}

    inline Flags<BitType> operator|(const Flags<BitType>& flags) const { 
        Flags<BitType> result(*this);
        result |= flags;
        return result;
    }

    inline Flags<BitType> operator|=(const Flags<BitType>& flags) {
        _mask |= flags._mask;
        return *this;
    }

    inline Flags<BitType> operator&(const Flags<BitType>& flags) const {
        Flags<BitType> result(*this);
        result &= flags;
        return result;
    }

    inline Flags<BitType> operator&=(const Flags<BitType>& flags) {
        _mask &= flags._mask;
        return *this;
    }

    inline Flags<BitType> operator^(const Flags<BitType>& flags) const {
        Flags<BitType> result(*this);
        result ^= flags;
        return result;
    }

    inline Flags<BitType> operator^=(const Flags<BitType>& flags) {
        _mask ^= flags._mask;
        return *this;
    }

    inline bool operator==(const Flags<BitType>& flags) const { return _mask == flags._mask; }

    inline bool operator!=(const Flags<BitType>& flags) const { return _mask != flags._mask; }

    inline bool isSet(BitType bit) const {
        MaskType mask = static_cast<MaskType>(bit);
        return (mask == (_mask & mask));
    }

    explicit operator bool() const { return !!_mask; }

private:
    MaskType _mask { 0 };
};

using SpatiallyNestableFlags = Flags<SpatiallyNestableFlagBits>;

class SpatiallyNestable : public std::enable_shared_from_this<SpatiallyNestable> {
public:
    SpatiallyNestable(const SpatiallyNestableFlags& flags, const QUuid& id)
        : _id(id), _flags(flags) {
        // Valid objects always have a non-zero stamp
        ++_stamp;
    }

    virtual ~SpatiallyNestable() { }

    const SpatiallyNestableFlags& getNestableFlags() const { return _flags; }

    virtual const QUuid& getID() const { return _id; }
    virtual void setID(const QUuid& id);

    virtual const QUuid getParentID() const;
    virtual void setParentID(const QUuid& parentID);

    virtual quint16 getParentJointIndex() const { quint16 result;  _locker.withConsistentRead([&](const Data& data) { result = data._parentJointIndex;  });  return result; }
    virtual void setParentJointIndex(quint16 parentJointIndex);

    static glm::vec3 worldToLocal(const glm::vec3& position, const QUuid& parentID, int parentJointIndex, bool& success);
    static glm::quat worldToLocal(const glm::quat& orientation, const QUuid& parentID, int parentJointIndex, bool& success);

    static glm::vec3 localToWorld(const glm::vec3& position, const QUuid& parentID, int parentJointIndex, bool& success);
    static glm::quat localToWorld(const glm::quat& orientation, const QUuid& parentID, int parentJointIndex, bool& success);

    // world frame
    virtual const Transform getTransform(bool& success) const;
    virtual void setTransform(const Transform& transform, bool& success);

    virtual Transform getParentTransform(bool& success) const;

    virtual glm::vec3 getPosition(bool& success) const;
    virtual glm::vec3 getPosition() const;
    virtual void setPosition(const glm::vec3& position, bool& success, bool tellPhysics = true);
    virtual void setPosition(const glm::vec3& position);

    virtual glm::quat getOrientation(bool& success) const;
    virtual glm::quat getOrientation() const;
    virtual glm::quat getOrientation(int jointIndex, bool& success) const;
    virtual void setOrientation(const glm::quat& orientation, bool& success, bool tellPhysics = true);
    virtual void setOrientation(const glm::quat& orientation);

    virtual glm::vec3 getVelocity(bool& success) const;
    virtual glm::vec3 getVelocity() const;
    virtual void setVelocity(const glm::vec3& velocity, bool& success);
    virtual void setVelocity(const glm::vec3& velocity);
    virtual glm::vec3 getParentVelocity(bool& success) const;

    virtual glm::vec3 getAngularVelocity(bool& success) const;
    virtual glm::vec3 getAngularVelocity() const;
    virtual void setAngularVelocity(const glm::vec3& angularVelocity, bool& success);
    virtual void setAngularVelocity(const glm::vec3& angularVelocity);
    virtual glm::vec3 getParentAngularVelocity(bool& success) const;

    virtual AACube getMaximumAACube(bool& success) const;
    virtual void checkAndAdjustQueryAACube();
    virtual bool computePuffedQueryAACube();

    virtual void setQueryAACube(const AACube& queryAACube);
    virtual bool queryAABoxNeedsUpdate() const;
    virtual AACube getQueryAACube(bool& success) const;
    virtual AACube getQueryAACube() const;

    virtual glm::vec3 getScale() const;
    virtual void setScale(const glm::vec3& scale);

    // get world-frame values for a specific joint
    virtual const Transform getTransform(int jointIndex, bool& success) const;
    virtual glm::vec3 getPosition(int jointIndex, bool& success) const;
    virtual glm::vec3 getScale(int jointIndex) const;

    // object's parent's frame
    virtual const Transform getLocalTransform() const;
    virtual void setLocalTransform(const Transform& transform);

    virtual glm::vec3 getLocalPosition() const;
    virtual void setLocalPosition(const glm::vec3& position, bool tellPhysics = true);

    virtual glm::quat getLocalOrientation() const;
    virtual void setLocalOrientation(const glm::quat& orientation);

    virtual glm::vec3 getLocalVelocity() const;
    virtual void setLocalVelocity(const glm::vec3& velocity);

    virtual glm::vec3 getLocalAngularVelocity() const;
    virtual void setLocalAngularVelocity(const glm::vec3& angularVelocity);

    virtual glm::vec3 getLocalScale() const;
    virtual void setLocalScale(const glm::vec3& scale);

    QList<SpatiallyNestablePointer> getChildren() const;
    bool hasChildren() const;

    // this object's frame
    virtual const Transform getAbsoluteJointTransformInObjectFrame(int jointIndex) const;
    virtual glm::quat getAbsoluteJointRotationInObjectFrame(int index) const = 0;
    virtual glm::vec3 getAbsoluteJointTranslationInObjectFrame(int index) const = 0;
    virtual bool setAbsoluteJointRotationInObjectFrame(int index, const glm::quat& rotation) = 0;
    virtual bool setAbsoluteJointTranslationInObjectFrame(int index, const glm::vec3& translation) = 0;

    SpatiallyNestablePointer getThisPointer() const;

    void markAncestorMissing(bool value) { _missingAncestor = value; }
    bool getAncestorMissing() { return _missingAncestor; }

    void forEachChild(std::function<void(SpatiallyNestablePointer)> actor);
    void forEachDescendant(std::function<void(SpatiallyNestablePointer)> actor);

    void die() { _locker.withWriteLock([](Data& data) { data._isDead = true;  }); }

    bool isDead() const { bool result;  _locker.withConsistentRead([&](const Data& data) { result = data._isDead;  });  return result; }

    bool isParentIDValid() const { bool success = false; getParentPointer(success); return success; }
    virtual SpatialParentTree* getParentTree() const { return nullptr; }

    bool hasAncestorOfType(const SpatiallyNestableFlags& flags) const;
    bool hasAncestorOfType(SpatiallyNestableFlagBits flag) const;

    void getLocalTransformAndVelocities(Transform& localTransform,
                                        glm::vec3& localVelocity,
                                        glm::vec3& localAngularVelocity) const;

    void setLocalTransformAndVelocities(
            const Transform& localTransform,
            const glm::vec3& localVelocity,
            const glm::vec3& localAngularVelocity);

protected:
    SpatiallyNestablePointer getParentPointer(bool& success) const;

    virtual void beParentOfChild(SpatiallyNestablePointer newChild) const;
    virtual void forgetChild(SpatiallyNestablePointer newChild) const;
    virtual void locationChanged(bool tellPhysics = true); // called when a this object's location has changed
    virtual void dimensionsChanged() { } // called when a this object's dimensions have changed

    // _queryAACube is used to decide where something lives in the octree
    mutable AACube _queryAACube;
    mutable bool _queryAACubeSet { false };

    bool _missingAncestor { false };
    const QUuid _id;

private:
    bool ensureParentClean(int depth) const;

    struct Data {
        mutable SpatiallyNestableWeakPointer _parent;
        mutable QHash<QUuid, SpatiallyNestableWeakPointer> _children;
        SpatiallyNestableFlags _parentFlags; // EntityItem or an AvatarData
        Transform _parentTransform; // this is to be combined with parent's world-transform to produce this' world-transform.
        glm::vec3 _parentVelocity;
        glm::vec3 _parentAngularVelocity;
        Transform _transform; // this is to be combined with parent's world-transform to produce this' world-transform.
        glm::vec3 _velocity;
        glm::vec3 _angularVelocity;
        QUuid _parentID; // what is this thing's transform relative to?
        quint16 _parentJointIndex { 0 }; // which joint of the parent is this relative to?
        mutable bool _parentKnowsMe { false };
        bool _isDead { false };
    };

    mutable WriteLocklessRead<Data> _locker;
    const SpatiallyNestableFlags _flags; // EntityItem or an AvatarData
    mutable std::atomic<uint64_t> _parentStamp;
    std::atomic<uint64_t> _stamp;
};


#endif // hifi_SpatiallyNestable_h
