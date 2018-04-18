#include <GLType/OGLTypes.h>

namespace OGLTypes
{
    GLbitfield translate(GraphicsUsageFlags usage)
    {
        GLbitfield flags = GL_ZERO;
        if (usage & GraphicsUsageFlagBits::GraphicsUsageFlagReadBit)
            flags |= GL_MAP_READ_BIT;
        if (usage & GraphicsUsageFlagBits::GraphicsUsageFlagWriteBit)
            flags |= GL_MAP_WRITE_BIT;
        if (usage & GraphicsUsageFlagBits::GraphicsUsageFlagPersistentBit)
            flags |= GL_MAP_PERSISTENT_BIT;
        if (usage & GraphicsUsageFlagBits::GraphicsUsageFlagCoherentBit)
            flags |= GL_MAP_COHERENT_BIT;
        if (usage & GraphicsUsageFlagBits::GraphicsUsageFlagFlushExplicitBit)
            flags |= GL_MAP_FLUSH_EXPLICIT_BIT;
        if (usage & GraphicsUsageFlagBits::GraphicsUsageFlagDynamicStorageBit)
            flags |= GL_DYNAMIC_STORAGE_BIT;
        if (usage & GraphicsUsageFlagBits::GraphicsUsageFlagClientStorageBit)
            flags |= GL_CLIENT_STORAGE_BIT;
        return flags;
    }

#ifndef TEST
    GLenum translate(GraphicsTarget target)
    {
        gli::gl GL(gli::gl::PROFILE_GL33);
        return GL.translate(target);
    }

    GLenum translate(GraphicsFormat format)
    {
        using namespace gli;
        gli::gl GL(gli::gl::PROFILE_GL33);
        gli::swizzles swizzle(gl::SWIZZLE_RED, gl::SWIZZLE_GREEN, gl::SWIZZLE_BLUE, gl::SWIZZLE_ALPHA);
        auto Format = GL.translate(format, swizzle);
        return !gli::is_compressed(format) ? Format.Internal : Format.External;
    }
#else
    GLenum translate(GraphicsTarget target)
    {
		static GLenum const Table[] =
		{
            GL_TEXTURE_1D,
            GL_TEXTURE_1D_ARRAY,
            GL_TEXTURE_2D,
            GL_TEXTURE_2D_ARRAY,
            GL_TEXTURE_3D,
			GL_TEXTURE_RECTANGLE_EXT,
            GL_INVALID_ENUM, // RECT_ARRAY
			GL_TEXTURE_CUBE_MAP,
			GL_TEXTURE_CUBE_MAP_ARRAY
		};
		static_assert(sizeof(Table) / sizeof(Table[0]) == GraphicsTargetCount, "error: target descriptor list doesn't match number of supported targets");

		return Table[target];
	}
#endif

    GLenum getComponent(int Components)
    {
        switch (Components)
        {
        case 1u:
            return GL_RED;
        case 2u:
            return GL_RG;
        case 3u:
            return GL_RGB;
        case 4u:
            return GL_RGBA;
        default:
            assert(false);
        };
        return 0;
    }

    GLenum getInternalComponent(int Components, bool bFloat)
    {
        GLenum Base = getComponent(Components);
        if (bFloat)
        {
            switch (Base)
            {
            case GL_RED:
                return GL_R16F;
            case GL_RG:
                return GL_RG16F;
            case GL_RGB:
                return GL_RGB16F;
            case GL_RGBA:
                return GL_RGBA16F;
            }
        }
        else
        {
            switch (Base)
            {
            case GL_RED:
                return GL_R8;
            case GL_RG:
			    return GL_RG8;
            case GL_RGB:
                return GL_RGB8;
            case GL_RGBA:
                return GL_RGBA8;
            }
        }
        return Base;
    }
}
