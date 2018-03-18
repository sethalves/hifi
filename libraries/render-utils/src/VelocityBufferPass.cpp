//
//  VelocityBufferPass.cpp
//  libraries/render-utils/src/
//
//  Created by Sam Gateau 8/15/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "VelocityBufferPass.h"

#include <limits>

#include <gpu/Context.h>
#include <gpu/StandardShaderLib.h>
#include "StencilMaskPass.h"

const int VelocityBufferPass_FrameTransformSlot = 0;
const int VelocityBufferPass_DepthMapSlot = 0;


#include "velocityBuffer_cameraMotion_frag.h"

VelocityFramebuffer::VelocityFramebuffer() {
}


void VelocityFramebuffer::updatePrimaryDepth(const gpu::TexturePointer& depthBuffer) {
    //If the depth buffer or size changed, we need to delete our FBOs
    bool reset = false;
    if ((_primaryDepthTexture != depthBuffer)) {
        _primaryDepthTexture = depthBuffer;
        reset = true;
    }
    if (_primaryDepthTexture) {
        auto newFrameSize = glm::ivec2(_primaryDepthTexture->getDimensions());
        if (_frameSize != newFrameSize) {
            _frameSize = newFrameSize;
            _halfFrameSize = newFrameSize >> 1;

            reset = true;
        }
    }

    if (reset) {
        clear();
    }
}

void VelocityFramebuffer::clear() {
    _velocityFramebuffer.reset();
    _velocityTexture.reset();
}

void VelocityFramebuffer::allocate() {

    auto width = _frameSize.x;
    auto height = _frameSize.y;

    // For Velocity Buffer:
    _velocityTexture = gpu::Texture::createRenderBuffer(gpu::Element(gpu::VEC2, gpu::HALF, gpu::RGB), width, height, gpu::Texture::SINGLE_MIP,
        gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_LINEAR));
    _velocityFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create("velocity"));
    _velocityFramebuffer->setRenderBuffer(0, _velocityTexture);
    _velocityFramebuffer->setDepthStencilBuffer(_primaryDepthTexture, _primaryDepthTexture->getTexelFormat());
}

gpu::FramebufferPointer VelocityFramebuffer::getVelocityFramebuffer() {
    if (!_velocityFramebuffer) {
        allocate();
    }
    return _velocityFramebuffer;
}

gpu::TexturePointer VelocityFramebuffer::getVelocityTexture() {
    if (!_velocityTexture) {
        allocate();
    }
    return _velocityTexture;
}

VelocityBufferPass::VelocityBufferPass() {
}

void VelocityBufferPass::configure(const Config& config) {
}

void VelocityBufferPass::run(const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());

    RenderArgs* args = renderContext->args;

    const auto& frameTransform = inputs.get0();
    const auto& deferredFramebuffer = inputs.get1();

    if (!_gpuTimer) {
        _gpuTimer = std::make_shared < gpu::RangeTimer>(__FUNCTION__);
    }

    if (!_velocityFramebuffer) {
        _velocityFramebuffer = std::make_shared<VelocityFramebuffer>();
    }
    _velocityFramebuffer->updatePrimaryDepth(deferredFramebuffer->getPrimaryDepthTexture());

    auto depthBuffer = deferredFramebuffer->getPrimaryDepthTexture();

    auto velocityFBO = _velocityFramebuffer->getVelocityFramebuffer();
    auto velocityTexture = _velocityFramebuffer->getVelocityTexture();

    outputs.edit0() = _velocityFramebuffer;
    outputs.edit1() = velocityFBO;
    outputs.edit2() = velocityTexture;
   
    auto cameraMotionPipeline = getCameraMotionPipeline();

    auto fullViewport = args->_viewport;

    gpu::doInBatch("VelocityBufferPass::run", args->_context, [=](gpu::Batch& batch) {
        _gpuTimer->begin(batch);
        batch.enableStereo(false);

        batch.setViewportTransform(fullViewport);
        batch.setProjectionTransform(glm::mat4());
        batch.resetViewTransform();
        batch.setModelTransform(gpu::Framebuffer::evalSubregionTexcoordTransform(_velocityFramebuffer->getDepthFrameSize(), fullViewport));

        batch.setUniformBuffer(VelocityBufferPass_FrameTransformSlot, frameTransform->getFrameTransformBuffer());

        // Velocity buffer camera motion
        batch.setFramebuffer(velocityFBO);
        batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLOR0, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
        batch.setPipeline(cameraMotionPipeline);
        batch.setResourceTexture(VelocityBufferPass_DepthMapSlot, depthBuffer);
        batch.draw(gpu::TRIANGLE_STRIP, 4);

        _gpuTimer->end(batch);
    });

    auto config = std::static_pointer_cast<Config>(renderContext->jobConfig);
    config->setGPUBatchRunTime(_gpuTimer->getGPUAverage(), _gpuTimer->getBatchAverage());
}


const gpu::PipelinePointer& VelocityBufferPass::getCameraMotionPipeline() {
    if (!_cameraMotionPipeline) {
        auto vs = gpu::StandardShaderLib::getDrawViewportQuadTransformTexcoordVS();
        auto ps = velocityBuffer_cameraMotion_frag::getShader();
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("deferredFrameTransformBuffer"), VelocityBufferPass_FrameTransformSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("depthMap"), VelocityBufferPass_DepthMapSlot));
        gpu::Shader::makeProgram(*program, slotBindings);


        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        // Stencil test the curvature pass for objects pixels only, not the background
       // PrepareStencil::testShape(*state);

        state->setColorWriteMask(true, true, false, false);
        
        // Good to go add the brand new pipeline
        _cameraMotionPipeline = gpu::Pipeline::create(program, state);
    }

    return _cameraMotionPipeline;
}



