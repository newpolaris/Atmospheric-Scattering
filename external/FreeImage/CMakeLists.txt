## ==========================================================
## FreeImage 3
##
## Design and implementation by
## - thfabian (fabian_thuering@hotmail.com)
##
##
## This file is part of FreeImage 3
##
## COVERED CODE IS PROVIDED UNDER THIS LICENSE ON AN "AS IS" BASIS, WITHOUT WARRANTY
## OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, WITHOUT LIMITATION, WARRANTIES
## THAT THE COVERED CODE IS FREE OF DEFECTS, MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE
## OR NON-INFRINGING. THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE COVERED
## CODE IS WITH YOU. SHOULD ANY COVERED CODE PROVE DEFECTIVE IN ANY RESPECT, YOU (NOT
## THE INITIAL DEVELOPER OR ANY OTHER CONTRIBUTOR) ASSUME THE COST OF ANY NECESSARY
## SERVICING, REPAIR OR CORRECTION. THIS DISCLAIMER OF WARRANTY CONSTITUTES AN ESSENTIAL
## PART OF THIS LICENSE. NO USE OF ANY COVERED CODE IS AUTHORIZED HEREUNDER EXCEPT UNDER
## THIS DISCLAIMER.
##
## Use at your own risk!
## ==========================================================

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING 
      "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

project(FreeImage C CXX)
cmake_minimum_required(VERSION 3.0)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" "${CMAKE_SOURCE_DIR}/cmake/modules")

option(FREEIMAGE_TESTING "Build tests" OFF)
option(BUILD_SHARED_LIBS "Allow linking against shared libraries (adds -fPIC)" ON)

# Windows
if(WIN32) 
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /w")

# MacOSX
elseif(APPLE)
  if(BUILD_SHARED_LIBS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
  endif()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")

# Linux and other *nixs
else()
  if(BUILD_SHARED_LIBS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
  endif()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")
endif()

set(FREEIMAGE_VERSION_MAJOR 3)
set(FREEIMAGE_VERSION_MINOR 15)
set(FREEIMAGE_VERSION_PATCH 3)

set(FreeImage_SOURCES
  src/CacheFile.h
  src/DeprecationManager/Deprecated.cpp
  src/DeprecationManager/DeprecationMgr.cpp
  src/DeprecationManager/DeprecationMgr.h
  src/FreeImage.h
  src/FreeImage/BitmapAccess.cpp
  src/FreeImage/CacheFile.cpp
  src/FreeImage/ColorLookup.cpp
  src/FreeImage/Conversion.cpp
  src/FreeImage/Conversion16_555.cpp
  src/FreeImage/Conversion16_565.cpp
  src/FreeImage/Conversion24.cpp
  src/FreeImage/Conversion32.cpp
  src/FreeImage/ConversionFloat.cpp
  src/FreeImage/ConversionRGB16.cpp
  src/FreeImage/Conversion4.cpp
  src/FreeImage/Conversion8.cpp
  src/FreeImage/ConversionRGBF.cpp
  src/FreeImage/ConversionType.cpp
  src/FreeImage/ConversionUINT16.cpp
  src/FreeImage/FreeImage.cpp
  src/FreeImage/FreeImageIO.cpp
  src/FreeImage/GetType.cpp
  src/FreeImage/Halftoning.cpp
  src/FreeImage/J2KHelper.cpp
  src/FreeImage/MemoryIO.cpp
  src/FreeImage/MNGHelper.cpp
  src/FreeImage/MultiPage.cpp
  src/FreeImage/NNQuantizer.cpp
  src/FreeImage/PSDParser.cpp
  src/FreeImage/PSDParser.h
  src/FreeImage/PixelAccess.cpp
  src/FreeImage/Plugin.cpp
  src/FreeImage/PluginBMP.cpp
  src/FreeImage/PluginCUT.cpp
  src/FreeImage/PluginDDS.cpp
  src/FreeImage/PluginEXR.cpp
  src/FreeImage/PluginG3.cpp
  src/FreeImage/PluginGIF.cpp
  src/FreeImage/PluginHDR.cpp
  src/FreeImage/PluginICO.cpp
  src/FreeImage/PluginIFF.cpp
  src/FreeImage/PluginJ2K.cpp
  src/FreeImage/PluginJNG.cpp
  src/FreeImage/PluginJP2.cpp
  src/FreeImage/PluginJPEG.cpp
  src/FreeImage/PluginKOALA.cpp
  src/FreeImage/PluginMNG.cpp
  src/FreeImage/PluginPCD.cpp
  src/FreeImage/PluginPCX.cpp
  src/FreeImage/PluginPFM.cpp
  src/FreeImage/PluginPICT.cpp
  src/FreeImage/PluginPNG.cpp
  src/FreeImage/PluginPNM.cpp
  src/FreeImage/PluginPSD.cpp
  src/FreeImage/PluginRAS.cpp
  src/FreeImage/PluginRAW.cpp
  src/FreeImage/PluginSGI.cpp
  src/FreeImage/PluginTARGA.cpp
  src/FreeImage/PluginTIFF.cpp
  src/FreeImage/PluginWBMP.cpp
  src/FreeImage/PluginXBM.cpp
  src/FreeImage/PluginXPM.cpp
  src/FreeImage/TIFFLogLuv.cpp
  src/FreeImage/ToneMapping.cpp
  src/FreeImage/WuQuantizer.cpp
  src/FreeImage/ZLibInterface.cpp
  src/FreeImage/tmoColorConvert.cpp
  src/FreeImage/tmoDrago03.cpp
  src/FreeImage/tmoFattal02.cpp
  src/FreeImage/tmoReinhard05.cpp
  src/FreeImageIO.h
  src/FreeImageToolkit/BSplineRotate.cpp
  src/FreeImageToolkit/Background.cpp
  src/FreeImageToolkit/Channels.cpp
  src/FreeImageToolkit/ClassicRotate.cpp
  src/FreeImageToolkit/Colors.cpp
  src/FreeImageToolkit/CopyPaste.cpp
  src/FreeImageToolkit/Display.cpp
  src/FreeImageToolkit/Filters.h
  src/FreeImageToolkit/Flip.cpp
  src/FreeImageToolkit/JPEGTransform.cpp
  src/FreeImageToolkit/MultigridPoissonSolver.cpp
  src/FreeImageToolkit/Rescale.cpp
  src/FreeImageToolkit/Resize.cpp
  src/FreeImageToolkit/Resize.h
  src/LibJPEG/jaricom.c
  src/LibJPEG/jcapimin.c
  src/LibJPEG/jcapistd.c
  src/LibJPEG/jcarith.c
  src/LibJPEG/jccoefct.c
  src/LibJPEG/jccolor.c
  src/LibJPEG/jcdctmgr.c
  src/LibJPEG/jchuff.c
  src/LibJPEG/jcinit.c
  src/LibJPEG/jcmainct.c
  src/LibJPEG/jcmarker.c
  src/LibJPEG/jcmaster.c
  src/LibJPEG/jcomapi.c
  src/LibJPEG/jconfig.h
  src/LibJPEG/jcparam.c
  src/LibJPEG/jcprepct.c
  src/LibJPEG/jcsample.c
  src/LibJPEG/jctrans.c
  src/LibJPEG/jdapimin.c
  src/LibJPEG/jdapistd.c
  src/LibJPEG/jdarith.c
  src/LibJPEG/jdatadst.c
  src/LibJPEG/jdatasrc.c
  src/LibJPEG/jdcoefct.c
  src/LibJPEG/jdcolor.c
  src/LibJPEG/jdct.h
  src/LibJPEG/jddctmgr.c
  src/LibJPEG/jdhuff.c
  src/LibJPEG/jdinput.c
  src/LibJPEG/jdmainct.c
  src/LibJPEG/jdmarker.c
  src/LibJPEG/jdmaster.c
  src/LibJPEG/jdmerge.c
  src/LibJPEG/jdpostct.c
  src/LibJPEG/jdsample.c
  src/LibJPEG/jdtrans.c
  src/LibJPEG/jerror.c
  src/LibJPEG/jerror.h
  src/LibJPEG/jfdctflt.c
  src/LibJPEG/jfdctfst.c
  src/LibJPEG/jfdctint.c
  src/LibJPEG/jidctflt.c
  src/LibJPEG/jidctfst.c
  src/LibJPEG/jidctint.c
  src/LibJPEG/jinclude.h
  src/LibJPEG/jmemmgr.c
  src/LibJPEG/jmemnobs.c
  src/LibJPEG/jmemsys.h
  src/LibJPEG/jmorecfg.h
  src/LibJPEG/jpegint.h
  src/LibJPEG/jpeglib.h
  src/LibJPEG/jquant1.c
  src/LibJPEG/jquant2.c
  src/LibJPEG/jutils.c
  src/LibJPEG/jversion.h
  src/LibJPEG/transupp.c
  src/LibJPEG/transupp.h
  src/LibOpenJPEG/bio.c
  src/LibOpenJPEG/bio.h
  src/LibOpenJPEG/cidx_manager.c
  src/LibOpenJPEG/cidx_manager.h
  src/LibOpenJPEG/cio.c
  src/LibOpenJPEG/cio.h
  src/LibOpenJPEG/dwt.c
  src/LibOpenJPEG/dwt.h
  src/LibOpenJPEG/event.c
  src/LibOpenJPEG/event.h
  src/LibOpenJPEG/fix.h
  src/LibOpenJPEG/image.c
  src/LibOpenJPEG/image.h
  src/LibOpenJPEG/indexbox_manager.h
  src/LibOpenJPEG/int.h
  src/LibOpenJPEG/j2k.c
  src/LibOpenJPEG/j2k.h
  src/LibOpenJPEG/j2k_lib.c
  src/LibOpenJPEG/j2k_lib.h
  src/LibOpenJPEG/jp2.c
  src/LibOpenJPEG/jp2.h
  src/LibOpenJPEG/jpt.c
  src/LibOpenJPEG/jpt.h
  src/LibOpenJPEG/mct.c
  src/LibOpenJPEG/mct.h
  src/LibOpenJPEG/mqc.c
  src/LibOpenJPEG/mqc.h
  src/LibOpenJPEG/openjpeg.c
  src/LibOpenJPEG/openjpeg.h
  src/LibOpenJPEG/opj_config.h
  src/LibOpenJPEG/opj_includes.h
  src/LibOpenJPEG/opj_malloc.h
  src/LibOpenJPEG/phix_manager.c
  src/LibOpenJPEG/pi.c
  src/LibOpenJPEG/pi.h
  src/LibOpenJPEG/ppix_manager.c
  src/LibOpenJPEG/raw.c
  src/LibOpenJPEG/raw.h
  src/LibOpenJPEG/t1.c
  src/LibOpenJPEG/t1.h
  src/LibOpenJPEG/t1_luts.h
  src/LibOpenJPEG/t2.c
  src/LibOpenJPEG/t2.h
  src/LibOpenJPEG/tcd.c
  src/LibOpenJPEG/tcd.h
  src/LibOpenJPEG/tgt.c
  src/LibOpenJPEG/tgt.h
  src/LibOpenJPEG/thix_manager.c
  src/LibOpenJPEG/tpix_manager.c
  src/LibPNG/png.c
  src/LibPNG/png.h
  src/LibPNG/pngconf.h
  src/LibPNG/pngdebug.h
  src/LibPNG/pngerror.c
  src/LibPNG/pnggccrd.c
  src/LibPNG/pngget.c
  src/LibPNG/pnginfo.h
  src/LibPNG/pnglibconf.h
  src/LibPNG/pngmem.c
  src/LibPNG/pngpread.c
  src/LibPNG/pngread.c
  src/LibPNG/pngpriv.h
  src/LibPNG/pngrio.c
  src/LibPNG/pngrtran.c
  src/LibPNG/pngrutil.c
  src/LibPNG/pngset.c
  src/LibPNG/pngstruct.h
  src/LibPNG/pngtrans.c
  src/LibPNG/pngvcrd.c
  src/LibPNG/pngwio.c
  src/LibPNG/pngwrite.c
  src/LibPNG/pngwtran.c
  src/LibPNG/pngwutil.c
  src/LibRawLite/internal/dcraw_common.cpp
  src/LibRawLite/internal/dcraw_fileio.cpp
  src/LibRawLite/internal/defines.h
  src/LibRawLite/internal/demosaic_packs.cpp
  src/LibRawLite/internal/libraw_bytebuffer.h
  src/LibRawLite/internal/libraw_internal_funcs.h
  src/LibRawLite/internal/var_defines.h
  src/LibRawLite/libraw/libraw.h
  src/LibRawLite/libraw/libraw_alloc.h
  src/LibRawLite/libraw/libraw_const.h
  src/LibRawLite/libraw/libraw_datastream.h
  src/LibRawLite/libraw/libraw_internal.h
  src/LibRawLite/libraw/libraw_types.h
  src/LibRawLite/libraw/libraw_version.h
  src/LibRawLite/src/libraw_c_api.cpp
  src/LibRawLite/src/libraw_cxx.cpp
  src/LibRawLite/src/libraw_datastream.cpp
  src/LibTIFF4/t4.h
  src/LibTIFF4/tif_aux.c
  src/LibTIFF4/tif_close.c
  src/LibTIFF4/tif_codec.c
  src/LibTIFF4/tif_color.c
  src/LibTIFF4/tif_compress.c
  src/LibTIFF4/tif_config.h
  src/LibTIFF4/tif_dir.c
  src/LibTIFF4/tif_dir.h
  src/LibTIFF4/tif_dirinfo.c
  src/LibTIFF4/tif_dirread.c
  src/LibTIFF4/tif_dirwrite.c
  src/LibTIFF4/tif_dumpmode.c
  src/LibTIFF4/tif_error.c
  src/LibTIFF4/tif_extension.c
  src/LibTIFF4/tif_fax3.c
  src/LibTIFF4/tif_fax3.h
  src/LibTIFF4/tif_fax3sm.c
  src/LibTIFF4/tif_flush.c
  src/LibTIFF4/tif_getimage.c
  src/LibTIFF4/tif_jpeg.c
  src/LibTIFF4/tif_jpeg_12.c
  src/LibTIFF4/tif_luv.c
  src/LibTIFF4/tif_lzma.c
  src/LibTIFF4/tif_lzw.c
  src/LibTIFF4/tif_next.c
  src/LibTIFF4/tif_ojpeg.c
  src/LibTIFF4/tif_open.c
  src/LibTIFF4/tif_packbits.c
  src/LibTIFF4/tif_pixarlog.c
  src/LibTIFF4/tif_predict.c
  src/LibTIFF4/tif_predict.h
  src/LibTIFF4/tif_print.c
  src/LibTIFF4/tif_read.c
  src/LibTIFF4/tif_strip.c
  src/LibTIFF4/tif_swab.c
  src/LibTIFF4/tif_thunder.c
  src/LibTIFF4/tif_tile.c
  src/LibTIFF4/tif_version.c
  src/LibTIFF4/tif_warning.c
  src/LibTIFF4/tif_write.c
  src/LibTIFF4/tif_zip.c
  src/LibTIFF4/tiff.h
  src/LibTIFF4/tiffio.h
  src/LibTIFF4/tiffiop.h
  src/LibTIFF4/tiffvers.h
  src/LibTIFF4/uvcode.h
  src/Metadata/Exif.cpp
  src/Metadata/FIRational.cpp
  src/Metadata/FIRational.h
  src/Metadata/FreeImageTag.cpp
  src/Metadata/FreeImageTag.h
  src/Metadata/IPTC.cpp
  src/Metadata/TagConversion.cpp
  src/Metadata/TagLib.cpp
  src/Metadata/XTIFF.cpp
  src/OpenEXR/Half/eLut.h
  src/OpenEXR/Half/half.cpp
  src/OpenEXR/Half/half.h
  src/OpenEXR/Half/halfFunction.h
  src/OpenEXR/Half/halfLimits.h
  src/OpenEXR/Half/toFloat.h
  src/OpenEXR/Iex/Iex.h
  src/OpenEXR/Iex/IexBaseExc.cpp
  src/OpenEXR/Iex/IexBaseExc.h
  src/OpenEXR/Iex/IexErrnoExc.h
  src/OpenEXR/Iex/IexMacros.h
  src/OpenEXR/Iex/IexMathExc.h
  src/OpenEXR/Iex/IexThrowErrnoExc.cpp
  src/OpenEXR/Iex/IexThrowErrnoExc.h
  src/OpenEXR/IlmBaseConfig.h
  src/OpenEXR/IlmImf/b44ExpLogTable.h
  src/OpenEXR/IlmImf/ImfAcesFile.cpp
  src/OpenEXR/IlmImf/ImfAcesFile.h
  src/OpenEXR/IlmImf/ImfArray.h
  src/OpenEXR/IlmImf/ImfAttribute.cpp
  src/OpenEXR/IlmImf/ImfAttribute.h
  src/OpenEXR/IlmImf/ImfAutoArray.h
  src/OpenEXR/IlmImf/ImfB44Compressor.cpp
  src/OpenEXR/IlmImf/ImfB44Compressor.h
  src/OpenEXR/IlmImf/ImfBoxAttribute.cpp
  src/OpenEXR/IlmImf/ImfBoxAttribute.h
  src/OpenEXR/IlmImf/ImfCRgbaFile.cpp
  src/OpenEXR/IlmImf/ImfCRgbaFile.h
  src/OpenEXR/IlmImf/ImfChannelList.cpp
  src/OpenEXR/IlmImf/ImfChannelList.h
  src/OpenEXR/IlmImf/ImfChannelListAttribute.cpp
  src/OpenEXR/IlmImf/ImfChannelListAttribute.h
  src/OpenEXR/IlmImf/ImfChromaticities.cpp
  src/OpenEXR/IlmImf/ImfChromaticities.h
  src/OpenEXR/IlmImf/ImfChromaticitiesAttribute.cpp
  src/OpenEXR/IlmImf/ImfChromaticitiesAttribute.h
  src/OpenEXR/IlmImf/ImfCompression.h
  src/OpenEXR/IlmImf/ImfCompressionAttribute.cpp
  src/OpenEXR/IlmImf/ImfCompressionAttribute.h
  src/OpenEXR/IlmImf/ImfCompressor.cpp
  src/OpenEXR/IlmImf/ImfCompressor.h
  src/OpenEXR/IlmImf/ImfConvert.cpp
  src/OpenEXR/IlmImf/ImfConvert.h
  src/OpenEXR/IlmImf/ImfDoubleAttribute.cpp
  src/OpenEXR/IlmImf/ImfDoubleAttribute.h
  src/OpenEXR/IlmImf/ImfEnvmap.cpp
  src/OpenEXR/IlmImf/ImfEnvmap.h
  src/OpenEXR/IlmImf/ImfEnvmapAttribute.cpp
  src/OpenEXR/IlmImf/ImfEnvmapAttribute.h
  src/OpenEXR/IlmImf/ImfFloatAttribute.cpp
  src/OpenEXR/IlmImf/ImfFloatAttribute.h
  src/OpenEXR/IlmImf/ImfFrameBuffer.cpp
  src/OpenEXR/IlmImf/ImfFrameBuffer.h
  src/OpenEXR/IlmImf/ImfFramesPerSecond.cpp
  src/OpenEXR/IlmImf/ImfFramesPerSecond.h
  src/OpenEXR/IlmImf/ImfHeader.cpp
  src/OpenEXR/IlmImf/ImfHeader.h
  src/OpenEXR/IlmImf/ImfHuf.cpp
  src/OpenEXR/IlmImf/ImfHuf.h
  src/OpenEXR/IlmImf/ImfIO.cpp
  src/OpenEXR/IlmImf/ImfIO.h
  src/OpenEXR/IlmImf/ImfInputFile.cpp
  src/OpenEXR/IlmImf/ImfInputFile.h
  src/OpenEXR/IlmImf/ImfInt64.h
  src/OpenEXR/IlmImf/ImfIntAttribute.cpp
  src/OpenEXR/IlmImf/ImfIntAttribute.h
  src/OpenEXR/IlmImf/ImfKeyCode.cpp
  src/OpenEXR/IlmImf/ImfKeyCode.h
  src/OpenEXR/IlmImf/ImfKeyCodeAttribute.cpp
  src/OpenEXR/IlmImf/ImfKeyCodeAttribute.h
  src/OpenEXR/IlmImf/ImfLineOrder.h
  src/OpenEXR/IlmImf/ImfLineOrderAttribute.cpp
  src/OpenEXR/IlmImf/ImfLineOrderAttribute.h
  src/OpenEXR/IlmImf/ImfLut.cpp
  src/OpenEXR/IlmImf/ImfLut.h
  src/OpenEXR/IlmImf/ImfMatrixAttribute.cpp
  src/OpenEXR/IlmImf/ImfMatrixAttribute.h
  src/OpenEXR/IlmImf/ImfMisc.cpp
  src/OpenEXR/IlmImf/ImfMisc.h
  src/OpenEXR/IlmImf/ImfMultiView.cpp
  src/OpenEXR/IlmImf/ImfMultiView.h
  src/OpenEXR/IlmImf/ImfName.h
  src/OpenEXR/IlmImf/ImfOpaqueAttribute.cpp
  src/OpenEXR/IlmImf/ImfOpaqueAttribute.h
  src/OpenEXR/IlmImf/ImfOutputFile.cpp
  src/OpenEXR/IlmImf/ImfOutputFile.h
  src/OpenEXR/IlmImf/ImfPixelType.h
  src/OpenEXR/IlmImf/ImfPizCompressor.cpp
  src/OpenEXR/IlmImf/ImfPizCompressor.h
  src/OpenEXR/IlmImf/ImfPreviewImage.cpp
  src/OpenEXR/IlmImf/ImfPreviewImage.h
  src/OpenEXR/IlmImf/ImfPreviewImageAttribute.cpp
  src/OpenEXR/IlmImf/ImfPreviewImageAttribute.h
  src/OpenEXR/IlmImf/ImfPxr24Compressor.cpp
  src/OpenEXR/IlmImf/ImfPxr24Compressor.h
  src/OpenEXR/IlmImf/ImfRational.cpp
  src/OpenEXR/IlmImf/ImfRational.h
  src/OpenEXR/IlmImf/ImfRationalAttribute.cpp
  src/OpenEXR/IlmImf/ImfRationalAttribute.h
  src/OpenEXR/IlmImf/ImfRgba.h
  src/OpenEXR/IlmImf/ImfRgbaFile.cpp
  src/OpenEXR/IlmImf/ImfRgbaFile.h
  src/OpenEXR/IlmImf/ImfRgbaYca.cpp
  src/OpenEXR/IlmImf/ImfRgbaYca.h
  src/OpenEXR/IlmImf/ImfRleCompressor.cpp
  src/OpenEXR/IlmImf/ImfRleCompressor.h
  src/OpenEXR/IlmImf/ImfScanLineInputFile.cpp
  src/OpenEXR/IlmImf/ImfScanLineInputFile.h
  src/OpenEXR/IlmImf/ImfStandardAttributes.cpp
  src/OpenEXR/IlmImf/ImfStandardAttributes.h
  src/OpenEXR/IlmImf/ImfStdIO.cpp
  src/OpenEXR/IlmImf/ImfStdIO.h
  src/OpenEXR/IlmImf/ImfStringAttribute.cpp
  src/OpenEXR/IlmImf/ImfStringAttribute.h
  src/OpenEXR/IlmImf/ImfStringVectorAttribute.cpp
  src/OpenEXR/IlmImf/ImfStringVectorAttribute.h
  src/OpenEXR/IlmImf/ImfTestFile.cpp
  src/OpenEXR/IlmImf/ImfTestFile.h
  src/OpenEXR/IlmImf/ImfThreading.cpp
  src/OpenEXR/IlmImf/ImfThreading.h
  src/OpenEXR/IlmImf/ImfTileDescription.h
  src/OpenEXR/IlmImf/ImfTileDescriptionAttribute.cpp
  src/OpenEXR/IlmImf/ImfTileDescriptionAttribute.h
  src/OpenEXR/IlmImf/ImfTileOffsets.cpp
  src/OpenEXR/IlmImf/ImfTileOffsets.h
  src/OpenEXR/IlmImf/ImfTiledInputFile.cpp
  src/OpenEXR/IlmImf/ImfTiledInputFile.h
  src/OpenEXR/IlmImf/ImfTiledMisc.cpp
  src/OpenEXR/IlmImf/ImfTiledMisc.h
  src/OpenEXR/IlmImf/ImfTiledOutputFile.cpp
  src/OpenEXR/IlmImf/ImfTiledOutputFile.h
  src/OpenEXR/IlmImf/ImfTiledRgbaFile.cpp
  src/OpenEXR/IlmImf/ImfTiledRgbaFile.h
  src/OpenEXR/IlmImf/ImfTimeCode.cpp
  src/OpenEXR/IlmImf/ImfTimeCode.h
  src/OpenEXR/IlmImf/ImfTimeCodeAttribute.cpp
  src/OpenEXR/IlmImf/ImfTimeCodeAttribute.h
  src/OpenEXR/IlmImf/ImfVecAttribute.cpp
  src/OpenEXR/IlmImf/ImfVecAttribute.h
  src/OpenEXR/IlmImf/ImfVersion.cpp
  src/OpenEXR/IlmImf/ImfVersion.h
  src/OpenEXR/IlmImf/ImfWav.cpp
  src/OpenEXR/IlmImf/ImfWav.h
  src/OpenEXR/IlmImf/ImfXdr.h
  src/OpenEXR/IlmImf/ImfZipCompressor.cpp
  src/OpenEXR/IlmImf/ImfZipCompressor.h
  src/OpenEXR/IlmThread/IlmThread.cpp
  src/OpenEXR/IlmThread/IlmThread.h
  src/OpenEXR/IlmThread/IlmThreadMutex.cpp
  src/OpenEXR/IlmThread/IlmThreadMutex.h
  src/OpenEXR/IlmThread/IlmThreadPool.cpp
  src/OpenEXR/IlmThread/IlmThreadPool.h
  src/OpenEXR/IlmThread/IlmThreadSemaphore.cpp
  src/OpenEXR/IlmThread/IlmThreadSemaphore.h
  src/OpenEXR/Imath/ImathBox.cpp
  src/OpenEXR/Imath/ImathBox.h
  src/OpenEXR/Imath/ImathBoxAlgo.h
  src/OpenEXR/Imath/ImathColor.h
  src/OpenEXR/Imath/ImathColorAlgo.cpp
  src/OpenEXR/Imath/ImathColorAlgo.h
  src/OpenEXR/Imath/ImathEuler.h
  src/OpenEXR/Imath/ImathExc.h
  src/OpenEXR/Imath/ImathFrame.h
  src/OpenEXR/Imath/ImathFrustum.h
  src/OpenEXR/Imath/ImathFun.cpp
  src/OpenEXR/Imath/ImathFun.h
  src/OpenEXR/Imath/ImathGL.h
  src/OpenEXR/Imath/ImathGLU.h
  src/OpenEXR/Imath/ImathHalfLimits.h
  src/OpenEXR/Imath/ImathInt64.h
  src/OpenEXR/Imath/ImathInterval.h
  src/OpenEXR/Imath/ImathLimits.h
  src/OpenEXR/Imath/ImathLine.h
  src/OpenEXR/Imath/ImathLineAlgo.h
  src/OpenEXR/Imath/ImathMath.h
  src/OpenEXR/Imath/ImathMatrix.h
  src/OpenEXR/Imath/ImathMatrixAlgo.cpp
  src/OpenEXR/Imath/ImathMatrixAlgo.h
  src/OpenEXR/Imath/ImathPlane.h
  src/OpenEXR/Imath/ImathPlatform.h
  src/OpenEXR/Imath/ImathQuat.h
  src/OpenEXR/Imath/ImathRandom.cpp
  src/OpenEXR/Imath/ImathRandom.h
  src/OpenEXR/Imath/ImathRoots.h
  src/OpenEXR/Imath/ImathShear.cpp
  src/OpenEXR/Imath/ImathShear.h
  src/OpenEXR/Imath/ImathSphere.h
  src/OpenEXR/Imath/ImathVec.cpp
  src/OpenEXR/Imath/ImathVec.h
  src/OpenEXR/Imath/ImathVecAlgo.h
  src/OpenEXR/OpenEXRConfig.h
  src/Plugin.h
  src/Quantizers.h
  src/ToneMapping.h
  src/Utilities.h
  src/ZLib/adler32.c
  src/ZLib/compress.c
  src/ZLib/crc32.c
  src/ZLib/crc32.h
  src/ZLib/deflate.c
  src/ZLib/deflate.h
  src/ZLib/gzclose.c
  src/ZLib/gzguts.h
  src/ZLib/gzlib.c
  src/ZLib/gzread.c
  src/ZLib/gzwrite.c
  src/ZLib/infback.c
  src/ZLib/inffast.c
  src/ZLib/inffast.h
  src/ZLib/inffixed.h
  src/ZLib/inflate.c
  src/ZLib/inflate.h
  src/ZLib/inftrees.c
  src/ZLib/inftrees.h
  src/ZLib/trees.c
  src/ZLib/trees.h
  src/ZLib/uncompr.c
  src/ZLib/zconf.h
  src/ZLib/zlib.h
  src/ZLib/zutil.c
  src/ZLib/zutil.h
)

include_directories(SYSTEM src)
include_directories(SYSTEM src/FreeImageToolkit)
include_directories(SYSTEM src/LibJPEG)
include_directories(SYSTEM src/LibMNG)
include_directories(SYSTEM src/LibOpenJPEG)
include_directories(SYSTEM src/LibPNG)
include_directories(SYSTEM src/LibRawLite)
include_directories(SYSTEM src/LibTIFF)
include_directories(SYSTEM src/LibJXR)
include_directories(SYSTEM src/LibJXR/jxrgluelib)
include_directories(SYSTEM src/LibJXR/image/sys)
include_directories(SYSTEM src/LibJXR/image/decode)
include_directories(SYSTEM src/LibJXR/image/x86)
include_directories(SYSTEM src/LibJXR/common/include)
include_directories(SYSTEM src/OpenEXR)
include_directories(SYSTEM src/OpenEXR/Half)
include_directories(SYSTEM src/OpenEXR/Iex)
include_directories(SYSTEM src/OpenEXR/IlmImf)
include_directories(SYSTEM src/OpenEXR/IlmThread)
include_directories(SYSTEM src/OpenEXR/IexMath)
include_directories(SYSTEM src/OpenEXR/Imath)
include_directories(SYSTEM src/ZLib)

add_definitions(-DOPJ_STATIC -DLIBRAW_NODLL -DLIBRAW_LIBRARY_BUILD -DFREEIMAGE_LIB -DNO_LCMS)
add_library(FreeImage STATIC ${FreeImage_SOURCES})

install(FILES src/FreeImage.h DESTINATION include/FreeImage)
install(TARGETS FreeImage 
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

if(FREEIMAGE_TESTING)
  enable_testing()
  set(FreeImageTest_SOURCES
    test/TestSuite.h 
    test/MainTestSuite.cpp 
    test/testHeaderOnly.cpp 
    test/testChannels.cpp 
    test/testImageType.cpp 
    test/testMemIO.cpp 
    test/testMPage.cpp 
    test/testMPageMemory.cpp 
    test/testMPageStream.cpp 
    test/testPlugins.cpp 
    test/testThumbnail.cpp 
    test/testTools.cpp
    test/TestSuite.h 
  )

  add_executable(FreeImageTest ${FreeImageTest_SOURCES} )
  target_link_libraries(FreeImageTest FreeImage)
  add_test(NAME FreeImageTest COMMAND FreeImageTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/test)

  add_custom_command(
    TARGET FreeImageTest
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/test/ressource
                                               ${CMAKE_BINARY_DIR}/test/ressource
  )
endif()

