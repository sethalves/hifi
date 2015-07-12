//
//  Created by Bradley Austin Davis on 2015/05/12
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "WebEntityItem.h"

#include <glm/gtx/transform.hpp>

#include <QDebug>

#include <ByteCountCoding.h>
#include <PlaneShape.h>

#include "EntityTree.h"
#include "EntityTreeElement.h"
#include "EntitiesLogging.h"


const QString WebEntityItem::DEFAULT_SOURCE_URL("http://www.google.com");

EntityItemPointer WebEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return EntityItemPointer(new WebEntityItem(entityID, properties));
}

WebEntityItem::WebEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        EntityItem(entityItemID)
{
    _type = EntityTypes::Web;
    _created = properties.getCreated();
    setProperties(properties);
}

const float WEB_ENTITY_ITEM_FIXED_DEPTH = 0.01f;

void WebEntityItem::setDimensions(const glm::vec3& value) {
    // NOTE: Web Entities always have a "depth" of 1cm.
    EntityItem::setDimensions(glm::vec3(value.x, value.y, WEB_ENTITY_ITEM_FIXED_DEPTH));
}

EntityItemProperties WebEntityItem::getProperties(bool doLocking) const {
    if (doLocking) {
        assertUnlocked();
        lockForRead();
    } else {
        assertLocked();
    }
    EntityItemProperties properties = EntityItem::getProperties(false); // get the properties from our base class
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(sourceUrl, getSourceUrlInternal);

    if (doLocking) {
        unlock();
    }

    return properties;
}

bool WebEntityItem::setProperties(const EntityItemProperties& properties, bool doLocking) {
    if (doLocking) {
        assertUnlocked();
        lockForWrite();
    } else {
        assertWriteLocked();
    }

    bool somethingChanged = false;
    somethingChanged = EntityItem::setProperties(properties, false); // set the properties in our base class

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(sourceUrl, setSourceUrlInternal);

    if (somethingChanged) {
        bool wantDebug = false;
        if (wantDebug) {
            uint64_t now = usecTimestampNow();
            int elapsed = now - getLastEditedInternal();
            qCDebug(entities) << "WebEntityItem::setProperties() AFTER update... edited AGO=" << elapsed <<
                    "now=" << now << " getLastEdited()=" << getLastEditedInternal();
        }
        setLastEditedInternal(properties._lastEdited);
    }

    if (doLocking) {
        unlock();
    }
    return somethingChanged;
}

int WebEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData) {
    assertWriteLocked();
    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_SOURCE_URL, QString, setSourceUrlInternal);

    return bytesRead;
}


// TODO: eventually only include properties changed since the params.lastViewFrustumSent time
EntityPropertyFlags WebEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);
    requestedProperties += PROP_SOURCE_URL;
    return requestedProperties;
}

void WebEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                    EntityTreeElementExtraEncodeData* modelTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount,
                                    OctreeElement::AppendState& appendState) const {
    assertLocked();
    bool successPropertyFits = true;
    APPEND_ENTITY_PROPERTY(PROP_SOURCE_URL, _sourceUrl);
}


bool WebEntityItem::findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                     bool& keepSearching, OctreeElement*& element, float& distance, BoxFace& face,
                     void** intersectedObject, bool precisionPicking) const {
    assertUnlocked();
    lockForRead();

    RayIntersectionInfo rayInfo;
    rayInfo._rayStart = origin;
    rayInfo._rayDirection = direction;
    rayInfo._rayLength = std::numeric_limits<float>::max();

    PlaneShape plane;

    const glm::vec3 UNROTATED_NORMAL(0.0f, 0.0f, -1.0f);
    glm::vec3 normal = getRotationInternal() * UNROTATED_NORMAL;
    plane.setNormal(normal);
    plane.setPoint(getPositionInternal()); // the position is definitely a point on our plane

    bool intersects = plane.findRayIntersection(rayInfo);

    if (intersects) {
        glm::vec3 hitAt = origin + (direction * rayInfo._hitDistance);
        // now we know the point the ray hit our plane

        glm::mat4 rotation = glm::mat4_cast(getRotationInternal());
        glm::mat4 translation = glm::translate(getPositionInternal());
        glm::mat4 entityToWorldMatrix = translation * rotation;
        glm::mat4 worldToEntityMatrix = glm::inverse(entityToWorldMatrix);

        glm::vec3 dimensions = getDimensionsInternal();
        glm::vec3 registrationPoint = getRegistrationPointInternal();
        glm::vec3 corner = -(dimensions * registrationPoint);
        AABox entityFrameBox(corner, dimensions);

        glm::vec3 entityFrameHitAt = glm::vec3(worldToEntityMatrix * glm::vec4(hitAt, 1.0f));

        intersects = entityFrameBox.contains(entityFrameHitAt);
    }

    if (intersects) {
        distance = rayInfo._hitDistance;
    }
    unlock();
    return intersects;
}

void WebEntityItem::setSourceUrl(const QString& value) {
    assertUnlocked();
    lockForWrite();
    setSourceUrlInternal(value);
    unlock();
}

void WebEntityItem::setSourceUrlInternal(const QString& value) {
    assertWriteLocked();
    if (_sourceUrl != value) {
        _sourceUrl = value;
    }
}

QString WebEntityItem::getSourceUrl() const {
    assertUnlocked();
    lockForRead();
    auto result = getSourceUrlInternal();
    unlock();
    return result;
}

QString WebEntityItem::getSourceUrlInternal() const {
    assertLocked();
    return _sourceUrl;
}
