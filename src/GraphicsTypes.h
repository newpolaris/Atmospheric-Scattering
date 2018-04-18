#pragma once

#include <vector>
#include <memory>
#include <tools/Rtti.h>
#include <gli/gli.hpp>

class GraphicsDeviceDesc;
class GraphicsDataDesc;
class GraphicsTextureDesc;
class GraphicsFramebufferDesc;

typedef std::shared_ptr<class GraphicsDevice> GraphicsDevicePtr;
typedef std::shared_ptr<class GraphicsData> GraphicsDataPtr;
typedef std::shared_ptr<class GraphicsTexture> GraphicsTexturePtr;
typedef std::shared_ptr<class OGLTexture> OGLTexturePtr;
typedef std::shared_ptr<class OGLCoreTexture> OGLCoreTexturePtr;
typedef std::shared_ptr<class GraphicsFramebuffer> GraphicsFramebufferPtr;
typedef std::shared_ptr<class OGLCoreFramebuffer> OGLCoreFramebufferPtr;
typedef std::shared_ptr<class OGLFramebuffer> OGLFramebufferPtr;

typedef std::weak_ptr<class GraphicsDevice> GraphicsDeviceWeakPtr;
typedef std::weak_ptr<class GraphicsData> GraphicsDataWeakPtr;
typedef std::weak_ptr<class GraphicsFramebuffer> GraphicsFramebufferWeakPtr;
typedef std::weak_ptr<class OGLCoreFramebuffer> OGLCoreFramebufferWeakPtr;
typedef std::weak_ptr<class OGLFramebuffer> OGLFramebufferWeakPtr;

typedef std::vector<class GraphicsAttachmentBinding> AttachmentBindings;

enum GraphicsDeviceType
{
	GraphicsDeviceTypeD3D9 = 0,
	GraphicsDeviceTypeD3D11 = 1,
	GraphicsDeviceTypeD3D12 = 2,
	GraphicsDeviceTypeOpenGL = 3,
	GraphicsDeviceTypeOpenGLCore = 4,
	GraphicsDeviceTypeOpenGLES2 = 5,
	GraphicsDeviceTypeOpenGLES3 = 6,
	GraphicsDeviceTypeOpenGLES31 = 7,
	GraphicsDeviceTypeOpenGLES32 = 8,
	GraphicsDeviceTypeVulkan = 9,
	GraphicsDeviceTypeBeginRange = GraphicsDeviceTypeD3D9,
	GraphicsDeviceTypeEndRange = GraphicsDeviceTypeVulkan,
	GraphicsDeviceTypeRangeSize = (GraphicsDeviceTypeEndRange - GraphicsDeviceTypeBeginRange + 1),
	GraphicsDeviceTypeMaxEnum = 0x7FFFFFFF
};

enum class GraphicsDataType : uint32_t
{
	None = 0,
	TransferSrc = 1,
	TransferDst = 2,
	UniformTexelBuffer = 3,
	UniformBuffer = 4,
	StorageTexelBuffer = 5,
	StorageBuffer = 6,
	StorageVertexBuffer = 7,
	StorageIndexBuffer = 8,
};

enum GraphicsUsageFlagBits
{
	GraphicsUsageFlagReadBit = 0x00000001,
	GraphicsUsageFlagWriteBit = 0x00000002,
	GraphicsUsageFlagPersistentBit = 0x00000004,
	GraphicsUsageFlagCoherentBit = 0x00000008,
	GraphicsUsageFlagFlushExplicitBit = 0x00000010,
	GraphicsUsageFlagDynamicStorageBit = 0x00000020,
	GraphicsUsageFlagClientStorageBit = 0x00000040
};

typedef std::uint32_t GraphicsUsageFlags;

#ifndef _TEST
typedef gli::target GraphicsTarget;
typedef gli::format GraphicsFormat;
#else
enum GraphicsTarget
{
    GraphicsTarget1D = 0, GraphicsTargetFirst = GraphicsTarget1D,
    GraphicsTarget1DArray,
    GraphicsTarget2D,
    GraphicsTarget2DArray,
    GraphicsTarget3D,
    GraphicsTargetRect,
    GraphicsTargetRectArray,
    GraphicsTargetCube,
    GraphicsTargetCubeArray, GraphicsTargetLast = GraphicsTargetCubeArray
};

enum
{
    GraphicsTargetCount = GraphicsTargetLast - GraphicsTargetFirst + 1,
};

enum GraphicsFormat
{
};
#endif
