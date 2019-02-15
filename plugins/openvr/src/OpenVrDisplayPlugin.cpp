//
//  Created by Bradley Austin Davis on 2015/05/12
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OpenVrDisplayPlugin.h"

// Odd ordering of header is required to avoid 'macro redinition warnings'
#include <AudioClient.h>

#include <QtCore/QThread>
#include <QtCore/QLoggingCategory>
#include <QtCore/QFileInfo>
#include <QtCore/QDateTime>

#include <GLMHelpers.h>

#include <gl/Context.h>
#include <gl/GLShaders.h>

#include <gpu/Frame.h>
#include <gpu/gl/GLBackend.h>

#include <ViewFrustum.h>
#include <PathUtils.h>
#include <shared/NsightHelpers.h>
#include <controllers/Pose.h>
#include <display-plugins/CompositorHelper.h>
#include <ui-plugins/PluginContainer.h>
#include <gl/OffscreenGLCanvas.h>

#include "OpenVrHelpers.h"

Q_DECLARE_LOGGING_CATEGORY(displayplugins)

const char* OpenVrThreadedSubmit{ "OpenVR Threaded Submit" };  // this probably shouldn't be hardcoded here

PoseData _nextRenderPoseData;
PoseData _nextSimPoseData;

#define MIN_CORES_FOR_NORMAL_RENDER 5
bool forceInterleavedReprojection = (QThread::idealThreadCount() < MIN_CORES_FOR_NORMAL_RENDER);

static std::array<vr::Hmd_Eye, 2> VR_EYES{ { vr::Eye_Left, vr::Eye_Right } };
bool _openVrDisplayActive{ false };
// Flip y-axis since GL UV coords are backwards.
static vr::VRTextureBounds_t OPENVR_TEXTURE_BOUNDS_LEFT{ 0, 0, 0.5f, 1 };
static vr::VRTextureBounds_t OPENVR_TEXTURE_BOUNDS_RIGHT{ 0.5f, 0, 1, 1 };

#define REPROJECTION_BINDING 1

struct Reprojection {
    mat4 projections[2];
    mat4 inverseProjections[2];
    mat4 reprojection;

    float visionSqueezeX;
    float visionSqueezeY;
    float ipd;
    float spareB;
    mat4 hmdSensorMatrix;
    float visionSqueezeTransition;
    int visionSqueezePerEye;
    float visionSqueezeGroundPlaneY;
    float visionSqueezeSpotlightSize;
};

glm::mat4 hmd34_to_mat4(const vr::HmdMatrix34_t &m) {
    return glm::mat4(
        m.m[0][0], m.m[1][0], m.m[2][0], 0.f,
        m.m[0][1], m.m[1][1], m.m[2][1], 0.f,
        m.m[0][2], m.m[1][2], m.m[2][2], 0.f,
        m.m[0][3], m.m[1][3], m.m[2][3], 1.f);
}

class OpenVrSubmitThread : public QThread, public Dependency {
public:
    using Mutex = std::mutex;
    using Condition = std::condition_variable;
    using Lock = std::unique_lock<Mutex>;
    friend class OpenVrDisplayPlugin;
    std::shared_ptr<gl::OffscreenContext> _canvas;

    OpenVrSubmitThread(OpenVrDisplayPlugin& plugin) : _plugin(plugin) { setObjectName("OpenVR Submit Thread"); }

    void updateSource() {
        _plugin.withNonPresentThreadLock([&] {
            while (!_queue.empty()) {
                auto& front = _queue.front();

                auto result = glClientWaitSync((GLsync)front.fence, 0, 0);

                if (GL_TIMEOUT_EXPIRED == result || GL_WAIT_FAILED == result) {
                    break;
                } else if (GL_CONDITION_SATISFIED == result || GL_ALREADY_SIGNALED == result) {
                    glDeleteSync((GLsync)front.fence);
                } else {
                    assert(false);
                }

                front.fence = 0;
                _current = front;
                _queue.pop();
            }
        });
    }

    GLuint _program{ 0 };

    void updateProgram() {
        if (!_program) {
            gpu::ShaderPointer program = gpu::Shader::createProgram(shader::display_plugins::program::OpenVRReproject);
            uint32_t vertexShaderID = shader::getVertexId(program->getID());
            uint32_t fragmentShaderID = shader::getFragmentId(program->getID());
            shader::Dialect dialect = shader::Dialect::glsl450;
            shader::Variant variant = shader::Variant::Stereo;

            std::string vsSource = gpu::Shader::getVertexShaderSource(vertexShaderID).getSource(dialect, variant);
            std::string fsSource = gpu::Shader::getFragmentShaderSource(fragmentShaderID).getSource(dialect, variant);
            GLuint vertexShader{ 0 }, fragmentShader{ 0 };
            std::string error;
            ::gl::compileShader(GL_VERTEX_SHADER, vsSource, vertexShader, error);
            ::gl::compileShader(GL_FRAGMENT_SHADER, fsSource, fragmentShader, error);
            _program = ::gl::buildProgram({ { vertexShader, fragmentShader } });
            ::gl::linkProgram(_program, error);
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);
            qDebug() << "Rebuild program";
        }
    }

#define COLOR_BUFFER_COUNT 4


    void run() override {
        GLuint _framebuffer{ 0 };
        std::array<GLuint, COLOR_BUFFER_COUNT> _colors;
        size_t currentColorBuffer{ 0 };
        size_t globalColorBufferCount{ 0 };
        GLuint _uniformBuffer{ 0 };
        GLuint _vao{ 0 };
        GLuint _depth{ 0 };
        Reprojection _reprojection;

        QThread::currentThread()->setPriority(QThread::Priority::TimeCriticalPriority);
        _canvas->makeCurrent();

        glCreateBuffers(1, &_uniformBuffer);
        glNamedBufferStorage(_uniformBuffer, sizeof(Reprojection), 0, GL_DYNAMIC_STORAGE_BIT);
        glCreateVertexArrays(1, &_vao);
        glBindVertexArray(_vao);

        glCreateFramebuffers(1, &_framebuffer);
        {
            glCreateRenderbuffers(1, &_depth);
            glNamedRenderbufferStorage(_depth, GL_DEPTH24_STENCIL8, _plugin._renderTargetSize.x, _plugin._renderTargetSize.y);
            glNamedFramebufferRenderbuffer(_framebuffer, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depth);
            glCreateTextures(GL_TEXTURE_2D, COLOR_BUFFER_COUNT, &_colors[0]);
            for (size_t i = 0; i < COLOR_BUFFER_COUNT; ++i) {
                glTextureStorage2D(_colors[i], 1, GL_RGBA8, _plugin._renderTargetSize.x, _plugin._renderTargetSize.y);
            }
        }

        glDisable(GL_DEPTH_TEST);
        glViewport(0, 0, _plugin._renderTargetSize.x, _plugin._renderTargetSize.y);
        _canvas->doneCurrent();
        while (!_quit) {
            _canvas->makeCurrent();
            updateSource();
            if (!_current.texture) {
                _canvas->doneCurrent();
                QThread::usleep(1);
                continue;
            }

            updateProgram();
            {
                auto presentRotation = glm::mat3(_nextRender.poses[0]);
                auto renderRotation = glm::mat3(_current.pose);
                for (size_t i = 0; i < 2; ++i) {
                    _reprojection.projections[i] = _plugin._eyeProjections[i];
                    _reprojection.inverseProjections[i] = _plugin._eyeInverseProjections[i];
                }

                _reprojection.reprojection = glm::inverse(renderRotation) * presentRotation;
                _reprojection.visionSqueezeX = _plugin._visionSqueezeX;
                _reprojection.visionSqueezeY = _plugin._visionSqueezeY;
                _reprojection.visionSqueezeTransition = _plugin._visionSqueezeTransition;
                _reprojection.visionSqueezePerEye = _plugin._visionSqueezePerEye;
                _reprojection.visionSqueezeGroundPlaneY = _plugin._visionSqueezeGroundPlaneY;
                _reprojection.visionSqueezeSpotlightSize = _plugin._visionSqueezeSpotlightSize;
                _reprojection.hmdSensorMatrix = hmd34_to_mat4(_plugin._lastGoodHMDPose);
                // ipd is currently unused

                glNamedBufferSubData(_uniformBuffer, 0, sizeof(Reprojection), &_reprojection);
                glNamedFramebufferTexture(_framebuffer, GL_COLOR_ATTACHMENT0, _colors[currentColorBuffer], 0);
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _framebuffer);
                {
                    glClearColor(1, 1, 0, 1);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                    glTextureParameteri(_current.textureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTextureParameteri(_current.textureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glUseProgram(_program);
                    glBindBufferBase(GL_UNIFORM_BUFFER, REPROJECTION_BINDING, _uniformBuffer);
                    glBindTexture(GL_TEXTURE_2D, _current.textureID);
                    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                }

                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
                static const vr::VRTextureBounds_t leftBounds{ 0, 0, 0.5f, 1 };
                static const vr::VRTextureBounds_t rightBounds{ 0.5f, 0, 1, 1 };

                vr::Texture_t texture{ (void*)(uintptr_t)_colors[currentColorBuffer], vr::TextureType_OpenGL,
                                       vr::ColorSpace_Auto };
                vr::VRCompositor()->Submit(vr::Eye_Left, &texture, &leftBounds);
                vr::VRCompositor()->Submit(vr::Eye_Right, &texture, &rightBounds);
                _plugin._presentRate.increment();
                PoseData nextRender, nextSim;
                nextRender.frameIndex = _plugin.presentCount();
                vr::VRCompositor()->WaitGetPoses(nextRender.vrPoses, vr::k_unMaxTrackedDeviceCount, nextSim.vrPoses,
                                                 vr::k_unMaxTrackedDeviceCount);

                // Copy invalid poses in nextSim from nextRender
                for (uint32_t i = 0; i < vr::k_unMaxTrackedDeviceCount; ++i) {
                    if (!nextSim.vrPoses[i].bPoseIsValid) {
                        nextSim.vrPoses[i] = nextRender.vrPoses[i];
                    }
                }

                mat4 sensorResetMat;
                _plugin.withNonPresentThreadLock([&] { sensorResetMat = _plugin._sensorResetMat; });

                nextRender.update(sensorResetMat);
                nextSim.update(sensorResetMat);
                _plugin.withNonPresentThreadLock([&] {
                    _nextRender = nextRender;
                    _nextSim = nextSim;
                    ++_presentCount;
                    _presented.notify_one();
                });

                ++globalColorBufferCount;
                currentColorBuffer = globalColorBufferCount % COLOR_BUFFER_COUNT;
            }
            _canvas->doneCurrent();
        }

        _canvas->makeCurrent();
        glDeleteBuffers(1, &_uniformBuffer);
        glDeleteFramebuffers(1, &_framebuffer);
        CHECK_GL_ERROR();
        glDeleteTextures(4, &_colors[0]);
        glDeleteProgram(_program);
        glBindVertexArray(0);
        glDeleteVertexArrays(1, &_vao);
        _canvas->doneCurrent();
        _canvas->moveToThread(_plugin.thread());
    }

    void update(const CompositeInfo& newCompositeInfo) { _queue.push(newCompositeInfo); }

    void waitForPresent() {
        auto lastCount = _presentCount.load();
        Lock lock(_plugin._presentMutex);
        _presented.wait(lock, [&]() -> bool { return _presentCount.load() > lastCount; });
        _nextSimPoseData = _nextSim;
        _nextRenderPoseData = _nextRender;
    }

    CompositeInfo _current;
    CompositeInfo::Queue _queue;

    PoseData _nextRender, _nextSim;
    bool _quit{ false };
    GLuint _currentTexture{ 0 };
    std::atomic<uint32_t> _presentCount{ 0 };
    Condition _presented;
    OpenVrDisplayPlugin& _plugin;
};

bool OpenVrDisplayPlugin::isSupported() const {
    return openVrSupported();
}

glm::mat4 OpenVrDisplayPlugin::getEyeProjection(Eye eye, const glm::mat4& baseProjection) const {
    if (_system) {
        ViewFrustum baseFrustum;
        baseFrustum.setProjection(baseProjection);
        float baseNearClip = baseFrustum.getNearClip();
        float baseFarClip = baseFrustum.getFarClip();
        vr::EVREye openVrEye = (eye == Left) ? vr::Eye_Left : vr::Eye_Right;
        return toGlm(_system->GetProjectionMatrix(openVrEye, baseNearClip, baseFarClip));
    } else {
        return baseProjection;
    }
}

glm::mat4 OpenVrDisplayPlugin::getCullingProjection(const glm::mat4& baseProjection) const {
    if (_system) {
        ViewFrustum baseFrustum;
        baseFrustum.setProjection(baseProjection);
        float baseNearClip = baseFrustum.getNearClip();
        float baseFarClip = baseFrustum.getFarClip();
        // FIXME Calculate the proper combined projection by using GetProjectionRaw values from both eyes
        return toGlm(_system->GetProjectionMatrix((vr::EVREye)0, baseNearClip, baseFarClip));
    } else {
        return baseProjection;
    }
}

float OpenVrDisplayPlugin::getTargetFrameRate() const {
    if (forceInterleavedReprojection && !_asyncReprojectionActive) {
        return TARGET_RATE_OpenVr / 2.0f;
    }
    return TARGET_RATE_OpenVr;
}

void OpenVrDisplayPlugin::init() {
    Plugin::init();

    _lastGoodHMDPose.m[0][0] = 1.0f;
    _lastGoodHMDPose.m[0][1] = 0.0f;
    _lastGoodHMDPose.m[0][2] = 0.0f;
    _lastGoodHMDPose.m[0][3] = 0.0f;
    _lastGoodHMDPose.m[1][0] = 0.0f;
    _lastGoodHMDPose.m[1][1] = 1.0f;
    _lastGoodHMDPose.m[1][2] = 0.0f;
    _lastGoodHMDPose.m[1][3] = 1.8f;
    _lastGoodHMDPose.m[2][0] = 0.0f;
    _lastGoodHMDPose.m[2][1] = 0.0f;
    _lastGoodHMDPose.m[2][2] = 1.0f;
    _lastGoodHMDPose.m[2][3] = 0.0f;

    emit deviceConnected(getName());
}

const QString OpenVrDisplayPlugin::getName() const {
    std::string headsetName = getOpenVrDeviceName();
    if (headsetName == "HTC") {
        headsetName += " Vive";
    }

    return QString::fromStdString(headsetName);
}

bool OpenVrDisplayPlugin::internalActivate() {
    if (!_system) {
        _system = acquireOpenVrSystem();
    }
    if (!_system) {
        qWarning() << "Failed to initialize OpenVR";
        return false;
    }

    // If OpenVR isn't running, then the compositor won't be accessible
    // FIXME find a way to launch the compositor?
    if (!vr::VRCompositor()) {
        qWarning() << "Failed to acquire OpenVR compositor";
        releaseOpenVrSystem();
        _system = nullptr;
        return false;
    }

    vr::Compositor_FrameTiming timing;
    memset(&timing, 0, sizeof(timing));
    timing.m_nSize = sizeof(vr::Compositor_FrameTiming);
    vr::VRCompositor()->GetFrameTiming(&timing);
    auto usingOpenVRForOculus = oculusViaOpenVR();
    _asyncReprojectionActive = (timing.m_nReprojectionFlags & VRCompositor_ReprojectionAsync) || usingOpenVRForOculus;

    _threadedSubmit = !_asyncReprojectionActive;
    if (usingOpenVRForOculus) {
        qDebug() << "Oculus active via OpenVR:  " << usingOpenVRForOculus;
    }
    qDebug() << "OpenVR Async Reprojection active:  " << _asyncReprojectionActive;
    qDebug() << "OpenVR Threaded submit enabled:  " << _threadedSubmit;

    _openVrDisplayActive = true;
    _system->GetRecommendedRenderTargetSize(&_renderTargetSize.x, &_renderTargetSize.y);
    // Recommended render target size is per-eye, so double the X size for
    // left + right eyes
    _renderTargetSize.x *= 2;

    withNonPresentThreadLock([&] {
        openvr_for_each_eye([&](vr::Hmd_Eye eye) {
            _eyeOffsets[eye] = toGlm(_system->GetEyeToHeadTransform(eye));
            _eyeProjections[eye] = toGlm(_system->GetProjectionMatrix(eye, DEFAULT_NEAR_CLIP, DEFAULT_FAR_CLIP));
        });
        // FIXME Calculate the proper combined projection by using GetProjectionRaw values from both eyes
        _cullingProjection = _eyeProjections[0];
    });

    // enable async time warp
    if (forceInterleavedReprojection) {
        vr::VRCompositor()->ForceInterleavedReprojectionOn(true);
    }

    // set up default sensor space such that the UI overlay will align with the front of the room.
    auto chaperone = vr::VRChaperone();
    if (chaperone) {
        float const UI_RADIUS = 1.0f;
        float const UI_HEIGHT = 0.0f;
        float const UI_Z_OFFSET = 0.5;

        float xSize, zSize;
        chaperone->GetPlayAreaSize(&xSize, &zSize);
        glm::vec3 uiPos(0.0f, UI_HEIGHT, UI_RADIUS - (0.5f * zSize) - UI_Z_OFFSET);
        _sensorResetMat = glm::inverse(createMatFromQuatAndPos(glm::quat(), uiPos));
    } else {
#if DEV_BUILD
        qDebug() << "OpenVR: error could not get chaperone pointer";
#endif
    }

    if (_threadedSubmit) {
        _submitThread = std::make_shared<OpenVrSubmitThread>(*this);
        if (!_submitCanvas) {
            withOtherThreadContext([&] {
                _submitCanvas = std::make_shared<gl::OffscreenContext>();
                _submitCanvas->create();
                _submitCanvas->doneCurrent();
            });
        }
        _submitCanvas->moveToThread(_submitThread.get());
    }

    return Parent::internalActivate();
}

void OpenVrDisplayPlugin::internalDeactivate() {
    Parent::internalDeactivate();

    _openVrDisplayActive = false;
    if (_system) {
        // TODO: Invalidate poses. It's fine if someone else sets these shared values, but we're about to stop updating them, and
        // we don't want ViveControllerManager to consider old values to be valid.
        _container->makeRenderingContextCurrent();
        releaseOpenVrSystem();
        _system = nullptr;
    }
}

void OpenVrDisplayPlugin::customizeContext() {
    // Display plugins in DLLs must initialize GL locally
    gl::initModuleGl();
    Parent::customizeContext();

    if (_threadedSubmit) {
        _compositeInfos[0].texture = _compositeFramebuffer->getRenderBuffer(0);
        for (size_t i = 0; i < COMPOSITING_BUFFER_SIZE; ++i) {
            if (0 != i) {
                _compositeInfos[i].texture = gpu::Texture::createRenderBuffer(gpu::Element::COLOR_RGBA_32, _renderTargetSize.x,
                                                                              _renderTargetSize.y, gpu::Texture::SINGLE_MIP,
                                                                              gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_POINT));
            }
            _compositeInfos[i].textureID = getGLBackend()->getTextureID(_compositeInfos[i].texture);
        }
        _submitThread->_canvas = _submitCanvas;
        _submitThread->start(QThread::HighPriority);
    }
}

void OpenVrDisplayPlugin::uncustomizeContext() {
    Parent::uncustomizeContext();

    if (_threadedSubmit) {
        _submitThread->_quit = true;
        _submitThread->wait();
        _submitThread.reset();
    }
}

void OpenVrDisplayPlugin::resetSensors() {
    glm::mat4 m;
    withNonPresentThreadLock([&] { m = toGlm(_nextSimPoseData.vrPoses[0].mDeviceToAbsoluteTracking); });
    _sensorResetMat = glm::inverse(cancelOutRollAndPitch(m));
}

static bool isBadPose(vr::HmdMatrix34_t* mat) {
    if (mat->m[1][3] < -0.2f) {
        return true;
    }
    return false;
}

bool OpenVrDisplayPlugin::beginFrameRender(uint32_t frameIndex) {
    PROFILE_RANGE_EX(render, __FUNCTION__, 0xff7fff00, frameIndex)
    handleOpenVrEvents();
    if (openVrQuitRequested()) {
        QMetaObject::invokeMethod(qApp, "quit");
        return false;
    }
    _currentRenderFrameInfo = FrameInfo();

    PoseData nextSimPoseData;
    withNonPresentThreadLock([&] { nextSimPoseData = _nextSimPoseData; });

    // HACK: when interface is launched and steam vr is NOT running, openvr will return bad HMD poses for a few frames
    // To workaround this, filter out any hmd poses that are obviously bad, i.e. beneath the floor.
    if (isBadPose(&nextSimPoseData.vrPoses[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking)) {
        // qDebug() << "WARNING: ignoring bad hmd pose from openvr";

        // use the last known good HMD pose
        nextSimPoseData.vrPoses[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking = _lastGoodHMDPose;
    } else {
        _lastGoodHMDPose = nextSimPoseData.vrPoses[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking;
    }

    vr::TrackedDeviceIndex_t handIndices[2]{ vr::k_unTrackedDeviceIndexInvalid, vr::k_unTrackedDeviceIndexInvalid };
    {
        vr::TrackedDeviceIndex_t controllerIndices[2];
        auto trackedCount =
            _system->GetSortedTrackedDeviceIndicesOfClass(vr::TrackedDeviceClass_Controller, controllerIndices, 2);
        // Find the left and right hand controllers, if they exist
        for (uint32_t i = 0; i < std::min<uint32_t>(trackedCount, 2); ++i) {
            if (nextSimPoseData.vrPoses[i].bPoseIsValid) {
                auto role = _system->GetControllerRoleForTrackedDeviceIndex(controllerIndices[i]);
                if (vr::TrackedControllerRole_LeftHand == role) {
                    handIndices[0] = controllerIndices[i];
                } else if (vr::TrackedControllerRole_RightHand == role) {
                    handIndices[1] = controllerIndices[i];
                }
            }
        }
    }

    _currentRenderFrameInfo.renderPose = nextSimPoseData.poses[vr::k_unTrackedDeviceIndex_Hmd];
    bool keyboardVisible = isOpenVrKeyboardShown();

    std::array<mat4, 2> handPoses;
    if (!keyboardVisible) {
        for (int i = 0; i < 2; ++i) {
            if (handIndices[i] == vr::k_unTrackedDeviceIndexInvalid) {
                continue;
            }
            auto deviceIndex = handIndices[i];
            const mat4& mat = nextSimPoseData.poses[deviceIndex];
            const vec3& linearVelocity = nextSimPoseData.linearVelocities[deviceIndex];
            const vec3& angularVelocity = nextSimPoseData.angularVelocities[deviceIndex];
            auto correctedPose = openVrControllerPoseToHandPose(i == 0, mat, linearVelocity, angularVelocity);
            static const glm::quat HAND_TO_LASER_ROTATION = glm::rotation(Vectors::UNIT_Z, Vectors::UNIT_NEG_Y);
            handPoses[i] = glm::translate(glm::mat4(), correctedPose.translation) *
                           glm::mat4_cast(correctedPose.rotation * HAND_TO_LASER_ROTATION);
        }
    }

    withNonPresentThreadLock([&] { _frameInfos[frameIndex] = _currentRenderFrameInfo; });
    return Parent::beginFrameRender(frameIndex);
}

void OpenVrDisplayPlugin::compositeLayers() {
    if (_threadedSubmit) {
        ++_renderingIndex;
        _renderingIndex %= COMPOSITING_BUFFER_SIZE;

        auto& newComposite = _compositeInfos[_renderingIndex];
        newComposite.pose = _currentPresentFrameInfo.presentPose;
        _compositeFramebuffer->setRenderBuffer(0, newComposite.texture);
    }

    Parent::compositeLayers();

    if (_threadedSubmit) {
        auto& newComposite = _compositeInfos[_renderingIndex];
        newComposite.fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        // https://www.opengl.org/registry/specs/ARB/sync.txt:
        // > The simple flushing behavior defined by
        // > SYNC_FLUSH_COMMANDS_BIT will not help when waiting for a fence
        // > command issued in another context's command stream to complete.
        // > Applications which block on a fence sync object must take
        // > additional steps to assure that the context from which the
        // > corresponding fence command was issued has flushed that command
        // > to the graphics pipeline.
        glFlush();

        if (!newComposite.textureID) {
            newComposite.textureID = getGLBackend()->getTextureID(newComposite.texture);
        }
        withPresentThreadLock([&] { _submitThread->update(newComposite); });
    }
}

void OpenVrDisplayPlugin::hmdPresent() {
//    PROFILE_RANGE_EX(render, __FUNCTION__, 0xff00ff00, (uint64_t)_currentFrame->frameIndex)

    if (_threadedSubmit) {
        _submitThread->waitForPresent();
    } else {

        _parametersBuffer.edit<Parameters>()._visionSqueezeX = _visionSqueezeX;
        _parametersBuffer.edit<Parameters>()._visionSqueezeY = _visionSqueezeY;

        // _parametersBuffer.edit<Parameters>()._leftProjection = _eyeProjections[0];
        // _parametersBuffer.edit<Parameters>()._rightProjection = _eyeProjections[1];
        // _parametersBuffer.edit<Parameters>()._hmdSensorMatrix = _currentPresentFrameInfo.presentPose;
        // _parametersBuffer.edit<Parameters>()._ipd = _ipd;

        GLuint glTexId = getGLBackend()->getTextureID(_compositeFramebuffer->getRenderBuffer(0));
        vr::Texture_t vrTexture{ (void*)(uintptr_t)glTexId, vr::TextureType_OpenGL, vr::ColorSpace_Auto };
        vr::VRCompositor()->Submit(vr::Eye_Left, &vrTexture, &OPENVR_TEXTURE_BOUNDS_LEFT);
        vr::VRCompositor()->Submit(vr::Eye_Right, &vrTexture, &OPENVR_TEXTURE_BOUNDS_RIGHT);
        vr::VRCompositor()->PostPresentHandoff();
        _presentRate.increment();
    }

    vr::Compositor_FrameTiming frameTiming;
    memset(&frameTiming, 0, sizeof(vr::Compositor_FrameTiming));
    frameTiming.m_nSize = sizeof(vr::Compositor_FrameTiming);
    vr::VRCompositor()->GetFrameTiming(&frameTiming);
    _stutterRate.increment(frameTiming.m_nNumDroppedFrames);
}

void OpenVrDisplayPlugin::postPreview() {
//    PROFILE_RANGE_EX(render, __FUNCTION__, 0xff00ff00, (uint64_t)_currentFrame->frameIndex)
    PoseData nextRender, nextSim;
    nextRender.frameIndex = presentCount();

    if (!_threadedSubmit) {
        vr::VRCompositor()->WaitGetPoses(nextRender.vrPoses, vr::k_unMaxTrackedDeviceCount, nextSim.vrPoses,
                                         vr::k_unMaxTrackedDeviceCount);

        glm::mat4 resetMat;
        withPresentThreadLock([&] { resetMat = _sensorResetMat; });
        nextRender.update(resetMat);
        nextSim.update(resetMat);
        withPresentThreadLock([&] { _nextSimPoseData = nextSim; });
        _nextRenderPoseData = nextRender;
    }

    if (isHmdMounted() != _hmdMounted) {
        _hmdMounted = !_hmdMounted;
        emit hmdMountedChanged();
    }
}

bool OpenVrDisplayPlugin::isHmdMounted() const {
    return isHeadInHeadset();
}

void OpenVrDisplayPlugin::updatePresentPose() {
    _currentPresentFrameInfo.presentPose = _nextRenderPoseData.poses[vr::k_unTrackedDeviceIndex_Hmd];
}

bool OpenVrDisplayPlugin::suppressKeyboard() {
    if (isOpenVrKeyboardShown()) {
        return false;
    }
    if (!_keyboardSupressionCount.fetch_add(1)) {
        disableOpenVrKeyboard();
    }
    return true;
}

void OpenVrDisplayPlugin::unsuppressKeyboard() {
    if (_keyboardSupressionCount == 0) {
        qWarning() << "Attempted to unsuppress a keyboard that was not suppressed";
        return;
    }
    if (1 == _keyboardSupressionCount.fetch_sub(1)) {
        enableOpenVrKeyboard(_container);
    }
}

bool OpenVrDisplayPlugin::isKeyboardVisible() {
    return isOpenVrKeyboardShown();
}

int OpenVrDisplayPlugin::getRequiredThreadCount() const {
    return Parent::getRequiredThreadCount() + (_threadedSubmit ? 1 : 0);
}

QString OpenVrDisplayPlugin::getPreferredAudioInDevice() const {
    QString device = getVrSettingString(vr::k_pch_audio_Section, vr::k_pch_audio_OnPlaybackDevice_String);
    if (!device.isEmpty()) {
        static const WCHAR INIT = 0;
        size_t size = device.size() + 1;
        std::vector<WCHAR> deviceW;
        deviceW.assign(size, INIT);
        device.toWCharArray(deviceW.data());
        device = AudioClient::getWinDeviceName(deviceW.data());
    }
    return device;
}

QString OpenVrDisplayPlugin::getPreferredAudioOutDevice() const {
    QString device = getVrSettingString(vr::k_pch_audio_Section, vr::k_pch_audio_OnRecordDevice_String);
    if (!device.isEmpty()) {
        static const WCHAR INIT = 0;
        size_t size = device.size() + 1;
        std::vector<WCHAR> deviceW;
        deviceW.assign(size, INIT);
        device.toWCharArray(deviceW.data());
        device = AudioClient::getWinDeviceName(deviceW.data());
    }
    return device;
}

QRectF OpenVrDisplayPlugin::getPlayAreaRect() {
    auto chaperone = vr::VRChaperone();
    if (!chaperone) {
        qWarning() << "No chaperone";
        return QRectF();
    }

    if (chaperone->GetCalibrationState() >= vr::ChaperoneCalibrationState_Error) {
        qWarning() << "Chaperone status =" << chaperone->GetCalibrationState();
        return QRectF();
    }

    vr::HmdQuad_t rect;
    if (!chaperone->GetPlayAreaRect(&rect)) {
        qWarning() << "Chaperone rect not obtained";
        return QRectF();
    }

    auto minXZ = transformPoint(_sensorResetMat, toGlm(rect.vCorners[0]));
    auto maxXZ = minXZ;
    for (int i = 1; i < 4; i++) {
        auto point = transformPoint(_sensorResetMat, toGlm(rect.vCorners[i]));
        minXZ.x = std::min(minXZ.x, point.x);
        minXZ.z = std::min(minXZ.z, point.z);
        maxXZ.x = std::max(maxXZ.x, point.x);
        maxXZ.z = std::max(maxXZ.z, point.z);
    }

    glm::vec2 center = glm::vec2((minXZ.x + maxXZ.x) / 2, (minXZ.z + maxXZ.z) / 2);
    glm::vec2 dimensions = glm::vec2(maxXZ.x - minXZ.x, maxXZ.z - minXZ.z);

    return QRectF(center.x, center.y, dimensions.x, dimensions.y);
}

void OpenVrDisplayPlugin::updateParameters(float visionSqueezeX, float visionSqueezeY, float visionSqueezeTransition,
                                           int visionSqueezePerEye, float visionSqueezeGroundPlaneY,
                                           float visionSqueezeSpotlightSize) {
    _visionSqueezeX = visionSqueezeX;
    _visionSqueezeY = visionSqueezeY;
    _visionSqueezeTransition = visionSqueezeTransition;
    _visionSqueezePerEye = visionSqueezePerEye;
    _visionSqueezeGroundPlaneY = visionSqueezeGroundPlaneY;
    _visionSqueezeSpotlightSize = visionSqueezeSpotlightSize;
}
