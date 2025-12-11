#include "Renderer.h"
#include <iostream>
#include <limits>

void Renderer::run() {
	createWindow();
	initializeVulkan();
	mainLoop();
	clean();
}

void Renderer::createWindow() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Heart And Gun", nullptr, nullptr);
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, framebufferResize);

	if (window == NULL) throw std::runtime_error("Window creation failed!");
	LOGGER.logInformation("GLFW window created");
}

void Renderer::framebufferResize(GLFWwindow* window, int width, int height) {
	Renderer* renderer = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
	renderer->framebufferResized = true;
}

void Renderer::initializeVulkan() {
	createInstance();
	createSurface();
	selectPhysicalDevice();
	createLogicalDevice();
	createSwapchain();
	createImageViews();
	createDescriptorSetLayout();
	createGraphicsPipeline();
	createCommandPool();
	createCommandBuffers();
	createVertexBuffer();
	createIndexBuffer();
	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();
	createSyncObjects();

	LOGGER.logInformation("-------------------------------------------------------------------------------------------------------\n"
						  "FINISHED VULKAN INITIALIZATION\n"
						  "-------------------------------------------------------------------------------------------------------");
}

void Renderer::createInstance() {
	std::pair<const char**, uint32_t> requiredGlfwExtensions = HELPER.verifyGlfwExtensionsPresent(*this);

	vk::ApplicationInfo appInfo = {
		.apiVersion = vk::ApiVersion14
	};
	
	if (ENABLE_VALIDATION_LAYER) {
		const std::vector<const char*> validationLayers = HELPER.verifyValidationLayers(*this);

		vk::InstanceCreateInfo instanceCreateInfo = {
			.pApplicationInfo = &appInfo,
			.enabledLayerCount = static_cast<uint32_t>(validationLayers.size()),
			.ppEnabledLayerNames = validationLayers.data(),
			.enabledExtensionCount = requiredGlfwExtensions.second,
			.ppEnabledExtensionNames = requiredGlfwExtensions.first,
		};
		instance = vk::raii::Instance(context, instanceCreateInfo);
	} else {
		vk::InstanceCreateInfo instanceCreateInfo = {
			.pApplicationInfo = &appInfo,
			.enabledExtensionCount = requiredGlfwExtensions.second,
			.ppEnabledExtensionNames = requiredGlfwExtensions.first
		};
		instance = vk::raii::Instance(context, instanceCreateInfo);
	}

	LOGGER.logInformation("Vulkan instance created");
}

void Renderer::createSurface() {
	VkSurfaceKHR tempSurface;
	glfwCreateWindowSurface(*instance, window, nullptr, &tempSurface);
	surface = vk::raii::SurfaceKHR(instance, tempSurface);

	LOGGER.logInformation("Vulkan surface created");
}

void Renderer::selectPhysicalDevice() {
	std::vector<uint32_t> physicalDeviceAbilitiesInFour = HELPER.grokPhysicalDevices(instance.enumeratePhysicalDevices(), *this);

	if ((physicalDeviceAbilitiesInFour.size() % 4) != 0) {
		throw std::runtime_error("Something went wrong with GPU selection");
	}

	bool foundOne = false;
	for(uint32_t i = 0; i < physicalDeviceAbilitiesInFour.size(); i += 4) {
		if( (physicalDeviceAbilitiesInFour[i + 0] == 1) &&
			(physicalDeviceAbilitiesInFour[i + 1] == 1) &&
			(physicalDeviceAbilitiesInFour[i + 2] == 1) &&
			(physicalDeviceAbilitiesInFour[i + 3] == 1)) {
			physicalDevice = instance.enumeratePhysicalDevices()[i / 4];
			LOGGER.logInformation("Selected physical device (" + std::string(physicalDevice.getProperties().deviceName.data()) + ")");
			foundOne = true;
			break;
		}
	}

	if(!foundOne) {
		throw std::runtime_error("No appropriate GPU found on your computer");
	}
}

void Renderer::createLogicalDevice() {
	vk::StructureChain<vk::PhysicalDeviceFeatures2,
		vk::PhysicalDeviceVulkan11Features,
		vk::PhysicalDeviceVulkan13Features,
		vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> enabledFeatures =
	{   {},
		{.shaderDrawParameters = true},
		{.synchronization2 = true, .dynamicRendering = true, },
		{.extendedDynamicState = true}	};

	std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
	graphicsFamilyIndex = HELPER.findGraphicsQueueFamilyIndex(queueFamilyProperties, *this);

	float arbitraryPriority = 0.5f;
	vk::DeviceQueueCreateInfo queueCreateInfo = {
		.queueFamilyIndex = graphicsFamilyIndex,
		.queueCount = 1,
		.pQueuePriorities = &arbitraryPriority
	};

	vk::DeviceCreateInfo logicalDeviceCreateInfo = {
		.pNext = &enabledFeatures.get<vk::PhysicalDeviceFeatures2>(),
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queueCreateInfo,
		.enabledExtensionCount = static_cast<uint32_t>(REQUIRED_DEVICE_EXTENSIONS.size()),
		.ppEnabledExtensionNames = REQUIRED_DEVICE_EXTENSIONS.data()
	};

	logicalDevice = vk::raii::Device(physicalDevice, logicalDeviceCreateInfo);
	graphicsQueue = vk::raii::Queue(logicalDevice, graphicsFamilyIndex, 0);
	LOGGER.logInformation("Created logical device");
}

void Renderer::createSwapchain() {
	surfaceFormat = HELPER.getSurfaceFormat(physicalDevice.getSurfaceFormatsKHR(surface), *this);
	presentMode = HELPER.getPresentMode(physicalDevice.getSurfacePresentModesKHR(surface), *this);
	extent = HELPER.getSurfaceExtent(physicalDevice.getSurfaceCapabilitiesKHR(surface), *this);
	swapchainImageCount = HELPER.getSwapchainImageCount(physicalDevice.getSurfaceCapabilitiesKHR(surface), *this);

	vk::SwapchainCreateInfoKHR swapchainCreateInfo = {
		.surface = surface,
		.minImageCount = swapchainImageCount,
		.imageFormat = surfaceFormat.format,
		.imageColorSpace = surfaceFormat.colorSpace,
		.imageExtent = extent,
		.imageArrayLayers = 1,
		.imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
		.imageSharingMode = vk::SharingMode::eExclusive,
		.queueFamilyIndexCount = 1,
		.pQueueFamilyIndices = &graphicsFamilyIndex,
		.preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity,
		.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
		.presentMode = presentMode,
		.clipped = true
	};

	swapchain = vk::raii::SwapchainKHR(logicalDevice, swapchainCreateInfo);
	swapchainImageCount = swapchain.getImages().size();
	LOGGER.logInformation("Created swapchain with " + std::to_string(swapchainImageCount) + " images");
}

void Renderer::createImageViews() {
	std::vector<vk::Image> images = swapchain.getImages();

	vk::ImageViewCreateInfo imageViewCreateInfo = {
		.image = {},
		.viewType = vk::ImageViewType::e2D,
		.format = surfaceFormat.format,
		.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
	};

	for(vk::Image const& image : images) {
		imageViewCreateInfo.image = image;

		imageViews.push_back(vk::raii::ImageView(logicalDevice, imageViewCreateInfo));
	}
	LOGGER.logInformation("Created image views");
}

void Renderer::createDescriptorSetLayout() {
	vk::DescriptorSetLayoutBinding binding = {
		.binding = 0,
		.descriptorType = vk::DescriptorType::eUniformBuffer,
		.descriptorCount = 1,
		.stageFlags = vk::ShaderStageFlagBits::eVertex,
	};
	vk::DescriptorSetLayoutCreateInfo descriptorSetCreateInfo = {
		.bindingCount = 1,
		.pBindings = &binding
	};
	descriptorSetLayout = vk::raii::DescriptorSetLayout(logicalDevice, descriptorSetCreateInfo);
}

void Renderer::createGraphicsPipeline() {
	std::vector<char> sprivBytes = HELPER.readSprivFileBytes("src/shaders/shaders.spv", *this);
	vk::ShaderModuleCreateInfo shaderModuleInfo = {
		.codeSize = sprivBytes.size() * sizeof(char),
		.pCode = reinterpret_cast<uint32_t*>(sprivBytes.data())
	};
	vk::raii::ShaderModule module(logicalDevice, shaderModuleInfo);

	vk::PipelineShaderStageCreateInfo vertexShaderInfo = {
		.stage = vk::ShaderStageFlagBits::eVertex,
		.module = module,
		.pName = "vertexMain",
	};
	vk::PipelineShaderStageCreateInfo fragmentShaderInfo = {
		.stage = vk::ShaderStageFlagBits::eFragment,
		.module = module,
		.pName = "fragmentMain",
	};
	vk::PipelineShaderStageCreateInfo shaderStagesInfo[] = { vertexShaderInfo , fragmentShaderInfo };

	vk::VertexInputBindingDescription bindingDesc = VertexAttributes::getBindingDesc();
	std::vector<vk::VertexInputAttributeDescription> attribDesc = VertexAttributes::getAttribDesc();

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &bindingDesc,
		.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribDesc.size()),
		.pVertexAttributeDescriptions = attribDesc.data()
	};

	vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {
		.topology = vk::PrimitiveTopology::eTriangleList,
		.primitiveRestartEnable = vk::False
	};

	vk::PipelineTessellationStateCreateInfo tessellationInfo{};

	vk::Viewport viewport = {
		.x = 0.0f,
		.y = 0.0f,
		.width = static_cast<float>(extent.width),
		.height = static_cast<float>(extent.height),
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};
	vk::Rect2D scissor = {
		.offset = vk::Offset2D(0, 0),
		.extent = extent
	};
	vk::PipelineViewportStateCreateInfo viewportInfo = {
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissor
	};

	vk::PipelineRasterizationStateCreateInfo rasterizationInfo = {
		.depthClampEnable = vk::False,
		.rasterizerDiscardEnable = vk::False,
		.polygonMode = vk::PolygonMode::eFill,
		.cullMode = vk::CullModeFlagBits::eBack,
		.frontFace = vk::FrontFace::eCounterClockwise,
		.depthBiasEnable = vk::False,
		.depthBiasSlopeFactor = 1.0f,
		.lineWidth = 1.0f
	};

	vk::PipelineMultisampleStateCreateInfo multisamplingInfo = {
		.rasterizationSamples = vk::SampleCountFlagBits::e1,
		.sampleShadingEnable = vk::False
	};

	vk::PipelineColorBlendAttachmentState colorBlendAttachmentInfo = {
		.blendEnable = vk::False,
		.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
	};

	vk::PipelineColorBlendStateCreateInfo colorBlendingInfo = {
		.logicOpEnable = vk::False,
		.logicOp = vk::LogicOp::eCopy,
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachmentInfo
	};

	vk::PipelineRenderingCreateInfo attachmentInfo = {
		.colorAttachmentCount = 1,
		.pColorAttachmentFormats = &surfaceFormat.format
	};

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {
		.setLayoutCount = 1,
		.pSetLayouts = &*descriptorSetLayout,
		.pushConstantRangeCount = 0
	};
	pipelineLayout = vk::raii::PipelineLayout(logicalDevice, pipelineLayoutInfo);

	vk::GraphicsPipelineCreateInfo pipelineInfo = {
		.pNext = &attachmentInfo,
		.stageCount = 2,
		.pStages = shaderStagesInfo,
		.pVertexInputState = &vertexInputInfo,
		.pInputAssemblyState = &inputAssemblyInfo,
		.pViewportState = &viewportInfo,
		.pRasterizationState = &rasterizationInfo,
		.pMultisampleState = &multisamplingInfo,
		.pColorBlendState = &colorBlendingInfo,
		.layout = pipelineLayout,
		.renderPass = nullptr
	};

	graphicsPipeline = vk::raii::Pipeline(logicalDevice, nullptr, pipelineInfo);
	LOGGER.logInformation("Pipeline created");
}

void Renderer::createCommandPool() {
	vk::CommandPoolCreateInfo commandPoolCreateInfo = {
		.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		.queueFamilyIndex = graphicsFamilyIndex
	};

	commandPool = vk::raii::CommandPool(logicalDevice, commandPoolCreateInfo);
}

void Renderer::createCommandBuffers() {
	vk::CommandBufferAllocateInfo commandBufferAllocateInfo = {
		.commandPool = commandPool,
		.level = vk::CommandBufferLevel::ePrimary,
		.commandBufferCount = FRAMES_IN_FLIGHT
	};

	commandBuffers = vk::raii::CommandBuffers(logicalDevice, commandBufferAllocateInfo);
}

void Renderer::createVertexBuffer() {
	vk::DeviceSize bufferSize = sizeof(verticies[0]) * verticies.size();

	vk::raii::Buffer stagingBuffer = nullptr;
	vk::raii::DeviceMemory stagingMemory = nullptr;

	HELPER.createBuffer(bufferSize,
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
		stagingMemory,
		stagingBuffer,
		*this);

	void* dataAddress = stagingMemory.mapMemory(0, bufferSize);
	memcpy(dataAddress, verticies.data(), bufferSize);
	stagingMemory.unmapMemory();

	HELPER.createBuffer(bufferSize,
		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal,
		vertexBufferMemory,
		vertexBuffer,
		*this);

	HELPER.copyBuffer(stagingBuffer, vertexBuffer, bufferSize, *this);
}

void Renderer::createIndexBuffer() {
	vk::DeviceSize bufferSize = sizeof(vertexIndices[0]) * vertexIndices.size();

	vk::raii::Buffer stagingBuffer = nullptr;
	vk::raii::DeviceMemory stagingMemory = nullptr;

	HELPER.createBuffer(bufferSize,
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
		stagingMemory,
		stagingBuffer,
		*this);

	void* dataAddress = stagingMemory.mapMemory(0, bufferSize);
	memcpy(dataAddress, vertexIndices.data(), bufferSize);
	stagingMemory.unmapMemory();

	HELPER.createBuffer(bufferSize,
		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal,
		indexMemory,
		indexBuffer,
		*this);

	HELPER.copyBuffer(stagingBuffer, indexBuffer, bufferSize, *this);
}

void Renderer::createUniformBuffers() {
	for(uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
		vk::DeviceSize size = sizeof(UniformBufferObject);
		vk::raii::Buffer buffer({});
		vk::raii::DeviceMemory memory({});

		HELPER.createBuffer(size,
			vk::BufferUsageFlagBits::eUniformBuffer,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
			memory,
			buffer,
			*this);

		uniformBuffers.push_back(std::move(buffer));
		uniformBuffersMemory.push_back(std::move(memory));
		uniformBuffersMapped.push_back(uniformBuffersMemory[i].mapMemory(0, size));
	}
}

void Renderer::createDescriptorPool() {
	vk::DescriptorPoolSize poolSize = {
		.type = vk::DescriptorType::eUniformBuffer,
		.descriptorCount = FRAMES_IN_FLIGHT
	};

	vk::DescriptorPoolCreateInfo poolInfo = { 
		.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 
		.maxSets = FRAMES_IN_FLIGHT, 
		.poolSizeCount = 1, 
		.pPoolSizes = &poolSize 
	};

	descriptorPool = vk::raii::DescriptorPool(logicalDevice, poolInfo);
}

void Renderer::createDescriptorSets() {
	std::vector<vk::DescriptorSetLayout> layouts(FRAMES_IN_FLIGHT, *descriptorSetLayout);

	vk::DescriptorSetAllocateInfo allocInfo = { 
		.descriptorPool = descriptorPool, 
		.descriptorSetCount = static_cast<uint32_t>(layouts.size()), 
		.pSetLayouts = layouts.data() 
	};

	descriptorSets = logicalDevice.allocateDescriptorSets(allocInfo);

	for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
		vk::DescriptorBufferInfo bufferInfo = { 
			.buffer = uniformBuffers[i], 
			.offset = 0, 
			.range = sizeof(UniformBufferObject) 
		};

		vk::WriteDescriptorSet descriptorWrite = { 
			.dstSet = descriptorSets[i], 
			.dstBinding = 0, 
			.dstArrayElement = 0, 
			.descriptorCount = 1, 
			.descriptorType = vk::DescriptorType::eUniformBuffer, 
			.pBufferInfo = &bufferInfo 
		};

		logicalDevice.updateDescriptorSets(descriptorWrite, {});
	}
}

void Renderer::createSyncObjects() {
	for(uint32_t i = 0; i < swapchainImageCount; ++i) {
		renderReady.push_back(vk::raii::Semaphore(logicalDevice, vk::SemaphoreCreateInfo()));
		renderDone.push_back(vk::raii::Semaphore(logicalDevice, vk::SemaphoreCreateInfo()));
	}

	for(uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
		commandBufferDone.push_back(vk::raii::Fence(logicalDevice, { .flags = vk::FenceCreateFlagBits::eSignaled }));
	}

	LOGGER.logInformation(std::string("Command pool, command buffers, and sync objects created (Frames in flight:") + 
		std::to_string(FRAMES_IN_FLIGHT) + ", semaphores:" + std::to_string(renderDone.size()) + ")");
}

void Renderer::drawFrame() {
	while (logicalDevice.waitForFences(*commandBufferDone[frameInFlight], vk::True, UINT64_MAX) == vk::Result::eTimeout);

	std::pair<vk::Result, uint32_t> imagePair = swapchain.acquireNextImage(UINT64_MAX, *renderReady[semaphoreIndex], nullptr);

	if ((imagePair.first == vk::Result::eErrorOutOfDateKHR) || framebufferResized) {
		recreateSwapchain();
		framebufferResized = false;
		return;
	}

	logicalDevice.resetFences(*commandBufferDone[frameInFlight]);

	commandBuffers[frameInFlight].reset();
	HELPER.recordCommandBuffer(imagePair.second, *this);

	updateUniformBuffer(frameInFlight);

	vk::PipelineStageFlags waitMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
	vk::SubmitInfo submitCommandBuffer = {
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &*renderReady[semaphoreIndex],
		.pWaitDstStageMask = &waitMask,
		.commandBufferCount = 1,
		.pCommandBuffers = &*commandBuffers[frameInFlight],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &*renderDone[semaphoreIndex]
	};
	graphicsQueue.submit(submitCommandBuffer, *commandBufferDone[frameInFlight]);

	vk::PresentInfoKHR presentInfo = {
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &*renderDone[semaphoreIndex],
		.swapchainCount = 1,
		.pSwapchains = &*swapchain,
		.pImageIndices = &imagePair.second
	};

	imagePair.first = graphicsQueue.presentKHR(presentInfo);

	if ((imagePair.first == vk::Result::eErrorOutOfDateKHR) || framebufferResized) {
		recreateSwapchain();
		framebufferResized = false;
		return;
	}

	frameInFlight = (frameInFlight + 1) % FRAMES_IN_FLIGHT;
	semaphoreIndex = (semaphoreIndex + 1) % swapchainImageCount;
}

void Renderer::updateUniformBuffer(uint32_t index) {
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto  currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float>(currentTime - startTime).count();

	UniformBufferObject ubo{};
	ubo.model = rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.projection = glm::perspective(glm::radians(45.0f), static_cast<float>(extent.width) / static_cast<float>(extent.height), 0.1f, 10.0f);
	ubo.projection[1][1] *= -1;

	memcpy(uniformBuffersMapped[index], &ubo, sizeof(ubo));
}

void Renderer::mainLoop() {
	frameInFlight = 0;
	semaphoreIndex = 0;
	uint32_t nextSecond = 1;
	uint32_t framesInSecond = 0;

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);

		drawFrame();

		if (glfwGetTime() <= nextSecond) {
			++framesInSecond;
		} else {
			LOGGER.logSpecial("FPS:" + std::to_string(framesInSecond));
			++nextSecond;
			framesInSecond = 0;
		}
	}

	logicalDevice.waitIdle();
}

void Renderer::clean() {
	glfwDestroyWindow(window);
	glfwTerminate();
}

void Renderer::cleanCurrentSwapchain() {
	swapchain = nullptr;
	imageViews.clear();
}

void Renderer::cleanCurrentSyncObjects() {
	commandBufferDone.clear();
	renderReady.clear();
	renderDone.clear();
}

void Renderer::recreateSwapchain() {
	int width = 0, height = 0;
	glfwGetFramebufferSize(window, &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	logicalDevice.waitIdle();

	frameInFlight = 0;
	semaphoreIndex = 0;
	cleanCurrentSyncObjects();
	cleanCurrentSwapchain();

	createSwapchain();
	createImageViews();
	createSyncObjects();
}
