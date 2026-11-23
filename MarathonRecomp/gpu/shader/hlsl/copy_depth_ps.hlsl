#include "copy_common.hlsli"

Texture2D<float> g_Texture2DDescriptorHeap[MARATHON_COPY_COUNT] : register(t0, space0);

float shaderMain(in float4 position : SV_Position) : SV_Depth
{
    return g_Texture2DDescriptorHeap[MARATHON_COPY_INDEX].Load(int3(position.xy, 0));
}
