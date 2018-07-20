
//
//  RenderForwardTask.cpp
//  render-utils/src/
//
//  Created by Zach Pomerantz on 12/13/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RenderForwardTask.h"

#include <PerfStat.h>
#include <PathUtils.h>
#include <ViewFrustum.h>
#include <gpu/Context.h>
#include <gpu/Texture.h>
#include <gpu/StandardShaderLib.h>

#include <render/ShapePipeline.h>

#include <render/FilterTask.h>

#include "RenderHifi.h"
#include "StencilMaskPass.h"
#include "ZoneRenderer.h"
#include "FadeEffect.h"
#include "ToneMappingEffect.h"
#include "BackgroundStage.h"
#include "FramebufferCache.h"
#include "TextureCache.h"
#include "RenderCommonTask.h"
#include "LightStage.h"

#include "nop_frag.h"

using namespace render;
extern void initForwardPipelines(ShapePlumber& plumber);

void RenderForwardTask::build(JobModel& task, const render::Varying& input, render::Varying& output) {
    auto items = input.get<Input>();
    auto fadeEffect = DependencyManager::get<FadeEffect>();

    // Prepare the ShapePipelines
    ShapePlumberPointer shapePlumber = std::make_shared<ShapePlumber>();
    initForwardPipelines(*shapePlumber);

    // Extract opaques / transparents / lights / metas / overlays / background
    const auto& opaques = items.get0()[RenderFetchCullSortTask::OPAQUE_SHAPE];
    const auto& transparents = items.get0()[RenderFetchCullSortTask::TRANSPARENT_SHAPE];
    //    const auto& lights = items.get0()[RenderFetchCullSortTask::LIGHT];
    const auto& metas = items.get0()[RenderFetchCullSortTask::META];
    const auto& overlayOpaques = items.get0()[RenderFetchCullSortTask::OVERLAY_OPAQUE_SHAPE];
    const auto& overlayTransparents = items.get0()[RenderFetchCullSortTask::OVERLAY_TRANSPARENT_SHAPE];

    //const auto& background = items.get0()[RenderFetchCullSortTask::BACKGROUND];
    //    const auto& spatialSelection = items[1];

    fadeEffect->build(task, opaques);

    // Prepare objects shared by several jobs
    const auto deferredFrameTransform = task.addJob<GenerateDeferredFrameTransform>("DeferredFrameTransform");
    const auto lightingModel = task.addJob<MakeLightingModel>("LightingModel");

    // Filter zones from the general metas bucket
    const auto zones = task.addJob<ZoneRendererTask>("ZoneRenderer", metas);

    // GPU jobs: Start preparing the main framebuffer
    const auto framebuffer = task.addJob<PrepareFramebuffer>("PrepareFramebuffer");

    task.addJob<PrepareForward>("PrepareForward", lightingModel);

    // draw a stencil mask in hidden regions of the framebuffer.
    task.addJob<PrepareStencil>("PrepareStencil", framebuffer);

    // Layered Overlays
    const auto filteredOverlaysOpaque = task.addJob<FilterLayeredItems>("FilterOverlaysLayeredOpaque", overlayOpaques, render::hifi::LAYER_3D_FRONT);
    const auto filteredOverlaysTransparent = task.addJob<FilterLayeredItems>("FilterOverlaysLayeredTransparent", overlayTransparents, render::hifi::LAYER_3D_FRONT);
    const auto overlaysInFrontOpaque = filteredOverlaysOpaque.getN<FilterLayeredItems::Outputs>(0);
    const auto overlaysInFrontTransparent = filteredOverlaysTransparent.getN<FilterLayeredItems::Outputs>(0);

    const auto overlayInFrontOpaquesInputs = DrawOverlay3D::Inputs(overlaysInFrontOpaque, lightingModel, nullptr).asVarying();
    const auto overlayInFrontTransparentsInputs = DrawOverlay3D::Inputs(overlaysInFrontTransparent, lightingModel, nullptr).asVarying();
    task.addJob<DrawOverlay3D>("DrawOverlayInFrontOpaque", overlayInFrontOpaquesInputs, true);
    task.addJob<DrawOverlay3D>("DrawOverlayInFrontTransparent", overlayInFrontTransparentsInputs, false);

    // Draw opaques forward
    const auto opaqueInputs = DrawForward::Inputs(opaques, lightingModel).asVarying();
    task.addJob<DrawForward>("DrawOpaques", opaqueInputs, shapePlumber);

    // Similar to light stage, background stage has been filled by several potential render items and resolved for the frame in this job
    task.addJob<DrawBackgroundStage>("DrawBackgroundDeferred", lightingModel);

    // Draw transparent objects forward
    const auto transparentInputs = DrawForward::Inputs(transparents, lightingModel).asVarying();
    task.addJob<DrawForward>("DrawTransparents", transparentInputs, shapePlumber);

    {  // Debug the bounds of the rendered items, still look at the zbuffer

        task.addJob<DrawBounds>("DrawMetaBounds", metas);
        task.addJob<DrawBounds>("DrawBounds", opaques);
        task.addJob<DrawBounds>("DrawTransparentBounds", transparents);

        task.addJob<DrawBounds>("DrawZones", zones);
        task.addJob<DebugZoneLighting>("DrawZoneStack", deferredFrameTransform);

    }

    // Lighting Buffer ready for tone mapping
    // Forward rendering on GLES doesn't support tonemapping to and from the same FBO, so we specify 
    // the output FBO as null, which causes the tonemapping to target the blit framebuffer
    const auto toneMappingInputs = ToneMappingDeferred::Inputs(framebuffer, static_cast<gpu::FramebufferPointer>(nullptr) ).asVarying();
    task.addJob<ToneMappingDeferred>("ToneMapping", toneMappingInputs);

    // Layered Overlays
    // Composite the HUD and HUD overlays
    task.addJob<CompositeHUD>("HUD");

    // Disable blit because we do tonemapping and compositing directly to the blit FBO
    // Blit!
    // task.addJob<Blit>("Blit", framebuffer);
}

void PrepareFramebuffer::run(const RenderContextPointer& renderContext, gpu::FramebufferPointer& framebuffer) {
    glm::uvec2 frameSize(renderContext->args->_viewport.z, renderContext->args->_viewport.w);

    // Resizing framebuffers instead of re-building them seems to cause issues with threaded rendering
    if (_framebuffer && _framebuffer->getSize() != frameSize) {
        _framebuffer.reset();
    }

    if (!_framebuffer) {
        _framebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create("forward"));

        auto colorFormat = gpu::Element::COLOR_SRGBA_32;
        auto defaultSampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_POINT);
        auto colorTexture =
            gpu::Texture::createRenderBuffer(colorFormat, frameSize.x, frameSize.y, gpu::Texture::SINGLE_MIP, defaultSampler);
        _framebuffer->setRenderBuffer(0, colorTexture);

        auto depthFormat = gpu::Element(gpu::SCALAR, gpu::UINT32, gpu::DEPTH_STENCIL);  // Depth24_Stencil8 texel format
        auto depthTexture =
            gpu::Texture::createRenderBuffer(depthFormat, frameSize.x, frameSize.y, gpu::Texture::SINGLE_MIP, defaultSampler);
        _framebuffer->setDepthStencilBuffer(depthTexture, depthFormat);
    }

    auto args = renderContext->args;
    gpu::doInBatch("PrepareFramebuffer::run", args->_context, [&](gpu::Batch& batch) {
        batch.enableStereo(false);
        batch.setViewportTransform(args->_viewport);
        batch.setStateScissorRect(args->_viewport);

        batch.setFramebuffer(_framebuffer);
        batch.clearFramebuffer(gpu::Framebuffer::BUFFER_COLOR0 | gpu::Framebuffer::BUFFER_DEPTH |
            gpu::Framebuffer::BUFFER_STENCIL,
            vec4(vec3(0), 0), 1.0, 0, true);
    });

    framebuffer = _framebuffer;
}

void PrepareForward::run(const RenderContextPointer& renderContext, const Inputs& inputs) {
    RenderArgs* args = renderContext->args;

    gpu::doInBatch("RenderForward::Draw::run", args->_context, [&](gpu::Batch& batch) {
        args->_batch = &batch;

        graphics::LightPointer keySunLight;
        auto lightStage = args->_scene->getStage<LightStage>();
        if (lightStage) {
            keySunLight = lightStage->getCurrentKeyLight();
        }

        graphics::LightPointer keyAmbiLight;
        if (lightStage) {
            keyAmbiLight = lightStage->getCurrentAmbientLight();
        }

        if (keySunLight) {
            batch.setUniformBuffer(render::ShapePipeline::Slot::KEY_LIGHT, keySunLight->getLightSchemaBuffer());
        }

        if (keyAmbiLight) {
            batch.setUniformBuffer(render::ShapePipeline::Slot::LIGHT_AMBIENT_BUFFER, keyAmbiLight->getAmbientSchemaBuffer());

            if (keyAmbiLight->getAmbientMap()) {
                batch.setResourceTexture(render::ShapePipeline::Slot::LIGHT_AMBIENT_MAP, keyAmbiLight->getAmbientMap());
            }
        }
    });
}

void DrawForward::run(const RenderContextPointer& renderContext, const Inputs& inputs) {
    RenderArgs* args = renderContext->args;

    const auto& inItems = inputs.get0();
    const auto& lightingModel = inputs.get1();

    gpu::doInBatch("DrawForward::run", args->_context, [&](gpu::Batch& batch) {
        args->_batch = &batch;


        // Setup projection
        glm::mat4 projMat;
        Transform viewMat;
        args->getViewFrustum().evalProjectionMatrix(projMat);
        args->getViewFrustum().evalViewTransform(viewMat);
        batch.setProjectionTransform(projMat);
        batch.setViewTransform(viewMat);
        batch.setModelTransform(Transform());

        // Setup lighting model for all items;
        batch.setUniformBuffer(render::ShapePipeline::Slot::LIGHTING_MODEL, lightingModel->getParametersBuffer());

        // From the lighting model define a global shapeKey ORED with individiual keys
        ShapeKey::Builder keyBuilder;
        if (lightingModel->isWireframeEnabled()) {
            keyBuilder.withWireframe();
        }
        ShapeKey globalKey = keyBuilder.build();
        args->_globalShapeKey = globalKey._flags.to_ulong();

        // Render items
        renderStateSortShapes(renderContext, _shapePlumber, inItems, -1, globalKey);

        args->_batch = nullptr;
        args->_globalShapeKey = 0;
    });
}


