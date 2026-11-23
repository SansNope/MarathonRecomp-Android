#pragma once

struct PushConstants
{
    uint ResourceDescriptorIndex;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> g_PushConstants : register(b3, space4);

#ifdef MARATHON_FIXED_DESCRIPTORS
#define MARATHON_COPY_COUNT 1
#define MARATHON_COPY_INDEX 0
#else
#define MARATHON_COPY_COUNT
#define MARATHON_COPY_INDEX g_PushConstants.ResourceDescriptorIndex
#endif
