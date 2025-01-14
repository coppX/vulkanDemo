//
// Created by 最上川 on 2022/1/9.
//

#ifndef HOMURA_VULKANBUFFER_H
#define HOMURA_VULKANBUFFER_H
#include <vulkan/vulkan.h>
#include <vulkanTypes.h>

namespace Homura
{
    class ENGINE_API VulkanBuffer
    {
    public:
        VulkanBuffer(std::shared_ptr<VulkanDevice> device, VulkanCommandBufferPtr commandBuffer, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props);
        ~VulkanBuffer() = default;

        void create();
        void destroy();

        void fillBuffer(void *inData, uint64_t size);
        void copyBuffer(VulkanBuffer& srcBuffer, VulkanBuffer& dstBuffer, VkDeviceSize size);
        void copyToTexture(VulkanTexture2DPtr texture, uint32_t width, uint32_t height);

        void updateBufferByStaging(void *pData, uint32_t size);
        VkBuffer getHandle()
        {
            return mBuffer;
        }

        VkDeviceSize& getSize()
        {
            return mSize;
        }

    private:
        uint32_t  findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

        VulkanDevicePtr         mDevice;
        VulkanCommandBufferPtr  mCommandBuffer;

        VkBufferUsageFlags      mUsage;
        VkMemoryPropertyFlags   mProperties;
    public:
        VkDeviceSize            mSize;
        VkBuffer                mBuffer;
        VkDeviceMemory          mBufferMemory;
        VulkanBuffer*           mStagingBuffer;
    };

    class ENGINE_API VulkanVertexBuffer : public VulkanBuffer
    {
    public:
        VulkanVertexBuffer(VulkanDevicePtr device, VulkanCommandBufferPtr commandBuffer, VkDeviceSize size, void* pData)
            : VulkanBuffer(device, commandBuffer, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
        {
            updateBufferByStaging(pData, size);
        }
    };

    class ENGINE_API VulkanIndexBuffer : public VulkanBuffer
    {
    public:
        VulkanIndexBuffer(VulkanDevicePtr device, VulkanCommandBufferPtr commandBuffer, VkDeviceSize size, void* pData)
            : VulkanBuffer(device, commandBuffer, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
        {
            updateBufferByStaging(pData, size);
        }
    };

    class ENGINE_API VulkanUniformBuffer : public VulkanBuffer
    {
    public:
        VulkanUniformBuffer(VulkanDevicePtr device, VulkanCommandBufferPtr commandBuffer, VkDeviceSize size, uint32_t binding)
            : VulkanBuffer(device, commandBuffer, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
            , mCallback{}
            , mDataBuffer{}
            , mBinding{binding}
            , mBufferInfo{}
        {
            mDataBuffer.resize(size);
        }
        
        VkWriteDescriptorSet createWriteDescriptorSet(VkDescriptorSet descriptorSet)
        {
            
            mBufferInfo.buffer                  = mBuffer;
            mBufferInfo.offset                  = 0;
            mBufferInfo.range                   = mSize;
            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType               = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet              = descriptorSet;
            descriptorWrite.dstBinding          = mBinding;
            descriptorWrite.dstArrayElement     = 0;
            descriptorWrite.descriptorType      = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount     = 1;
            descriptorWrite.pBufferInfo         = &mBufferInfo;
            return descriptorWrite;
        }

        int getBinding()
        {
            return mBinding;
        }

        void setUpdateCallBack(UnifromUpdateCallback callback)
        {
            mCallback = callback;
        }

        void update()
        {
            char* userData = mDataBuffer.data();
            uint32_t size = mCallback(userData, mDataBuffer.size());
            assert(size == mDataBuffer.size());
            fillBuffer(userData, size);
        }
    private:
        UnifromUpdateCallback   mCallback;
        std::vector<char>       mDataBuffer;
        uint32_t                mBinding;
        VkDescriptorBufferInfo  mBufferInfo;
    };

    class ENGINE_API VulkanStagingBuffer : public VulkanBuffer
    {
    public:
        VulkanStagingBuffer(VulkanDevicePtr device, VulkanCommandBufferPtr commandBuffer, VkDeviceSize size, void* pData)
            : VulkanBuffer(device, commandBuffer, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
        {
            fillBuffer(pData, size);
        }
    };
}
#endif //HOMURA_VULKANBUFFER_H
