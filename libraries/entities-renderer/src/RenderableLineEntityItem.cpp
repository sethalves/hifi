//
//  RenderableLineEntityItem.cpp
//  libraries/entities-renderer/src/
//
//  Created by Seth Alves on 5/11/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/gtx/quaternion.hpp>

#include <gpu/GPUConfig.h>
#include <gpu/Batch.h>
#include <GeometryCache.h>

#include <DeferredLightingEffect.h>
#include <PerfStat.h>

#include "RenderableLineEntityItem.h"

EntityItemPointer RenderableLineEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return EntityItemPointer(new RenderableLineEntityItem(entityID, properties));
}

void RenderableLineEntityItem::updateGeometry() {
    assertWriteLocked();
    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (_lineVerticesID == GeometryCache::UNKNOWN_ID) {
        _lineVerticesID = geometryCache ->allocateID();
    }
    if (_pointsChanged) {
        glm::vec4 lineColor(toGlm(getXColorInternal()), getLocalRenderAlphaInternal());
        geometryCache->updateVertices(_lineVerticesID, getLinePointsInternal(), lineColor);
        _pointsChanged = false;
    }
}

void RenderableLineEntityItem::render(RenderArgs* args) {
    assertUnlocked();
    lockForWrite();

    PerformanceTimer perfTimer("RenderableLineEntityItem::render");
    Q_ASSERT(getTypeInternal() == EntityTypes::Line);
    updateGeometry();

    Q_ASSERT(args->_batch);
    gpu::Batch& batch = *args->_batch;
    Transform transform = Transform();
    transform.setTranslation(getPositionInternal());
    transform.setRotation(getRotationInternal());
    batch.setModelTransform(transform);

    batch._glLineWidth(getLineWidthInternal());
    if (getLinePointsInternal().size() > 1) {
        DependencyManager::get<DeferredLightingEffect>()->bindSimpleProgram(batch);
        DependencyManager::get<GeometryCache>()->renderVertices(batch, gpu::LINE_STRIP, _lineVerticesID);
    }
    batch._glLineWidth(1.0f);

    RenderableDebugableEntityItem::render(this, args);
    unlock();
}
