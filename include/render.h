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

#include "pipeline.h"

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
    bool completion_flag = false;
    VkCommandBuffer vk_command_buffer = VK_NULL_HANDLE;
};
struct CommandPool {
    VkCommandPool vk_command_pool = VK_NULL_HANDLE;
    std::deque<CommandBuffer*> free_command_buffer_queue{};

    bool active = true;
    std::thread record_thread{};
    std::mutex mutex{};
    std::condition_variable condition_variable{};

    std::mutex completion_mutex{};
    std::condition_variable completion_condition_variable{};
    std::deque<std::function<void()>> record_queue{};
};
CommandPool* CreateCommandPool();
void DestroyCommandPool(CommandPool* pool);
namespace command_pool {
CommandBuffer* BorrowCommandBuffer(CommandPool* pool);
void ReturnCommandBuffer(CommandPool* pool, CommandBuffer* command_buffer);
void ResetCommandBuffer(CommandPool* pool, CommandBuffer* command_buffer);

void RecordAsync(CommandPool* pool, CommandBuffer* command_buffer, std::function<void()> function);
void AwaitRecord(CommandPool* pool, CommandBuffer* command_buffer);

void RecordThreadFunction(CommandPool* pool);
} // namespace command_pool

struct Semaphore {
    VkSemaphore vk_semaphore;
};
namespace semaphore {
void Initialize(Semaphore* pointer);
void Finalize(Semaphore* pointer);
} // namespace semaphore
Semaphore CreateSemaphore();
void DestroySemaphore(Semaphore semaphore);

struct Fence {
    bool submission_flag = true;
    VkFence vk_fence = VK_NULL_HANDLE;
};
namespace fence {
enum FenceInitializationState {
    INITIALIZE_UNSIGNALED = 0,
    INITIALIZE_SIGNALED = 1,
};
void Initialize(Fence* pointer, FenceInitializationState init_state);
void Finalize(Fence* pointer);

void Await(Fence* fence);
void Reset(Fence* fence);
} // namespace fence
Fence* CreateFence(fence::FenceInitializationState init_state);
void DestroyFence(Fence* fence);

struct Swapchain {
    core::Window window;

    std::mutex usage_mutex{};
    Extent3D extent;
    VkSurfaceKHR vk_surface;
    VkSurfaceFormatKHR vk_surface_format;
    VkSwapchainKHR vk_swapchain;
    std::vector<VkImage> vk_images;
    std::vector<VkImageView> vk_image_views;

    std::vector<std::function<void()>> recreation_functions{};
};
namespace swapchain {
void Initialize(Swapchain* swapchain);
void Finalize(Swapchain* swapchain);

void Recreate(Swapchain* swapchain);
void AcquireImage(Swapchain* swapchain, uint32_t* image_index, Semaphore semaphore, Fence* fence);

void BindRecreationFunction(Swapchain* swapchain, std::function<void()> function);
} // namespace swapchain
Swapchain* CreateSwapchain(core::Window window);
void DestroySwapchain(Swapchain* swapchain);

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
void Initialize(Renderpass* renderpass, RenderpassInfo info);
void Finalize(Renderpass* renderpass);

void Recreate(Renderpass* renderpass, RenderpassInfo info);
} // namespace renderpass
Renderpass* CreateRenderpass(RenderpassInfo info);
void DestroyRenderpass(Renderpass* renderpass);

struct FramebufferInfo {
    Renderpass* renderpass = nullptr;
    std::vector<VkImageView> attachments{};
    Extent3D extent{};
    Swapchain* swapchain = nullptr;
};
struct Framebuffer {
    std::optional<FramebufferInfo> recreation_info{};
    std::vector<VkFramebuffer> vk_framebuffer{};
};
namespace framebuffer {
void Initialize(Framebuffer* framebuffer, FramebufferInfo info);
void Finalize(Framebuffer* framebuffer);

void Recreate(Framebuffer* framebuffer, FramebufferInfo info);
} // namespace framebuffer
Framebuffer* CreateFramebuffer(FramebufferInfo info);
void DestroyFramebuffer(Framebuffer* framebuffer);

namespace command {
void BeginCommandBuffer(CommandPool* pool, CommandBuffer* command_buffer);
void EndCommandBuffer(CommandPool* pool, CommandBuffer* command_buffer);

void BindPipeline(CommandBuffer* command_buffer, Pipeline* pipeline);
} // namespace command

extern std::mutex submission_queue_mutex;
extern std::deque<std::function<void()>> submission_function_queue;

extern std::mutex submission_mutex;
extern std::condition_variable submission_condition;

struct SubmitInfo {
    std::vector<Semaphore> wait_semaphores;
    VkPipelineStageFlags wait_stage_flags;
    std::vector<Semaphore> signal_semaphores;
    Fence* fence;
    CommandPool* command_pool;
    CommandBuffer* command_buffer;
};
struct PresentInfo {
    std::vector<Semaphore> wait_semaphores;
    std::vector<Swapchain*> swapchains;
    std::vector<uint32_t> image_indices;
    Fence* fence;
};

void SubmissionThread();
void SubmitUniversalAsync(SubmitInfo submit_info);
void SubmitCompute(SubmitInfo submit_info);
void SubmitStaging(SubmitInfo submit_info);

void SubmitPresentAsync(PresentInfo present_info);

void AwaitIdle();

void InitializeSubmission();
void FinalizeSubmission();
} // namespace render