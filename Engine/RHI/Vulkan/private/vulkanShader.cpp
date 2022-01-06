//
// Created by FDC on 2022/1/6.
//

#include <vulkanShader.h>
#include <debugUtils.h>
#include <fstream>
#include <vector>

namespace Homura
{
    VulkanShader::VulkanShader(std::shared_ptr<VkDevice> device)
        : mModule{VK_NULL_HANDLE}
        , mDevice{device}
        , pAllocator{nullptr}
    {

    }

    VulkanShader::~VulkanShader()
    {
        destroyShaderModule();
    }

    void VulkanShader::createShaderModule(std::string filename, VkAllocationCallbacks *allocator)
    {
        std::vector<char> shaderCode = readFile(filename);
        pAllocator = allocator;

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = shaderCode.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());
        VERIFYVULKANRESULT(vkCreateShaderModule(*mDevice.get(), &createInfo, pAllocator, &mModule));
    }

    void VulkanShader::destroyShaderModule()
    {
        if (mModule != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(*mDevice.get(), mModule, pAllocator);
        }
    }

    std::vector<char> VulkanShader::readFile(const std::string &filename)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t) file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }
}