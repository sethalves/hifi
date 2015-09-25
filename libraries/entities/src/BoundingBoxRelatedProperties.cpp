//
//  BoundingBoxReleatedProperties.cpp
//  libraries/entities/src
//
//  Created by Seth Alves on 2015-9-24
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "BoundingBoxRelatedProperties.h"
#include "EntityTree.h"

BoundingBoxReleatedProperties::BoundingBoxReleatedProperties(EntityItemPointer entity) :
    position(entity->getGlobalPosition()),
    rotation(entity->getGlobalRotation()),
    registrationPoint(entity->getRegistrationPoint()),
    dimensions(entity->getDimensions()),
    parentID(entity->getParentID()) {
}

BoundingBoxReleatedProperties::BoundingBoxReleatedProperties(EntityItemPointer entity,
                                                             const EntityItemProperties& propertiesWithUpdates) :
    BoundingBoxReleatedProperties(entity) {

    if (propertiesWithUpdates.parentIDChanged()) {
        parentID = propertiesWithUpdates.getParentID();
    }

    bool parentFound = false;
    if (parentID != UNKNOWN_ENTITY_ID) {
        EntityTreePointer tree = entity->getTree();
        EntityItemPointer parentZone = tree->findEntityByID(parentID);
        if (parentZone) {
            parentFound = true;
            glm::vec3 localPosition = propertiesWithUpdates.containsPositionChange() ?
                propertiesWithUpdates.getPosition() :
                entity->getPosition();

            glm::quat localRotation = propertiesWithUpdates.rotationChanged() ?
                propertiesWithUpdates.getRotation() :
                entity->getRotation();

            const Transform parentTransform = parentZone->getTransformToCenter();
            Transform parentDescaled(parentTransform.getRotation(), glm::vec3(1.0f), parentTransform.getTranslation());

            Transform localTransform(localRotation, glm::vec3(1.0f), localPosition);
            Transform result;
            Transform::mult(result, parentDescaled, localTransform);
            position = result.getTranslation();
            rotation = result.getRotation();
        }
    }

    if (!parentFound) {
        if (propertiesWithUpdates.containsPositionChange()) {
            position = propertiesWithUpdates.getPosition();
        }
        if (propertiesWithUpdates.rotationChanged()) {
            rotation = propertiesWithUpdates.getRotation();
        }
    }

    if (propertiesWithUpdates.registrationPointChanged()) {
        registrationPoint = propertiesWithUpdates.getRegistrationPoint();
    }

    if (propertiesWithUpdates.dimensionsChanged()) {
        dimensions = propertiesWithUpdates.getDimensions();
    }
}

AACube BoundingBoxReleatedProperties::getMaximumAACube() const {
    // see EntityItem::getMaximumAACube for comments which explain the following.
    glm::vec3 scaledRegistrationPoint = (dimensions * registrationPoint);
    glm::vec3 registrationRemainder = (dimensions * (glm::vec3(1.0f, 1.0f, 1.0f) - registrationPoint));
    glm::vec3 furthestExtentFromRegistration = glm::max(scaledRegistrationPoint, registrationRemainder);
    float radius = glm::length(furthestExtentFromRegistration);
    glm::vec3 minimumCorner = position - glm::vec3(radius, radius, radius);
    return AACube(minimumCorner, radius * 2.0f);
}
