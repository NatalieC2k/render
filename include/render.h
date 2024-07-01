#pragma once

#include <algorithm>
#include <cstdint>
#include <deque>
#include <functional>
#include <limits>
#include <optional>

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

#define RENDER_LOG_INITIALIZE                                                                                          \
    render::logger = spdlog::stdout_logger_mt("render");                                                               \
    render::logger->info("RENDER LOG INITIALIZED!");
#define RENDER_LOG_FINALIZE render::logger->info("RENDER LOG FINALIZED!");

#define RENDER_LOG_INFO(...) render::logger->info(__VA_ARGS__)
#define RENDER_LOG_ERROR(...) render::logger->error(__VA_ARGS__)

RENDER_LOGGER_DECLARATION

#ifndef MAX_FRAMES_IN_FLIGHT
#define MAX_FRAMES_IN_FLIGHT 2
#endif

#ifndef RENDER_FIF_ARRAY
#define RENDER_FIF_ARRAY(type, name) type name[MAX_FRAMES_IN_FLIGHT]
#endif
#ifndef RENDER_FIF_CURRENT
#define RENDER_FIF_CURRENT(fif_array) fif_array[render::frame];
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
    std::optional<core::Window> window;
    bool enable_validation_layers;
    const char* applcation_name;
    const char* engine_name;

    void* p_api_context_info;
};
struct VulkanContextInfo {
    // vk_physical_device, requested extension names
    uint32_t (*fp_rate_vk_physical_device)(VkPhysicalDevice, std::vector<const char*>);
    VulkanQueueIndices (*fp_query_vk_physical_device_queue_support)(VkPhysicalDevice);
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

struct CommandBuffer {
    bool complete = false;
    VkCommandBuffer vk_command_buffer = VK_NULL_HANDLE;
};
struct CommandPool {
    VkCommandPool vk_command_pool = VK_NULL_HANDLE;
    std::deque<CommandBuffer*> free_command_buffer_queue{};

    bool active;
    std::thread record_thread{};
    std::mutex mutex{};
    std::condition_variable condition_variable{};
    std::deque<std::function<void()>> record_queue{};
};
CommandPool* CreateCommandPool();
void DestroyCommandPool(CommandPool* pool);
namespace command_pool {
CommandBuffer* BorrowCommandBuffer(CommandPool* pool);
void ReturnCommandBuffer(CommandPool* pool, CommandBuffer* command_buffer);

void RecordAsync(CommandPool* pool, std::function<void()> function);

void RecordThreadFunction(CommandPool* pool);
} // namespace command_pool

enum class LoadOp {
    LOAD = 0,
    CLEAR = 1,
    DONT_CARE = 2,
};
enum class StoreOp {
    DONT_CARE,
    STORE,
};
struct Attachment {
    VkImageLayout initial_layout;
    VkImageLayout final_layout;
    VkFormat format;
    // depth_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
    LoadOp load_op;
    StoreOp store_op;

    // depth_attachment_description.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    // depth_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
};
struct SwapchainAttachment {
    VkImageLayout initial_layout;
    VkImageLayout final_layout;

    LoadOp load_op;
    StoreOp store_op;

    void* swapchain = nullptr;
};
struct AttachmentReference {
    uint32_t index;
    VkImageLayout layout;
};
struct Subpass {
    std::vector<AttachmentReference> input_attachments;
    std::vector<AttachmentReference> color_attachments;
    AttachmentReference* depth_stencil_attachment = nullptr;
};
struct RenderpassInfo {
    Extent3D extent{};
    std::vector<Attachment> attachments{};
    std::vector<Subpass> subpasses{};
    SwapchainAttachment* swapchain_attachment = nullptr;
};
struct Renderpass {
    std::optional<RenderpassInfo> recreation_info;
    VkRenderPass vk_render_pass;
};
namespace renderpass {
typedef uint32_t InstanceIndex;
}
Renderpass* CreateRenderpass(RenderpassInfo info);
void DestroyRenderpass(Renderpass* renderpass);

struct FramebufferInfo {
    Renderpass* renderpass = nullptr;
    std::vector<VkImageView> attachments{};
    void* swapchain_attachment = nullptr;
};
struct Framebuffer {
    std::optional<FramebufferInfo> recreation_info{};
    std::vector<VkFramebuffer> vk_framebuffer{};
};
Framebuffer* CreateFramebuffer(FramebufferInfo info);
void DestroyFramebuffer(Framebuffer* framebuffer);

struct Semaphore {};
struct Fence {};

struct Swapchain {
    core::Window window;

    Extent3D extent;
    VkSurfaceKHR vk_surface;
    VkSurfaceFormatKHR vk_surface_format;
    VkSwapchainKHR vk_swapchain;
    std::vector<VkImage> vk_images;
    std::vector<VkImageView> vk_image_views;

    std::vector<Renderpass*> tied_renderpasses;
    std::vector<Framebuffer*> tied_framebuffers;
};
namespace swapchain {
void Initialize(Swapchain* swapchain);
void Finalize(Swapchain* swapchain);

void Recreate(Swapchain* swapchain);
void AcquireImage(Swapchain* swapchain, uint32_t* image_index);
} // namespace swapchain
Swapchain* CreateSwapchain(core::Window window);
void DestroySwapchain(Swapchain* swapchain);

namespace command {}

struct SubmitInfo {};
void SubmitUniversal(SubmitInfo submit_info);
void SubmitCompute(SubmitInfo submit_info);
void SubmitStaging(SubmitInfo submit_info);

extern uint32_t frame;
void EndFrame();

extern std::mutex frame_loop_function_queue_mutex;
extern std::deque<std::function<void()>> frame_loop_function_queue[MAX_FRAMES_IN_FLIGHT];
void EnqueueFrameLoopFunction(std::function<void()> function);
} // namespace render