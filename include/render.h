#pragma once

#include "window.h"

#include "vk_mem_alloc.h"
#include "vulkan/vulkan.h"

#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

#define RENDER_LOGGER_DECLARATION                                                                                      \
    namespace render {                                                                                                 \
    extern std::shared_ptr<spdlog::logger> logger;                                                                     \
    };
#define RENDER_LOGGER_DEFINITION                                                                                       \
    namespace render {                                                                                                 \
    std::shared_ptr<spdlog::logger> logger;                                                                            \
    };

#define RENDER_LOG_INITIALIZE render::logger = spdlog::stdout_logger_mt("RENDER");
#define RENDER_LOG_FINALIZE

#define RENDER_LOG_INFO(...) render::logger->info(__VA_ARGS__)
#define RENDER_LOG_ERROR(...) render::logger->error(__VA_ARGS__)

RENDER_LOGGER_DECLARATION

#ifndef MAX_FRAMES_IN_FLIGHT
#define MAX_FRAMES_IN_FLIGHT 2
#endif

namespace render {
struct VulkanQueueIndices {
    uint32_t universal_queue_count;
    uint32_t universal_family_index;
};
struct DeviceQueue {
    uint32_t vk_family_index;
    VkQueue vk_queue;
};

struct ContextInfo {
    core::Window window;
    bool enable_validation_layers;
    const char* applcation_name;
    const char* engine_name;
};
struct Context {
    VkInstance vk_instance;
    VkDebugUtilsMessengerEXT vk_debug_utils_messenger;

    VkPhysicalDevice vk_physical_device;
    VkDevice vk_device;
    DeviceQueue universal_queue;
    DeviceQueue compute_queue;
    DeviceQueue staging_queue;
    DeviceQueue present_queue;

    VmaAllocator vma_allocator;
};
extern render::Context context;
Context CreateContext(ContextInfo info);
void DestroyContext(Context context);

namespace command {}
} // namespace render