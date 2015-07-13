//
//  RenderableLightEntityItem.cpp
//  interface/src
//
//  Created by Brad Hefta-Gaub on 8/6/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/gtx/quaternion.hpp>

#include <gpu/GPUConfig.h>
#include <gpu/Batch.h>

#include <DeferredLightingEffect.h>
#include <GLMHelpers.h>
#include <PerfStat.h>

#include "RenderableLightEntityItem.h"

EntityItemPointer RenderableLightEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return EntityItemPointer(new RenderableLightEntityItem(entityID, properties));
}

void RenderableLightEntityItem::render(RenderArgs* args) {
    assertUnlocked();
    lockForRead();

    PerformanceTimer perfTimer("RenderableLightEntityItem::render");
    assert(getTypeInternal() == EntityTypes::Light);
    glm::vec3 position = getPositionInternal();
    glm::vec3 dimensions = getDimensionsInternal();
    glm::quat rotation = getRotationInternal();
    float largestDiameter = glm::max(dimensions.x, dimensions.y, dimensions.z);

    glm::vec3 color = toGlm(getXColorInternal());

    float intensity = getIntensityInternal();
    float exponent = getExponentInternal();
    float cutoff = glm::radians(getCutoffInternal());

    if (_isSpotlight) {
        DependencyManager::get<DeferredLightingEffect>()->addSpotLight(position, largestDiameter / 2.0f,
            color, intensity, rotation, exponent, cutoff);
    } else {
        DependencyManager::get<DeferredLightingEffect>()->addPointLight(position, largestDiameter / 2.0f,
            color, intensity);
    }

#ifdef WANT_DEBUG
    Q_ASSERT(args->_batch);
    gpu::Batch& batch = *args->_batch;
    batch.setModelTransform(getTransformToCenterInternal());
    DependencyManager::get<DeferredLightingEffect>()->renderWireSphere(batch, 0.5f, 15, 15, glm::vec4(color, 1.0f));
#endif

    unlock();
};

bool RenderableLightEntityItem::findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                         bool& keepSearching, OctreeElement*& element, float& distance, BoxFace& face,
                         void** intersectedObject, bool precisionPicking) const {
    // TODO: consider if this is really what we want to do. We've made it so that "lights are pickable" is a global state
    // this is probably reasonable since there's typically only one tree you'd be picking on at a time. Technically we could
    // be on the clipboard and someone might be trying to use the ray intersection API there. Anyway... if you ever try to
    // do ray intersection testing off of trees other than the main tree of the main entity renderer, then we'll need to
    // fix this mechanism.
    assertUnlocked();
    lockForRead();
    auto result = _lightsArePickable;
    unlock();
    return result;
}
