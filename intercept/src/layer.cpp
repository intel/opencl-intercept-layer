/*
// Copyright (c) 2018-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/

#include "intercept.h"

#include "CL/cl_layer.h"

extern "C" CL_API_ENTRY cl_int CL_API_CALL clGetLayerInfo(
    cl_layer_info param_name,
    size_t param_value_size,
    void *param_value,
    size_t *param_value_size_ret)
{
    switch (param_name) {
        case CL_LAYER_API_VERSION: {
            const cl_layer_api_version version = CL_LAYER_API_VERSION_100;
            auto* ptr = (cl_layer_api_version*)param_value;
            return CLIntercept::writeParamToMemory(
                param_value_size,
                version,
                param_value_size_ret,
                ptr );
        }
        case CL_LAYER_NAME: {
            std::string name("CLIntercept");
#if defined(CLINTERCEPT_CMAKE)
            name += " (Version: ";
            name += CLIntercept::sc_GitDescribe;
            name += ")";
#endif
            char* ptr = (char*)param_value;
            return CLIntercept::writeStringToMemory(
                param_value_size,
                name,
                param_value_size_ret,
                ptr );
        }

        default:
            return CL_INVALID_VALUE;
    }
}

extern "C" CL_API_ENTRY cl_int CL_API_CALL clInitLayerWithProperties(
    cl_uint num_entries,
    const cl_icd_dispatch *target_dispatch,
    cl_uint *num_entries_out,
    const cl_icd_dispatch **layer_dispatch_ret,
    const cl_layer_properties* properties)
{
    if ( target_dispatch == NULL || num_entries_out == NULL || layer_dispatch_ret == NULL ) {
        return CL_INVALID_VALUE;
    }

    CLIntercept*    pIntercept = GetIntercept();
    pIntercept->initLayer(
        num_entries,
        target_dispatch,
        num_entries_out,
        layer_dispatch_ret );

    return CL_SUCCESS;
}

extern "C" CL_API_ENTRY cl_int CL_API_CALL clInitLayer(
    cl_uint num_entries,
    const cl_icd_dispatch *target_dispatch,
    cl_uint *num_entries_out,
    const cl_icd_dispatch **layer_dispatch_ret)
{
    return clInitLayerWithProperties(
        num_entries,
        target_dispatch,
        num_entries_out,
        layer_dispatch_ret,
        nullptr);
}
