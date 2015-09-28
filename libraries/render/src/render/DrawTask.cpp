//
//  DrawTask.cpp
//  render/src/render
//
//  Created by Sam Gateau on 5/21/15.
//  Copyright 20154 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DrawTask.h"

#include <algorithm>
#include <assert.h>

#include <PerfStat.h>
#include <RenderArgs.h>
#include <ViewFrustum.h>
#include <gpu/Context.h>


using namespace render;

DrawSceneTask::DrawSceneTask() : Task() {
}

DrawSceneTask::~DrawSceneTask() {
}

void DrawSceneTask::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    // sanity checks
    assert(sceneContext);
    if (!sceneContext->_scene) {
        return;
    }


    // Is it possible that we render without a viewFrustum ?
    if (!(renderContext->args && renderContext->args->_viewFrustum)) {
        return;
    }

    for (auto job : _jobs) {
        job.run(sceneContext, renderContext);
    }
};

Job::~Job() {
}





void render::cullItems(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemIDsBounds& inItems, ItemIDsBounds& outItems) {
    assert(renderContext->args);
    assert(renderContext->args->_viewFrustum);

    RenderArgs* args = renderContext->args;
    auto renderDetails = renderContext->args->_details._item;

    renderDetails->_considered += inItems.size();
    
    // Culling / LOD
    for (auto item : inItems) {
        if (item.bounds.isNull()) {
            outItems.emplace_back(item); // One more Item to render
            continue;
        }

        // TODO: some entity types (like lights) might want to be rendered even
        // when they are outside of the view frustum...
        bool outOfView;
        {
            PerformanceTimer perfTimer("boxInFrustum");
            outOfView = args->_viewFrustum->boxInFrustum(item.bounds) == ViewFrustum::OUTSIDE;
        }
        if (!outOfView) {
            bool bigEnoughToRender;
            {
                PerformanceTimer perfTimer("shouldRender");
                bigEnoughToRender = (args->_shouldRender) ? args->_shouldRender(args, item.bounds) : true;
            }
            if (bigEnoughToRender) {
                outItems.emplace_back(item); // One more Item to render
            } else {
                renderDetails->_tooSmall++;
            }
        } else {
            renderDetails->_outOfView++;
        }
    }
    renderDetails->_rendered += outItems.size();
}


void FetchItems::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, ItemIDsBounds& outItems) {
    auto& scene = sceneContext->_scene;
    auto& items = scene->getMasterBucket().at(_filter);

    outItems.clear();
    outItems.reserve(items.size());
    for (auto id : items) {
        auto& item = scene->getItem(id);
        outItems.emplace_back(ItemIDAndBounds(id, item.getBound()));
    }

    if (_probeNumItems) {
        _probeNumItems(renderContext, outItems.size());
    }
}

void CullItems::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemIDsBounds& inItems, ItemIDsBounds& outItems) {

    outItems.clear();
    outItems.reserve(inItems.size());
    cullItems(sceneContext, renderContext, inItems, outItems);
}


struct ItemBound {
    float _centerDepth = 0.0f;
    float _nearDepth = 0.0f;
    float _farDepth = 0.0f;
    ItemID _id = 0;
    AABox _bounds;

    ItemBound() {}
    ItemBound(float centerDepth, float nearDepth, float farDepth, ItemID id, const AABox& bounds) : _centerDepth(centerDepth), _nearDepth(nearDepth), _farDepth(farDepth), _id(id), _bounds(bounds) {}
};

struct FrontToBackSort {
    bool operator() (const ItemBound& left, const ItemBound& right) {
        return (left._centerDepth < right._centerDepth);
    }
};

struct BackToFrontSort {
    bool operator() (const ItemBound& left, const ItemBound& right) {
        return (left._centerDepth > right._centerDepth);
    }
};

void render::depthSortItems(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, bool frontToBack, const ItemIDsBounds& inItems, ItemIDsBounds& outItems) {
    assert(renderContext->args);
    assert(renderContext->args->_viewFrustum);
    
    auto& scene = sceneContext->_scene;
    RenderArgs* args = renderContext->args;
    

    // Allocate and simply copy
    outItems.clear();
    outItems.reserve(inItems.size());


    // Make a local dataset of the center distance and closest point distance
    std::vector<ItemBound> itemBounds;
    itemBounds.reserve(outItems.size());

    for (auto itemDetails : inItems) {
        auto item = scene->getItem(itemDetails.id);
        auto bound = itemDetails.bounds; // item.getBound();
        float distance = args->_viewFrustum->distanceToCamera(bound.calcCenter());

        itemBounds.emplace_back(ItemBound(distance, distance, distance, itemDetails.id, bound));
    }

    // sort against Z
    if (frontToBack) {
        FrontToBackSort frontToBackSort;
        std::sort (itemBounds.begin(), itemBounds.end(), frontToBackSort); 
    } else {
        BackToFrontSort  backToFrontSort;
        std::sort (itemBounds.begin(), itemBounds.end(), backToFrontSort); 
    }

    // FInally once sorted result to a list of itemID
    for (auto& itemBound : itemBounds) {
       outItems.emplace_back(ItemIDAndBounds(itemBound._id, itemBound._bounds));
    }
}


void DepthSortItems::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemIDsBounds& inItems, ItemIDsBounds& outItems) {
    outItems.clear();
    outItems.reserve(inItems.size());
    depthSortItems(sceneContext, renderContext, _frontToBack, inItems, outItems);
}

void render::renderItems(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemIDsBounds& inItems, int maxDrawnItems) {
    auto& scene = sceneContext->_scene;
    RenderArgs* args = renderContext->args;
    // render
    if ((maxDrawnItems < 0) || (maxDrawnItems > (int) inItems.size())) {
        for (auto itemDetails : inItems) {
            auto item = scene->getItem(itemDetails.id);
            item.render(args);
        }
    } else {
        int numItems = 0;
        for (auto itemDetails : inItems) {
            auto item = scene->getItem(itemDetails.id);
            if (numItems + 1 >= maxDrawnItems) {
                item.render(args);
                return;
            }
            item.render(args);
            numItems++;
            if (numItems >= maxDrawnItems) {
                return;
            }
        }
    }
}

void DrawLight::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    assert(renderContext->args);
    assert(renderContext->args->_viewFrustum);

    // render lights
    auto& scene = sceneContext->_scene;
    auto& items = scene->getMasterBucket().at(ItemFilter::Builder::light());


    ItemIDsBounds inItems;
    inItems.reserve(items.size());
    for (auto id : items) {
        auto item = scene->getItem(id);
        inItems.emplace_back(ItemIDAndBounds(id, item.getBound()));
    }

    ItemIDsBounds culledItems;
    culledItems.reserve(inItems.size());
    cullItems(sceneContext, renderContext, inItems, culledItems);

    RenderArgs* args = renderContext->args;
    gpu::doInBatch(args->_context, [=](gpu::Batch& batch) {
        args->_batch = &batch;
        renderItems(sceneContext, renderContext, culledItems);
    });
    args->_batch = nullptr;
}

void DrawBackground::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    assert(renderContext->args);
    assert(renderContext->args->_viewFrustum);

    // render backgrounds
    auto& scene = sceneContext->_scene;
    auto& items = scene->getMasterBucket().at(ItemFilter::Builder::background());


    ItemIDsBounds inItems;
    inItems.reserve(items.size());
    for (auto id : items) {
        inItems.emplace_back(id);
    }
    RenderArgs* args = renderContext->args;
    doInBatch(args->_context, [=](gpu::Batch& batch) {
        args->_batch = &batch;
        batch.enableSkybox(true);
        batch.setViewportTransform(args->_viewport);
        batch.setStateScissorRect(args->_viewport);

        glm::mat4 projMat;
        Transform viewMat;
        args->_viewFrustum->evalProjectionMatrix(projMat);
        args->_viewFrustum->evalViewTransform(viewMat);

        batch.setProjectionTransform(projMat);
        batch.setViewTransform(viewMat);

        renderItems(sceneContext, renderContext, inItems);
    });
    args->_batch = nullptr;
}

void ItemMaterialBucketMap::insert(const ItemID& id, const model::MaterialKey& key) {
    // Insert the itemID in every bucket where it filters true
    for (auto& bucket : (*this)) {
        if (bucket.first.test(key)) {
            bucket.second.push_back(id);
        }
    }
}

void ItemMaterialBucketMap::allocateStandardMaterialBuckets() {
    (*this)[model::MaterialFilter::Builder::opaqueDiffuse()];
}
