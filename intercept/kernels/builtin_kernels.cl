/*
// Copyright (c) 2018-2020 Intel Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
*/

#if defined(cl_intel_device_side_vme_enable)

__kernel __attribute__((reqd_work_group_size(16,1,1)))
void block_motion_estimate_intel(
    sampler_t vmeAccelerator,
    __read_only image2d_t srcImage,
    __read_only image2d_t refImage,
    __global short2* predMVs,
    __global short2* outMVs,
    __global ushort* outDist,
    int iterations )
{
    __local uint dst[64];
    __local ushort* dist = (__local ushort*)&dst[ 8 * 5 ];

    int gid_0 = get_group_id(0);
    int gid_1 = 0;

    for( int i = 0; i < iterations; i++, gid_1++ )
    {
        int2 srcCoord = 0;
        int2 refCoord = 0;

        srcCoord.x = gid_0 * 16 + get_global_offset(0);
        srcCoord.y = gid_1 * 16 + get_global_offset(1);

        short2 predMV = 0;

    #ifndef HW_NULL_CHECK
        if( predMVs != NULL )
    #endif
        {
            predMV = predMVs[ gid_0 + gid_1 * get_num_groups(0) ];
            refCoord.x = predMV.x / 4;
            refCoord.y = predMV.y / 4;
            refCoord.y = refCoord.y & 0xFFFE;
        }

        intel_work_group_vme_mb_query( dst, srcCoord, refCoord, srcImage, refImage, vmeAccelerator );
        barrier(CLK_LOCAL_MEM_FENCE);

        // Write Out Result

        // 4x4
        if( intel_get_accelerator_mb_block_type( vmeAccelerator ) == 0x2 )
        {
            int x = get_local_id(0) % 4;
            int y = get_local_id(0) / 4;
            int index =
                ( gid_0 * 4 + x ) +
                ( gid_1 * 4 + y ) * get_num_groups(0) * 4;

            short2  val = as_short2( dst[ 8 + ( y * 4 + x ) * 2 ] );
            outMVs[ index ] = val;

    #ifndef HW_NULL_CHECK
            if( outDist != NULL )
    #endif
            {
                outDist[ index ] = dist[ y * 4 + x ];
            }
        }

        // 8x8
        if( intel_get_accelerator_mb_block_type( vmeAccelerator ) == 0x1 )
        {
            if( get_local_id(0) < 4 )
            {
                int x = get_local_id(0) % 2;
                int y = get_local_id(0) / 2;
                int index =
                    ( gid_0 * 2 + x ) +
                    ( gid_1 * 2 + y ) * get_num_groups(0) * 2;
                short2  val = as_short2( dst[ 8 + ( y * 2 + x ) * 8 ] );
                outMVs[ index ] = val;

    #ifndef HW_NULL_CHECK
                if( outDist != NULL )
    #endif
                {
                    outDist[ index ] = dist[ ( y * 2 + x ) * 4 ];
                }
            }
        }

        // 16x16
        if( intel_get_accelerator_mb_block_type( vmeAccelerator ) == 0x0 )
        {
            if( get_local_id(0) == 0 )
            {
                int index =
                    gid_0 +
                    gid_1 * get_num_groups(0);

                short2  val = as_short2( dst[8] );
                outMVs[ index ] = val;

    #ifndef HW_NULL_CHECK
                if( outDist != NULL )
    #endif
                {
                    outDist[ index ] = dist[ 0 ];
                }
            }
        }
    }
}

#endif
