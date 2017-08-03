//
//  Created by Bradley Austin Davis on 2016/05/09
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderableShapeEntityItem_h
#define hifi_RenderableShapeEntityItem_h

#include <ShapeEntityItem.h>
#include <procedural/Procedural.h>

#include "RenderableEntityItem.h"

class RenderableShapeEntityItem : public ShapeEntityItem, private SimplerRenderableEntitySupport {
    using Pointer = std::shared_ptr<RenderableShapeEntityItem>;
    static Pointer baseFactory(const EntityItemID& entityID, const EntityItemProperties& properties);
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);
    static EntityItemPointer boxFactory(const EntityItemID& entityID, const EntityItemProperties& properties);
    static EntityItemPointer sphereFactory(const EntityItemID& entityID, const EntityItemProperties& properties);
    RenderableShapeEntityItem(const EntityItemID& entityItemID) : ShapeEntityItem(entityItemID) {}

    void render(RenderArgs* args) override;
    void setUserData(const QString& value) override;

    bool isTransparent() override;

    virtual void computeShapeInfo(ShapeInfo& info) override;
    ShapeType getShapeType() const override;


private:
    std::unique_ptr<Procedural> _procedural { nullptr };

    //! This is SHAPE_TYPE_ELLIPSOID rather than SHAPE_TYPE_NONE to maintain
    //! prior functionality where new or unsupported shapes are treated as
    //! ellipsoids.
    ShapeType _collisionShapeType{ ShapeType::SHAPE_TYPE_ELLIPSOID };

    SIMPLE_RENDERABLE();
};


#endif // hifi_RenderableShapeEntityItem_h
