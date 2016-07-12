//
//  SpatiallyNestable.cpp
//  libraries/shared/src/
//
//  Created by Seth Alves on 2015-10-18
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SpatiallyNestable.h"
#include <QQueue>

#include "DependencyManager.h"

const float defaultAACubeSize = 1.0f;
const int maxParentingChain = 30;

void SpatiallyNestable::setID(const QUuid& id) {
    _locker.withWriteLock([&](Data& data){
        const_cast<QUuid&>(_id) = id;
    });
}

const QUuid SpatiallyNestable::getParentID() const {
    QUuid result;
    _locker.withConsistentRead([&](const Data& data){
        result = data._parentID;
    });
    return result;
}

void SpatiallyNestable::setParentID(const QUuid& parentID) {
    _locker.withWriteLock([&](Data& data){
        data._parentID = parentID;
        data._parentKnowsMe = false;
        ++_stamp;
    });
}

bool SpatiallyNestable::ensureParentClean(int depth) const {
    bool result = false;

    if (depth > maxParentingChain) {
        // someone created a loop.  break it...
        qDebug() << "Parenting loop detected.";
        SpatiallyNestablePointer _this = getThisPointer();
        _this->setParentID(QUuid());
        bool setPositionSuccess;
        AACube aaCube = getQueryAACube(setPositionSuccess);
        if (setPositionSuccess) {
            _this->setPosition(aaCube.calcCenter());
        }
    }


    SpatiallyNestablePointer parent = getParentPointer(result);
    if (!result) {
        return false;
    }

    // If we don't have a parent, make sure our cached data is clear
    if (!parent) {
        // No parent, make sure our parent stamp is 0, and if not 
        // reset all the cached parent data
        if (_parentStamp.exchange(0)) {
            _locker.withWriteLock([&](Data& data) {
                data._parentTransform = Transform();
                data._parentAngularVelocity = vec3();
                data._parentVelocity = vec3();
                data._parentID = QUuid();
                data._parentJointIndex = -1;
                data._parentFlags = SpatiallyNestableFlags();
                ++const_cast<std::atomic<uint64_t>&>(_stamp);
            });
        }
        return true;
    }

    // Possibly out of date, update our parent info cache
    if (parent->_stamp.load() != _parentStamp.load()) {
        auto parentStampValue = parent->_stamp.load();

        parent->ensureParentClean(depth + 1);

        // Make sure we're not updating against the same parent stamp in multiple threads.
        if (parentStampValue != _parentStamp.exchange(parentStampValue)) {
            // Cache the parent information
            _locker.withWriteLock([&](Data& data) {
                data._parentTransform = parent->getTransform(data._parentJointIndex, result);
                data._parentTransform.setScale(1.0f); // TODO: scaling
                parent->_locker.withConsistentRead([&](const Data& parentData) {
                    data._parentAngularVelocity = parentData._angularVelocity;
                    data._parentVelocity = parentData._velocity;
                    data._parentFlags = parent->_flags | parentData._parentFlags;
                });
                ++const_cast<std::atomic<uint64_t>&>(_stamp);
            });
        }
    }
    return result;
}




Transform SpatiallyNestable::getParentTransform(bool& success) const {
    Transform result;
    success = ensureParentClean(0);
    if (!success) {
        return result;
    }

    _locker.withConsistentRead([&](const Data& data) {
        result = data._parentTransform;
    });

    return result;
}

SpatiallyNestablePointer SpatiallyNestable::getParentPointer(bool& success) const {
    SpatiallyNestablePointer parent;
    QUuid parentID;
    bool parentKnowsMe = false;
    _locker.withConsistentRead([&](const Data& data) {
        success = false;
        parent = data._parent.lock();
        parentID = data._parentID;

        if (!parent && parentID.isNull()) {
            // no parent
            success = true;
            return;
        }

        parentKnowsMe = data._parentKnowsMe;
    });

    // Parent is null and parentID is null, we're done, no parent;
    if (success) {
        return parent;
    }

    // if parent pointer is up-to-date
    if (parent && parent->_id == parentID) {
        // Make sure the parent knows about this child
        if (!parentKnowsMe) {
            parent->beParentOfChild(getThisPointer());
            _locker.withWriteLock([&](Data& data) {
                data._parentKnowsMe = true;
            });
        }

        success = true;
        return parent;
    }

    // If we haven't returned yet, there's a mismatch between the actual parent and the 
    // parent we think we should have.
    if (parent) {
        // we have a parent pointer but our _parentID doesn't indicate this parent.
        parent->forgetChild(getThisPointer());
        _locker.withWriteLock([&](Data& data) {
            data._parentKnowsMe = false;
            data._parent.reset();
        });
        parent.reset();
    }

    if (parentID.isNull()) {
        success = true;
        return parent;
    }

    // we have a _parentID but no parent pointer, or our parent pointer was to the wrong thing
    auto parentFinder = DependencyManager::get<SpatialParentFinder>();
    if (!parentFinder) {
        success = false;
        return nullptr;
    }

    auto weakParent = parentFinder->find(parentID, success, getParentTree());
    if (!success) {
        return nullptr;
    }

    parent = weakParent.lock();
    if (parent) {
        parent->beParentOfChild(getThisPointer());
        _locker.withWriteLock([&](Data& data) {
            data._parentKnowsMe = true;
        });
    }

    success = (parent || parentID.isNull());
    return parent;
}

void SpatiallyNestable::beParentOfChild(SpatiallyNestablePointer newChild) const {
    _locker.withWriteLock([&](Data& data) {
        data._children[newChild->getID()] = newChild;
    });
}

void SpatiallyNestable::forgetChild(SpatiallyNestablePointer newChild) const {
    _locker.withWriteLock([&](Data& data) {
        data._children.remove(newChild->getID());
    });
}

void SpatiallyNestable::setParentJointIndex(quint16 parentJointIndex) {
    _locker.withWriteLock([&](Data& data) {
        if (data._parentJointIndex != parentJointIndex) {
            data._parentJointIndex = parentJointIndex;
            // Invalidate the parent cache information because the joint index changed
            _parentStamp = 0;
            // Invalidate myself
            ++_stamp;
        }
    });
}

glm::vec3 SpatiallyNestable::worldToLocal(const glm::vec3& position,
                                          const QUuid& parentID, int parentJointIndex,
                                          bool& success) {
    Transform result;
    QSharedPointer<SpatialParentFinder> parentFinder = DependencyManager::get<SpatialParentFinder>();
    if (!parentFinder) {
        success = false;
        return glm::vec3(0.0f);
    }

    Transform parentTransform;
    auto parentWP = parentFinder->find(parentID, success);
    if (!success) {
        return glm::vec3(0.0f);
    }

    auto parent = parentWP.lock();
    if (!parentID.isNull() && !parent) {
        success = false;
        return glm::vec3(0.0f);
    }

    if (parent) {
        parentTransform = parent->getTransform(parentJointIndex, success);
        if (!success) {
            return glm::vec3(0.0f);
        }
        parentTransform.setScale(1.0f); // TODO: scale
    }
    success = true;

    Transform positionTransform;
    positionTransform.setTranslation(position);
    Transform myWorldTransform;
    Transform::mult(myWorldTransform, parentTransform, positionTransform);
    myWorldTransform.setTranslation(position);
    Transform::inverseMult(result, parentTransform, myWorldTransform);
    return result.getTranslation();
}

glm::quat SpatiallyNestable::worldToLocal(const glm::quat& orientation,
                                          const QUuid& parentID, int parentJointIndex,
                                          bool& success) {
    Transform result;
    QSharedPointer<SpatialParentFinder> parentFinder = DependencyManager::get<SpatialParentFinder>();
    if (!parentFinder) {
        success = false;
        return glm::quat();
    }

    Transform parentTransform;
    auto parentWP = parentFinder->find(parentID, success);
    if (!success) {
        return glm::quat();
    }

    auto parent = parentWP.lock();
    if (!parentID.isNull() && !parent) {
        success = false;
        return glm::quat();
    }

    if (parent) {
        parentTransform = parent->getTransform(parentJointIndex, success);
        if (!success) {
            return glm::quat();
        }
        parentTransform.setScale(1.0f); // TODO: scale
    }
    success = true;

    Transform orientationTransform;
    orientationTransform.setRotation(orientation);
    Transform myWorldTransform;
    Transform::mult(myWorldTransform, parentTransform, orientationTransform);
    myWorldTransform.setRotation(orientation);
    Transform::inverseMult(result, parentTransform, myWorldTransform);
    return result.getRotation();
}

glm::vec3 SpatiallyNestable::localToWorld(const glm::vec3& position,
                                          const QUuid& parentID, int parentJointIndex,
                                          bool& success) {
    Transform result;
    QSharedPointer<SpatialParentFinder> parentFinder = DependencyManager::get<SpatialParentFinder>();
    if (!parentFinder) {
        success = false;
        return glm::vec3(0.0f);
    }

    Transform parentTransform;
    auto parentWP = parentFinder->find(parentID, success);
    if (!success) {
        return glm::vec3(0.0f);
    }

    auto parent = parentWP.lock();
    if (!parentID.isNull() && !parent) {
        success = false;
        return glm::vec3(0.0f);
    }

    if (parent) {
        parentTransform = parent->getTransform(parentJointIndex, success);
        if (!success) {
            return glm::vec3(0.0f);
        }
        parentTransform.setScale(1.0f); // TODO: scale
    }
    success = true;

    Transform positionTransform;
    positionTransform.setTranslation(position);
    Transform::mult(result, parentTransform, positionTransform);
    return result.getTranslation();
}

glm::quat SpatiallyNestable::localToWorld(const glm::quat& orientation,
                                          const QUuid& parentID, int parentJointIndex,
                                          bool& success) {
    Transform result;
    QSharedPointer<SpatialParentFinder> parentFinder = DependencyManager::get<SpatialParentFinder>();
    if (!parentFinder) {
        success = false;
        return glm::quat();
    }

    Transform parentTransform;
    auto parentWP = parentFinder->find(parentID, success);
    if (!success) {
        return glm::quat();
    }

    auto parent = parentWP.lock();
    if (!parentID.isNull() && !parent) {
        success = false;
        return glm::quat();
    }

    if (parent) {
        parentTransform = parent->getTransform(parentJointIndex, success);
        if (!success) {
            return glm::quat();
        }
        parentTransform.setScale(1.0f);
    }
    success = true;

    Transform orientationTransform;
    orientationTransform.setRotation(orientation);
    Transform::mult(result, parentTransform, orientationTransform);
    return result.getRotation();
}

glm::vec3 SpatiallyNestable::getPosition(bool& success) const {
    return getTransform(success).getTranslation();
}

glm::vec3 SpatiallyNestable::getPosition() const {
    bool success;
    auto result = getPosition(success);
    #ifdef WANT_DEBUG
    if (!success) {
        qDebug() << "Warning -- getPosition failed" << getID();
    }
    #endif
    return result;
}

glm::vec3 SpatiallyNestable::getPosition(int jointIndex, bool& success) const {
    return getTransform(jointIndex, success).getTranslation();
}

void SpatiallyNestable::setPosition(const glm::vec3& position, bool& success, bool tellPhysics) {
    // guard against introducing NaN into the transform
    if (isNaN(position)) {
        success = false;
        return;
    }

    success = ensureParentClean(0);
    if (!success) {
        return;
    }
    _locker.withWriteLock([&](Data& data) {
        Transform myWorldTransform;
        Transform::mult(myWorldTransform, data._parentTransform, data._transform);
        myWorldTransform.setTranslation(position);
        Transform::inverseMult(data._transform, data._parentTransform, myWorldTransform);
    });
    if (success) {
        locationChanged(tellPhysics);
    } else {
        qDebug() << "setPosition failed for" << getID();
    }
}

void SpatiallyNestable::setPosition(const glm::vec3& position) {
    bool success;
    setPosition(position, success);
    #ifdef WANT_DEBUG
    if (!success) {
        qDebug() << "Warning -- setPosition failed" << getID();
    }
    #endif
}

glm::quat SpatiallyNestable::getOrientation(bool& success) const {
    return getTransform(success).getRotation();
}

glm::quat SpatiallyNestable::getOrientation() const {
    bool success;
    auto result = getOrientation(success);
    #ifdef WANT_DEBUG
    if (!success) {
        qDebug() << "Warning -- getOrientation failed" << getID();
    }
    #endif
    return result;
}

glm::quat SpatiallyNestable::getOrientation(int jointIndex, bool& success) const {
    return getTransform(jointIndex, success).getRotation();
}

void SpatiallyNestable::setOrientation(const glm::quat& orientation, bool& success, bool tellPhysics) {
    // guard against introducing NaN into the transform
    if (isNaN(orientation)) {
        success = false;
        return;
    }

    success = ensureParentClean(0);
    if (!success) {
        return;
    }

    _locker.withWriteLock([&](Data& data) {
        Transform myWorldTransform;
        Transform::mult(myWorldTransform, data._parentTransform, data._transform);
        myWorldTransform.setRotation(orientation);
        Transform::inverseMult(data._transform, data._parentTransform, myWorldTransform);
        ++_stamp;
    });

    locationChanged(tellPhysics);
}

void SpatiallyNestable::setOrientation(const glm::quat& orientation) {
    bool success;
    setOrientation(orientation, success);
    #ifdef WANT_DEBUG
    if (!success) {
        qDebug() << "Warning -- setOrientation failed" << getID();
    }
    #endif
}

glm::vec3 SpatiallyNestable::getVelocity(bool& success) const {
    glm::vec3 result;
    success = ensureParentClean(0);
    if (!success) {
        return result;
    }

    _locker.withConsistentRead([&](const Data& data) {
        if (data._parentTransform.isIdentity()) {
            result = data._velocity;
        } else {
            // TODO: take parent angularVelocity into account.
            result = data._parentVelocity + data._parentTransform.getRotation() * data._velocity;
        }
    });
    return result;
}

glm::vec3 SpatiallyNestable::getVelocity() const {
    bool success;
    glm::vec3 result = getVelocity(success);
    if (!success) {
        qDebug() << "Warning -- setVelocity failed" << getID();
    }
    return result;
}

bool SpatiallyNestable::hasAncestorOfType(const SpatiallyNestableFlags& flags) const {
    if (ensureParentClean(0)) {
        return false;
    }

    SpatiallyNestableFlags parentFlags;
    _locker.withConsistentRead([&](const Data& data) {
        parentFlags = data._parentFlags;
    });
    return flags == (parentFlags & flags);
}

bool SpatiallyNestable::hasAncestorOfType(const SpatiallyNestableFlagBits flag) const {
    if (ensureParentClean(0)) {
        return false;
    }

    bool result = false;
    _locker.withConsistentRead([&](const Data& data) {
        result = data._parentFlags.isSet(flag);
    });
    return result;
}

void SpatiallyNestable::setVelocity(const glm::vec3& velocity, bool& success) {
    success = ensureParentClean(0);
    if (!success) {
        return;
    }

    bool avatarAncestor = hasAncestorOfType(SpatiallyNestableFlagBits::Avatar);
    _locker.withWriteLock([&](Data& data) {
        // HACK: until we are treating _velocity the same way we treat _position (meaning,
        // _velocity is a vs parent value and any request for a world-frame velocity must
        // be computed), do this to avoid equipped (parenting-grabbed) things from drifting.
        // turning a zero velocity into a non-zero _velocity (because the avatar is moving)
        // causes EntityItem::stepKinematicMotion to have an effect on the equipped entity,
        // which causes it to drift from the hand.
        if (avatarAncestor) {
            data._velocity = velocity;
            return;
        }

        data._velocity = velocity - data._parentVelocity;
        // TODO: take parent angularVelocity into account.
        if (data._parentTransform.isRotating()) {
            data._velocity = glm::inverse(data._parentTransform.getRotation()) * data._velocity;
        }
        ++_stamp;
    });
}

void SpatiallyNestable::setVelocity(const glm::vec3& velocity) {
    bool success;
    setVelocity(velocity, success);
    if (!success) {
        qDebug() << "Warning -- setVelocity failed" << getID();
    }
}

glm::vec3 SpatiallyNestable::getParentVelocity(bool& success) const {
    glm::vec3 result;
    success = ensureParentClean(0);
    if (!success) {
        return result;
    }
    _locker.withConsistentRead([&](const Data& data) {
        result = data._parentVelocity;
    });
    return result;
}

glm::vec3 SpatiallyNestable::getAngularVelocity(bool& success) const {
    glm::vec3 result;
    success = ensureParentClean(0);
    if (!success) {
        return result;
    }

    _locker.withConsistentRead([&](const Data& data) {
        if (data._parentTransform.isIdentity()) {
            result = data._angularVelocity;
        } else {
            // TODO: take parent angularVelocity into account.
            result = data._parentAngularVelocity + data._parentTransform.getRotation() * data._angularVelocity;  
        }
    });
    return result;
}

glm::vec3 SpatiallyNestable::getAngularVelocity() const {
    bool success;
    glm::vec3 result = getAngularVelocity(success);
    if (!success) {
        qDebug() << "Warning -- getAngularVelocity failed" << getID();
    }
    return result;
}

void SpatiallyNestable::setAngularVelocity(const glm::vec3& angularVelocity, bool& success) {
    success = ensureParentClean(0);
    if (!success) {
        return;
    }

    glm::vec3 result;
    _locker.withConsistentRead([&](const Data& data) {
        result = angularVelocity - data._parentAngularVelocity;
        if (data._parentTransform.isRotating()) {
            result = glm::inverse(data._parentTransform.getRotation()) * result;
        }
    });

    _locker.withWriteLock([&](Data& data) {
        data._angularVelocity = result;
        ++_stamp;
    });
}

void SpatiallyNestable::setAngularVelocity(const glm::vec3& angularVelocity) {
    bool success;
    setAngularVelocity(angularVelocity, success);
    if (!success) {
        qDebug() << "Warning -- setAngularVelocity failed" << getID();
    }
}

glm::vec3 SpatiallyNestable::getParentAngularVelocity(bool& success) const {
    glm::vec3 result;
    success = ensureParentClean(0);
    if (!success) {
        return result;
    }
    _locker.withConsistentRead([&](const Data& data) {
        result = data._parentAngularVelocity;
    });
    return result;
}

const Transform SpatiallyNestable::getTransform(bool& success) const {
    Transform result;
    success = ensureParentClean(0);
    if (!success) {
        return result;
    }

    // return a world-space transform for this object's location
    _locker.withConsistentRead([&](const Data& data) {
        if (data._parentTransform.isIdentity()) {
            result = data._transform;
        } else {
            Transform::mult(result, data._parentTransform, data._transform);
        }
    });

    return result;
}

const Transform SpatiallyNestable::getTransform(int jointIndex, bool& success) const {
    success = ensureParentClean(0);
    // this returns the world-space transform for this object.  It finds its parent's transform (which may
    // cause this object's parent to query its parent, etc) and multiplies this object's local transform onto it.
    Transform jointInWorldFrame;

    if (!success) {
        return jointInWorldFrame;
    }

    Transform worldTransform = getTransform(success);
    if (!success) {
        return jointInWorldFrame;
    }

    worldTransform.setScale(1.0f); // TODO -- scale;
    Transform jointInObjectFrame = getAbsoluteJointTransformInObjectFrame(jointIndex);
    Transform::mult(jointInWorldFrame, worldTransform, jointInObjectFrame);
    success = true;
    return jointInWorldFrame;
}

void SpatiallyNestable::setTransform(const Transform& transform, bool& success) {
    if (transform.containsNaN()) {
        success = false;
        return;
    }

    success = ensureParentClean(0);
    if (!success) {
        return;
    }

    Transform result = transform;
    _locker.withWriteLock([&](Data& data) {
        if (!data._parentTransform.isIdentity()) {
            Transform::inverseMult(result, data._parentTransform, transform);
        }
        data._transform = result;
        ++_stamp;
    });

    if (success) {
        locationChanged();
    }
}

glm::vec3 SpatiallyNestable::getScale() const {
    // TODO: scale
    glm::vec3 result;
    bool success = ensureParentClean(0);
    if (!success) {
        return result;
    }
    _locker.withConsistentRead([&](const Data& data) {
        result = data._transform.getScale();
    });
    return result;
}

glm::vec3 SpatiallyNestable::getScale(int jointIndex) const {
    // TODO: scale
    return getScale();
}

void SpatiallyNestable::setScale(const glm::vec3& scale) {
    // guard against introducing NaN into the transform
    if (glm::any(glm::isnan(scale))) {
        qDebug() << "SpatiallyNestable::setLocalScale -- scale contains NaN";
        return;
    }
    // TODO: scale
    _locker.withWriteLock([&](Data& data) {
        data._transform.setScale(scale);
        // Note that we don't actually increment our stamp here
        // because right now scale has no affect on child transforms
        // ++_stamp;
    });
    dimensionsChanged();
}

const Transform SpatiallyNestable::getLocalTransform() const {
    Transform result;
    _locker.withConsistentRead([&](const Data& data) {
        result = data._transform;
    });
    return result;
}

void SpatiallyNestable::setLocalTransform(const Transform& transform) {
    // guard against introducing NaN into the transform
    if (transform.containsNaN()) {
        qDebug() << "SpatiallyNestable::setLocalTransform -- transform contains NaN";
        return;
    }
    _locker.withWriteLock([&](Data& data) {
        data._transform = transform;
    });
    locationChanged();
}

glm::vec3 SpatiallyNestable::getLocalPosition() const {
    glm::vec3 result;
    _locker.withConsistentRead([&](const Data& data) {
        result = data._transform.getTranslation();
    });
    return result;
}

void SpatiallyNestable::setLocalPosition(const glm::vec3& position, bool tellPhysics) {
    // guard against introducing NaN into the transform
    if (isNaN(position)) {
        qDebug() << "SpatiallyNestable::setLocalPosition -- position contains NaN";
        return;
    }
    _locker.withWriteLock([&](Data& data) {
        data._transform.setTranslation(position);
        ++_stamp;
    });
    locationChanged(tellPhysics);
}

glm::quat SpatiallyNestable::getLocalOrientation() const {
    glm::quat result;
    _locker.withConsistentRead([&](const Data& data) {
        result = data._transform.getRotation();
    });
    return result;
}

void SpatiallyNestable::setLocalOrientation(const glm::quat& orientation) {
    // guard against introducing NaN into the transform
    if (isNaN(orientation)) {
        qDebug() << "SpatiallyNestable::setLocalOrientation -- orientation contains NaN";
        return;
    }
    _locker.withWriteLock([&](Data& data) {
        data._transform.setRotation(orientation);
        ++_stamp;
    });
    locationChanged();
}

glm::vec3 SpatiallyNestable::getLocalVelocity() const {
    glm::vec3 result(glm::vec3::_null);
    _locker.withConsistentRead([&](const Data& data) {
        result = data._velocity;
    });
    return result;
}

void SpatiallyNestable::setLocalVelocity(const glm::vec3& velocity) {
    _locker.withWriteLock([&](Data& data) {
        data._velocity = velocity;
        ++_stamp;
    });
}

glm::vec3 SpatiallyNestable::getLocalAngularVelocity() const {
    glm::vec3 result(glm::vec3::_null);
    _locker.withConsistentRead([&](const Data& data) {
        result = data._angularVelocity;
    });
    return result;
}

void SpatiallyNestable::setLocalAngularVelocity(const glm::vec3& angularVelocity) {
    _locker.withWriteLock([&](Data& data) {
        data._angularVelocity = angularVelocity;
        ++_stamp;
    });
}

glm::vec3 SpatiallyNestable::getLocalScale() const {
    // TODO: scale
    glm::vec3 result;
    _locker.withConsistentRead([&](const Data& data) {
        result = data._transform.getScale();
    });
    return result;
}

void SpatiallyNestable::setLocalScale(const glm::vec3& scale) {
    // guard against introducing NaN into the transform
    if (isNaN(scale)) {
        qDebug() << "SpatiallyNestable::setLocalScale -- scale contains NaN";
        return;
    }
    // TODO: scale
    _locker.withWriteLock([&](Data& data) {
        data._transform.setScale(scale);
        // Scale doesn't propagate, so we don't increment our stamp here.
        // ++_stamp;
    });
    dimensionsChanged();
}

QList<SpatiallyNestablePointer> SpatiallyNestable::getChildren() const {
    QHash<QUuid, SpatiallyNestableWeakPointer> children;
    _locker.withConsistentRead([&](const Data& data) {
        children = data._children;
    });
    QList<SpatiallyNestablePointer> result;
    if (!children.size()) {
        return result;
    }

    result.reserve(children.size());
    QUuid childParentID;
    bool childParentKnowsMe;
    for (const SpatiallyNestableWeakPointer& childWP : children) {
        SpatiallyNestablePointer child = childWP.lock();
        if (!child) {
            continue;
        }
        
        child->_locker.withConsistentRead([&](const Data& childData) {
            childParentKnowsMe = childData._parentKnowsMe;
            childParentID = childData._parentID;
        }); 

        if (!childParentKnowsMe) {
            continue;
        }

        if (_id != childParentID) {
            continue;
        }

        result << child;
    }

    return result;
}

bool SpatiallyNestable::hasChildren() const {
    bool result = false;
    _locker.withConsistentRead([&](const Data& data) {
        result = !data._children.isEmpty();
    });
    return result;
}

const Transform SpatiallyNestable::getAbsoluteJointTransformInObjectFrame(int jointIndex) const {
    Transform jointTransformInObjectFrame;
    glm::vec3 position = getAbsoluteJointTranslationInObjectFrame(jointIndex);
    glm::quat orientation = getAbsoluteJointRotationInObjectFrame(jointIndex);
    jointTransformInObjectFrame.setRotation(orientation);
    jointTransformInObjectFrame.setTranslation(position);
    return jointTransformInObjectFrame;
}

SpatiallyNestablePointer SpatiallyNestable::getThisPointer() const {
    SpatiallyNestableConstPointer constThisPointer = shared_from_this();
    SpatiallyNestablePointer thisPointer = std::const_pointer_cast<SpatiallyNestable>(constThisPointer); // ermahgerd !!!
    return thisPointer;
}

void SpatiallyNestable::forEachChild(std::function<void(SpatiallyNestablePointer)> actor) {
    foreach(SpatiallyNestablePointer child, getChildren()) {
        actor(child);
    }
}

void SpatiallyNestable::forEachDescendant(std::function<void(SpatiallyNestablePointer)> actor) {
    QQueue<SpatiallyNestablePointer> toProcess;
    foreach(SpatiallyNestablePointer child, getChildren()) {
        toProcess.enqueue(child);
    }

    while (!toProcess.empty()) {
        SpatiallyNestablePointer object = toProcess.dequeue();
        actor(object);
        foreach (SpatiallyNestablePointer child, object->getChildren()) {
            toProcess.enqueue(child);
        }
    }
}

void SpatiallyNestable::locationChanged(bool tellPhysics) {
    forEachChild([&](SpatiallyNestablePointer object) {
        object->locationChanged(tellPhysics);
    });
}

AACube SpatiallyNestable::getMaximumAACube(bool& success) const {
    return AACube(getPosition(success) - glm::vec3(defaultAACubeSize / 2.0f), defaultAACubeSize);
}

void SpatiallyNestable::checkAndAdjustQueryAACube() {
    bool success;
    AACube maxAACube = getMaximumAACube(success);
    if (success && (!_queryAACubeSet || !_queryAACube.contains(maxAACube))) {
        setQueryAACube(maxAACube);
    }
}

void SpatiallyNestable::setQueryAACube(const AACube& queryAACube) {
    if (queryAACube.containsNaN()) {
        qDebug() << "SpatiallyNestable::setQueryAACube -- cube contains NaN";
        return;
    }
    _queryAACube = queryAACube;
    if (queryAACube.getScale() > 0.0f) {
        _queryAACubeSet = true;
    }
}

bool SpatiallyNestable::queryAABoxNeedsUpdate() const {
    bool success;
    AACube currentAACube = getMaximumAACube(success);
    if (!success) {
        qDebug() << "can't getMaximumAACube for" << getID();
        return false;
    }

    // make sure children are still in their boxes, also.
    bool childNeedsUpdate = false;
    getThisPointer()->forEachDescendant([&](SpatiallyNestablePointer descendant) {
        if (!childNeedsUpdate && descendant->queryAABoxNeedsUpdate()) {
            childNeedsUpdate = true;
        }
    });
    if (childNeedsUpdate) {
        return true;
    }

    if (_queryAACubeSet && _queryAACube.contains(currentAACube)) {
        return false;
    }

    return true;
}

bool SpatiallyNestable::computePuffedQueryAACube() {
    if (!queryAABoxNeedsUpdate()) {
        return false;
    }
    bool success;
    AACube currentAACube = getMaximumAACube(success);
    // make an AACube with edges thrice as long and centered on the object
    _queryAACube = AACube(currentAACube.getCorner() - glm::vec3(currentAACube.getScale()), currentAACube.getScale() * 3.0f);
    _queryAACubeSet = true;

    getThisPointer()->forEachDescendant([&](SpatiallyNestablePointer descendant) {
        bool success;
        AACube descendantAACube = descendant->getQueryAACube(success);
        if (success) {
            if (_queryAACube.contains(descendantAACube)) {
                return;
            }
            _queryAACube += descendantAACube.getMinimumPoint();
            _queryAACube += descendantAACube.getMaximumPoint();
        }
    });

    return true;
}

AACube SpatiallyNestable::getQueryAACube(bool& success) const {
    if (_queryAACubeSet) {
        success = true;
        return _queryAACube;
    }
    success = false;
    bool getPositionSuccess;
    return AACube(getPosition(getPositionSuccess) - glm::vec3(defaultAACubeSize / 2.0f), defaultAACubeSize);
}

AACube SpatiallyNestable::getQueryAACube() const {
    bool success;
    auto result = getQueryAACube(success);
    if (!success) {
        qDebug() << "getQueryAACube failed for" << getID();
    }
    return result;
}

void SpatiallyNestable::getLocalTransformAndVelocities(
        Transform& transform,
        glm::vec3& velocity,
        glm::vec3& angularVelocity) const {
    _locker.withConsistentRead([&](const Data& data) {
        // transform
        transform = data._transform;
        // linear velocity
        velocity = data._velocity;
        // angular velocity
        angularVelocity = data._angularVelocity;
    });
}

void SpatiallyNestable::setLocalTransformAndVelocities(
        const Transform& localTransform,
        const glm::vec3& localVelocity,
        const glm::vec3& localAngularVelocity) {
    _locker.withWriteLock([&](Data& data) {
        // transform
        data._transform = localTransform;
        // linear velocity
        data._velocity = localVelocity;
        // angular velocity
        data._angularVelocity = localAngularVelocity;
        ++_stamp;
    });
    locationChanged(false);
}
