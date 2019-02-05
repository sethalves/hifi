//
//  ToneMappingEffect.h
//  libraries/render-utils/src
//
//  Created by Sam Gateau on 12/7/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ToneMappingEffect_h
#define hifi_ToneMappingEffect_h

#include <DependencyManager.h>
#include <NumericalConstants.h>

#include <gpu/Resource.h>
#include <gpu/Pipeline.h>
#include <render/Forward.h>
#include <render/DrawTask.h>


class ToneMappingEffect {
public:
    ToneMappingEffect();
    virtual ~ToneMappingEffect() {}

    void render(RenderArgs* args, const gpu::TexturePointer& lightingBuffer, const gpu::FramebufferPointer& destinationBuffer);

    void setExposure(float exposure);
    float getExposure() const { return _parametersBuffer.get<Parameters>()._exposure; }

    // Different tone curve available
    enum ToneCurve {
        None = 0,
        Gamma22,
        Reinhard,
        Filmic,
    };
    void setToneCurve(ToneCurve curve);
    ToneCurve getToneCurve() const { return (ToneCurve)_parametersBuffer.get<Parameters>()._toneCurve; }

    void setVisionSqueeze(float visionSqueeze);
    void setSensorToCameraTransform(glm::mat4 sensorToCameraTransform);

    // TODO -- remove these after tuning / debugging
    void setVisionSqueezeTransition(float value);
    void setVisionSqueezePerEye(float value);
    // void setVisionSqueezeSensorSpaceEyeOffset(float value);
    void setVisionSqueezeEyeOffsets(glm::mat4 value[2]);
    void setVisionSqueezeGroundPlaneY(float value);
    void setVisionSqueezeSpotlightSize(float value);

private:

    gpu::PipelinePointer _blitLightBuffer;

    // Class describing the uniform buffer with all the parameters common to the tone mapping shaders
    class Parameters {
    public:
        float _exposure = 0.0f;
        float _twoPowExposure = 1.0f;
        float _visionSqueeze = 0.0f;
        float spareA = 0.0f;
        int _toneCurve = Gamma22;
        glm::vec3 spareB;
        glm::mat4 _sensorToCameraTransform;

        // TODO -- remove these after tuning / debugging
        float _visionSqueezeTransition = 0.15f;
        int _visionSqueezePerEye = 0;
        // float _visionSqueezeSensorSpaceEyeOffset = 0.3f;
        glm::mat4 _leftEyeOffset;
        glm::mat4 _rightEyeOffset;
        float _visionSqueezeGroundPlaneY = 0.0f;
        float _visionSqueezeSpotlightSize = 0.0f;

        Parameters() {}
    };
    typedef gpu::BufferView UniformBufferView;
    gpu::BufferView _parametersBuffer;

    void init(RenderArgs* args);
};

class ToneMappingConfig : public render::Job::Config {
    Q_OBJECT
    Q_PROPERTY(float exposure MEMBER exposure WRITE setExposure);
    Q_PROPERTY(int curve MEMBER curve WRITE setCurve);
public:
    ToneMappingConfig() : render::Job::Config(true) {}

    void setExposure(float newExposure) { exposure = newExposure; emit dirty(); }
    void setCurve(int newCurve) { curve = std::max((int)ToneMappingEffect::None, std::min((int)ToneMappingEffect::Filmic, newCurve)); emit dirty(); }


    float exposure{ 0.0f };
    int curve{ ToneMappingEffect::Gamma22 };
signals:
    void dirty();
};

class ToneMappingDeferred {
public:
    // Inputs: lightingFramebuffer, destinationFramebuffer
    using Inputs = render::VaryingSet2<gpu::FramebufferPointer, gpu::FramebufferPointer>;
    using Config = ToneMappingConfig;
    using JobModel = render::Job::ModelI<ToneMappingDeferred, Inputs, Config>;

    void configure(const Config& config);
    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs);

    ToneMappingEffect _toneMappingEffect;
};

#endif // hifi_ToneMappingEffect_h
