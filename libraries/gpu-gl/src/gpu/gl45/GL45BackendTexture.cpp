//
//  GL45BackendTexture.cpp
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 1/19/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GL45Backend.h"

#include <mutex>
#include <condition_variable>
#include <unordered_set>
#include <unordered_map>
#include <glm/gtx/component_wise.hpp>

#include <QtCore/QDebug>
#include <QtCore/QThread>

#include "../gl/GLTexelFormat.h"

using namespace gpu;
using namespace gpu::gl;
using namespace gpu::gl45;

// Allocate 1 MB of buffer space for paged transfers
#define DEFAULT_PAGE_BUFFER_SIZE (1024*1024)
#define DEFAULT_GL_PIXEL_ALIGNMENT 4

using GL45Texture = GL45Backend::GL45Texture;

static std::map<uint16_t, std::unordered_set<GL45Texture*>> texturesByMipCounts;
static Mutex texturesByMipCountsMutex;
using TextureTypeFormat = std::pair<GLenum, GLenum>;
std::map<TextureTypeFormat, std::vector<uvec3>> sparsePageDimensionsByFormat;
Mutex sparsePageDimensionsByFormatMutex;

static std::vector<uvec3> getPageDimensionsForFormat(const TextureTypeFormat& typeFormat) {
    {
        Lock lock(sparsePageDimensionsByFormatMutex);
        if (sparsePageDimensionsByFormat.count(typeFormat)) {
            return sparsePageDimensionsByFormat[typeFormat];
        }
    }
    GLint count = 0;
    glGetInternalformativ(typeFormat.first, typeFormat.second, GL_NUM_VIRTUAL_PAGE_SIZES_ARB, 1, &count);

    std::vector<uvec3> result;
    if (count > 0) {
        std::vector<GLint> x, y, z;
        x.resize(count);
        glGetInternalformativ(typeFormat.first, typeFormat.second, GL_VIRTUAL_PAGE_SIZE_X_ARB, 1, &x[0]);
        y.resize(count);
        glGetInternalformativ(typeFormat.first, typeFormat.second, GL_VIRTUAL_PAGE_SIZE_Y_ARB, 1, &y[0]);
        z.resize(count);
        glGetInternalformativ(typeFormat.first, typeFormat.second, GL_VIRTUAL_PAGE_SIZE_Z_ARB, 1, &z[0]);

        result.resize(count);
        for (GLint i = 0; i < count; ++i) {
            result[i] = uvec3(x[i], y[i], z[i]);
        }
    }

    {
        Lock lock(sparsePageDimensionsByFormatMutex);
        if (0 == sparsePageDimensionsByFormat.count(typeFormat)) {
            sparsePageDimensionsByFormat[typeFormat] = result;
        }
    }

    return result;
}

static std::vector<uvec3> getPageDimensionsForFormat(GLenum target, GLenum format) {
    return getPageDimensionsForFormat({ target, format });
}

GLTexture* GL45Backend::syncGPUObject(const TexturePointer& texture, bool transfer) {
    return GL45Texture::sync<GL45Texture>(*this, texture, transfer);
}

using SparseInfo = GL45Backend::GL45Texture::SparseInfo;

SparseInfo::SparseInfo(GL45Texture& texture)
    : texture(texture) {
}

void SparseInfo::maybeMakeSparse() {
    // Don't enable sparse for objects with explicitly managed mip levels
    if (!texture._gpuObject.isAutogenerateMips()) {
        return;
    }
    return;

    const uvec3 dimensions = texture._gpuObject.getDimensions();
    auto allowedPageDimensions = getPageDimensionsForFormat(texture._target, texture._internalFormat);
    // In order to enable sparse the texture size must be an integer multiple of the page size
    for (size_t i = 0; i < allowedPageDimensions.size(); ++i) {
        pageDimensionsIndex = (uint32_t) i;
        pageDimensions = allowedPageDimensions[i];
        // Is this texture an integer multiple of page dimensions?
        if (uvec3(0) == (dimensions % pageDimensions)) {
            qCDebug(gpugl45logging) << "Enabling sparse for texture " << texture._source.c_str();
            sparse = true;
            break;
        }
    }

    if (sparse) {
        glTextureParameteri(texture._id, GL_TEXTURE_SPARSE_ARB, GL_TRUE);
        glTextureParameteri(texture._id, GL_VIRTUAL_PAGE_SIZE_INDEX_ARB, pageDimensionsIndex);
    } else {
        qCDebug(gpugl45logging) << "Size " << dimensions.x << " x " << dimensions.y << 
            " is not supported by any sparse page size for texture" << texture._source.c_str();
    }
}

#define SPARSE_PAGE_SIZE_OVERHEAD_ESTIMATE 1.3f

// This can only be called after we've established our storage size
void SparseInfo::update() {
    if (!sparse) {
        return;
    }
    glGetTextureParameterIuiv(texture._id, GL_NUM_SPARSE_LEVELS_ARB, &maxSparseLevel);
    pageBytes = texture._gpuObject.getTexelFormat().getSize();
    pageBytes *= pageDimensions.x * pageDimensions.y * pageDimensions.z;
    // Testing with a simple texture allocating app shows an estimated 20% GPU memory overhead for 
    // sparse textures as compared to non-sparse, so we acount for that here.
    pageBytes = (uint32_t)(pageBytes * SPARSE_PAGE_SIZE_OVERHEAD_ESTIMATE);

    for (uint16_t mipLevel = 0; mipLevel <= maxSparseLevel; ++mipLevel) {
        auto mipDimensions = texture._gpuObject.evalMipDimensions(mipLevel);
        auto mipPageCount = getPageCount(mipDimensions);
        maxPages += mipPageCount;
    }
    if (texture._target == GL_TEXTURE_CUBE_MAP) {
        maxPages *= GLTexture::CUBE_NUM_FACES;
    }
}

uvec3 SparseInfo::getPageCounts(const uvec3& dimensions) const {
    auto result = (dimensions / pageDimensions) +
        glm::clamp(dimensions % pageDimensions, glm::uvec3(0), glm::uvec3(1));
    return result;
}

uint32_t SparseInfo::getPageCount(const uvec3& dimensions) const {
    auto pageCounts = getPageCounts(dimensions);
    return pageCounts.x * pageCounts.y * pageCounts.z;
}


uint32_t SparseInfo::getSize() const {
    return allocatedPages * pageBytes;
}

void GL45Backend::initTextureManagementStage() {
    // enable the Sparse Texture on gl45
    _textureManagement._sparseCapable = true;

    // But now let s refine the behavior based on vendor
    std::string vendor { (const char*)glGetString(GL_VENDOR) };
    if ((vendor.find("AMD") != std::string::npos) || (vendor.find("ATI") != std::string::npos) || (vendor.find("INTEL") != std::string::npos)) {
        qCDebug(gpugllogging) << "GPU is sparse capable but force it off, vendor = " << vendor.c_str();
        _textureManagement._sparseCapable = false;
    } else {
        qCDebug(gpugllogging) << "GPU is sparse capable, vendor = " << vendor.c_str();
    }
}


GLuint GL45Texture::allocate(const Texture& texture) {
    GLuint result;
    glCreateTextures(getGLTextureType(texture), 1, &result);
    return result;
}

GLuint GL45Backend::getTextureID(const TexturePointer& texture, bool transfer) {
    return GL45Texture::getId<GL45Texture>(*this, texture, transfer);
}

GL45Texture::GL45Texture(const std::weak_ptr<GLBackend>& backend, const Texture& texture, GLuint externalId)
    : GLTexture(backend, texture, externalId), _sparseInfo(*this) 
{
}

GL45Texture::GL45Texture(const std::weak_ptr<GLBackend>& backend, const Texture& texture, bool transferrable)
    : GLTexture(backend, texture, allocate(texture), transferrable), _sparseInfo(*this) 
    {

    auto theBackend = _backend.lock();
    if (_transferrable && theBackend && theBackend->isTextureManagementSparseEnabled()) {
        _sparseInfo.maybeMakeSparse();
        if (_sparseInfo.sparse) {
            Backend::incrementTextureGPUSparseCount();
        }
    }
}

GL45Texture::~GL45Texture() {
    // Remove this texture from the candidate list of derezzable textures
    if (_transferrable) {
        auto mipLevels = usedMipLevels();
        Lock lock(texturesByMipCountsMutex);
        if (texturesByMipCounts.count(mipLevels)) {
            auto& textures = texturesByMipCounts[mipLevels];
            textures.erase(this);
            if (textures.empty()) {
                texturesByMipCounts.erase(mipLevels);
            }
        }
    }

    if (_sparseInfo.sparse) {
        Backend::decrementTextureGPUSparseCount();

        // Experimenation suggests that allocating sparse textures on one context/thread and deallocating 
        // them on another is buggy.  So for sparse textures we need to queue a lambda with the deallocation 
        // callls to the transfer thread
        auto id = _id;
        // Set the class _id to 0 so we don't try to double delete
        const_cast<GLuint&>(_id) = 0;
        std::list<std::function<void()>> destructionFunctions;
        
        uint8_t maxFace = (uint8_t)((_target == GL_TEXTURE_CUBE_MAP) ? GLTexture::CUBE_NUM_FACES : 1);
        auto maxSparseMip = std::min<uint16_t>(_maxMip, _sparseInfo.maxSparseLevel);
        for (uint16_t mipLevel = _minMip; mipLevel <= maxSparseMip; ++mipLevel) {
            auto mipDimensions = _gpuObject.evalMipDimensions(mipLevel);
            destructionFunctions.push_back([id, maxFace, mipLevel, mipDimensions] {
                glTexturePageCommitmentEXT(id, mipLevel, 0, 0, 0, mipDimensions.x, mipDimensions.y, maxFace, GL_FALSE);
            });

            auto deallocatedPages = _sparseInfo.getPageCount(mipDimensions) * maxFace;
            assert(deallocatedPages <= _sparseInfo.allocatedPages);
            _sparseInfo.allocatedPages -= deallocatedPages;
        }

        if (0 != _sparseInfo.allocatedPages) {
            qCWarning(gpugl45logging) << "Allocated pages remaining " << _id << " " << _sparseInfo.allocatedPages;
        }

        auto size = _size;
        const_cast<GLuint&>(_size) = 0;
        _textureTransferHelper->queueExecution([id, size, destructionFunctions] {
            for (auto function : destructionFunctions) {
                function();
            }
            glDeleteTextures(1, &id);
            Backend::decrementTextureGPUCount();
            Backend::updateTextureGPUMemoryUsage(size, 0);
            Backend::updateTextureGPUSparseMemoryUsage(size, 0);
        });
    }
}

void GL45Texture::withPreservedTexture(std::function<void()> f) const {
    f();
}

void GL45Texture::generateMips() const {
    glGenerateTextureMipmap(_id);
    (void)CHECK_GL_ERROR();
}

void GL45Texture::allocateStorage() const {
    if (_gpuObject.getTexelFormat().isCompressed()) {
        qFatal("Compressed textures not yet supported");
    }
    glTextureParameteri(_id, GL_TEXTURE_BASE_LEVEL, 0);
    glTextureParameteri(_id, GL_TEXTURE_MAX_LEVEL, _maxMip - _minMip);
    // Get the dimensions, accounting for the downgrade level
    Vec3u dimensions = _gpuObject.evalMipDimensions(_minMip + _mipOffset);
    glTextureStorage2D(_id, usedMipLevels(), _internalFormat, dimensions.x, dimensions.y);
    (void)CHECK_GL_ERROR();
}

void GL45Texture::updateSize() const {
    if (_gpuObject.getTexelFormat().isCompressed()) {
        qFatal("Compressed textures not yet supported");
    }

    if (_transferrable && _sparseInfo.sparse) {
        auto size = _sparseInfo.getSize();
        Backend::updateTextureGPUSparseMemoryUsage(_size, size);
        setSize(size);
    } else {
        setSize(_gpuObject.evalTotalSize(_mipOffset));
    }
}

void GL45Texture::startTransfer() {
    Parent::startTransfer();
    _sparseInfo.update();
}

bool GL45Texture::continueTransfer() {
    PROFILE_RANGE(render_gpu_gl, "continueTransfer")
    size_t maxFace = GL_TEXTURE_CUBE_MAP == _target ? CUBE_NUM_FACES : 1;
    for (uint8_t face = 0; face < maxFace; ++face) {
        for (uint16_t mipLevel = _minMip; mipLevel <= _maxMip; ++mipLevel) {
            auto size = _gpuObject.evalMipDimensions(mipLevel);
            if (_sparseInfo.sparse && mipLevel <= _sparseInfo.maxSparseLevel) {
                glTexturePageCommitmentEXT(_id, mipLevel, 0, 0, face, size.x, size.y, 1, GL_TRUE);
                _sparseInfo.allocatedPages += _sparseInfo.getPageCount(size);
            }
            if (_gpuObject.isStoredMipFaceAvailable(mipLevel, face)) {
                PROFILE_RANGE_EX(render_gpu_gl, "texSubImage", 0x0000ffff, (size.x * size.y * maxFace / 1024));

                auto mip = _gpuObject.accessStoredMipFace(mipLevel, face);
                GLTexelFormat texelFormat = GLTexelFormat::evalGLTexelFormat(_gpuObject.getTexelFormat(), mip->getFormat());
                if (GL_TEXTURE_2D == _target) {
                    glTextureSubImage2D(_id, mipLevel, 0, 0, size.x, size.y, texelFormat.format, texelFormat.type, mip->readData());
                } else if (GL_TEXTURE_CUBE_MAP == _target) {
                    // DSA ARB does not work on AMD, so use EXT
                    // unless EXT is not available on the driver
                    if (glTextureSubImage2DEXT) {
                        auto target = CUBE_FACE_LAYOUT[face];
                        glTextureSubImage2DEXT(_id, target, mipLevel, 0, 0, size.x, size.y, texelFormat.format, texelFormat.type, mip->readData());
                    } else {
                        glTextureSubImage3D(_id, mipLevel, 0, 0, face, size.x, size.y, 1, texelFormat.format, texelFormat.type, mip->readData());
                    }
                } else {
                    Q_ASSERT(false);
                }
                (void)CHECK_GL_ERROR();
            }
        }
    }
    return false;
}

void GL45Texture::finishTransfer() {
    Parent::finishTransfer();
}

void GL45Texture::syncSampler() const {
    const Sampler& sampler = _gpuObject.getSampler();

    const auto& fm = FILTER_MODES[sampler.getFilter()];
    glTextureParameteri(_id, GL_TEXTURE_MIN_FILTER, fm.minFilter);
    glTextureParameteri(_id, GL_TEXTURE_MAG_FILTER, fm.magFilter);

    if (sampler.doComparison()) {
        glTextureParameteri(_id, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
        glTextureParameteri(_id, GL_TEXTURE_COMPARE_FUNC, COMPARISON_TO_GL[sampler.getComparisonFunction()]);
    } else {
        glTextureParameteri(_id, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    }

    glTextureParameteri(_id, GL_TEXTURE_WRAP_S, WRAP_MODES[sampler.getWrapModeU()]);
    glTextureParameteri(_id, GL_TEXTURE_WRAP_T, WRAP_MODES[sampler.getWrapModeV()]);
    glTextureParameteri(_id, GL_TEXTURE_WRAP_R, WRAP_MODES[sampler.getWrapModeW()]);
    glTextureParameterfv(_id, GL_TEXTURE_BORDER_COLOR, (const float*)&sampler.getBorderColor());
    // FIXME account for mip offsets here
    auto baseMip = std::max<uint16_t>(sampler.getMipOffset(), _minMip);
    glTextureParameteri(_id, GL_TEXTURE_BASE_LEVEL, baseMip);
    glTextureParameterf(_id, GL_TEXTURE_MIN_LOD, (float)sampler.getMinMip());
    glTextureParameterf(_id, GL_TEXTURE_MAX_LOD, (sampler.getMaxMip() == Sampler::MAX_MIP_LEVEL ? 1000.f : sampler.getMaxMip() - _mipOffset));
    glTextureParameterf(_id, GL_TEXTURE_MAX_ANISOTROPY_EXT, sampler.getMaxAnisotropy());
}

void GL45Texture::postTransfer() {
    Parent::postTransfer();
    auto mipLevels = usedMipLevels();
    if (_transferrable && mipLevels > 1 && _minMip < _sparseInfo.maxSparseLevel) {
        Lock lock(texturesByMipCountsMutex);
        texturesByMipCounts[mipLevels].insert(this);
    }
}

void GL45Texture::stripToMip(uint16_t newMinMip) {
    if (newMinMip < _minMip) {
        qCWarning(gpugl45logging) << "Cannot decrease the min mip";
        return;
    }

    if (_sparseInfo.sparse && newMinMip > _sparseInfo.maxSparseLevel) {
        qCWarning(gpugl45logging) << "Cannot increase the min mip into the mip tail";
        return;
    }

    PROFILE_RANGE(render_gpu_gl, "GL45Texture::stripToMip");

    auto mipLevels = usedMipLevels();
    {
        Lock lock(texturesByMipCountsMutex);
        assert(0 != texturesByMipCounts.count(mipLevels));
        assert(0 != texturesByMipCounts[mipLevels].count(this));
        texturesByMipCounts[mipLevels].erase(this);
        if (texturesByMipCounts[mipLevels].empty()) {
            texturesByMipCounts.erase(mipLevels);
        }
    }

    // If we weren't generating mips before, we need to now that we're stripping down mip levels.
    if (!_gpuObject.isAutogenerateMips()) {
        qCDebug(gpugl45logging) << "Force mip generation for texture";
        glGenerateTextureMipmap(_id);
    }


    uint8_t maxFace = (uint8_t)((_target == GL_TEXTURE_CUBE_MAP) ? GLTexture::CUBE_NUM_FACES : 1);
    if (_sparseInfo.sparse) {
        for (uint16_t mip = _minMip; mip < newMinMip; ++mip) {
            auto id = _id;
            auto mipDimensions = _gpuObject.evalMipDimensions(mip);
            _textureTransferHelper->queueExecution([id, mip, mipDimensions, maxFace] {
                glTexturePageCommitmentEXT(id, mip, 0, 0, 0, mipDimensions.x, mipDimensions.y, maxFace, GL_FALSE);
            });

            auto deallocatedPages = _sparseInfo.getPageCount(mipDimensions) * maxFace;
            assert(deallocatedPages < _sparseInfo.allocatedPages);
            _sparseInfo.allocatedPages -= deallocatedPages;
        }
        _minMip = newMinMip;
    } else {
        GLuint oldId = _id;
        // Find the distance between the old min mip and the new one
        uint16 mipDelta = newMinMip - _minMip;
        _mipOffset += mipDelta;
        const_cast<uint16&>(_maxMip) -= mipDelta;
        auto newLevels = usedMipLevels();

        // Create and setup the new texture (allocate)
        {
            Vec3u newDimensions = _gpuObject.evalMipDimensions(_mipOffset);
            PROFILE_RANGE_EX(render_gpu_gl, "Re-Allocate", 0xff0000ff, (newDimensions.x * newDimensions.y));

            glCreateTextures(_target, 1, &const_cast<GLuint&>(_id));
            glTextureParameteri(_id, GL_TEXTURE_BASE_LEVEL, 0);
            glTextureParameteri(_id, GL_TEXTURE_MAX_LEVEL, _maxMip - _minMip);
            glTextureStorage2D(_id, newLevels, _internalFormat, newDimensions.x, newDimensions.y);
        }

        // Copy the contents of the old texture to the new
        {
            PROFILE_RANGE(render_gpu_gl, "Blit"); 
            // Preferred path only available in 4.3
            for (uint16 targetMip = _minMip; targetMip <= _maxMip; ++targetMip) {
                uint16 sourceMip = targetMip + mipDelta;
                Vec3u mipDimensions = _gpuObject.evalMipDimensions(targetMip + _mipOffset);
                for (GLenum target : getFaceTargets(_target)) {
                    glCopyImageSubData(
                        oldId, target, sourceMip, 0, 0, 0,
                        _id, target, targetMip, 0, 0, 0,
                        mipDimensions.x, mipDimensions.y, 1
                        );
                    (void)CHECK_GL_ERROR();
                }
            }

            glDeleteTextures(1, &oldId);
        }
    }

    // Re-sync the sampler to force access to the new mip level
    syncSampler();
    updateSize();

    // Re-insert into the texture-by-mips map if appropriate
    mipLevels = usedMipLevels();
    if (mipLevels > 1 && (!_sparseInfo.sparse || _minMip < _sparseInfo.maxSparseLevel)) {
        Lock lock(texturesByMipCountsMutex);
        texturesByMipCounts[mipLevels].insert(this);
    }
}

void GL45Texture::updateMips() {
    if (!_sparseInfo.sparse) {
        return;
    }
    auto newMinMip = std::min<uint16_t>(_gpuObject.minMip(), _sparseInfo.maxSparseLevel);
    if (_minMip < newMinMip) {
        stripToMip(newMinMip);
    }
}

void GL45Texture::derez() {
    if (_sparseInfo.sparse) {
        assert(_minMip < _sparseInfo.maxSparseLevel);
    }
    assert(_minMip < _maxMip);
    assert(_transferrable);
    stripToMip(_minMip + 1);
}

void GL45Backend::derezTextures() const {
    if (GLTexture::getMemoryPressure() < 1.0f) {
        return;
    }

    Lock lock(texturesByMipCountsMutex);
    if (texturesByMipCounts.empty()) {
        // No available textures to derez
        return;
    }

    auto mipLevel = texturesByMipCounts.rbegin()->first;
    if (mipLevel <= 1) {
        // No mips available to remove
        return;
    }

    GL45Texture* targetTexture = nullptr;
    {
        auto& textures = texturesByMipCounts[mipLevel];
        assert(!textures.empty());
        targetTexture = *textures.begin();
    }
    lock.unlock();
    targetTexture->derez();
}
