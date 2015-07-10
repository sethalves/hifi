//
//  Transform.h
//  shared/src/gpu
//
//  Created by Sam Gateau on 11/4/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_Transform_h
#define hifi_gpu_Transform_h

#include <assert.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <bitset>

#include <memory>

inline bool isValidScale(glm::vec3 scale) {
    bool result = scale.x != 0.0f && scale.y != 0.0f && scale.z != 0.0f;
    assert(result);
    return result;
}

inline bool isValidScale(float scale) {
    bool result = scale != 0.0f && !glm::isnan(scale) && !glm::isinf(scale);
    assert(result);
    return result;
}

class Transform {
public:
    typedef glm::mat4 Mat4;
    typedef glm::mat3 Mat3;
    typedef glm::vec4 Vec4;
    typedef glm::vec3 Vec3;
    typedef glm::vec2 Vec2;
    typedef glm::quat Quat;

    Transform() :
        _rotation(1.0f, 0.0f, 0.0f, 0.0f),
        _scale(1.0f),
        _translation(0.0f),
        _flags(FLAG_CACHE_INVALID_BITSET) // invalid cache
    {
    }
    Transform(Quat rotation, Vec3 scale, Vec3 translation) :
        _rotation(rotation),
        _scale(scale),
        _translation(translation),
        _flags(FLAG_CACHE_INVALID_BITSET) // invalid cache
    {
        if (!isValidScale(_scale)) {
            _scale = Vec3(1.0f);
        }
    }
    Transform(const Transform& transform) :
        _rotation(transform._rotation),
        _scale(transform._scale),
        _translation(transform._translation),
        _flags(transform._flags)
    {
        invalidCache();
    }
    Transform(const Mat4& raw) {
        evalFromRawMatrix(raw);
    }
    ~Transform() {}

    Transform& operator=(const Transform& transform) {
        _rotation = transform._rotation;
        _scale = transform._scale;
        _translation = transform._translation;
        _flags = transform._flags;
        invalidCache();
        return (*this);
    }

    void setIdentity();

    const Vec3& getTranslation() const;
    void setTranslation(const Vec3& translation); // [new this] = [translation] * [this.rotation] * [this.scale]
    void preTranslate(const Vec3& translation);   // [new this] = [translation] * [this]
    void postTranslate(const Vec3& translation);  // [new this] = [this] * [translation] equivalent to glTranslate

    const Quat& getRotation() const;
    void setRotation(const Quat& rotation); // [new this] = [this.translation] * [rotation] * [this.scale]
    void preRotate(const Quat& rotation);   // [new this] = [rotation] * [this]
    void postRotate(const Quat& rotation);  // [new this] = [this] * [rotation] equivalent to glRotate

    const Vec3& getScale() const;
    void setScale(float scale);
    void setScale(const Vec3& scale);  // [new this] = [this.translation] * [this.rotation] * [scale]
    void preScale(float scale);
    void preScale(const Vec3& scale);
    void postScale(float scale);       // [new this] = [this] * [scale] equivalent to glScale
    void postScale(const Vec3& scale); // [new this] = [this] * [scale] equivalent to glScale

    bool isIdentity() const { return (_flags & ~Flags(FLAG_CACHE_INVALID_BITSET)).none(); }
    bool isTranslating() const { return _flags[FLAG_TRANSLATION]; }
    bool isRotating() const { return _flags[FLAG_ROTATION]; }
    bool isScaling() const { return _flags[FLAG_SCALING]; }
    bool isUniform() const { return !isNonUniform(); }
    bool isNonUniform() const { return _flags[FLAG_NON_UNIFORM]; }

    void evalFromRawMatrix(const Mat4& matrix);
    void evalFromRawMatrix(const Mat3& rotationScalematrix);

    Mat4& getMatrix(Mat4& result) const;
    Mat4& getInverseMatrix(Mat4& result) const;
    Mat4& getInverseTransposeMatrix(Mat4& result) const;

    Mat4& getRotationScaleMatrix(Mat4& result) const;
    Mat4& getRotationScaleMatrixInverse(Mat4& result) const;

    Transform& evalInverse(Transform& result) const;

    static void evalRotationScale(Quat& rotation, Vec3& scale, const Mat3& rotationScaleMatrix);

    static Transform& mult(Transform& result, const Transform& left, const Transform& right);

    // Left will be inversed before the multiplication
    static Transform& inverseMult(Transform& result, const Transform& left, const Transform& right);

    Vec4 transform(const Vec4& pos) const;

protected:

    enum Flag {
        FLAG_CACHE_INVALID = 0,

        FLAG_TRANSLATION,
        FLAG_ROTATION,
        FLAG_SCALING,
        FLAG_NON_UNIFORM,
        FLAG_ZERO_SCALE,

        FLAG_PROJECTION,

        NUM_FLAGS,

        FLAG_CACHE_INVALID_BITSET = 1,
    };
    typedef std::bitset<NUM_FLAGS> Flags;


    // TRS
    Quat _rotation;
    Vec3 _scale;
    Vec3 _translation;

    mutable Flags _flags;

    // Cached transform
    // TODO: replace this auto ptr by a "unique ptr" as soon as we are compiling in C++11
    mutable std::auto_ptr<Mat4> _matrix;

    bool isCacheInvalid() const { return _flags[FLAG_CACHE_INVALID]; }
    void validCache() const { _flags.set(FLAG_CACHE_INVALID, false); }
    void invalidCache() const { _flags.set(FLAG_CACHE_INVALID, true); }

    void flagTranslation() { _flags.set(FLAG_TRANSLATION, true); }
    void unflagTranslation() { _flags.set(FLAG_TRANSLATION, false); }

    void flagRotation() { _flags.set(FLAG_ROTATION, true); }
    void unflagRotation() { _flags.set(FLAG_ROTATION, false); }

    void flagScaling() { _flags.set(FLAG_SCALING, true); }
    void unflagScaling() { _flags.set(FLAG_SCALING, false); }


    void flagUniform() { _flags.set(FLAG_NON_UNIFORM, false); }
    void flagNonUniform() { _flags.set(FLAG_NON_UNIFORM, true); }

    void updateCache() const;
    Mat4& getCachedMatrix(Mat4& result) const;
};

inline void Transform::setIdentity() {
    _translation = Vec3(0.0f);
    _rotation = Quat(1.0f, 0.0f, 0.0f, 0.0f);
    _scale = Vec3(1.0f);
    _flags = Flags(FLAG_CACHE_INVALID_BITSET);
}

inline const Transform::Vec3& Transform::getTranslation() const {
    return _translation;
}

inline void Transform::setTranslation(const Vec3& translation) {
    invalidCache();
    if (translation == Vec3()) {
        unflagTranslation();
    } else {
        flagTranslation();
    }
    _translation = translation;
}

inline void Transform::preTranslate(const Vec3& translation) {
    if (translation == Vec3()) {
        return;
    }
    invalidCache();
    flagTranslation();
    _translation += translation;
}

inline void Transform::postTranslate(const Vec3& translation) {
    if (translation == Vec3()) {
        return;
    }
    invalidCache();
    flagTranslation();

    Vec3 scaledT = translation;
    if (isScaling()) {
        scaledT *= _scale;
    }

    if (isRotating()) {
        _translation += glm::rotate(_rotation, scaledT);
    } else {
        _translation += scaledT;
    }
}

inline const Transform::Quat& Transform::getRotation() const {
    return _rotation;
}

inline void Transform::setRotation(const Quat& rotation) {
    invalidCache();
    if (rotation == Quat()) {
        unflagRotation();
    } else {
        flagRotation();
    }
    _rotation = rotation;
}

inline void Transform::preRotate(const Quat& rotation) {
    if (rotation == Quat()) {
        return;
    }
    invalidCache();
    if (isRotating()) {
        _rotation = rotation * _rotation;
    } else {
        _rotation = rotation;
    }
    flagRotation();

    _translation = glm::rotate(rotation, _translation);
}

inline void Transform::postRotate(const Quat& rotation) {
    if (rotation == Quat()) {
        return;
    }
    invalidCache();

    if (isNonUniform()) {
        Quat newRot;
        Vec3 newScale;
        Mat3 scaleRot(glm::mat3_cast(rotation));
        scaleRot[0] *= _scale;
        scaleRot[1] *= _scale;
        scaleRot[2] *= _scale;
        evalRotationScale(newRot, newScale, scaleRot);

        if (isRotating()) {
            _rotation *= newRot;
        } else {
            _rotation = newRot;
        }
        setScale(newScale);
    } else {
        if (isRotating()) {
            _rotation *= rotation;
        } else {
            _rotation = rotation;
        }
    }
    flagRotation();
}

inline const Transform::Vec3& Transform::getScale() const {
    return _scale;
}

inline void Transform::setScale(float scale) {
    if (!isValidScale(scale)) {
        return;
    }
    invalidCache();
    flagUniform();
    
    if (scale == 1.0f) {
        unflagScaling();
    } else {
        flagScaling();
    }
    _scale = Vec3(scale);
}

inline void Transform::setScale(const Vec3& scale) {
    if (!isValidScale(scale)) {
        return;
    }
    if ((scale.x == scale.y) && (scale.x == scale.z)) {
        setScale(scale.x);
    } else {
        invalidCache();
        flagScaling();
        flagNonUniform();
        _scale = scale;
    }
}

inline void Transform::preScale(float scale) {
    setScale(getScale() * scale);
}

inline void Transform::preScale(const Vec3& scale) {
    setScale(getScale() * scale);
}

inline void Transform::postScale(float scale) {
    if (!isValidScale(scale) || scale == 1.0f) {
        return;
    }
    if (isScaling()) {
        // if already scaling, just invalid cache and apply uniform scale
        invalidCache();
        _scale *= scale;
    } else {
        setScale(scale);
    }
}

inline void Transform::postScale(const Vec3& scale) {
    if (!isValidScale(scale)) {
        return;
    }
    invalidCache();
    if (isScaling()) {
        _scale *= scale;
    } else {
        _scale = scale;
    }
    flagScaling();
}

inline Transform::Mat4& Transform::getMatrix(Transform::Mat4& result) const {
    if (isRotating()) {
        Mat3 rot = glm::mat3_cast(_rotation);

        if (isScaling()) {
            rot[0] *= _scale.x;
            rot[1] *= _scale.y;
            rot[2] *= _scale.z;
        }

        result[0] = Vec4(rot[0], 0.0f);
        result[1] = Vec4(rot[1], 0.0f);
        result[2] = Vec4(rot[2], 0.0f);
    } else {
        result[0] = Vec4(_scale.x, 0.0f, 0.0f, 0.0f);
        result[1] = Vec4(0.0f, _scale.y, 0.0f, 0.0f);
        result[2] = Vec4(0.0f, 0.0f, _scale.z, 0.0f);
    }

    result[3] = Vec4(_translation, 1.0f);
    return result;
}

inline Transform::Mat4& Transform::getInverseMatrix(Transform::Mat4& result) const {
    Transform inverse;
    evalInverse(inverse);
    return inverse.getMatrix(result);
}

inline Transform::Mat4& Transform::getInverseTransposeMatrix(Transform::Mat4& result) const {
    getInverseMatrix(result);
    result = glm::transpose(result);
    return result;
}

inline Transform::Mat4& Transform::getRotationScaleMatrix(Mat4& result) const {
    getMatrix(result);
    result[3] = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
    return result;
}

inline Transform::Mat4& Transform::getRotationScaleMatrixInverse(Mat4& result) const {
    getInverseMatrix(result);
    result[3] = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
    return result;
}

inline void Transform::evalFromRawMatrix(const Mat4& matrix) {
    // for now works only in the case of TRS transformation
    if ((matrix[0][3] == 0.0f) && (matrix[1][3] == 0.0f) && (matrix[2][3] == 0.0f) && (matrix[3][3] == 1.0f)) {
        setTranslation(Vec3(matrix[3]));
        evalFromRawMatrix(Mat3(matrix));
    }
}

inline void Transform::evalFromRawMatrix(const Mat3& rotationScaleMatrix) {
    Quat rotation;
    Vec3 scale;
    evalRotationScale(rotation, scale, rotationScaleMatrix);
    setRotation(rotation);
    setScale(scale);
}

inline Transform& Transform::evalInverse(Transform& inverse) const {
    inverse.setIdentity();
    if (isScaling()) {
        if (isNonUniform()) {
            inverse.setScale(Vec3(1.0f) / _scale);
        } else {
            inverse.setScale(1.0f / _scale.x);
        }
    }
    if (isRotating()) {
        inverse.postRotate(glm::conjugate(_rotation));
    }
    if (isTranslating()) {
        inverse.postTranslate(-_translation);
    }
    return inverse;
}

inline Transform& Transform::mult( Transform& result, const Transform& left, const Transform& right) {
    result = left;
    if (right.isTranslating()) {
        result.postTranslate(right.getTranslation());
    }
    if (right.isRotating()) {
        result.postRotate(right.getRotation());
    }
    if (right.isScaling()) {
        result.postScale(right.getScale());
    }

    // HACK: In case of an issue in the Transform multiplication results, to make sure this code is
    // working properly uncomment the next 2 lines and compare the results, they should be the same...
    // Transform::Mat4 mv = left.getMatrix() * right.getMatrix();
    // Transform::Mat4 mv2 = result.getMatrix();

    return result;
}

inline Transform& Transform::inverseMult( Transform& result, const Transform& left, const Transform& right) {
    result.setIdentity();

    if (left.isScaling()) {
        result.setScale(Vec3(1.0f) / left.getScale());
    }
    if (left.isRotating()) {
        result.postRotate(glm::conjugate(left.getRotation()));
    }
    if (left.isTranslating() || right.isTranslating()) {
        result.postTranslate(right.getTranslation() - left.getTranslation());
    }
    if (right.isRotating()) {
        result.postRotate(right.getRotation());
    }
    if (right.isScaling()) {
        result.postScale(right.getScale());
    }

    // HACK: In case of an issue in the Transform multiplication results, to make sure this code is
    // working properly uncomment the next 2 lines and compare the results, they should be the same...
    // Transform::Mat4 mv = left.getMatrix() * right.getMatrix();
    // Transform::Mat4 mv2 = result.getMatrix();

    return result;
}

inline Transform::Vec4 Transform::transform(const Vec4& pos) const {
    Mat4 m;
    getMatrix(m);
    return m * pos;
}


inline Transform::Mat4& Transform::getCachedMatrix(Transform::Mat4& result) const {
    updateCache();
    result = (*_matrix);
    return result;
}

inline void Transform::updateCache() const {
    if (isCacheInvalid()) {
        if (!_matrix.get()) {
            _matrix.reset(new Mat4());
        }
        getMatrix((*_matrix));
        validCache();
    }
}

#endif
