//
//  LineEntityItem.h
//  libraries/entities/src
//
//  Created by Seth Alves on 5/11/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_LineEntityItem_h
#define hifi_LineEntityItem_h

#include "EntityItem.h"

class LineEntityItem : public EntityItem {
 public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    LineEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties);

    ALLOW_INSTANTIATION // This class can be instantiated

    // methods for getting/setting all properties of an entity
    virtual EntityItemProperties getProperties(bool doLocking = true) const;
    virtual bool setProperties(const EntityItemProperties& properties, bool doLocking = true);

    // TODO: eventually only include properties changed since the params.lastViewFrustumSent time
    virtual EntityPropertyFlags getEntityProperties(EncodeBitstreamParams& params) const;

    virtual void appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                    EntityTreeElementExtraEncodeData* modelTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount,
                                    OctreeElement::AppendState& appendState) const;

    virtual int readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                 ReadBitstreamToTreeParams& args,
                                                 EntityPropertyFlags& propertyFlags, bool overwriteLocalData);

    const rgbColor& getColor() const;
    xColor getXColor() const;
    void setColor(const rgbColor& value);
    void setColor(const xColor& value);

    void setLineWidth(float lineWidth);
    float getLineWidth() const;

    bool setLinePoints(const QVector<glm::vec3>& points);
    bool appendPoint(const glm::vec3& point);

    QVector<glm::vec3> getLinePoints() const;

    // never have a ray intersection pick a LineEntityItem.
    virtual bool supportsDetailedRayIntersection() const { return true; }
    virtual bool findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                             bool& keepSearching, OctreeElement*& element, float& distance, BoxFace& face,
                                             void** intersectedObject, bool precisionPicking) const { return false; }

    static const float DEFAULT_LINE_WIDTH;
    static const int MAX_POINTS_PER_LINE;

 protected:
    const rgbColor& getColorInternal() const;
    xColor getXColorInternal() const;
    void setColorInternal(const rgbColor& value);
    void setColorInternal(const xColor& value);
    void setLineWidthInternal(float lineWidth);
    float getLineWidthInternal() const;
    QVector<glm::vec3> getLinePointsInternal() const;
    virtual ShapeType getShapeTypeInternal() const { assertLocked(); return SHAPE_TYPE_LINE; }
    bool setLinePointsInternal(const QVector<glm::vec3>& points);

    virtual void debugDump() const;

    rgbColor _color;
    float _lineWidth;
    bool _pointsChanged;
    QVector<glm::vec3> _points;
};

#endif // hifi_LineEntityItem_h
