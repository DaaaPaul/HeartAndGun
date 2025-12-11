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
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <chrono>
#include "Logger.h"

class Renderer {
public:
	void run();

	Renderer(int const& width, int const& height, bool const& enable, uint32_t const& framesInFlight) : 
		WINDOW_WIDTH(width), WINDOW_HEIGHT(height), ENABLE_VALIDATION_LAYER(enable), FRAMES_IN_FLIGHT(framesInFlight) {}

private:
	struct Helper {
		const std::vector<const char*> verifyValidationLayers(Renderer const& renderer) const;
		std::pair<const char**, uint32_t> verifyGlfwExtensionsPresent(Renderer const& renderer) const;
		std::vector<uint32_t> grokPhysicalDevices(std::vector<vk::raii::PhysicalDevice> const& physicalDevices, Renderer const& renderer) const;
		uint32_t findGraphicsQueueFamilyIndex(std::vector<vk::QueueFamilyProperties> const& queueFamilyProperties, Renderer const& renderer) const;
		vk::SurfaceFormatKHR getSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& surfaceFormats, Renderer const& renderer) const;
		vk::PresentModeKHR getPresentMode(std::vector<vk::PresentModeKHR> const& presentModes, Renderer const& renderer) const;
		vk::Extent2D getSurfaceExtent(vk::SurfaceCapabilitiesKHR const& capabilities, Renderer const& renderer) const;
		uint32_t getSwapchainImageCount(vk::SurfaceCapabilitiesKHR const& capabilities, Renderer const& renderer) const;
		const std::vector<char> readSprivFileBytes(std::string const& filePath, Renderer const& renderer) const;
		uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties, Renderer const& renderer) const;
		void createBuffer(vk::DeviceSize bufferSize, vk::Flags<vk::BufferUsageFlagBits> usage, vk::MemoryPropertyFlags memoryTypeNeeds,
			vk::raii::DeviceMemory& memory, vk::raii::Buffer& buffer,
			Renderer const& renderer) const;
		void copyBuffer(vk::raii::Buffer& src, vk::raii::Buffer& dst, vk::DeviceSize const& size, Renderer const& renderer) const;
		void transitionImageLayout(uint32_t const& imageIndex,
			vk::ImageLayout const& oldLayout, vk::ImageLayout const& newLayout,
			vk::PipelineStageFlags2 const& srcStageMask, vk::AccessFlags2 const& srcAccessMask,
			vk::PipelineStageFlags2 const& dstStageMask, vk::AccessFlags2 const& dstAccessMask,
			Renderer const& renderer) const;
		void recordCommandBuffer(uint32_t const& imageIndex, Renderer const& renderer) const;
	};

	struct VertexAttributes {
		glm::vec2 pos;
		glm::vec4 colour;

		static vk::VertexInputBindingDescription getBindingDesc() {
			return { 0, sizeof(VertexAttributes), vk::VertexInputRate::eVertex };
		}

		static std::vector<vk::VertexInputAttributeDescription> getAttribDesc() {
			return {
				vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32Sfloat, offsetof(VertexAttributes, pos)),
				vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(VertexAttributes, colour))
			};
		}
	};

	struct UniformBufferObject {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 projection;
	};

	const std::vector<VertexAttributes> verticies = {
		{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f, 1.0f}}, // top left
		{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f, 1.0f}}, // top right
		{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f, 1.0f}}, // bottom right
		{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}} // bottom left
	};

	const std::vector<uint32_t> vertexIndices = {
		0, 1, 2, 2, 3, 0
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
	vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;
	std::vector<vk::raii::Buffer> uniformBuffers;
	std::vector<vk::raii::DeviceMemory> uniformBuffersMemory;
	std::vector<void*> uniformBuffersMapped;
	vk::raii::DescriptorPool descriptorPool = nullptr;
	std::vector<vk::raii::DescriptorSet> descriptorSets{};
	vk::raii::PipelineLayout pipelineLayout = nullptr;
	vk::raii::Pipeline graphicsPipeline = nullptr;
	vk::raii::Buffer vertexBuffer = nullptr;
	vk::raii::DeviceMemory vertexBufferMemory = nullptr;
	vk::raii::Buffer indexBuffer = nullptr;
	vk::raii::DeviceMemory indexMemory = nullptr;
	vk::raii::CommandPool commandPool = nullptr;
	std::vector<vk::raii::CommandBuffer> commandBuffers{};
	std::vector<vk::raii::Semaphore> renderReady{};
	std::vector<vk::raii::Semaphore> renderDone{};
	std::vector<vk::raii::Fence> commandBufferDone{};

	uint32_t frameInFlight = ~0;
	uint32_t semaphoreIndex = ~0;

	bool framebufferResized = false;

	void createWindow();
	void initializeVulkan();
	void createInstance();
	void createSurface();
	void selectPhysicalDevice();
	void createLogicalDevice();
	void createSwapchain();
	void createImageViews();
	void createDescriptorSetLayout();
	void createGraphicsPipeline();
	void createCommandPool();
	void createCommandBuffers();
	void createVertexBuffer();
	void createIndexBuffer();
	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSets();
	void createSyncObjects();
	void drawFrame();
	void mainLoop();
	void clean();

	void updateUniformBuffer(uint32_t index);
	static void framebufferResize(GLFWwindow* window, int width, int height);
	void cleanCurrentSyncObjects();
	void cleanCurrentSwapchain();
	void recreateSwapchain();
};