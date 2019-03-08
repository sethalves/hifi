//
//  Planar3DOverlay.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Planar3DOverlay.h"

#include <Extents.h>
#include <GeometryUtil.h>
#include <RegisteredMetaTypes.h>

Planar3DOverlay::Planar3DOverlay() :
    Base3DOverlay(),
    _dimensions{1.0f, 1.0f}
{
}

Planar3DOverlay::Planar3DOverlay(const Planar3DOverlay* planar3DOverlay) :
    Base3DOverlay(planar3DOverlay),
    _dimensions(planar3DOverlay->_dimensions)
{
}

AABox Planar3DOverlay::getBounds() const {
    auto halfDimensions = glm::vec3{_dimensions / 2.0f, 0.01f};

    auto extents = Extents{-halfDimensions, halfDimensions};
    extents.transform(getTransform());

    return AABox(extents);
}

void Planar3DOverlay::setDimensions(const glm::vec2& value) {
    _dimensions = value;
    notifyRenderVariableChange();
}

void Planar3DOverlay::setProperties(const QVariantMap& properties) {
    Base3DOverlay::setProperties(properties);

    auto dimensions = properties["dimensions"];

    // if "dimensions" property was not there, check to see if they included aliases: scale
    if (!dimensions.isValid()) {
        dimensions = properties["scale"];
        if (!dimensions.isValid()) {
            dimensions = properties["size"];
        }
    }

    if (dimensions.isValid()) {
        setDimensions(vec2FromVariant(dimensions));
    }
}

// JSDoc for copying to @typedefs of overlay types that inherit Planar3DOverlay.
/**jsdoc
 * @property {Vec2} dimensions=1,1 - The dimensions of the overlay. Synonyms: <code>scale</code>, <code>size</code>.
 */
QVariant Planar3DOverlay::getProperty(const QString& property) {
    if (property == "dimensions" || property == "scale" || property == "size") {
        return vec2ToVariant(getDimensions());
    }

    return Base3DOverlay::getProperty(property);
}

bool Planar3DOverlay::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                          float& distance, BoxFace& face, glm::vec3& surfaceNormal, bool precisionPicking) {
    glm::vec2 xyDimensions = getDimensions();
    glm::quat rotation = getWorldOrientation();
    glm::vec3 position = getWorldPosition();

    if (findRayRectangleIntersection(origin, direction, rotation, position, xyDimensions, distance)) {
        glm::vec3 forward = rotation * Vectors::FRONT;
        if (glm::dot(forward, direction) > 0.0f) {
            face = MAX_Z_FACE;
            surfaceNormal = -forward;
        } else {
            face = MIN_Z_FACE;
            surfaceNormal = forward;
        }
        return true;
    }
    return false;
}

bool Planar3DOverlay::findParabolaIntersection(const glm::vec3& origin, const glm::vec3& velocity, const glm::vec3& acceleration,
                                               float& parabolicDistance, BoxFace& face, glm::vec3& surfaceNormal, bool precisionPicking) {
    glm::vec2 xyDimensions = getDimensions();
    glm::quat rotation = getWorldOrientation();
    glm::vec3 position = getWorldPosition();

    glm::quat inverseRot = glm::inverse(rotation);
    glm::vec3 localOrigin = inverseRot * (origin - position);
    glm::vec3 localVelocity = inverseRot * velocity;
    glm::vec3 localAcceleration = inverseRot * acceleration;

    if (findParabolaRectangleIntersection(localOrigin, localVelocity, localAcceleration, xyDimensions, parabolicDistance)) {
        float localIntersectionVelocityZ = localVelocity.z + localAcceleration.z * parabolicDistance;
        glm::vec3 forward = rotation * Vectors::FRONT;
        if (localIntersectionVelocityZ > 0.0f) {
            face = MIN_Z_FACE;
            surfaceNormal = forward;
        } else {
            face = MAX_Z_FACE;
            surfaceNormal = -forward;
        }
        return true;
    }
    return false;
}

Transform Planar3DOverlay::evalRenderTransform() {
    auto transform = getTransform();
    transform.setScale(1.0f); // ignore inherited scale factor from parents
    if (glm::length2(getDimensions()) != 1.0f) {
        transform.postScale(vec3(getDimensions(), 1.0f));
    }
    return transform;
}