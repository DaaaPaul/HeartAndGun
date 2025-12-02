#include "Renderer.h"
#include <iostream>

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

std::vector<uint32_t> Renderer::Helper::ratePhysicalDevices(std::vector<vk::raii::PhysicalDevice> const& physicalDevices) const {
	std::vector<uint32_t> correspondingRatings{};

	for (vk::raii::PhysicalDevice const& physicalDevice : physicalDevices) {
		uint32_t requirementsMetCount = 0;

		if (physicalDevice.getProperties().apiVersion >= VK_VERSION_1_3) {
			requirementsMetCount++;
		}

		std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
		for (vk::QueueFamilyProperties const& queueFamily : queueFamilyProperties) {
			if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
				requirementsMetCount++;
				break;
			}
		}

		vk::StructureChain<VkPhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> requiredFeaturesAvailability =
		physicalDevice.getFeatures2<VkPhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();

		if(
		requiredFeaturesAvailability.get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
		requiredFeaturesAvailability.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
		requiredFeaturesAvailability.get<vk::PhysicalDeviceVulkan13Features>().synchronization2 &&
		requiredFeaturesAvailability.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState) {
			requirementsMetCount++;
		}


	}
}