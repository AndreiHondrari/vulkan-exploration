#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include <memory>
#include <vector>
#include <cstring>
#include <optional>
#include <set>
#include <fstream>

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#define NDEBUG

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif


VkResult CreateDebugUtilsMessengerEXT(
  VkInstance instance,
  const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
  const VkAllocationCallbacks* pAllocator,
  VkDebugUtilsMessengerEXT* pDebugMessenger
) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
      instance,
      "vkCreateDebugUtilsMessengerEXT"
    );

    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(
  VkInstance instance,
  VkDebugUtilsMessengerEXT debugMessenger,
  const VkAllocationCallbacks* pAllocator
) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
      instance,
      "vkDestroyDebugUtilsMessengerEXT"
    );
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("ERROR_OPEN_FILE - " + filename);
    }

    size_t fileSize = file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentationFamily;

  bool isComplete() {
    return graphicsFamily.has_value() && presentationFamily.has_value();
  }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class HelloTriangleApp {

private:
  GLFWwindow * m_window;
  uint32_t m_windowWidth = 100;
  uint32_t m_windowHeight = 100;

  VkInstance m_vkInstance;

  VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
  VkDevice m_logicalDevice;

  VkSurfaceKHR m_vkSurface;

  VkSwapchainKHR m_vkSwapChain;
  std::vector<VkImage> m_swapChainImages;
  VkFormat m_swapChainImageFormat;
  VkExtent2D m_swapChainExtent;
  std::vector<VkImageView> m_swapChainImageViews;
  std::vector<VkFramebuffer> m_swapChainFramebuffers;

  VkQueue m_graphicsQueue;
  VkQueue m_presentationQueue;

  VkPipeline m_graphicsPipeline;
  VkRenderPass m_renderPass;
  VkPipelineLayout m_pipelineLayout;

  VkCommandPool m_commandPool;
  VkCommandBuffer m_commandBuffer;

  VkSemaphore m_imageAvailableSemaphore;
  VkSemaphore m_renderFinishedSemaphore;
  VkFence m_inFlightFence;

  VkDebugUtilsMessengerEXT m_debugMessenger;

public:

  HelloTriangleApp(
    const int windowWidth = 640,
    const int windowHeight = 480
  ) {
    m_windowWidth = windowWidth;
    m_windowHeight = windowHeight;
  }

  void run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
  }


private:

  void initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_window = glfwCreateWindow(
      m_windowWidth, m_windowHeight, "Hello Triangle", nullptr, nullptr
    );
  }

  void setupDebugMessenger() {
      if (!enableValidationLayers) return;

      VkDebugUtilsMessengerCreateInfoEXT createInfo;
      populateDebugMessengerCreateInfo(createInfo);

      VkResult result = CreateDebugUtilsMessengerEXT(
        m_vkInstance, &createInfo, nullptr, &m_debugMessenger
      );

      if (result != VK_SUCCESS) {
          throw std::runtime_error("ERROR_FAILED_SETUP_DEBUG_MESSENGER");
      }
  }

  void createVkInstance() {
    std::cout << "CREATE_VK_INSTANCE" << '\n';

    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    // declare necessary structures
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // obtain GLFW extensions
    auto extensions = getRequiredExtensions();

    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // debugging
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    // display some info about VK extensions
    uint32_t extPropertyCount = 0;
    vkEnumerateInstanceExtensionProperties(
      nullptr,
      &extPropertyCount,
      nullptr
    );

    std::vector<VkExtensionProperties> extProperties(extPropertyCount);
    vkEnumerateInstanceExtensionProperties(
      nullptr,
      &extPropertyCount,
      extProperties.data()
    );

    std::cout << "VK_EXT_PROP_COUNT: " << extPropertyCount << '\n';

    std::cout << "EXT_PROP_VECTOR_SIZE: " << extProperties.size() << '\n';;
    for (const auto& ext : extProperties) {
      std::cout << '\t' << ext.extensionName << std::endl;
    }

    // proceed with creating the VK instance
    VkResult result = vkCreateInstance(&createInfo, nullptr, &m_vkInstance);
    if (result != VK_SUCCESS) {
      throw std::runtime_error("ERROR_FAILED_TO_CREATE_VK_INSTANCE");
    }
  }

  void createSurface() {
    VkResult result = glfwCreateWindowSurface(
      m_vkInstance, m_window, nullptr, &m_vkSurface
    );

    if (result != VK_SUCCESS) {
      throw std::runtime_error("ERROR_FAILED_CREATE_WINDOW_SURFACE");
    }
  }

  void pickPhysicalDevice() {
    // count the devices
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, nullptr);

    // no devices handling
    if (deviceCount == 0) {
      throw std::runtime_error("ERROR_NO_PHYSICAL_DEVICES_AVAILABLE");
    }

    // get all devices
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, devices.data());

    // select the first suitable device
    for (const auto& device: devices) {
      if (isDeviceSuitable(device)) {
        m_physicalDevice = device;
        break;
      }
    }

    // no device could be used
    if (m_physicalDevice == VK_NULL_HANDLE) {
      throw std::runtime_error("ERROR_NO_PHYISICAL_DEVICE_SUITABLE");
    }
  }

  void createLogicalDevice() {

    // define create info for the device queues
    QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> devQueueCreateInfoVector;
    std::set<uint32_t> uniqueQueueFamilies = {
      indices.graphicsFamily.value(),
      indices.presentationFamily.value()
    };

    float queuePriority = 1.0f;

    for (const uint32_t queueFamily : uniqueQueueFamilies) {
      VkDeviceQueueCreateInfo devQueueCreateInfo{};
      devQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      devQueueCreateInfo.queueFamilyIndex = queueFamily;
      devQueueCreateInfo.queueCount = 1;
      devQueueCreateInfo.pQueuePriorities = &queuePriority;

      devQueueCreateInfoVector.push_back(devQueueCreateInfo);
    }

    // phyisical device features
    VkPhysicalDeviceFeatures physicalDevFeatures{};

    // define create info for the logical device
    VkDeviceCreateInfo devCreateInfo{};
    devCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    devCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(
      devQueueCreateInfoVector.size()
    );
    devCreateInfo.pQueueCreateInfos = devQueueCreateInfoVector.data();

    devCreateInfo.pEnabledFeatures = &physicalDevFeatures;

    devCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    devCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers) {
      devCreateInfo.enabledLayerCount = static_cast<uint32_t>(
        validationLayers.size()
      );
      devCreateInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
      devCreateInfo.enabledLayerCount = 0;
    }

    // proceed with creating the logical device
    VkResult result = vkCreateDevice(
      m_physicalDevice, &devCreateInfo, nullptr, &m_logicalDevice
    );

    if (result != VK_SUCCESS) {
      throw std::runtime_error("ERROR_LOGICAL_DEVICE_CREATE_FAIL");
    }

    // initialize device queue
    vkGetDeviceQueue(
      m_logicalDevice,
      indices.graphicsFamily.value(),
      0,
      &m_graphicsQueue
    );
    vkGetDeviceQueue(
      m_logicalDevice,
      indices.presentationFamily.value(),
      0,
      &m_presentationQueue
    );
  }

  void createSwapChain() {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (
      swapChainSupport.capabilities.maxImageCount > 0 &&
      imageCount > swapChainSupport.capabilities.maxImageCount
    ) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_vkSurface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);
    uint32_t queueFamilyIndices[] = {
      indices.graphicsFamily.value(),
      indices.presentationFamily.value()
    };

    if (indices.graphicsFamily != indices.presentationFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        // createInfo.queueFamilyIndexCount = 0; // Optional
        // createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VkResult result = vkCreateSwapchainKHR(
      m_logicalDevice, &createInfo, nullptr, &m_vkSwapChain
    );

    if (result != VK_SUCCESS) {
      throw std::runtime_error("ERROR_FAIL_CREATE_SWAPCHAIN");
    }

    vkGetSwapchainImagesKHR(
      m_logicalDevice, m_vkSwapChain, &imageCount, nullptr
    );
    m_swapChainImages.resize(imageCount);

    vkGetSwapchainImagesKHR(
      m_logicalDevice, m_vkSwapChain, &imageCount, m_swapChainImages.data()
    );

    m_swapChainImageFormat = surfaceFormat.format;
    m_swapChainExtent = extent;
  }

  void createImageViews() {
    m_swapChainImageViews.resize(m_swapChainImages.size());

    for (size_t i = 0; i < m_swapChainImages.size(); ++i) {
      VkImageViewCreateInfo createInfo{};
      createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      createInfo.image = m_swapChainImages[i];
      createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
      createInfo.format = m_swapChainImageFormat;

      createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

      createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      createInfo.subresourceRange.baseMipLevel = 0;
      createInfo.subresourceRange.levelCount = 1;
      createInfo.subresourceRange.baseArrayLayer = 0;
      createInfo.subresourceRange.layerCount = 1;

      VkResult result = vkCreateImageView(
        m_logicalDevice, &createInfo, nullptr, &m_swapChainImageViews[i]
      );

      if (result != VK_SUCCESS) {
        throw std::runtime_error("ERROR_FAIL_CREATE_IMAGE_VIEW");
      }
    }
  }

  void createRenderPass() {
    // attachment description
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = m_swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // subpasses and attachment references
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription{};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency subpassDependency{};
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass = 0;
    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.srcAccessMask = 0;
    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    // render pass
    VkRenderPassCreateInfo renderPassCreateInfo{};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments = &colorAttachment;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpassDescription;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &subpassDependency;

    VkResult result = vkCreateRenderPass(
      m_logicalDevice, &renderPassCreateInfo, nullptr, &m_renderPass
    );

    if (result != VK_SUCCESS) {
      throw std::runtime_error("ERROR_FAIL_CREATE_RENDER_PASS");
    }

  }

  void createGraphicsPipeline() {
    // obtain shaders SPIR-V code
    auto vertShaderCode = readFile("shaders/vert.spv");
    auto fragShaderCode = readFile("shaders/frag.spv");

    // create shader modules
    VkShaderModule vertModule = createShaderModule(vertShaderCode);
    VkShaderModule fragModule = createShaderModule(fragShaderCode);

    // create shaders stages
    VkPipelineShaderStageCreateInfo vertStageCreateInfo{};
    vertStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStageCreateInfo.module = vertModule;
    vertStageCreateInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragStageCreateInfo{};
    fragStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStageCreateInfo.module = fragModule;
    fragStageCreateInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {
      vertStageCreateInfo,
      fragStageCreateInfo
    };

    // vertext input
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

    // input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{};
    inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

    // viewport
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) m_swapChainExtent.width;
    viewport.height = (float) m_swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // scissors
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
    viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;

    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.pViewports = &viewport;

    viewportStateCreateInfo.scissorCount = 1;
    viewportStateCreateInfo.pScissors = &scissor;

    // rasterizer
    VkPipelineRasterizationStateCreateInfo rasterStateCreateInfo{};
    rasterStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterStateCreateInfo.depthClampEnable = VK_FALSE;
    rasterStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterStateCreateInfo.lineWidth = 1.0f;
    rasterStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterStateCreateInfo.depthBiasEnable = VK_FALSE;
    rasterStateCreateInfo.depthBiasConstantFactor = 0.0f;
    rasterStateCreateInfo.depthBiasClamp = 0.0f;
    rasterStateCreateInfo.depthBiasSlopeFactor = 0.0f;

    // multisampling
    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo{};
    multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
    multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleStateCreateInfo.minSampleShading = 1.0f; // Optional
    multisampleStateCreateInfo.pSampleMask = nullptr; // Optional
    multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE; // Optional

    // color blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};
    colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlendStateCreateInfo.attachmentCount = 1;
    colorBlendStateCreateInfo.pAttachments = &colorBlendAttachment;
    colorBlendStateCreateInfo.blendConstants[0] = 0.0f; // Optional
    colorBlendStateCreateInfo.blendConstants[1] = 0.0f; // Optional
    colorBlendStateCreateInfo.blendConstants[2] = 0.0f; // Optional
    colorBlendStateCreateInfo.blendConstants[3] = 0.0f; // Optional

    // dynamic state
    std::vector<VkDynamicState> dynamicStates = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_LINE_WIDTH
    };
    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

    // pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 0;
    pipelineLayoutCreateInfo.pSetLayouts = nullptr; // Optional
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr; // Optional

    VkResult result = vkCreatePipelineLayout(
      m_logicalDevice, &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout
    );

    if (result != VK_SUCCESS) {
      throw std::runtime_error("ERROR_FAIL_CREATE_PIPELINE_LAYOUT");
    }

    // graphics pipeline
    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
    graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    graphicsPipelineCreateInfo.stageCount = 2;
    graphicsPipelineCreateInfo.pStages = shaderStages;

    graphicsPipelineCreateInfo.pVertexInputState = &vertexInputInfo;
    graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
    graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    graphicsPipelineCreateInfo.pRasterizationState = &rasterStateCreateInfo;
    graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;

    graphicsPipelineCreateInfo.pDepthStencilState = nullptr;
    graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
    graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;

    graphicsPipelineCreateInfo.layout = m_pipelineLayout;
    graphicsPipelineCreateInfo.renderPass = m_renderPass;
    graphicsPipelineCreateInfo.subpass = 0;

    graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    graphicsPipelineCreateInfo.basePipelineIndex = -1;

    result = vkCreateGraphicsPipelines(
      m_logicalDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &m_graphicsPipeline
    );

    if (result != VK_SUCCESS) {
      throw std::runtime_error("ERROR_FAIL_CREATE_GRAPHICS_PIPELINE");
    }

    // destroy shaders
    vkDestroyShaderModule(m_logicalDevice, vertModule, nullptr);
    vkDestroyShaderModule(m_logicalDevice, fragModule, nullptr);
  }

  void createFramebuffers() {
    m_swapChainFramebuffers.resize(m_swapChainImageViews.size());

    for (size_t i = 0; i < m_swapChainFramebuffers.size(); ++i) {
      VkImageView attachments[] = {
          m_swapChainImageViews[i]
      };

      VkFramebufferCreateInfo fbCreateInfo{};
      fbCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      fbCreateInfo.renderPass = m_renderPass;
      fbCreateInfo.attachmentCount = 1;
      fbCreateInfo.pAttachments = attachments;
      fbCreateInfo.width = m_swapChainExtent.width;
      fbCreateInfo.height = m_swapChainExtent.height;
      fbCreateInfo.layers = 1;

      VkResult result = vkCreateFramebuffer(
        m_logicalDevice, &fbCreateInfo, nullptr, &m_swapChainFramebuffers[i]
      );

      if (result != VK_SUCCESS) {
        throw std::runtime_error("ERROR_FAIL_CREATE_FRAMEBUFFER");
      }
    }
  }

  void createCommandPool() {
    QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);

    VkCommandPoolCreateInfo cmdPoolCreateInfo{};
    cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmdPoolCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();

    VkResult result = vkCreateCommandPool(
      m_logicalDevice, &cmdPoolCreateInfo, nullptr, &m_commandPool
    );

    if (result != VK_SUCCESS) {
      throw std::runtime_error("ERROR_FAIL_CREATE_COMMAND_POOL");
    }
  }

  void createCommandBuffer() {
    VkCommandBufferAllocateInfo cmdBuffAllocInfo{};
    cmdBuffAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBuffAllocInfo.commandPool = m_commandPool;
    cmdBuffAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBuffAllocInfo.commandBufferCount = 1;

    VkResult result = vkAllocateCommandBuffers(
      m_logicalDevice, &cmdBuffAllocInfo, &m_commandBuffer
    );

    if (result != VK_SUCCESS) {
      throw std::runtime_error("ERROR_FAIL_CREATE_COMMAND_BUFFER");
    }
  }

  void createSyncObjects() {
    VkSemaphoreCreateInfo semCreateInfo{};
    semCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceCreateInfo{};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (
      vkCreateSemaphore(m_logicalDevice, &semCreateInfo, nullptr, &m_imageAvailableSemaphore) != VK_SUCCESS ||
      vkCreateSemaphore(m_logicalDevice, &semCreateInfo, nullptr, &m_renderFinishedSemaphore) != VK_SUCCESS ||
      vkCreateFence(m_logicalDevice, &fenceCreateInfo, nullptr, &m_inFlightFence) != VK_SUCCESS
    ) {
      throw std::runtime_error("ERROR_FAIL_SYNC_OBJECTS_CREATE");
    }
  }

  void initVulkan() {
    std::cout << "INIT_VULKAN" << '\n';
    createVkInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createCommandBuffer();
    createSyncObjects();
  }

  void drawFrame() {
    // wait for previous frame to be processed by the GPU
    vkWaitForFences(m_logicalDevice, 1, &m_inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(m_logicalDevice, 1, &m_inFlightFence);

    // acquire an image from swap chain
    uint32_t imageIndex;
    vkAcquireNextImageKHR(
      m_logicalDevice, m_vkSwapChain,
      UINT64_MAX,
      m_imageAvailableSemaphore, VK_NULL_HANDLE,
      &imageIndex
    );

    // record command buffer
    vkResetCommandBuffer(m_commandBuffer, 0);
    recordCommandBuffer(m_commandBuffer, imageIndex);

    // submit the command buffer
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffer;

    VkSemaphore signalSemaphores[] = {m_renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    VkResult result = vkQueueSubmit(
      m_graphicsQueue, 1, &submitInfo, m_inFlightFence
    );

    if (result != VK_SUCCESS) {
      throw std::runtime_error("ERROR_FAIL_SUBMIT_GRAPHICS_QUEUE");
    }

    VkSwapchainKHR swapChains[] = {m_vkSwapChain};
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optional

    vkQueuePresentKHR(m_presentationQueue, &presentInfo);
  }

  void mainLoop() {
    std::cout << "MAIN_LOOP_START" << '\n';
    while (!glfwWindowShouldClose(m_window)) {
      glfwPollEvents();
      drawFrame();
    }

    vkDeviceWaitIdle(m_logicalDevice);
  }

  void cleanup() {
    vkDestroySemaphore(m_logicalDevice, m_imageAvailableSemaphore, nullptr);
    vkDestroySemaphore(m_logicalDevice, m_renderFinishedSemaphore, nullptr);
    vkDestroyFence(m_logicalDevice, m_inFlightFence, nullptr);

    vkDestroyCommandPool(m_logicalDevice, m_commandPool, nullptr);

    for (auto framebuffer : m_swapChainFramebuffers) {
      vkDestroyFramebuffer(m_logicalDevice, framebuffer, nullptr);
    }

    vkDestroyRenderPass(m_logicalDevice, m_renderPass, nullptr);

    vkDestroyPipeline(m_logicalDevice, m_graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_logicalDevice, m_pipelineLayout, nullptr);

    for (VkImageView imageView : m_swapChainImageViews) {
      vkDestroyImageView(m_logicalDevice, imageView, nullptr);
    }

    vkDestroySwapchainKHR(m_logicalDevice, m_vkSwapChain, nullptr);

    vkDestroyDevice(m_logicalDevice, nullptr);

    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(m_vkInstance, m_debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(m_vkInstance, m_vkSurface, nullptr);

    vkDestroyInstance(m_vkInstance, nullptr);

    glfwDestroyWindow(m_window);
    glfwTerminate();
  }

  std::vector<const char*> getRequiredExtensions() {
      uint32_t glfwExtensionCount = 0;
      const char** glfwExtensions;
      glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

      std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

      if (enableValidationLayers) {
          extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
      }

      return extensions;
  }

  void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
      createInfo = {};
      createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
      createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
      createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
      createInfo.pfnUserCallback = debugCallback;
  }

  bool checkValidationLayerSupport() {
      uint32_t layerCount;
      vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

      std::vector<VkLayerProperties> availableLayers(layerCount);
      vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

      for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
  }

  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(
      physicalDevice, &queueFamilyCount, nullptr
    );

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(
      physicalDevice, &queueFamilyCount, queueFamilies.data()
    );

    VkBool32 presentationSupport = false;

    int i = 0;
    for (const auto& family : queueFamilies) {

      if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {

        // queue families query
        indices.graphicsFamily = i;

        // presentation support query
        vkGetPhysicalDeviceSurfaceSupportKHR(
          physicalDevice, i, m_vkSurface, &presentationSupport
        );

        if (presentationSupport) {
          indices.presentationFamily = i;
        }

        if (indices.isComplete()) {
          break;
        }
      }

      ++i;
    }

    return indices;
  }

  SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      device, m_vkSurface, &details.capabilities
    );

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_vkSurface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_vkSurface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_vkSurface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_vkSurface, &presentModeCount, details.presentModes.data());
    }

    return details;
  }

  bool isDeviceSuitable(VkPhysicalDevice device) {
    // acquire device properties
    VkPhysicalDeviceProperties deviceProperties{};
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    // acquire device features
    VkPhysicalDeviceFeatures deviceFeatures{};
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    std::cout << "VK_PD_TYPE: " << deviceProperties.deviceType << '\n';
    std::cout << "VK_PD_GEO_SHADER: " << deviceFeatures.geometryShader << '\n';

    // queue family indices
    QueueFamilyIndices indices = findQueueFamilies(device);

    // check extensions
    bool extensionsSupported = checkDeviceExtensionSupport(device);
    bool swapChainAdequate = false;

    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return indices.isComplete() && extensionsSupported && swapChainAdequate;
    // OBSERVATION: We could do more advanced stuff, but not for now
    // return (
    //   deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
    // );
  }

  bool checkDeviceExtensionSupport(VkPhysicalDevice phyiscalDevice) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(
      phyiscalDevice, nullptr, &extensionCount, nullptr
    );

    std::vector<VkExtensionProperties> extPropertiesVector(extensionCount);
    vkEnumerateDeviceExtensionProperties(
      phyiscalDevice, nullptr, &extensionCount, extPropertiesVector.data()
    );

    std::set<std::string> requiredExtensions(
      deviceExtensions.begin(),
      deviceExtensions.end()
    );

    for (const auto& extension : extPropertiesVector) {
      requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
  }

  VkSurfaceFormatKHR chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& availableFormats
  ) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
  }

  VkPresentModeKHR chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR>& availablePresentModes
  ) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
  }

  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(m_window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
  }

  VkShaderModule createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    VkResult result = vkCreateShaderModule(
      m_logicalDevice, &createInfo, nullptr, &shaderModule
    );

    if (result != VK_SUCCESS) {
      throw std::runtime_error("ERROR_FAIL_CREATE_SHADER_MODULE");
    }

    return shaderModule;
  }

  void recordCommandBuffer(
    VkCommandBuffer commandBuffer,
    uint32_t imageIndex
  ) {
    VkResult result;

    // begin command buffer recording
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    if (result != VK_SUCCESS) {
      throw std::runtime_error("ERROR_FAIL_COMMAND_BUFFER_BEGIN");
    }

    // render pass
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = m_swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_swapChainExtent;

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(
      commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE
    );

    // drawing commands
    vkCmdBindPipeline(
      commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline
    );
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    // end command buffer recording
    result = vkEndCommandBuffer(commandBuffer);
    if (result != VK_SUCCESS) {
      throw std::runtime_error("ERROR_FAIL_COMMAND_BUFFER_RECORDING");
    }

  }

  static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData
  ) {
      std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

      return VK_FALSE;
  }

};


int main(int argc, char const *argv[]) {
  std::cout << "START HELLO TRIANGLE APP" << '\n';

  HelloTriangleApp app(800, 600);

  try {
    app.run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "STOP HELLO TRIANGLE APP" << '\n';
  return EXIT_SUCCESS;
}
