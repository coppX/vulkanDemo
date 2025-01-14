//
// Created by 最上川 on 2022/1/13.
//

#ifndef HOMURA_VULKANFRAMEBUFFER_H
#define HOMURA_VULKANFRAMEBUFFER_H
#include <vulkan/vulkan.h>
#include <vulkanTypes.h>
#include <vector>

namespace Homura
{
    class ENGINE_API VulkanFramebuffer
    {
    public:
        VulkanFramebuffer(VulkanDevicePtr device, VulkanSwapChainPtr swapChain);
        ~VulkanFramebuffer() = default;

        void create(VulkanRenderPassPtr renderPass, std::vector<VulkanTexture2DPtr>& colorImages,
                    std::vector<VulkanTextureDepthPtr>& depthStencilImages);
        void destroy();

        VkFramebuffer& getHandle(uint32_t index);

        VkExtent2D& getExtent()
        {
            return mExtent;
        }
    private:
        VulkanDevicePtr                     mDevice;
        VulkanSwapChainPtr                  mSwapchain;
        VkExtent2D                          mExtent;
        std::vector<VkFramebuffer>          mFrameBuffers;
    };
}
#endif //HOMURA_VULKANFRAMEBUFFER_H
