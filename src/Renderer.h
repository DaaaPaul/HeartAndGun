#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <cstring>
#include <map>
#include <string>
#include <utility>
#include "Logger.h"

class Renderer {
public:
	void run();

	Renderer(int const& width, int const& height, bool const& enable, uint32_t const& framesInFlight) : 
		WINDOW_WIDTH(width), WINDOW_HEIGHT(height), ENABLE_VALIDATION_LAYER(enable), FRAMES_IN_FLIGHT(framesInFlight) {}

private:
	class Helper {
	public:
		const std::vector<const char*> verifyValidationLayers(Renderer const& renderer) const;
		
		std::pair<const char**, uint32_t> verifyGlfwExtensionsPresent(Renderer const& renderer) const;
		
		std::vector<uint32_t> grokPhysicalDevices(std::vector<vk::raii::PhysicalDevice> const& physicalDevices, Renderer const& renderer) const;
		
		uint32_t findGraphicsQueueFamilyIndex(std::vector<vk::QueueFamilyProperties> const& queueFamilyProperties, Renderer const& renderer) const;
		
		vk::SurfaceFormatKHR getSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& surfaceFormats, Renderer const& renderer) const;
		
		vk::PresentModeKHR getPresentMode(std::vector<vk::PresentModeKHR> const& presentModes, Renderer const& renderer) const;
		
		vk::Extent2D getSurfaceExtent(vk::SurfaceCapabilitiesKHR const& capabilities, Renderer const& renderer) const;
		
		uint32_t getSwapchainImageCount(vk::SurfaceCapabilitiesKHR const& capabilities, Renderer const& renderer) const;
		
		const std::vector<char> readSprivFileBytes(std::string const& filePath, Renderer const& renderer) const;
		
		void transitionImageLayout(uint32_t const& imageIndex,
			vk::ImageLayout const& oldLayout, vk::ImageLayout const& newLayout,
			vk::PipelineStageFlags2 const& srcStageMask, vk::AccessFlags2 const& srcAccessMask,
			vk::PipelineStageFlags2 const& dstStageMask, vk::AccessFlags2 const& dstAccessMask,
			Renderer const& renderer) const;
		
		void recordCommandBuffer(uint32_t const& imageIndex, Renderer const& renderer) const;
	};

	const Helper HELPER{};
	const Logger LOGGER{};

	const int WINDOW_WIDTH = ~0;
	const int WINDOW_HEIGHT = ~0;
	const uint32_t FRAMES_IN_FLIGHT = ~0;
	const bool ENABLE_VALIDATION_LAYER = true;

	const std::vector<const char*> VALIDATION_LAYERS = {
		"VK_LAYER_KHRONOS_validation"
	};

	const std::vector<const char*> REQUIRED_DEVICE_EXTENSIONS = {
		vk::KHRSwapchainExtensionName,
		vk::KHRSpirv14ExtensionName,
		vk::KHRSynchronization2ExtensionName,
		vk::KHRCreateRenderpass2ExtensionName
	};

	uint32_t graphicsFamilyIndex = ~0;

	vk::SurfaceFormatKHR surfaceFormat{};
	vk::Extent2D extent{};
	vk::PresentModeKHR presentMode{};
	uint32_t swapchainImageCount = ~0;

	GLFWwindow* window = nullptr;
	vk::raii::Context context{};
	vk::raii::Instance instance = nullptr;
	vk::raii::SurfaceKHR surface = nullptr;
	vk::raii::PhysicalDevice physicalDevice = nullptr;
	vk::raii::Device logicalDevice = nullptr;
	vk::raii::Queue graphicsQueue = nullptr;
	vk::raii::SwapchainKHR swapchain = nullptr;
	std::vector<vk::raii::ImageView> imageViews{};
	vk::raii::Pipeline graphicsPipeline = nullptr;
	vk::raii::CommandPool commandPool = nullptr;
	std::vector<vk::raii::CommandBuffer> commandBuffers{};
	std::vector<vk::raii::Semaphore> renderReady{};
	std::vector<vk::raii::Semaphore> renderDone{};
	std::vector<vk::raii::Fence> commandBufferDone{};

	uint32_t frameInFlight = ~0;
	uint32_t semaphoreIndex = ~0;

	void createWindow();
	void initializeVulkan();
	void createInstance();
	void createSurface();
	void selectPhysicalDevice();
	void createLogicalDevice();
	void createSwapchain();
	void createImageViews();
	void createGraphicsPipeline();
	void createCommandPool();
	void createCommandBuffers();
	void createSyncObjects();
	void drawFrame();
	void mainLoop();
	void clean();
};