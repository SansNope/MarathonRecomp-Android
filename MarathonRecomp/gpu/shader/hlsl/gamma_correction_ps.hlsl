#include "../../../../tools/XenosRecomp/XenosRecomp/shader_common.h"

#ifdef __spirv__

#define g_Gamma vk::RawBufferLoad<float>(g_PushConstants.SharedConstants + 0)
#define g_TextureDescriptorIndex vk::RawBufferLoad<uint>(g_PushConstants.SharedConstants + 4)

#define g_ViewportOffset vk::RawBufferLoad<int2>(g_PushConstants.SharedConstants + 8)
#define g_ViewportSize vk::RawBufferLoad<int2>(g_PushConstants.SharedConstants + 16)

#define g_SwapChainSize vk::RawBufferLoad<int2>(g_PushConstants.SharedConstants + 24)
#define g_PreRotation vk::RawBufferLoad<uint>(g_PushConstants.SharedConstants + 32)

#else

cbuffer SharedConstants : register(b2, space4)
{
    float g_Gamma;
    uint g_TextureDescriptorIndex;
    int2 g_ViewportOffset;
    int2 g_ViewportSize;
    int2 g_SwapChainSize;
    uint g_PreRotation;
};

#endif

float4 shaderMain(in float4 position : SV_Position) : SV_Target
{
    Texture2D<float4> texture = g_Texture2DDescriptorHeap[g_TextureDescriptorIndex];
    
    // Map the swap chain texture pixel back to window orientation when the swap chain is pre-rotated
    // clockwise; g_SwapChainSize is in window orientation.
    int2 windowPosition = int2(position.xy);
    if (g_PreRotation == 90)
        windowPosition = int2(windowPosition.y, g_SwapChainSize.y - 1 - windowPosition.x);
    else if (g_PreRotation == 180)
        windowPosition = g_SwapChainSize - 1 - windowPosition;
    else if (g_PreRotation == 270)
        windowPosition = int2(g_SwapChainSize.x - 1 - windowPosition.y, windowPosition.x);

    int2 movedPosition = windowPosition - g_ViewportOffset;
    bool boxed = any(movedPosition < 0) || any(movedPosition >= g_ViewportSize);
    if (boxed) movedPosition = 0;
    
    float4 color = boxed ? 0.0 : texture.Load(int3(movedPosition, 0));
    color.rgb = pow(color.rgb, g_Gamma);
    return color;
}
