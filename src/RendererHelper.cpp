#include "Renderer.h"
#include <iostream>
#include <map>
#include <fstream>
#include <string>

const std::vector<const char*> Renderer::Helper::verifyValidationLayers(Renderer const& renderer) const {
	std::vector<vk::LayerProperties> layerProperties = renderer.Vcontext.enumerateInstanceLayerProperties();

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
		else renderer.logger.logInformation("\tRequired validation layer supported:" + std::string(renderer.VALIDATION_LAYERS[i]));
	}

	return renderer.VALIDATION_LAYERS;
}

const char** Renderer::Helper::verifyGlfwExtensionsPresent(Renderer const& renderer) const {
	uint32_t requiredExtensionsCount = ~0;
	const char** requiredExtensions = glfwGetRequiredInstanceExtensions(&requiredExtensionsCount);

	std::vector<vk::ExtensionProperties> extensionProperties = renderer.Vcontext.enumerateInstanceExtensionProperties();

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
		else renderer.logger.logInformation("\tRequired extension by glfw supported:" + std::string(requiredExtensions[i]));
	}

	return requiredExtensions;
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
					renderer.logger.logInformation("\tRequired physical device extension supported:" + std::string(requiredExtension));
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
	int i = 0;
	for(vk::QueueFamilyProperties const& qfProperties : queueFamilyProperties) {
		if ((qfProperties.queueFlags & vk::QueueFlagBits::eGraphics) && renderer.VphysicalDevice.getSurfaceSupportKHR(i, renderer.Vsurface)) {
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
		renderer.logger.logError("Something went wrong with reading spriv file at " + filePath);
	}

	std::vector<char> buffer(file.tellg());
	file.seekg(0, std::ios::beg);
	file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

	return buffer;
}