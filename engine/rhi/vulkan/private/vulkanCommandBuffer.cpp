//
// Created by 最上川 on 2022/1/9.
//

#include <vulkanCommandBuffer.h>
#include <vulkanGfxPipeline.h>
#include <vulkanDevice.h>
#include <vulkanQueue.h>
#include <vulkanBuffer.h>
#include <vulkanTexture.h>
#include <vulkanLayout.h>
#include <vulkanDescriptorSet.h>
#include <vulkanRenderPass.h>
#include <vulkanFramebuffer.h>
#include <vulkanSwapChain.h>
#include <vulkanSynchronization.h>
#include <debugUtils.h>

namespace Homura
{
    VulkanCommandPool::VulkanCommandPool(VulkanDevicePtr device)
        : mDevice{device}
        , mCommandPool{VK_NULL_HANDLE}
    {
        create();
    }

    VulkanCommandPool::~VulkanCommandPool()
    {
        destroy();
    }

    void VulkanCommandPool::create()
    {
        VkCommandPoolCreateInfo createInfo{};
        createInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        createInfo.queueFamilyIndex = mDevice->getGraphicsQueue()->getFamilyIndex();
        VERIFYVULKANRESULT(vkCreateCommandPool(mDevice->getHandle(), &createInfo, nullptr, &mCommandPool));
    }

    void VulkanCommandPool::reset()
    {
        VERIFYVULKANRESULT(vkResetCommandPool(mDevice->getHandle(), mCommandPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT));
    }

    void VulkanCommandPool::destroy()
    {
        if (mCommandPool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(mDevice->getHandle(), mCommandPool, nullptr);
            mCommandPool = VK_NULL_HANDLE;
        }
    }

    VulkanCommandBuffer::VulkanCommandBuffer(VulkanDevicePtr device, VulkanSwapChainPtr swapChain, VulkanCommandPoolPtr commandPool, VulkanFramebufferPtr framebuffer, VulkanPipelinePtr pipeline)
        : mDevice{device}
        , mSwapChain{swapChain}
        , mCommandPool{commandPool}
        , mFramebuffer{framebuffer}
        , mPipeline{pipeline}
        , mCurrentFrame{0}
        , imageInFlight{}
        , inFlightFences{}
        , mImageAvailableSemaphores{}
        , mRenderFinishedSemaphores{}
        , mMaxFrameCount{3}
        , mCommandBuffers{}
        , mHasIndexBuffer{false}
        , mBufferDataCount{0}
    {
        create();
        createSyncObj();
    }

    VulkanCommandBuffer::~VulkanCommandBuffer()
    {

    }

    void VulkanCommandBuffer::create()
    {
        mCommandBuffers.resize(mSwapChain->getImageCount());
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType                 = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandBufferCount    = (uint32_t)mCommandBuffers.size();
        allocInfo.commandPool           = mCommandPool->getHandle();
        allocInfo.level                 = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        VERIFYVULKANRESULT(vkAllocateCommandBuffers(mDevice->getHandle(), &allocInfo, mCommandBuffers.data()));
    }

    void VulkanCommandBuffer::destroy()
    {
        for (auto& commandBuffer : mCommandBuffers)
        {
            vkFreeCommandBuffers(mDevice->getHandle(), mCommandPool->getHandle(), 1, &commandBuffer);
        }
        mCommandBuffers.clear();

        // imageInFlight and inFlightFences points to the same fences, so release it once!
        if (inFlightFences != nullptr)
        {
            inFlightFences->destroy();
            inFlightFences.reset();
        }

        imageInFlight.reset();

        if (mImageAvailableSemaphores != nullptr)
        {
            mImageAvailableSemaphores->destroy();
            mImageAvailableSemaphores.reset();
        }
        if (mRenderFinishedSemaphores != nullptr)
        {
            mRenderFinishedSemaphores->destroy();
            mRenderFinishedSemaphores.reset();
        }
    }

    void VulkanCommandBuffer::createSyncObj()
    {
        imageInFlight = std::make_shared<VulkanFences>(mDevice, mSwapChain->getImageCount());
        inFlightFences = std::make_shared<VulkanFences>(mDevice, mMaxFrameCount);
        inFlightFences->create();
        mImageAvailableSemaphores = std::make_shared<VulkanSemaphores>(mDevice);
        mImageAvailableSemaphores->create(mMaxFrameCount);
        mRenderFinishedSemaphores = std::make_shared<VulkanSemaphores>(mDevice);
        mRenderFinishedSemaphores->create(mMaxFrameCount);

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    }

    VkCommandBuffer VulkanCommandBuffer::beginSingleTimeCommands()
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType                 = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level                 = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool           = mCommandPool->getHandle();
        allocInfo.commandBufferCount    = 1;

        VkCommandBuffer commandBuffer;
        VERIFYVULKANRESULT(vkAllocateCommandBuffers(mDevice->getHandle(), &allocInfo, &commandBuffer));

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VERIFYVULKANRESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo));
        return commandBuffer;
    }

    void VulkanCommandBuffer::endSingleTimeCommands(VkCommandBuffer commandBuffer)
    {
        vkEndCommandBuffer(commandBuffer);

        submitSync(mDevice->getGraphicsQueue(), commandBuffer, false);
        vkFreeCommandBuffers(mDevice->getHandle(), mCommandPool->getHandle(), 1, &commandBuffer);
    }

    void VulkanCommandBuffer::begin()
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        for (const auto& commandBuffer : mCommandBuffers)
        {            
            VERIFYVULKANRESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo));
        }
    }

    void VulkanCommandBuffer::beginRenderPass(VulkanRenderPassPtr renderPass)
    {
        for (int i = 0; i < mCommandBuffers.size(); i++)
        {
            VkRenderPassBeginInfo Info{};
            Info.sType                      = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            Info.renderPass                 = renderPass->getHandle();
            Info.framebuffer                = mFramebuffer->getHandle(i);
            Info.renderArea.offset          = {0, 0};
            Info.renderArea.extent          = mFramebuffer->getExtent();

            std::vector<VkClearValue> clearValues{ 2 };
            clearValues[0].color            = { {0.0f, 0.0f, 0.0f, 1.0f} };
            clearValues[1].depthStencil     = { 1.0f, 0 };

            Info.clearValueCount            = static_cast<uint32_t>(clearValues.size());
            Info.pClearValues               = clearValues.data();
            vkCmdBeginRenderPass(mCommandBuffers[i], &Info, VK_SUBPASS_CONTENTS_INLINE);
        }
    }

    void VulkanCommandBuffer::bindGraphicPipeline()
    {
        for (const auto& commandBuffer : mCommandBuffers)
        {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline->getHandle());
        }
    }

    void VulkanCommandBuffer::bindVertexBuffer(VulkanVertexBufferPtr buffer, uint32_t count)
    {
        VkBuffer vertexBuffers[] = {buffer->getHandle()};
        VkDeviceSize offsets[] = {0};
        mBufferDataCount = count;
        for (const auto& commandBuffer : mCommandBuffers)
        {
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        }
    }

    void VulkanCommandBuffer::bindIndexBuffer(VulkanIndexBufferPtr buffer, uint32_t count)
    {
        mBufferDataCount = count;
        for (const auto& commandBuffer : mCommandBuffers)
        {
            vkCmdBindIndexBuffer(commandBuffer, buffer->getHandle(), 0, VK_INDEX_TYPE_UINT32);
        }
        mHasIndexBuffer = true;
    }

    void VulkanCommandBuffer::bindDescriptorSet()
    {
        const VulkanPipelineLayoutPtr layout = mPipeline->getPipelineLayout();
        const VulkanDescriptorSetPtr descriptorSet = mPipeline->getDescriptorSet();
        assert(mCommandBuffers.size() == descriptorSet->getCount());

        std::vector<VkDescriptorSet>& desSet = descriptorSet->getData();
        for (int i = 0; i < mCommandBuffers.size(); i++)
        {
            vkCmdBindDescriptorSets(mCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, layout->getHandle(), 0, 1, &desSet[i], 0, nullptr);
        }
    }

    void VulkanCommandBuffer::draw(uint32_t vertexCount)
    {
        for (const auto& commandBuffer : mCommandBuffers)
        {
            vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
        }
    }

    void VulkanCommandBuffer::drawIndex(uint32_t indexCount)
    {
        for (const auto& commandBuffer : mCommandBuffers)
        {
            vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
        }
    }

    void VulkanCommandBuffer::drawIndirect(VulkanVertexBufferPtr buffer)
    {
        for (const auto& commandBuffer : mCommandBuffers)
        {
            vkCmdDrawIndirect(commandBuffer, buffer->getHandle(), 0, 1, sizeof(VkDrawIndirectCommand));
        }
    }

    void VulkanCommandBuffer::draw()
    {
        if (mHasIndexBuffer != false)
        {
            drawIndex(mBufferDataCount);
        }
        else
        {
            draw(mBufferDataCount);
        }
    }

    void VulkanCommandBuffer::drawIndexIndirect(VulkanStagingBufferPtr buffer)
    {
        for (const auto& commandBuffer : mCommandBuffers)
        {
            vkCmdDrawIndexedIndirect(commandBuffer, buffer->getHandle(), 0, 1, sizeof(VkDrawIndexedIndirectCommand));
        }
    }

    void VulkanCommandBuffer::endRenderPass()
    {
        for (const auto& commandBuffer : mCommandBuffers)
        {
            vkCmdEndRenderPass(commandBuffer);
        }
    }

    void VulkanCommandBuffer::end()
    {
        for (const auto& commandBuffer : mCommandBuffers)
        {
            VERIFYVULKANRESULT(vkEndCommandBuffer(commandBuffer));
        }
    }

    void VulkanCommandBuffer::submitSync(VulkanQueuePtr queue, VkCommandBuffer commandBuffer, bool isSync)
    {
        VkSubmitInfo submitInfo{};
        submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount   = 1;
        submitInfo.pCommandBuffers      = &commandBuffer;

        VERIFYVULKANRESULT(vkQueueSubmit(queue->getHandle(), 1, &submitInfo, isSync ? inFlightFences->getFence(mCurrentFrame) : VK_NULL_HANDLE));
        VERIFYVULKANRESULT(vkQueueWaitIdle(queue->getHandle()));
    }

    void VulkanCommandBuffer::transferImageLayout(VkCommandBuffer commandBuffer, const VkImageMemoryBarrier& imageMemoryBarrier, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask)
    {
        vkCmdPipelineBarrier(
                commandBuffer,
                srcStageMask,
                dstStageMask,
                0,
                0, nullptr,// MemoryBarrier
                0, nullptr,//BufferMemoryBarrier
                1, &imageMemoryBarrier//imageMemoryBarrier
                );
    }

    void VulkanCommandBuffer::blitImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout,
                   VkImage dstImage, VkImageLayout dstImageLayout,
                   uint32_t regionCount, VkImageBlit* Regions, VkFilter filter)
    {
        vkCmdBlitImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, Regions, filter);
    }

    void VulkanCommandBuffer::copyBuffer(VulkanBuffer srcBuffer, VulkanBuffer dstBuffer, VkDeviceSize size)
    {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer.getHandle(), dstBuffer.getHandle(), 1, &copyRegion);

        endSingleTimeCommands(commandBuffer);
    }

    void VulkanCommandBuffer::copyBufferToTexture(VulkanBuffer buffer, VulkanTexture2DPtr texture, uint32_t width, uint32_t height)
    {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferImageCopy region{};
        region.bufferOffset                     = 0;
        region.bufferRowLength                  = 0;
        region.bufferImageHeight                = 0;
        region.imageSubresource.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel        = 0;
        region.imageSubresource.baseArrayLayer  = 0;
        region.imageSubresource.layerCount      = 1;
        region.imageOffset                      = { 0, 0, 0, };
        region.imageExtent                      = { width, height,1 };

        vkCmdCopyBufferToImage(commandBuffer, buffer.getHandle(), texture->getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        endSingleTimeCommands(commandBuffer);
    }

    void VulkanCommandBuffer::drawFrame(VulkanRHIPtr rhi)
    {
        inFlightFences->wait(mCurrentFrame);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(mDevice->getHandle(), mSwapChain->getHandle(), UINT64_MAX, mImageAvailableSemaphores->getSemaphore(mCurrentFrame), VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) 
        {
            rhi->recreateSwapChain();
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) 
        {
            std::cerr << "failed to acquire swap chain image!" << std::endl;
        }

        if (imageInFlight->getFence(imageIndex) != VK_NULL_HANDLE)
        {
            vkWaitForFences(mDevice->getHandle(), 1, &imageInFlight->getFence(imageIndex), VK_TRUE, UINT64_MAX);
        }
        rhi->updateUniformBuffer(imageIndex);
        imageInFlight->setValue(inFlightFences->getEntity(mCurrentFrame), imageIndex);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[]        = { mImageAvailableSemaphores->getSemaphore(mCurrentFrame) };
        VkSemaphore signalSemaphores[]      = { mRenderFinishedSemaphores->getSemaphore(mCurrentFrame) };
        VkPipelineStageFlags waitStages[]   = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount       = 1;
        submitInfo.pWaitSemaphores          = waitSemaphores;
        submitInfo.pWaitDstStageMask        = waitStages;
        submitInfo.commandBufferCount       = 1;
        submitInfo.pCommandBuffers          = &mCommandBuffers[imageIndex];
        submitInfo.signalSemaphoreCount     = 1;
        submitInfo.pSignalSemaphores        = signalSemaphores;

        vkResetFences(mDevice->getHandle(), 1, &inFlightFences->getFence(mCurrentFrame));

        if (vkQueueSubmit(mDevice->getGraphicsQueue()->getHandle(), 1, &submitInfo, inFlightFences->getFence(mCurrentFrame)) != VK_SUCCESS) 
        {
            std::cerr << "failed to submit draw command buffer!" << std::endl;
        }

        VkSwapchainKHR swapChains[]         = { mSwapChain->getHandle() };
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType                   = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount      = 1;
        presentInfo.pWaitSemaphores         = signalSemaphores;
        presentInfo.swapchainCount          = 1;
        presentInfo.pSwapchains             = swapChains;
        presentInfo.pImageIndices           = &imageIndex;

        result = vkQueuePresentKHR(mDevice->getPresentQueue()->getHandle(), &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR ||
            result == VK_SUBOPTIMAL_KHR) 
        {
            rhi->recreateSwapChain();
        }
        else if (result != VK_SUCCESS) 
        {
            std::cerr << "failed to present swap chain image!" << std::endl;
        }

        mCurrentFrame = (mCurrentFrame + 1) % mMaxFrameCount;
    }
}