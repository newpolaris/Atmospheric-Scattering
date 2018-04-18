#pragma once

#include <GL/glew.h>
#include <GraphicsTypes.h>

namespace OGLTypes
{
    GLbitfield translate(GraphicsUsageFlags usage);
    GLenum translate(GraphicsTarget target);
    GLenum translate(GraphicsFormat format);

    GLenum getComponent(int components);
    GLenum getInternalComponent(int components, bool bFloat);
}
