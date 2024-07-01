#include "render.h"

#ifndef VMA_IMPLEMENTATION
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
#endif

RENDER_LOGGER_DEFINITION
namespace render {
namespace vkutil {
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator,
                                      VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto function =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (function != nullptr) {
        return function(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks* pAllocator) {
    auto function =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (function != nullptr) {
        function(instance, debugMessenger, pAllocator);
    }
}
} // namespace vkutil
namespace validation {
std::vector<const char*> VkInstanceExtensionSupport(std::vector<const char*> extension_names) {
    uint32_t extension_property_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_property_count, nullptr);
    auto extension_properties = new VkExtensionProperties[extension_property_count];
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_property_count, extension_properties);
    for (unsigned int i = 0; i < extension_property_count; i++) {
        for (auto iterator = extension_names.begin(); iterator != extension_names.end(); iterator++) {
            if (strcmp(extension_properties[i].extensionName, *iterator) == 0) {
                extension_names.erase(iterator);
                break;
            }
        }
    }
    delete[] extension_properties;
    return extension_names;
}
std::vector<const char*> VkInstanceLayerSupport(std::vector<const char*> layer_names) {
    uint32_t layer_property_count = 0;
    vkEnumerateInstanceLayerProperties(&layer_property_count, nullptr);
    auto layer_properties = new VkLayerProperties[layer_property_count];
    vkEnumerateInstanceLayerProperties(&layer_property_count, layer_properties);
    for (unsigned int i = 0; i < layer_property_count; i++) {
        for (auto iterator = layer_names.begin(); iterator != layer_names.end(); iterator++) {
            if (strcmp(layer_properties[i].layerName, *iterator) == 0) {
                layer_names.erase(iterator);
                break;
            }
        }
    }
    delete[] layer_properties;
    return layer_names;
}
static VKAPI_ATTR VkBool32 VKAPI_CALL DefaultVkDebugUtilsMessengerEXTCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
    RENDER_LOG_ERROR("VULKAN VALIDATION LAYER: {}", pCallbackData->pMessage);
    return VK_FALSE;
}

std::vector<const char*> VkDeviceExtensionSupport(VkPhysicalDevice vk_physical_device,
                                                  std::vector<const char*> extension_names) {
    uint32_t extension_property_count = 0;
    vkEnumerateDeviceExtensionProperties(vk_physical_device, nullptr, &extension_property_count, nullptr);
    auto extension_properties = new VkExtensionProperties[extension_property_count];
    vkEnumerateDeviceExtensionProperties(vk_physical_device, nullptr, &extension_property_count, extension_properties);
    for (unsigned int i = 0; i < extension_property_count; i++) {
        for (auto iterator = extension_names.begin(); iterator != extension_names.end(); iterator++) {
            if (strcmp(extension_properties[i].extensionName, *iterator) == 0) {
                extension_names.erase(iterator);
                break;
            }
        }
    }
    delete[] extension_properties;
    return extension_names;
}
} // namespace validation

uint32_t RateVkPhysicalDevice(VkPhysicalDevice vk_physical_device, std::vector<const char*> device_extension_names) {
    auto unsupported_extensions = validation::VkDeviceExtensionSupport(vk_physical_device, device_extension_names);
    if (unsupported_extensions.size() > 0) {
        std::string unsupported_extension_string;
        for (const char* extension : unsupported_extensions) {
            unsupported_extension_string += extension;
            unsupported_extension_string += ", ";
        }
        RENDER_LOG_ERROR("The Following VkDevice Extensions are Unsupported: {}", unsupported_extension_string);
        return 0;
    }
    return 1;
}
VulkanQueueIndices QueryVkPhysicalDeviceQueueSupport(VkPhysicalDevice vk_physical_device) {
    VulkanQueueIndices queue_indices{};
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(vk_physical_device, &queue_family_count, nullptr);
    VkQueueFamilyProperties* queue_family_properties = new VkQueueFamilyProperties[queue_family_count];
    vkGetPhysicalDeviceQueueFamilyProperties(vk_physical_device, &queue_family_count, queue_family_properties);

    const uint32_t universal_queue_bitmask = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
    for (int i = 0; i < queue_family_count; i++) {
        if ((queue_family_properties[i].queueFlags & universal_queue_bitmask) == universal_queue_bitmask) {
            queue_indices.universal_queue_count = queue_family_properties[i].queueCount;
            queue_indices.universal_family_index = i;
        } else if (queue_family_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
        } else if (!(queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
                   queue_family_properties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
        }
    }
    return queue_indices;
}

render::Context context{};
Context CreateContext(ContextInfo info) {
    Context context{};
    std::vector<const char*> extension_names{};

    if (info.window) {
        unsigned int window_extension_count = 0;
        core::window::GetRequiredVkInstanceExtensions(info.window.value(), &window_extension_count, nullptr);
        extension_names.resize(window_extension_count);
        core::window::GetRequiredVkInstanceExtensions(info.window.value(), &window_extension_count,
                                                      extension_names.data());
    }

    extension_names.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    extension_names.emplace_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

    if (info.enable_validation_layers) {
        extension_names.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    auto unsupported_extensions = validation::VkInstanceExtensionSupport(extension_names);
    if (unsupported_extensions.size() > 0) {
        std::string unsupported_extension_string;
        for (const char* extension : unsupported_extensions) {
            unsupported_extension_string += extension;
            unsupported_extension_string += ", ";
        }
        RENDER_LOG_ERROR("The Following VkInstance Extensions are Unsupported: {}", unsupported_extension_string);
    }

    std::vector<const char*> layer_names;
    if (info.enable_validation_layers) {
        layer_names.emplace_back("VK_LAYER_KHRONOS_validation");
    }
    auto unsupported_layers = validation::VkInstanceExtensionSupport(extension_names);
    if (unsupported_layers.size() > 0) {
        std::string unsupported_layer_string;
        for (const char* layer : unsupported_layers) {
            unsupported_layer_string += layer;
            unsupported_layer_string += ", ";
        }
        RENDER_LOG_ERROR("The Following VkInstance Layers are Unsupported: {}", unsupported_layer_string);
    }

    VkApplicationInfo application_info{};
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.pNext = nullptr;

    application_info.pEngineName = info.engine_name;
    application_info.pApplicationName = info.applcation_name;
    application_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    application_info.engineVersion = VK_MAKE_VERSION(1, 2, 0);
    application_info.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo instance_create_info{};
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pNext = nullptr;
    instance_create_info.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

    instance_create_info.pApplicationInfo = &application_info;

    instance_create_info.enabledExtensionCount = (uint32_t)extension_names.size();
    instance_create_info.ppEnabledExtensionNames = extension_names.data();
    instance_create_info.enabledLayerCount = (uint32_t)layer_names.size();
    instance_create_info.ppEnabledLayerNames = layer_names.data();

    VkResult result = vkCreateInstance(&instance_create_info, nullptr, &context.vk_instance);
    if (result != VK_SUCCESS) {
        RENDER_LOG_ERROR("CONTEXT CREATION: Failed to Create VkInstance!");
    }

    if (info.enable_validation_layers) {
        VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info{};
        debug_messenger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debug_messenger_create_info.pNext = nullptr;
        debug_messenger_create_info.flags = 0;

        debug_messenger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug_messenger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                                  VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                                  VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug_messenger_create_info.pfnUserCallback = &validation::DefaultVkDebugUtilsMessengerEXTCallback;
        debug_messenger_create_info.pUserData = nullptr;

        result = vkutil::CreateDebugUtilsMessengerEXT(context.vk_instance, &debug_messenger_create_info, nullptr,
                                                      &context.vk_debug_utils_messenger);
        if (result != VK_SUCCESS) {
            RENDER_LOG_ERROR("CONTEXT CREATION: Failed to Create VkDebugUtilsMessengerEXT!");
        }
    }

    std::vector<const char*> device_extension_names{};
    device_extension_names.emplace_back("VK_KHR_portability_subset");
    if (info.window) {
        device_extension_names.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }

    uint32_t physical_device_count;
    vkEnumeratePhysicalDevices(context.vk_instance, &physical_device_count, nullptr);
    VkPhysicalDevice* physical_devices = new VkPhysicalDevice[physical_device_count];
    vkEnumeratePhysicalDevices(context.vk_instance, &physical_device_count, physical_devices);
    std::vector<std::tuple<uint32_t, VkPhysicalDevice>> rated_physical_devices;
    for (unsigned int i = 0; i < physical_device_count; i++) {
        rated_physical_devices.emplace_back(
            std::make_tuple(RateVkPhysicalDevice(physical_devices[i], device_extension_names), physical_devices[i]));
    }
    delete[] physical_devices;
    // Sort By First Element of Tuple / Device Rating
    std::sort(rated_physical_devices.begin(), rated_physical_devices.end());

    VulkanQueueIndices queue_indices{};
    for (std::tuple<uint32_t, VkPhysicalDevice> tuple : rated_physical_devices) {
        auto vk_physical_device = std::get<1>(tuple);

        queue_indices = QueryVkPhysicalDeviceQueueSupport(vk_physical_device);
        float priority = 1.0f;
        std::vector<VkDeviceQueueCreateInfo> device_queue_create_info{};
        if (queue_indices.universal_queue_count > 0) {
            VkDeviceQueueCreateInfo queue_create_info{};
            queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_info.pNext = nullptr;
            queue_create_info.flags = 0;

            queue_create_info.queueCount = 1;
            queue_create_info.queueFamilyIndex = queue_indices.universal_family_index;
            queue_create_info.pQueuePriorities = &priority;

            device_queue_create_info.emplace_back(queue_create_info);
        }

        VkPhysicalDeviceFeatures device_features{};

        VkDeviceCreateInfo device_create_info{};
        device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_create_info.pNext = nullptr;
        device_create_info.flags = 0;

        device_create_info.queueCreateInfoCount = (uint32_t)device_queue_create_info.size();
        device_create_info.pQueueCreateInfos = device_queue_create_info.data();

        device_create_info.enabledExtensionCount = (uint32_t)device_extension_names.size();
        device_create_info.ppEnabledExtensionNames = device_extension_names.data();
        device_create_info.enabledLayerCount = (uint32_t)layer_names.size();
        device_create_info.ppEnabledLayerNames = layer_names.data();
        device_create_info.pEnabledFeatures = &device_features;

        result = vkCreateDevice(vk_physical_device, &device_create_info, nullptr, &context.vk_device);
        if (result == VK_SUCCESS) {
            context.vk_physical_device = vk_physical_device;
            break;
        }
    }
    if (context.vk_physical_device == VK_NULL_HANDLE || context.vk_device == VK_NULL_HANDLE) {
        RENDER_LOG_ERROR("CONTEXT CREATION: Failed to Create VkDevice!");
    }
    context.universal_queue.vk_family_index = queue_indices.universal_family_index;
    vkGetDeviceQueue(context.vk_device, context.universal_queue.vk_family_index, 0, &context.universal_queue.vk_queue);

    VmaVulkanFunctions vma_vulkan_functions = {};
    vma_vulkan_functions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
    vma_vulkan_functions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocator_create_info = {};
    allocator_create_info.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
    allocator_create_info.vulkanApiVersion = VK_API_VERSION_1_2;
    allocator_create_info.physicalDevice = context.vk_physical_device;
    allocator_create_info.device = context.vk_device;
    allocator_create_info.instance = context.vk_instance;
    allocator_create_info.pVulkanFunctions = &vma_vulkan_functions;
    vmaCreateAllocator(&allocator_create_info, &context.vma_allocator);
    if (result != VK_SUCCESS) {
        RENDER_LOG_ERROR("CONTEXT CREATION: Failed to Create VmaAllocator!");
    }
    return context;
}
void DestroyContext(Context context) {
    vmaDestroyAllocator(context.vma_allocator);

    vkDestroyDevice(context.vk_device, nullptr);

    vkutil::DestroyDebugUtilsMessengerEXT(context.vk_instance, context.vk_debug_utils_messenger, nullptr);
    vkDestroyInstance(context.vk_instance, nullptr);
}

CommandPool* CreateCommandPool() {
    CommandPool* pool = new CommandPool{};
    VkCommandPoolCreateInfo pool_create_info{};
    pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.pNext = nullptr;
    pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    pool_create_info.queueFamilyIndex = context.universal_queue.vk_family_index;
    VkResult result =
        vkCreateCommandPool(render::context.vk_device, &pool_create_info, nullptr, &pool->vk_command_pool);
    if (result != VK_SUCCESS) {
        RENDER_LOG_ERROR("COMMAND POOL CREATION: Failed to Create VkCommandPool!");
    }

    pool->active = true;
    pool->record_thread = std::thread(command_pool::RecordThreadFunction, pool);
    return pool;
}
void DestroyCommandPool(CommandPool* pool) {
    pool->mutex.lock();
    pool->active = false;
    pool->record_queue.emplace_back([]() {});
    pool->mutex.unlock();
    pool->condition_variable.notify_one();

    pool->record_thread.join();

    vkDestroyCommandPool(context.vk_device, pool->vk_command_pool, nullptr);
    for (auto command_buffer : pool->free_command_buffer_queue) {
        delete command_buffer;
    }
    delete pool;
}
namespace command_pool {
CommandBuffer* BorrowCommandBuffer(CommandPool* pool) {
    if (pool->free_command_buffer_queue.size() != 0) {
        auto command_buffer = pool->free_command_buffer_queue.back();
        pool->free_command_buffer_queue.pop_back();
        return command_buffer;
    }
    auto command_buffer = new CommandBuffer{};
    VkCommandBufferAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.pNext = nullptr;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;
    allocate_info.commandPool = pool->vk_command_pool;
    VkResult result =
        vkAllocateCommandBuffers(render::context.vk_device, &allocate_info, &command_buffer->vk_command_buffer);
    if (result != VK_SUCCESS) {
        RENDER_LOG_ERROR("GET COMMAND BUFFER: Failed to Allocate VkCommandBuffer from VkCommandPool!");
    }
    return command_buffer;
}
void ReturnCommandBuffer(CommandPool* pool, CommandBuffer* command_buffer) {
    pool->free_command_buffer_queue.emplace_front(command_buffer);
}

void RecordThreadFunction(CommandPool* pool) {
    while (pool->active) {
        std::unique_lock<std::mutex> lock(pool->mutex);
        pool->condition_variable.wait(lock, [pool]() { return pool->record_queue.size() != 0; });
        auto function = pool->record_queue.back();
        pool->record_queue.pop_back();
        function();
        lock.unlock();
    }
}
void RecordAsync(CommandPool* pool, std::function<void()> function) {
    pool->mutex.lock();
    pool->record_queue.emplace_back(function);
    pool->mutex.unlock();
    pool->condition_variable.notify_one();
}
} // namespace command_pool

namespace renderpass {
void Initialize(Renderpass* renderpass, RenderpassInfo info) {
    std::vector<VkAttachmentDescription> vk_attachment_descriptions{};
    for (Attachment& attachment : info.attachments) {
        VkAttachmentDescription vk_attachment{};
        vk_attachment.flags = 0;
        vk_attachment.initialLayout = attachment.initial_layout;
        vk_attachment.finalLayout = attachment.final_layout;
        vk_attachment.format = attachment.format;
        vk_attachment.loadOp = (VkAttachmentLoadOp)attachment.load_op;
        vk_attachment.storeOp = (VkAttachmentStoreOp)attachment.store_op;
        vk_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        vk_attachment_descriptions.emplace_back(vk_attachment);
    }
    if (info.swapchain_attachment != nullptr) {
        VkAttachmentDescription vk_attachment{};
        vk_attachment.flags = 0;
        vk_attachment.initialLayout = info.swapchain_attachment->initial_layout;
        vk_attachment.finalLayout = info.swapchain_attachment->final_layout;
        vk_attachment.format =
            reinterpret_cast<Swapchain*>(info.swapchain_attachment->swapchain)->vk_surface_format.format;
        vk_attachment.loadOp = (VkAttachmentLoadOp)info.swapchain_attachment->load_op;
        vk_attachment.storeOp = (VkAttachmentStoreOp)info.swapchain_attachment->store_op;
        vk_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        vk_attachment_descriptions.emplace_back(vk_attachment);
    }

    std::vector<VkSubpassDescription> vk_subpass_descriptions{};
    for (Subpass& subpass : info.subpasses) {
        VkSubpassDescription vk_subpass{};
        vk_subpass.flags = 0;
        vk_subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        vk_subpass.colorAttachmentCount = (uint32_t)subpass.color_attachments.size();
        vk_subpass.pColorAttachments = (VkAttachmentReference*)subpass.color_attachments.data();
        vk_subpass.pDepthStencilAttachment = (VkAttachmentReference*)subpass.depth_stencil_attachment;
        vk_subpass_descriptions.emplace_back(vk_subpass);
    }

    VkRenderPassCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    create_info.pNext = nullptr;
    create_info.flags = 0;
    create_info.attachmentCount = (uint32_t)vk_attachment_descriptions.size();
    create_info.pAttachments = vk_attachment_descriptions.data();
    create_info.subpassCount = (uint32_t)vk_subpass_descriptions.size();
    create_info.pSubpasses = vk_subpass_descriptions.data();
    create_info.dependencyCount = 0;
    create_info.pDependencies = nullptr;
    VkResult result = vkCreateRenderPass(context.vk_device, &create_info, nullptr, &renderpass->vk_render_pass);
    if (result != VK_SUCCESS) {
        RENDER_LOG_ERROR("RENDERPASS CREATION: Failed to Create VKRenderPass!");
    }
}
void Finalize(Renderpass* renderpass) {
    vkDestroyRenderPass(render::context.vk_device, renderpass->vk_render_pass, nullptr);
}
void Recreate(Renderpass* renderpass) {
    Finalize(renderpass);
    Initialize(renderpass, *renderpass->recreation_info);
}
} // namespace renderpass
Renderpass* CreateRenderpass(RenderpassInfo info) {
    auto renderpass = new Renderpass{};
    renderpass->recreation_info = info;
    renderpass::Initialize(renderpass, info);
    return renderpass;
}
void DestroyRenderpass(Renderpass* renderpass) {
    renderpass::Finalize(renderpass);
    delete renderpass;
}
void CreateInstance();

namespace framebuffer {
void Initialize(Framebuffer* framebuffer, FramebufferInfo info) {
    VkFramebufferCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    create_info.flags = 0;
    create_info.pNext = nullptr;

    create_info.renderPass = info.renderpass->vk_render_pass;

    if (info.swapchain_attachment != nullptr) {
        Swapchain* swapchain_attachment = (Swapchain*)info.swapchain_attachment;
        create_info.width = swapchain_attachment->extent.x;
        create_info.height = swapchain_attachment->extent.y;
        create_info.layers = swapchain_attachment->extent.z;

        uint32_t swapchain_attachment_index = info.attachments.size();
        info.attachments.emplace_back(VkImageView{VK_NULL_HANDLE});
        create_info.attachmentCount = info.attachments.size();
        create_info.pAttachments = info.attachments.data();
        for (auto view : swapchain_attachment->vk_image_views) {
            info.attachments[swapchain_attachment_index] = view;
            VkFramebuffer vk_framebuffer = VK_NULL_HANDLE;
            VkResult result = vkCreateFramebuffer(context.vk_device, &create_info, nullptr, &vk_framebuffer);
            if (result != VK_SUCCESS) {
                RENDER_LOG_ERROR("RENDERPASS CREATION: Failed to Create VKRenderPass!");
            }
            framebuffer->vk_framebuffer.emplace_back(vk_framebuffer);
        }
        return;
    }

    create_info.attachmentCount = info.attachments.size();
    create_info.pAttachments = info.attachments.data();

    VkFramebuffer vk_framebuffer = VK_NULL_HANDLE;
    VkResult result = vkCreateFramebuffer(context.vk_device, &create_info, nullptr, &vk_framebuffer);
    if (result != VK_SUCCESS) {
        RENDER_LOG_ERROR("RENDERPASS CREATION: Failed to Create VKRenderPass!");
    }
    framebuffer->vk_framebuffer.emplace_back(vk_framebuffer);
    return;
}
void Finalize(Framebuffer* framebuffer) {
    for (auto vk_framebuffer : framebuffer->vk_framebuffer) {
        vkDestroyFramebuffer(context.vk_device, vk_framebuffer, nullptr);
    }
}
} // namespace framebuffer
Framebuffer* CreateFramebuffer(FramebufferInfo info) {
    auto framebuffer = new Framebuffer{};
    framebuffer->recreation_info = info;
    framebuffer::Initialize(framebuffer, info);
    return framebuffer;
}
void DestroyFramebuffer(Framebuffer* framebuffer) {
    framebuffer::Finalize(framebuffer);
    delete framebuffer;
}

VkSurfaceFormatKHR SelectVkSwapchainSurfaceFormat(VkSurfaceKHR vk_surface) {
    uint32_t available_format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(render::context.vk_physical_device, vk_surface, &available_format_count,
                                         nullptr);
    VkSurfaceFormatKHR* available_surface_formats = new VkSurfaceFormatKHR[available_format_count];
    vkGetPhysicalDeviceSurfaceFormatsKHR(render::context.vk_physical_device, vk_surface, &available_format_count,
                                         available_surface_formats);
    VkSurfaceFormatKHR chosen_surface_format = available_surface_formats[0];
    for (int i = 0; i < available_format_count; i++) {
        if (available_surface_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
            available_surface_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            chosen_surface_format = available_surface_formats[i];
            break;
        }
    }
    delete[] available_surface_formats;
    return chosen_surface_format;
}
VkPresentModeKHR SelectVkSwapchainPresentMode(VkSurfaceKHR vk_surface) {
    uint32_t available_present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(render::context.vk_physical_device, vk_surface,
                                              &available_present_mode_count, nullptr);
    VkPresentModeKHR* available_present_modes = new VkPresentModeKHR[available_present_mode_count];
    vkGetPhysicalDeviceSurfacePresentModesKHR(render::context.vk_physical_device, vk_surface,
                                              &available_present_mode_count, available_present_modes);

    VkPresentModeKHR chosen_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (int i = 0; i < available_present_mode_count; i++) {
        if (available_present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            chosen_present_mode = available_present_modes[i];
            break;
        }
    }
    delete[] available_present_modes;
    return chosen_present_mode;
}
struct VkSwapchainImageDetails {
    Extent2D extent;
    uint32_t image_count;
    VkSurfaceTransformFlagBitsKHR pre_transform;
};
VkSwapchainImageDetails QueryVkSwapchainImageDetails(core::Window window, VkSurfaceKHR vk_surface) {
    VkSwapchainImageDetails image_details{};
    VkSurfaceCapabilitiesKHR vk_surface_capabilities{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context.vk_physical_device, vk_surface, &vk_surface_capabilities);

    if (vk_surface_capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        image_details.extent = *(Extent2D*)&vk_surface_capabilities.currentExtent;
    } else {
        image_details.extent = core::window::GetFramebufferExtent(window);

        image_details.extent.x = std::clamp(image_details.extent.x, vk_surface_capabilities.minImageExtent.width,
                                            vk_surface_capabilities.maxImageExtent.width);
        image_details.extent.y = std::clamp(image_details.extent.y, vk_surface_capabilities.minImageExtent.height,
                                            vk_surface_capabilities.maxImageExtent.height);
    }
    image_details.image_count = vk_surface_capabilities.minImageCount + 1;
    if (vk_surface_capabilities.maxImageCount > 0 &&
        image_details.image_count > vk_surface_capabilities.maxImageCount) {
        image_details.image_count = vk_surface_capabilities.maxImageCount;
    }
    image_details.pre_transform = vk_surface_capabilities.currentTransform;
    return image_details;
}
void CreateSwapchainVkImageViews(Swapchain* swapchain) {
    VkImageViewCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format = swapchain->vk_surface_format.format;
    create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.levelCount = 1;
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.layerCount = 1;

    swapchain->vk_image_views.resize(swapchain->vk_images.size());
    for (int i = 0; i < swapchain->vk_image_views.size(); i++) {
        create_info.image = swapchain->vk_images[i];
        vkCreateImageView(render::context.vk_device, &create_info, nullptr, &swapchain->vk_image_views[i]);
    }
}
namespace swapchain {
void Initialize(Swapchain* swapchain) {
    core::window::CreateVkSurface(swapchain->window, &context.vk_instance, &swapchain->vk_surface);
    if (swapchain->vk_surface == VK_NULL_HANDLE) {
        RENDER_LOG_ERROR("SWAPCHAIN CREATION: Failed to Create VkSurfaceKHR!");
    }
    swapchain->vk_surface_format = SelectVkSwapchainSurfaceFormat(swapchain->vk_surface);
    auto present_mode = SelectVkSwapchainPresentMode(swapchain->vk_surface);
    VkSwapchainImageDetails details = QueryVkSwapchainImageDetails(swapchain->window, swapchain->vk_surface);

    swapchain->extent = {details.extent.x, details.extent.y, 1};
    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.pNext = nullptr;
    create_info.flags = 0;

    create_info.surface = swapchain->vk_surface;
    create_info.minImageCount = details.image_count;
    create_info.imageFormat = swapchain->vk_surface_format.format;
    create_info.imageColorSpace = swapchain->vk_surface_format.colorSpace;
    create_info.imageExtent = *(VkExtent2D*)&details.extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (context.universal_queue.vk_family_index != context.present_queue.vk_family_index) {
        uint32_t family_indices[] = {context.universal_queue.vk_family_index, context.present_queue.vk_family_index};
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = family_indices;
    } else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = nullptr;
    }
    create_info.preTransform = details.pre_transform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    auto result = vkCreateSwapchainKHR(context.vk_device, &create_info, nullptr, &swapchain->vk_swapchain);
    if (result != VK_SUCCESS) {
        RENDER_LOG_ERROR("SWAPCHAIN CREATION: Failed to Create VkSwapchainKHR!");
    }
    uint32_t image_count;
    vkGetSwapchainImagesKHR(context.vk_device, swapchain->vk_swapchain, &image_count, nullptr);
    swapchain->vk_images.resize(image_count);
    vkGetSwapchainImagesKHR(context.vk_device, swapchain->vk_swapchain, &image_count, swapchain->vk_images.data());

    CreateSwapchainVkImageViews(swapchain);
}
void Finalize(Swapchain* swapchain) {
    for (VkImageView image_view : swapchain->vk_image_views) {
        vkDestroyImageView(render::context.vk_device, image_view, nullptr);
    }
    vkDestroySwapchainKHR(context.vk_device, swapchain->vk_swapchain, nullptr);
    vkDestroySurfaceKHR(context.vk_instance, swapchain->vk_surface, nullptr);
}

void Recreate(Swapchain* swapchain) {
    RENDER_LOG_INFO("SWAPCHAIN RECREATION TRIGGERED");
    Finalize(swapchain);
    Initialize(swapchain);
};
void AcquireImage(Swapchain* swapchain, uint32_t* image_index) {
    VkResult result = vkAcquireNextImageKHR(context.vk_device, swapchain->vk_swapchain, UINT64_MAX,
                                            VK_NULL_HANDLE /*SEMAPHORE*/, VK_NULL_HANDLE /*FENCE*/, image_index);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        RENDER_LOG_INFO("SWAPCHAIN IMAGE ACQUISITION: Swapchain Out of Date");
        Recreate(swapchain);
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        RENDER_LOG_ERROR("SWAPCHAIN IMAGE ACQUISITION: Failed to Acquire Swapchain Image!");
    }
}
} // namespace swapchain
Swapchain* CreateSwapchain(core::Window window) {
    Swapchain* swapchain = new Swapchain{window};
    swapchain::Initialize(swapchain);
    return swapchain;
}
void DestroySwapchain(Swapchain* swapchain) {
    swapchain::Finalize(swapchain);
    delete swapchain;
}

namespace command {}

uint32_t frame;
void EndFrame() {
    frame = (frame + 1) % MAX_FRAMES_IN_FLIGHT;
    frame_loop_function_queue_mutex.lock();
    for (auto function : frame_loop_function_queue[frame]) {
        function();
    }
    frame_loop_function_queue[frame] = {};
    frame_loop_function_queue_mutex.unlock();
}

std::mutex frame_loop_function_queue_mutex;
std::deque<std::function<void()>> frame_loop_function_queue[MAX_FRAMES_IN_FLIGHT];
void EnqueueFrameLoopFunction(std::function<void()> function) {
    frame_loop_function_queue_mutex.lock();
    frame_loop_function_queue[render::frame].emplace_back(function);
    frame_loop_function_queue_mutex.unlock();
}
} // namespace render