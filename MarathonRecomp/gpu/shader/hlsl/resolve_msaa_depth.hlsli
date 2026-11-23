#pragma once

#include "copy_common.hlsli"

Texture2DMS<float, SAMPLE_COUNT> g_Texture2DMSDescriptorHeap[MARATHON_COPY_COUNT] : register(t0, space0);

float shaderMain(in float4 position : SV_Position) : SV_Depth
{
    float result = g_Texture2DMSDescriptorHeap[MARATHON_COPY_INDEX].Load(int2(position.xy), 0);
    
    [unroll] for (int i = 1; i < SAMPLE_COUNT; i++)
        result = min(result, g_Texture2DMSDescriptorHeap[MARATHON_COPY_INDEX].Load(int2(position.xy), i));

    return result;
}
