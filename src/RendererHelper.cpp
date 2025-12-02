#include "Renderer.h"
#include <iostream>
#include <map>
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
		else std::cout << "Required validation layer supported:" << renderer.VALIDATION_LAYERS[i] << '\n';
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
		else std::cout << "Required extension by glfw supported:" << requiredExtensions[i] << '\n';
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

		vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> requiredFeaturesAvailability =
		physicalDevice.getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
		if(
		requiredFeaturesAvailability.get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
		requiredFeaturesAvailability.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
		requiredFeaturesAvailability.get<vk::PhysicalDeviceVulkan13Features>().synchronization2 &&
		requiredFeaturesAvailability.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState) {
			physicalDeviceAbilities.push_back(1);
		} else {
			physicalDeviceAbilities.push_back(0);
		}

		std::vector<vk::ExtensionProperties> extensionProperties = physicalDevice.enumerateDeviceExtensionProperties();
		bool foundThisOne = false;
		bool allGood = true;
		for(const char* const& requiredExtension : renderer.REQUIRED_PHYSICAL_DEVICE_EXTENSIONS) {
			foundThisOne = false;

			for (vk::ExtensionProperties const& property : extensionProperties) {
				if (strcmp(property.extensionName, requiredExtension) == 0) {
					std::cout << "Required physical device extension supported:" << requiredExtension << '\n';
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