//
//  LineEntityItem.cpp
//  libraries/entities/src
//
//  Created by Seth Alves on 5/11/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include <QDebug>

#include <ByteCountCoding.h>

#include "LineEntityItem.h"
#include "EntityTree.h"
#include "EntitiesLogging.h"
#include "EntityTreeElement.h"
#include "OctreeConstants.h"



const float LineEntityItem::DEFAULT_LINE_WIDTH = 2.0f;
const int LineEntityItem::MAX_POINTS_PER_LINE = 70;


EntityItemPointer LineEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItemPointer result { new LineEntityItem(entityID, properties) };
    return result;
}

LineEntityItem::LineEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
    EntityItem(entityItemID) ,
    _lineWidth(DEFAULT_LINE_WIDTH),
    _pointsChanged(true),
    _points(QVector<glm::vec3>(0))
{
    _type = EntityTypes::Line;
    _created = properties.getCreated();
    setProperties(properties);
}

EntityItemProperties LineEntityItem::getProperties(bool doLocking) const {
    if (doLocking) {
        assertUnlocked();
        lockForRead();
    } else {
        assertLocked();
    }

    EntityItemProperties properties = EntityItem::getProperties(false); // get the properties from our base class


    properties._color = getXColorInternal();
    properties._colorChanged = false;


    COPY_ENTITY_PROPERTY_TO_PROPERTIES(lineWidth, getLineWidthInternal);

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(linePoints, getLinePointsInternal);


    properties._glowLevel = getGlowLevelInternal();
    properties._glowLevelChanged = false;

    if (doLocking) {
        unlock();
    }

    return properties;
}

bool LineEntityItem::setProperties(const EntityItemProperties& properties, bool doLocking) {
    if (doLocking) {
        assertUnlocked();
        lockForWrite();
    } else {
        assertWriteLocked();
    }

    bool somethingChanged = false;
    somethingChanged = EntityItem::setProperties(properties, false); // set the properties in our base class

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(color, setColorInternal);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(lineWidth, setLineWidthInternal);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(linePoints, setLinePointsInternal);


    if (somethingChanged) {
        bool wantDebug = false;
        if (wantDebug) {
            uint64_t now = usecTimestampNow();
            int elapsed = now - getLastEditedInternal();
            qCDebug(entities) << "LineEntityItem::setProperties() AFTER update... edited AGO=" << elapsed <<
                "now=" << now << " getLastEdited()=" << getLastEditedInternal();
        }
        setLastEditedInternal(properties._lastEdited);
    }

    if (doLocking) {
        unlock();
    }
    return somethingChanged;
}

bool LineEntityItem::appendPoint(const glm::vec3& point) {
    assertUnlocked();
    lockForWrite();
    if (_points.size() > MAX_POINTS_PER_LINE - 1) {
        qDebug() << "MAX POINTS REACHED!";
        unlock();
        return false;
    }
    glm::vec3 halfBox = getDimensions() * 0.5f;
    if ((point.x < - halfBox.x || point.x > halfBox.x) ||
        (point.y < -halfBox.y || point.y > halfBox.y) ||
        (point.z < - halfBox.z || point.z > halfBox.z)) {
        qDebug() << "Point is outside entity's bounding box";
        unlock();
        return false;
    }
    _points << point;
    _pointsChanged = true;
    unlock();
    return true;
}

bool LineEntityItem::setLinePoints(const QVector<glm::vec3>& points) {
    assertUnlocked();
    lockForWrite();
    auto result = setLinePointsInternal(points);
    unlock();
    return result;
}

bool LineEntityItem::setLinePointsInternal(const QVector<glm::vec3>& points) {
    assertWriteLocked();
    if (points.size() > MAX_POINTS_PER_LINE) {
        return false;
    }
    for (int i = 0; i < points.size(); i++) {
        glm::vec3 point = points.at(i);
        glm::vec3 halfBox = getDimensionsInternal() * 0.5f;
        if ((point.x < - halfBox.x || point.x > halfBox.x) ||
            (point.y < -halfBox.y || point.y > halfBox.y) ||
            (point.z < - halfBox.z || point.z > halfBox.z)) {
            qDebug() << "Point is outside entity's bounding box";
            return false;
        }

    }
    _points = points;
    _pointsChanged = true;
    return true;
}

int LineEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                     ReadBitstreamToTreeParams& args,
                                                     EntityPropertyFlags& propertyFlags, bool overwriteLocalData) {
    assertWriteLocked();
    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_COLOR, rgbColor, setColorInternal);
    READ_ENTITY_PROPERTY(PROP_LINE_WIDTH, float, setLineWidthInternal);
    READ_ENTITY_PROPERTY(PROP_LINE_POINTS, QVector<glm::vec3>, setLinePointsInternal);

    return bytesRead;
}

// TODO: eventually only include properties changed since the params.lastViewFrustumSent time
EntityPropertyFlags LineEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);
    requestedProperties += PROP_COLOR;
    requestedProperties += PROP_LINE_WIDTH;
    requestedProperties += PROP_LINE_POINTS;
    return requestedProperties;
}

void LineEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                        EntityTreeElementExtraEncodeData* modelTreeElementExtraEncodeData,
                                        EntityPropertyFlags& requestedProperties,
                                        EntityPropertyFlags& propertyFlags,
                                        EntityPropertyFlags& propertiesDidntFit,
                                        int& propertyCount,
                                        OctreeElement::AppendState& appendState) const {
    assertLocked();
    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_COLOR, getColorInternal());
    APPEND_ENTITY_PROPERTY(PROP_LINE_WIDTH, getLineWidthInternal());
    APPEND_ENTITY_PROPERTY(PROP_LINE_POINTS, getLinePointsInternal());
}

void LineEntityItem::debugDump() const {
    assertLocked();
    quint64 now = usecTimestampNow();
    qCDebug(entities) << "   LINE EntityItem id:" << getEntityItemIDInternal()
                      << "---------------------------------------------";
    qCDebug(entities) << "               color:" << _color[0] << "," << _color[1] << "," << _color[2];
    qCDebug(entities) << "            position:" << debugTreeVector(getPositionInternal());
    qCDebug(entities) << "          dimensions:" << debugTreeVector(getDimensionsInternal());
    qCDebug(entities) << "       getLastEdited:" << debugTime(getLastEditedInternal(), now);
}

const rgbColor& LineEntityItem::getColor() const {
    assertUnlocked();
    lockForRead();
    auto& result = getColorInternal();
    unlock();
    return result;
}

const rgbColor& LineEntityItem::getColorInternal() const {
    assertLocked();
    return _color;
}

xColor LineEntityItem::getXColor() const {
    assertUnlocked();
    lockForRead();
    auto result = getXColorInternal();
    unlock();
    return result;
}

xColor LineEntityItem::getXColorInternal() const {
    assertLocked();
    xColor color = { _color[RED_INDEX], _color[GREEN_INDEX], _color[BLUE_INDEX] };
    return color;
}

void LineEntityItem::setColor(const rgbColor& value) {
    assertUnlocked();
    lockForWrite();
    setColorInternal(value);
    unlock();
}

void LineEntityItem::setColorInternal(const rgbColor& value) {
    assertWriteLocked();
    memcpy(_color, value, sizeof(_color));
}

void LineEntityItem::setColor(const xColor& value) {
    assertUnlocked();
    lockForWrite();
    setColorInternal(value);
    unlock();
}

void LineEntityItem::setColorInternal(const xColor& value) {
    assertWriteLocked();
    _color[RED_INDEX] = value.red;
    _color[GREEN_INDEX] = value.green;
    _color[BLUE_INDEX] = value.blue;
}


void LineEntityItem::setLineWidth(float lineWidth){
    assertUnlocked();
    lockForWrite();
    setLineWidthInternal(lineWidth);
    unlock();
}

void LineEntityItem::setLineWidthInternal(float lineWidth){
    assertWriteLocked();
    _lineWidth = lineWidth;
}

float LineEntityItem::getLineWidth() const{
    assertUnlocked();
    lockForRead();
    auto result = getLineWidthInternal();
    unlock();
    return result;
}

float LineEntityItem::getLineWidthInternal() const{
    assertLocked();
    return _lineWidth;
}

QVector<glm::vec3> LineEntityItem::getLinePoints() const{
    assertUnlocked();
    lockForRead();
    auto result = getLinePointsInternal();
    unlock();
    return result;
}

QVector<glm::vec3> LineEntityItem::getLinePointsInternal() const{
    assertLocked();
    return _points;
}
