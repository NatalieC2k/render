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

    pool->record_thread = std::thread(command_pool::RecordThreadFunction, pool);
    return pool;
}
void DestroyCommandPool(CommandPool* pool) {
    pool->mutex.lock();
    pool->active = false;
    pool->record_queue.emplace_back([]() {});
    pool->mutex.unlock();
    pool->condition_variable.notify_all();

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
void ResetCommandBuffer(CommandPool* pool, CommandBuffer* command_buffer) {
    pool->completion_mutex.lock();
    command_buffer->completion_flag = false;
    vkResetCommandBuffer(command_buffer->vk_command_buffer, 0);
    pool->completion_mutex.unlock();
}

void RecordThreadFunction(CommandPool* pool) {
    while (pool->active) {
        std::unique_lock<std::mutex> lock(pool->mutex);
        pool->condition_variable.wait(lock, [pool]() { return pool->record_queue.size() > 0; });
        auto function = pool->record_queue.front();
        pool->record_queue.pop_front();
        lock.unlock();

        function();
    }
}
void RecordAsync(CommandPool* pool, CommandBuffer* command_buffer, std::function<void()> function) {
    pool->completion_mutex.lock();
    command_buffer->completion_flag = false;
    pool->completion_mutex.unlock();

    pool->mutex.lock();
    pool->record_queue.emplace_back(function);
    pool->mutex.unlock();

    pool->condition_variable.notify_all();
}
void AwaitRecord(CommandPool* pool, CommandBuffer* command_buffer) {
    std::unique_lock<std::mutex> lock(pool->completion_mutex);
    pool->completion_condition_variable.wait(lock, [command_buffer] { return command_buffer->completion_flag; });
    lock.unlock();
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
void Recreate(Renderpass* renderpass, RenderpassInfo info) {
    Finalize(renderpass);
    Initialize(renderpass, info);
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
    create_info.width = info.extent.x;
    create_info.height = info.extent.y;
    create_info.layers = info.extent.z;

    if (info.swapchain != nullptr) {
        Swapchain* swapchain = (Swapchain*)info.swapchain;

        uint32_t swapchain_attachment_index = info.attachments.size();
        info.attachments.emplace_back(VkImageView{VK_NULL_HANDLE});
        create_info.attachmentCount = info.attachments.size();
        create_info.pAttachments = info.attachments.data();
        for (auto view : swapchain->vk_image_views) {
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
    framebuffer->vk_framebuffer = {};
}

void Recreate(Framebuffer* framebuffer, FramebufferInfo info) {
    Finalize(framebuffer);
    Initialize(framebuffer, info);
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
    render::AwaitIdle();

    Finalize(swapchain);
    Initialize(swapchain);

    for (auto function : swapchain->recreation_functions) {
        function();
    }
};

void AcquireImage(Swapchain* swapchain, uint32_t* image_index, Semaphore semaphore, Fence* fence) {
    swapchain->usage_mutex.lock();
    VkResult result = vkAcquireNextImageKHR(context.vk_device, swapchain->vk_swapchain, UINT64_MAX,
                                            semaphore.vk_semaphore, fence->vk_fence, image_index);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        RENDER_LOG_INFO("SWAPCHAIN IMAGE ACQUISITION: Swapchain Out of Date");
        Recreate(swapchain);
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        RENDER_LOG_ERROR("SWAPCHAIN IMAGE ACQUISITION: Failed to Acquire Swapchain Image!");
    }
    swapchain->usage_mutex.unlock();
}

void BindRecreationFunction(Swapchain* swapchain, std::function<void()> function) {
    swapchain->recreation_functions.emplace_back(function);
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

namespace shader {
VkShaderModule CompileGLSL(size_t buffer_size, char* buffer) { return VK_NULL_HANDLE; }
VkShaderModule CompileSPIRV(size_t buffer_size, char* buffer) {
    VkShaderModuleCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.pNext = nullptr;
    create_info.flags = 0;
    create_info.codeSize = buffer_size;
    create_info.pCode = (uint32_t*)buffer;

    VkShaderModule vk_shader_module;
    VkResult vk_result = vkCreateShaderModule(render::context.vk_device, &create_info, nullptr, &vk_shader_module);
    if (vk_result != VK_SUCCESS) {
        throw std::runtime_error("FAILED TO CREATE SHADER MODULE FROM SPIRV");
    }
    return vk_shader_module;
}
void Initialize(Shader* pointer, ShaderInfo info) {
    size_t name_end = info.filepath.find_last_of(".");
    switch (info.shader_code_format) {
    case SHADER_FORMAT_GLSL: {
        std::string stage_argument("-fshader-stage=");
        switch (info.shader_stage) {
        case SHADER_STAGE_VERTEX: {
            stage_argument += "vertex ";
            break;
        }
        case SHADER_STAGE_FRAGMENT: {
            stage_argument += "fragment ";
            break;
        }
        }

        std::string cmd = "/Users/natalie/VulkanSDK/1.3.268.1/macOS/bin/glslc " + stage_argument + info.filepath +
                          " -o " + info.filepath.substr(0, name_end) + ".spirv";
        system(cmd.c_str());
    }
    case SHADER_FORMAT_SPIRV: {
        size_t size = std::filesystem::file_size(info.filepath);
        std::ifstream file(info.filepath);

        char* buffer = new char[size];
        file.read(buffer, size);
        pointer->vk_shader_module = CompileSPIRV(size, buffer);
        break;
    }
    }
}
void Finalize(Shader* pointer) { vkDestroyShaderModule(context.vk_device, pointer->vk_shader_module, nullptr); }
} // namespace shader
Shader* CreateShader(ShaderInfo info) {
    auto shader = new Shader{};
    shader::Initialize(shader, info);
    return shader;
}
void DestroyShader(Shader* shader) {
    shader::Finalize(shader);
    delete shader;
}

namespace pipeline {
void Initialize(Pipeline* pointer, PipelineInfo info) {
    std::vector<VkPipelineShaderStageCreateInfo> vk_shader_stage_info{};
    VkPipelineShaderStageCreateInfo stage_info{};
    stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_info.pName = "main";
    for (ShaderInfo shader_info : info.shaders) {
        stage_info.stage = (VkShaderStageFlagBits)shader_info.shader_stage;
        stage_info.module = CreateShader(shader_info)->vk_shader_module;
        vk_shader_stage_info.emplace_back(stage_info);
    }

    VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = 2;
    dynamic_state.pDynamicStates = dynamic_states;

    VkPipelineVertexInputStateCreateInfo vertex_info{};
    vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_info.vertexBindingDescriptionCount = (uint32_t)info.vertex_bindings.size();
    vertex_info.pVertexBindingDescriptions = (VkVertexInputBindingDescription*)info.vertex_bindings.data();
    vertex_info.vertexAttributeDescriptionCount = (uint32_t)info.vertex_attributes.size();
    vertex_info.pVertexAttributeDescriptions = (VkVertexInputAttributeDescription*)info.vertex_attributes.data();

    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;

    rasterizer.cullMode = (VkCullModeFlags)info.cull_mode;
    rasterizer.frontFace = (VkFrontFace)info.front_face;

    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState blend_attachment{};
    blend_attachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blend_attachment.blendEnable = VK_FALSE;
    blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineDepthStencilStateCreateInfo depth_stencil{};
    depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.flags = 0;
    depth_stencil.depthTestEnable = false;  // info.depth_test_enabled;
    depth_stencil.depthWriteEnable = false; // info.depth_write_enabled;
    depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;

    depth_stencil.depthBoundsTestEnable = VK_FALSE;
    depth_stencil.minDepthBounds = 0.0f;
    depth_stencil.maxDepthBounds = 1.0f;

    depth_stencil.stencilTestEnable = VK_FALSE;
    depth_stencil.front = {};
    depth_stencil.back = {};

    VkPipelineColorBlendStateCreateInfo blend_state{};
    blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend_state.logicOpEnable = VK_FALSE;
    blend_state.logicOp = VK_LOGIC_OP_COPY;
    blend_state.attachmentCount = 1;
    blend_state.pAttachments = &blend_attachment;
    blend_state.blendConstants[0] = 0.0f;
    blend_state.blendConstants[1] = 0.0f;
    blend_state.blendConstants[2] = 0.0f;
    blend_state.blendConstants[3] = 0.0f;

    /* Pipeline Layout */
    VkPipelineLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    /*layout_info.pushConstantRangeCount = (uint32_t)info.push_constant_ranges.size();
    layout_info.pPushConstantRanges = (VkPushConstantRange*)info.push_constant_ranges.data();
    layout_info.setLayoutCount = (uint32_t)info.descriptor_set_layouts.size();
    layout_info.pSetLayouts = (VkDescriptorSetLayout*)info.descriptor_set_layouts.data();*/

    VkResult vk_result = vkCreatePipelineLayout(context.vk_device, &layout_info, nullptr, &pointer->vk_pipeline_layout);
    if (vk_result != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = (uint32_t)vk_shader_stage_info.size();
    pipeline_info.pStages = vk_shader_stage_info.data();

    pipeline_info.pVertexInputState = &vertex_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pDepthStencilState = &depth_stencil;
    pipeline_info.pColorBlendState = &blend_state;
    pipeline_info.pDynamicState = &dynamic_state;

    pipeline_info.layout = pointer->vk_pipeline_layout;

    pipeline_info.renderPass = info.renderpass->vk_render_pass;
    pipeline_info.subpass = 0;

    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.basePipelineIndex = -1;

    vk_result =
        vkCreateGraphicsPipelines(context.vk_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pointer->vk_pipeline);

    if (vk_result != VK_SUCCESS) {
        throw std::runtime_error("FAILED TO CREATE GRAPHICS PIPELINE");
    }
    for (VkPipelineShaderStageCreateInfo info : vk_shader_stage_info) {
        vkDestroyShaderModule(context.vk_device, info.module, nullptr);
    }
}
void Finalize(Pipeline* pointer) {
    vkDestroyPipeline(context.vk_device, pointer->vk_pipeline, nullptr);
    vkDestroyPipelineLayout(context.vk_device, pointer->vk_pipeline_layout, nullptr);
}
} // namespace pipeline
Pipeline* CreatePipeline(PipelineInfo info) {
    auto pipeline = new Pipeline{};
    pipeline::Initialize(pipeline, info);
    return pipeline;
}
void DestroyPipeline(Pipeline* pointer) {
    pipeline::Finalize(pointer);
    delete pointer;
}

namespace semaphore {
void Initialize(Semaphore* pointer) {
    VkSemaphoreCreateInfo semaphore_create_info{};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_create_info.pNext = nullptr;
    semaphore_create_info.flags = 0;
    vkCreateSemaphore(render::context.vk_device, &semaphore_create_info, nullptr, &pointer->vk_semaphore);
}
void Finalize(Semaphore* pointer) { vkDestroySemaphore(render::context.vk_device, pointer->vk_semaphore, nullptr); }
} // namespace semaphore
Semaphore CreateSemaphore() {
    auto semaphore = Semaphore{};
    semaphore::Initialize(&semaphore);
    return semaphore;
}
void DestroySemaphore(Semaphore semaphore) { semaphore::Finalize(&semaphore); }

namespace fence {
void Initialize(Fence* pointer, FenceInitializationState init_state) {
    VkFenceCreateInfo fence_create_info{};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext = nullptr;
    fence_create_info.flags = init_state;
    vkCreateFence(render::context.vk_device, &fence_create_info, nullptr, &pointer->vk_fence);
}
void Finalize(Fence* pointer) { vkDestroyFence(render::context.vk_device, pointer->vk_fence, nullptr); }

void Await(Fence* fence) {
    std::unique_lock<std::mutex> lock(submission_mutex);
    submission_condition.wait(lock, [fence] { return fence->submission_flag; });
    lock.unlock();
    vkWaitForFences(render::context.vk_device, 1, &fence->vk_fence, VK_TRUE, UINT64_MAX);
}
void Reset(Fence* fence) {
    submission_mutex.lock();
    fence->submission_flag = false;
    vkResetFences(render::context.vk_device, 1, &fence->vk_fence);
    submission_mutex.unlock();
}
} // namespace fence
Fence* CreateFence(fence::FenceInitializationState init_state) {
    auto fence = new Fence{};
    fence::Initialize(fence, init_state);
    return fence;
}
void DestroyFence(Fence* fence) {
    fence::Finalize(fence);
    delete fence;
}

namespace command {
void BeginCommandBuffer(CommandPool* pool, CommandBuffer* command_buffer) {
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext = nullptr;
    begin_info.flags = 0;
    begin_info.pInheritanceInfo = nullptr;
    vkBeginCommandBuffer(command_buffer->vk_command_buffer, &begin_info);
}
void EndCommandBuffer(CommandPool* pool, CommandBuffer* command_buffer) {
    vkEndCommandBuffer(command_buffer->vk_command_buffer);
    pool->completion_mutex.lock();
    command_buffer->completion_flag = true;
    pool->completion_mutex.unlock();
    pool->completion_condition_variable.notify_all();
}

void BindPipeline(CommandBuffer* command_buffer, Pipeline* pipeline) {
    vkCmdBindPipeline(command_buffer->vk_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->vk_pipeline);
}
} // namespace command

bool submission_active = true;

std::thread submission_thread = std::thread(SubmissionThread);
std::mutex submission_queue_mutex{};
std::deque<std::function<void()>> submission_function_queue{};
std::condition_variable submission_queue_condition{};

std::condition_variable submission_empty_condition{};

std::mutex submission_mutex{};
std::condition_variable submission_condition{};

void SubmissionThread() {
    while (submission_active) {
        std::unique_lock lock(submission_queue_mutex);
        submission_queue_condition.wait(lock, []() { return submission_function_queue.size() > 0; });
        std::function<void()> function = submission_function_queue.front();
        lock.unlock();
        function();

        lock.lock();
        submission_function_queue.pop_front();
        lock.unlock();
        submission_empty_condition.notify_one();
    }
}
void SubmitUniversalAsync(SubmitInfo submit_info) {
    submission_queue_mutex.lock();
    submission_function_queue.emplace_back([submit_info]() {
        render::command_pool::AwaitRecord(submit_info.command_pool, submit_info.command_buffer);

        VkSubmitInfo vk_submit_info{};
        vk_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        vk_submit_info.pNext = nullptr;

        vk_submit_info.waitSemaphoreCount = (uint32_t)submit_info.wait_semaphores.size();
        vk_submit_info.pWaitSemaphores = (VkSemaphore*)submit_info.wait_semaphores.data();
        vk_submit_info.pWaitDstStageMask = &submit_info.wait_stage_flags;

        vk_submit_info.signalSemaphoreCount = (uint32_t)submit_info.signal_semaphores.size();
        vk_submit_info.pSignalSemaphores = (VkSemaphore*)submit_info.signal_semaphores.data();

        vk_submit_info.commandBufferCount = 1;
        vk_submit_info.pCommandBuffers = &submit_info.command_buffer->vk_command_buffer;

        vkQueueSubmit(render::context.universal_queue.vk_queue, 1, &vk_submit_info, submit_info.fence->vk_fence);

        if (submit_info.fence != nullptr) {
            submission_mutex.lock();
            submit_info.fence->submission_flag = true;
            submission_mutex.unlock();

            submission_condition.notify_all();
        }
    });
    submission_queue_mutex.unlock();
    submission_queue_condition.notify_one();
}
void SubmitCompute(SubmitInfo submit_info) {}
void SubmitStaging(SubmitInfo submit_info) {}

void SubmitPresentAsync(PresentInfo present_info) {
    submission_queue_mutex.lock();
    submission_function_queue.emplace_back([present_info]() {
        VkPresentInfoKHR vk_present_info{};
        vk_present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        vk_present_info.pNext = nullptr;

        vk_present_info.waitSemaphoreCount = (uint32_t)present_info.wait_semaphores.size();
        vk_present_info.pWaitSemaphores = (VkSemaphore*)present_info.wait_semaphores.data();

        vk_present_info.swapchainCount = (uint32_t)present_info.swapchains.size();
        vk_present_info.pSwapchains = &present_info.swapchains[0]->vk_swapchain;
        vk_present_info.pImageIndices = present_info.image_indices.data();

        present_info.swapchains[0]->usage_mutex.lock();
        vkQueuePresentKHR(context.universal_queue.vk_queue, &vk_present_info);
        present_info.swapchains[0]->usage_mutex.unlock();

        if (present_info.fence != nullptr) {
            submission_mutex.lock();
            present_info.fence->submission_flag = true;
            submission_mutex.unlock();

            submission_condition.notify_all();
        }
    });
    submission_queue_mutex.unlock();
    submission_queue_condition.notify_one();
}

void AwaitIdle() {
    std::unique_lock lock(submission_queue_mutex);
    submission_empty_condition.wait(lock, []() { return submission_function_queue.size() == 0; });
    vkDeviceWaitIdle(render::context.vk_device);
    lock.unlock();
}

void InitializeSubmission() {}
void FinalizeSubmission() {
    submission_queue_mutex.lock();
    submission_active = false;
    submission_function_queue.emplace_back([]() {});
    submission_queue_mutex.unlock();
    submission_queue_condition.notify_all();
    submission_thread.join();
}
} // namespace render