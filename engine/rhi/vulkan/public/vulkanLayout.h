//
// Created by FDC on 2022/1/29.
//

#ifndef HOMURA_VULKANLAYOUT_H
#define HOMURA_VULKANLAYOUT_H

#include <vulkan/vulkan.h>
#include <vulkanTypes.h>
#include <vector>

namespace Homura
{
    class ENGINE_API VulkanDescriptorSetLayout
    {
    public:
        explicit VulkanDescriptorSetLayout(VulkanDevicePtr device);
        ~VulkanDescriptorSetLayout() = default;

        void create(const std::vector<VkDescriptorSetLayoutBinding>& bindings);
        void destroy();
        VkDescriptorSetLayout& getHandle()
        {
            return mSetLayout;
        }

    private:
        VulkanDevicePtr         mDevice;
        VkDescriptorSetLayout   mSetLayout;
    };

    class ENGINE_API VulkanPipelineLayout
    {
    public:
        explicit VulkanPipelineLayout(VulkanDevicePtr device);
        ~VulkanPipelineLayout() = default;

        void create(VulkanDescriptorSetLayoutPtr descriptorSetLayout);
        void destroy();

        VkPipelineLayout& getHandle()
        {
            return mPipelineLayout;
        }
    private:
        VulkanDevicePtr     mDevice;
        VkPipelineLayout    mPipelineLayout;
    };
}

#endif //HOMURA_VULKANLAYOUT_H
