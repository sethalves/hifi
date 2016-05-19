//
//  GLBackendShared.h
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 1/22/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_GLBackend_Shared_h
#define hifi_gpu_GLBackend_Shared_h

#include <gl/Config.h>
#include <gpu/Forward.h>
#include <gpu/Format.h>

namespace gpu { namespace gl { 

static const GLenum _primitiveToGLmode[gpu::NUM_PRIMITIVES] = {
    GL_POINTS,
    GL_LINES,
    GL_LINE_STRIP,
    GL_TRIANGLES,
    GL_TRIANGLE_STRIP,
    GL_TRIANGLE_FAN,
};

static const GLenum _elementTypeToGLType[gpu::NUM_TYPES] = {
    GL_FLOAT,
    GL_INT,
    GL_UNSIGNED_INT,
    GL_HALF_FLOAT,
    GL_SHORT,
    GL_UNSIGNED_SHORT,
    GL_BYTE,
    GL_UNSIGNED_BYTE,
    // Normalized values
    GL_INT,
    GL_UNSIGNED_INT,
    GL_SHORT,
    GL_UNSIGNED_SHORT,
    GL_BYTE,
    GL_UNSIGNED_BYTE
};

class GLTexelFormat {
public:
    GLenum internalFormat;
    GLenum format;
    GLenum type;

    static GLTexelFormat evalGLTexelFormat(const gpu::Element& dstFormat) {
        return evalGLTexelFormat(dstFormat, dstFormat);
    }
    static GLTexelFormat evalGLTexelFormatInternal(const gpu::Element& dstFormat);

    static GLTexelFormat evalGLTexelFormat(const gpu::Element& dstFormat, const gpu::Element& srcFormat);
};

bool checkGLError(const char* name = nullptr);
bool checkGLErrorDebug(const char* name = nullptr);

} }

#define CHECK_GL_ERROR() gpu::gl::checkGLErrorDebug(__FUNCTION__)

#endif
