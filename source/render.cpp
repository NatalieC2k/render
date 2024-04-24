#include "render.h"

RENDER_LOGGER_DEFINITION
namespace render {
namespace vkutil {
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator,
                                      VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
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
    vk_physical_device = VK_NULL_HANDLE;
    return queue_indices;
}

render::Context context{};
Context CreateContext(ContextInfo info) {
    Context context{};
    unsigned int window_extension_count = 0;
    core::GetInstanceExtensions(info.window, &window_extension_count, nullptr);
    std::vector<const char*> extension_names(window_extension_count);
    core::GetInstanceExtensions(info.window, &window_extension_count, extension_names.data());

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
        RENDER_LOG_ERROR("The Following VkInstance Extensions are Unsupported: {}", unsupported_layer_string);
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
    device_extension_names.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

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
            break;
        }
    }
    if (context.vk_device == VK_NULL_HANDLE) {
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
    result = vmaCreateAllocator(&allocator_create_info, &context.vma_allocator);
    if (result != VK_SUCCESS) {
        RENDER_LOG_ERROR("CONTEXT CREATION: Failed to Create VmaAllocator!");
    }
    return context;
}
void DestroyContext(Context context) {}
} // namespace render