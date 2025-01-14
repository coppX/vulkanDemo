//
// Created by FDC on 2022/1/6.
//

#ifndef HOMURA_VULKANDEVICE_H
#define HOMURA_VULKANDEVICE_H

#include <vulkan/vulkan.h>
#include <pixelFormat.h>
#include <vulkanTypes.h>
#include <optional>
#include <vector>
#include <string>

namespace Homura
{
    const std::vector<const char*> deviceRequiredExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
//            "VK_KHR_portability_subset"
    };

    const std::vector<const char*> validationLayers = {
            "VK_LAYER_KHRONOS_validation"
    };

    struct ENGINE_API QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        bool isComplete()
        {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    class ENGINE_API VulkanDevice : public std::enable_shared_from_this<VulkanDevice>
    {
    public:
        VulkanDevice(VulkanInstancePtr instance, VulkanSurfacePtr surface);
        ~VulkanDevice() = default;

        void create();
        void destroy();
        void idle();

        VkDevice getHandle()
        {
            return mDevice;
        }

        VkPhysicalDevice getPhysicalHandle()
        {
            return mPhysicalDevice;
        }

        VulkanQueuePtr getGraphicsQueue()
        {
            return mGfxQueue;
        }

        VulkanQueuePtr getPresentQueue()
        {
            return mPresent;
        }

        const VkSampleCountFlagBits& getSampleCount() const
        {
            return mMsaaSamples;
        }

        void initializeQueue();
    private:
        void pickPhysicalDevice();
        void createLogicalDevice();
        bool isDeviceSuitable(VkPhysicalDevice device);
        VkSampleCountFlagBits getMaxUsableSampleCount();
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

    private:
        VkDevice                        mDevice;
        VkPhysicalDevice                mPhysicalDevice;

        VulkanInstancePtr               mInstance;
        VulkanSurfacePtr                mSurface;

        VulkanQueuePtr                  mGfxQueue;
        VulkanQueuePtr                  mPresent;

        VkSampleCountFlagBits           mMsaaSamples;
    };
}
#endif //HOMURA_VULKANDEVICE_H
