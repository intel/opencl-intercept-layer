/*
// Copyright (c) 2018-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/

#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable

// This is a very slow kernel, but is guaranteed to be correct.
//
// This kernel can work with any local work size.
// The global work size should be at least bytesToRead.

__kernel void CopyBufferBytes(
    const __global uchar* pSrc,
    __global uchar* pDst,
    uint srcOffsetInBytes,
    uint dstOffsetInBytes,
    uint bytesToRead )
{
    uint    index = get_global_id(0);

    pSrc += ( srcOffsetInBytes + index );
    pDst += ( dstOffsetInBytes + index );

    uint    lastIndex = bytesToRead / sizeof(uchar);

    if( index < lastIndex )
    {
        pDst[ 0 ] = pSrc[ 0 ];
    }
}

// This is a faster kernel but it only works when the source
// offset and dst offset are multiples of sizeof(uint).
//
// This kernel can work with any local work size.
// The global work size should be at least ceil( bytesToRead / sizeof(uint) ).

__kernel void CopyBufferUInts(
    const __global uint* pSrc,
    __global uint* pDst,
    uint srcOffsetInUInts,
    uint dstOffsetInUInts,
    uint bytesToRead )
{
    uint    index = get_global_id(0);

    pSrc += srcOffsetInUInts + index;
    pDst += dstOffsetInUInts + index;

    uint    lastIndex = bytesToRead / sizeof(uint);

    if( index < lastIndex )
    {
        pDst[ 0 ] = pSrc[ 0 ];
    }
    else
    {
        if( index == lastIndex )
        {
            const __global uchar*   pByteSrc = pSrc;
            __global uchar*         pByteDst = pDst;

            uint    bytesRemaining = bytesToRead % sizeof(uint);

            while( bytesRemaining )
            {
                pByteDst[ 0 ] = pByteSrc[ 0 ];

                bytesRemaining--;
                pByteSrc++;
                pByteDst++;
            }
        }
    }
}

// This is a faster kernel but it only works when the source
// offset and dst offset are multiples of sizeof(uint4).
//
// This kernel can work with any local work size.
// The global work size should be at least ceil( bytesToRead / sizeof(uint4) ).

__kernel void CopyBufferUInt4s(
    const __global uint4* pSrc,
    __global uint4* pDst,
    uint srcOffsetInUInt4s,
    uint dstOffsetInUInt4s,
    uint bytesToRead )
{
    uint    index = get_global_id(0);

    pSrc += srcOffsetInUInt4s + index;
    pDst += dstOffsetInUInt4s + index;

    uint    lastIndex = bytesToRead / sizeof(uint4);

    if( index < lastIndex )
    {
        pDst[ 0 ] = pSrc[ 0 ];
    }
    else
    {
        if( index == lastIndex )
        {
            const __global uchar*   pByteSrc = pSrc;
            __global uchar*         pByteDst = pDst;

            uint    bytesRemaining = bytesToRead % sizeof(uint4);

            while( bytesRemaining )
            {
                pByteDst[ 0 ] = pByteSrc[ 0 ];

                bytesRemaining--;
                pByteSrc++;
                pByteDst++;
            }
        }
    }
}

// This is an experimental kernel.  It only works when the source
// offset and dst offset are multiples of sizeof(uint16).
//
// This kernel can work with any local work size.
// The global work size should be at least ceil( bytesToRead / sizeof(uint16) ).

__kernel void CopyBufferUInt16s(
    const __global uint16* pSrc,
    __global uint16* pDst,
    uint srcOffsetInUInt16s,
    uint dstOffsetInUInt16s,
    uint bytesToRead )
{
    uint    index = get_global_id(0);

    pSrc += srcOffsetInUInt16s + index;
    pDst += dstOffsetInUInt16s + index;

    uint    lastIndex = bytesToRead / sizeof(uint16);

    if( index < lastIndex )
    {
        pDst[ 0 ] = pSrc[ 0 ];
    }
    else
    {
        if( index == lastIndex )
        {
            const __global uchar*   pByteSrc = pSrc;
            __global uchar*         pByteDst = pDst;

            uint    bytesRemaining = bytesToRead % sizeof(uint16);

            while( bytesRemaining )
            {
                pByteDst[ 0 ] = pByteSrc[ 0 ];

                bytesRemaining--;
                pByteSrc++;
                pByteDst++;
            }
        }
    }
}

#if __IMAGE_SUPPORT__

// Technically, this is probably required, but we can probably avoid it
// on 99% of GPU devices.
#define CHECK_IMAGE_BOUNDS()
//#define CHECK_IMAGE_BOUNDS()    if( x < regionX )

__kernel void CopyImage2Dto2DFloat(
    __read_only image2d_t srcImage,
    __write_only image2d_t dstImage,
    uint srcOriginX,
    uint srcOriginY,
    uint srcOriginZ,
    uint dstOriginX,
    uint dstOriginY,
    uint dstOriginZ,
    uint regionX,
    uint regionY,
    uint regionZ )
{
    const sampler_t samplerInline =
        CLK_NORMALIZED_COORDS_FALSE |
        CLK_ADDRESS_CLAMP_TO_EDGE |
        CLK_FILTER_NEAREST;

    uint x = get_global_id(0);
    uint y = get_global_id(1);

    CHECK_IMAGE_BOUNDS()
    {
        uint srcX = x + srcOriginX;
        uint srcY = y + srcOriginY;

        uint dstX = x + dstOriginX;
        uint dstY = y + dstOriginY;

        float4 color = read_imagef( srcImage, samplerInline, (int2)( srcX, srcY ) );

        write_imagef( dstImage, (int2)( dstX, dstY ), color );
    }
}

__kernel void CopyImage2Dto2DInt(
    __read_only image2d_t srcImage,
    __write_only image2d_t dstImage,
    uint srcOriginX,
    uint srcOriginY,
    uint srcOriginZ,
    uint dstOriginX,
    uint dstOriginY,
    uint dstOriginZ,
    uint regionX,
    uint regionY,
    uint regionZ )
{
    const sampler_t samplerInline =
        CLK_NORMALIZED_COORDS_FALSE |
        CLK_ADDRESS_CLAMP_TO_EDGE |
        CLK_FILTER_NEAREST;

    uint x = get_global_id(0);
    uint y = get_global_id(1);

    CHECK_IMAGE_BOUNDS()
    {
        uint srcX = x + srcOriginX;
        uint srcY = y + srcOriginY;

        uint dstX = x + dstOriginX;
        uint dstY = y + dstOriginY;

        int4 color = read_imagei( srcImage, samplerInline, (int2)( srcX, srcY ) );

        write_imagei( dstImage, (int2)( dstX, dstY ), color );
    }
}

__kernel void CopyImage2Dto2DUInt(
    __read_only image2d_t srcImage,
    __write_only image2d_t dstImage,
    uint srcOriginX,
    uint srcOriginY,
    uint srcOriginZ,
    uint dstOriginX,
    uint dstOriginY,
    uint dstOriginZ,
    uint regionX,
    uint regionY,
    uint regionZ )
{
    const sampler_t samplerInline =
        CLK_NORMALIZED_COORDS_FALSE |
        CLK_ADDRESS_CLAMP_TO_EDGE |
        CLK_FILTER_NEAREST;

    uint x = get_global_id(0);
    uint y = get_global_id(1);

    CHECK_IMAGE_BOUNDS()
    {
        uint srcX = x + srcOriginX;
        uint srcY = y + srcOriginY;

        uint dstX = x + dstOriginX;
        uint dstY = y + dstOriginY;

        uint4 color = read_imageui( srcImage, samplerInline, (int2)( srcX, srcY ) );

        write_imageui( dstImage, (int2)( dstX, dstY ), color );
    }
}

#endif // __IMAGE_SUPPORT__
