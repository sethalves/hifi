//
//  BoundingBoxReleatedProperties.h
//  libraries/entities/src
//
//  Created by Seth Alves on 2015-9-24
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "EntityItem.h"

#ifndef hifi_BoundingBoxReleatedProperties_h
#define hifi_BoundingBoxReleatedProperties_h

class BoundingBoxReleatedProperties {
 public:
    BoundingBoxReleatedProperties(EntityItemPointer entity);
    BoundingBoxReleatedProperties(EntityItemPointer entity, const EntityItemProperties& propertiesWithUpdates);
    AACube getMaximumAACube() const;

    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 registrationPoint;
    glm::vec3 dimensions;
    EntityItemID parentID;
};

#endif // hifi_BoundingBoxReleatedProperties_h
