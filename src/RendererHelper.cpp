#include "Renderer.h"
#include <iostream>
#include <map>
#include <fstream>
#include <string>

const std::vector<const char*> Renderer::Helper::verifyValidationLayers(Renderer const& renderer) const {
	std::vector<vk::LayerProperties> layerProperties = renderer.context.enumerateInstanceLayerProperties();

	bool layerFound = false;
	for (uint32_t i = 0; i < renderer.VALIDATION_LAYERS.size(); ++i) {
		layerFound = false;

		for (vk::LayerProperties const& property : layerProperties) {
			if (strcmp(property.layerName, renderer.VALIDATION_LAYERS[i]) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) throw std::runtime_error("Required validation layer not supported:" + std::string(renderer.VALIDATION_LAYERS[i]));
		else renderer.LOGGER.logInformation("\tRequired validation layer supported:" + std::string(renderer.VALIDATION_LAYERS[i]));
	}

	return renderer.VALIDATION_LAYERS;
}

std::pair<const char**, uint32_t> Renderer::Helper::verifyGlfwExtensionsPresent(Renderer const& renderer) const {
	uint32_t requiredExtensionsCount = ~0;
	const char** requiredExtensions = glfwGetRequiredInstanceExtensions(&requiredExtensionsCount);

	std::vector<vk::ExtensionProperties> extensionProperties = renderer.context.enumerateInstanceExtensionProperties();

	bool extensionFound = false;
	for (uint32_t i = 0; i < requiredExtensionsCount; ++i) {
		extensionFound = false;

		for (vk::ExtensionProperties const& property : extensionProperties) {
			if (strcmp(property.extensionName, requiredExtensions[i]) == 0) {
				extensionFound = true;
				break;
			}
		}

		if (!extensionFound)  throw std::runtime_error("Required extension by glfw not supported:" + std::string(requiredExtensions[i]));
		else renderer.LOGGER.logInformation("\tRequired extension by glfw supported:" + std::string(requiredExtensions[i]));
	}

	return { requiredExtensions, requiredExtensionsCount };
}

std::vector<uint32_t> Renderer::Helper::grokPhysicalDevices(std::vector<vk::raii::PhysicalDevice> const& physicalDevices, Renderer const& renderer) const {
	std::vector<uint32_t> physicalDeviceAbilities{};

	for (vk::raii::PhysicalDevice const& physicalDevice : physicalDevices) {
		if (physicalDevice.getProperties().apiVersion >= VK_VERSION_1_3) {
			physicalDeviceAbilities.push_back(1);
		} else {
			physicalDeviceAbilities.push_back(0);
		}

		bool foundGraphicsQueueFamily = false;
		std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
		for (vk::QueueFamilyProperties const& queueFamily : queueFamilyProperties) {
			if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
				physicalDeviceAbilities.push_back(1);
				foundGraphicsQueueFamily = true;
				break;
			}
		}
		if (!foundGraphicsQueueFamily) {
			physicalDeviceAbilities.push_back(0);
		}

		vk::StructureChain<vk::PhysicalDeviceFeatures2, 
			vk::PhysicalDeviceVulkan11Features, 
			vk::PhysicalDeviceVulkan13Features, 
			vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> requiredFeaturesAvailability =
		physicalDevice.getFeatures2<vk::PhysicalDeviceFeatures2, 
			vk::PhysicalDeviceVulkan11Features, 
			vk::PhysicalDeviceVulkan13Features, 
			vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
		if(requiredFeaturesAvailability.get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
		requiredFeaturesAvailability.get<vk::PhysicalDeviceVulkan13Features>().synchronization2 &&
		requiredFeaturesAvailability.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
		requiredFeaturesAvailability.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState) {
			physicalDeviceAbilities.push_back(1);
		} else {
			physicalDeviceAbilities.push_back(0);
		}

		std::vector<vk::ExtensionProperties> extensionProperties = physicalDevice.enumerateDeviceExtensionProperties();
		bool foundThisOne = false;
		bool allGood = true;
		for(const char* const& requiredExtension : renderer.REQUIRED_DEVICE_EXTENSIONS) {
			foundThisOne = false;

			for (vk::ExtensionProperties const& property : extensionProperties) {
				if (strcmp(property.extensionName, requiredExtension) == 0) {
					renderer.LOGGER.logInformation("\tRequired physical device extension supported:" + std::string(requiredExtension));
					foundThisOne = true;
					break;
				}
			}

			if (!foundThisOne) allGood = false;
		}

		if(allGood) {
			physicalDeviceAbilities.push_back(1);
		} else {
			physicalDeviceAbilities.push_back(0);
		}
	}

	return physicalDeviceAbilities;
}

uint32_t Renderer::Helper::findGraphicsQueueFamilyIndex(std::vector<vk::QueueFamilyProperties> const& queueFamilyProperties, Renderer const& renderer) const {
	uint32_t i = 0;
	for(vk::QueueFamilyProperties const& qfProperties : queueFamilyProperties) {
		if ((qfProperties.queueFlags & vk::QueueFlagBits::eGraphics) && renderer.physicalDevice.getSurfaceSupportKHR(i, renderer.surface)) {
			return i;
		}
		++i;
	}

	return ~0;
}

vk::SurfaceFormatKHR Renderer::Helper::getSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& surfaceFormats, Renderer const& renderer) const {
	bool hasRGBASrgb = false;

	for(vk::SurfaceFormatKHR const& surfaceFormat : surfaceFormats) {
		if(surfaceFormat.format == vk::Format::eR8G8B8A8Srgb && surfaceFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
			hasRGBASrgb = true;
		}
	}

	if (hasRGBASrgb) return vk::SurfaceFormatKHR(vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear);
	else throw std::runtime_error("Ideal surface format not found");
}

vk::PresentModeKHR Renderer::Helper::getPresentMode(std::vector<vk::PresentModeKHR> const& presentModes, Renderer const& renderer) const {
	bool hasMailbox = false;

	for(vk::PresentModeKHR const& presentMode : presentModes) {
		if(presentMode == vk::PresentModeKHR::eMailbox) {
			hasMailbox = true;
		}
	}

	if (hasMailbox) return vk::PresentModeKHR::eMailbox;
	else throw std::runtime_error("Ideal present mode not found");
}

vk::Extent2D Renderer::Helper::getSurfaceExtent(vk::SurfaceCapabilitiesKHR const& capabilities, Renderer const& renderer) const {
	if(capabilities.currentExtent.width != 0xFFFFFFFF) {
		return capabilities.currentExtent;
	} else {
		int width, height;

		glfwGetFramebufferSize(renderer.window, &width, &height);

		return vk::Extent2D(std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
							std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height));
	}
}

uint32_t Renderer::Helper::getSwapchainImageCount(vk::SurfaceCapabilitiesKHR const& capabilities, Renderer const& renderer) const {
	uint32_t minImageCount = capabilities.maxImageCount - 3;
	if (minImageCount < capabilities.minImageCount) {
		minImageCount = capabilities.minImageCount;
	}

	return minImageCount;
}

const std::vector<char> Renderer::Helper::readSprivFileBytes(std::string const& filePath, Renderer const& renderer) const {
	std::ifstream file(filePath, std::ios::ate | std::ios::binary);
	if(!file.good()) {
		throw std::runtime_error("Something went wrong with reading spriv file at " + filePath);
	}

	std::vector<char> buffer(file.tellg());
	file.seekg(0, std::ios::beg);
	file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

	return buffer;
}

uint32_t Renderer::Helper::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties, Renderer const& renderer) const {
	vk::PhysicalDeviceMemoryProperties gpuMemProperties = renderer.physicalDevice.getMemoryProperties();

	for(uint32_t i = 0; i < gpuMemProperties.memoryTypeCount; ++i) {
		if (((gpuMemProperties.memoryTypes[i].propertyFlags & properties) == properties) && (typeFilter & (1 << i))) {
			return i;
		}
	}

	throw std::runtime_error("Didn't find suitable GPU memory spot to store data");
}

void Renderer::Helper::createBuffer(vk::DeviceSize bufferSize, vk::Flags<vk::BufferUsageFlagBits> usage, vk::MemoryPropertyFlags memoryTypeNeeds,
									vk::raii::DeviceMemory& memory, vk::raii::Buffer& buffer,
									Renderer const& renderer) const {
	vk::BufferCreateInfo bufferInfo = {
		.size = bufferSize,
		.usage = usage,
		.sharingMode = vk::SharingMode::eExclusive,
	};
	buffer = vk::raii::Buffer(renderer.logicalDevice, bufferInfo);

	vk::MemoryRequirements bufferRequirements = buffer.getMemoryRequirements();

	vk::MemoryAllocateInfo memAllocInfo = {
		.allocationSize = bufferRequirements.size,
		.memoryTypeIndex = findMemoryType(bufferRequirements.memoryTypeBits,
			memoryTypeNeeds, renderer)
	};
	memory = vk::raii::DeviceMemory(renderer.logicalDevice, memAllocInfo);

	buffer.bindMemory(*memory, 0);
}

void Renderer::Helper::copyBuffer(vk::raii::Buffer& src, vk::raii::Buffer& dst, vk::DeviceSize const& size, Renderer const& renderer) const {
	vk::CommandBufferAllocateInfo tempAllocInfo = {
		.commandPool = renderer.commandPool,
		.level = vk::CommandBufferLevel::ePrimary,
		.commandBufferCount = 1
	};

	vk::raii::CommandBuffer tempTransBuffer = std::move(renderer.logicalDevice.allocateCommandBuffers(tempAllocInfo).front());
	tempTransBuffer.begin(vk::CommandBufferBeginInfo { .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit } );
	tempTransBuffer.copyBuffer(src, dst, vk::BufferCopy(0, 0, size));
	tempTransBuffer.end();

	renderer.graphicsQueue.submit(vk::SubmitInfo{ .commandBufferCount = 1, .pCommandBuffers = &*tempTransBuffer }, nullptr);
	renderer.graphicsQueue.waitIdle();
}

void Renderer::Helper::transitionImageLayout(uint32_t const& imageIndex,
											 vk::ImageLayout const& oldLayout, vk::ImageLayout const& newLayout, 
											 vk::PipelineStageFlags2 const& srcStageMask, vk::AccessFlags2 const& srcAccessMask, 
											 vk::PipelineStageFlags2 const& dstStageMask, vk::AccessFlags2 const& dstAccessMask,
											 Renderer const& renderer) const {
	vk::ImageMemoryBarrier2 barrier = {
		.srcStageMask = srcStageMask,
		.srcAccessMask = srcAccessMask,
		.dstStageMask = dstStageMask,
		.dstAccessMask = dstAccessMask,
		.oldLayout = oldLayout,
		.newLayout = newLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = renderer.swapchain.getImages()[imageIndex],
		.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
	};

	vk::DependencyInfo dependencyInfo = {
		.dependencyFlags = {},
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &barrier
	};

	renderer.commandBuffers[renderer.frameInFlight].pipelineBarrier2(dependencyInfo);
}

void Renderer::Helper::recordCommandBuffer(uint32_t const& imageIndex, Renderer const& renderer) const {
	renderer.commandBuffers[renderer.frameInFlight].begin({});

	transitionImageLayout(imageIndex,
		vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
		vk::PipelineStageFlagBits2::eTopOfPipe, {},
		vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite, 
		renderer);

	vk::RenderingAttachmentInfo colorAttachmentInfo = {
		.imageView = renderer.imageViews[imageIndex],
		.imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
		.loadOp = vk::AttachmentLoadOp::eClear,
		.storeOp = vk::AttachmentStoreOp::eStore,
		.clearValue = vk::ClearColorValue(0.3f, 0.3f, 0.3f, 1.0f)
	};
	vk::RenderingInfo renderingInfo = {
		.renderArea = vk::Rect2D(vk::Offset2D(0,0), renderer.extent),
		.layerCount = 1,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentInfo
	};

	renderer.commandBuffers[renderer.frameInFlight].beginRendering(renderingInfo);
	renderer.commandBuffers[renderer.frameInFlight].bindPipeline(vk::PipelineBindPoint::eGraphics, renderer.graphicsPipeline);
	renderer.commandBuffers[renderer.frameInFlight].bindVertexBuffers(0, *renderer.vertexBuffer, { 0 });
	renderer.commandBuffers[renderer.frameInFlight].bindIndexBuffer(*renderer.indexBuffer, 0, vk::IndexType::eUint32);
	renderer.commandBuffers[renderer.frameInFlight].drawIndexed(renderer.vertexIndices.size(), 1, 0, 0, 0);
	renderer.commandBuffers[renderer.frameInFlight].endRendering();

	transitionImageLayout(imageIndex,
		vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
		vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite,
		vk::PipelineStageFlagBits2::eBottomOfPipe, {},
		renderer);

	renderer.commandBuffers[renderer.frameInFlight].end();
}