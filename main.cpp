#include "include/render.h"
#include "include/window.h"

core::Window window{};
render::Swapchain* swapchain{};

render::Renderpass* renderpass;
render::Framebuffer* framebuffer;

render::Pipeline* pipeline;

render::CommandPool* command_pool;

render::Semaphore image_acquisition_semaphore;
render::Semaphore render_completion_semaphore;
render::Fence* fence;

void Initialize() {
    core::Initialize();

    core::WindowInfo window_info{};
    window_info.extent = {1000, 700};
    window_info.offset = {0, 0};
    window_info.flags = (core::WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    window = core::CreateWindow(window_info);

    RENDER_LOG_INITIALIZE
    render::ContextInfo context_info{};
    context_info.window = window;
    context_info.enable_validation_layers = true;
    render::context = render::CreateContext(context_info);

    swapchain = render::CreateSwapchain(window);
    render::SwapchainAttachment swapchain_attachment = {
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        render::LoadOp::CLEAR,
        render::StoreOp::STORE,
        swapchain,
    };
    renderpass = render::CreateRenderpass({swapchain->extent,
                                           {},
                                           {{
                                               {{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}},
                                               {},
                                               nullptr,
                                           }},
                                           &swapchain_attachment});
    auto framebuffer_info = render::FramebufferInfo{};
    framebuffer_info.renderpass = renderpass;
    framebuffer_info.swapchain = swapchain;
    framebuffer_info.extent = swapchain->extent;
    framebuffer = render::CreateFramebuffer(framebuffer_info);

    render::PipelineInfo pipeline_info{};
    pipeline_info.renderpass = renderpass;
    pipeline_info.shaders = {
        {render::SHADER_STAGE_VERTEX, render::SHADER_FORMAT_SPIRV,
         "/Users/natalie/Desktop/engine_rewrite/build/vert.spirv"},
        {render::SHADER_STAGE_FRAGMENT, render::SHADER_FORMAT_SPIRV,
         "/Users/natalie/Desktop/engine_rewrite/build/frag.spirv"},
    };
    pipeline_info.front_face = render::FRONT_FACE_CW;
    pipeline_info.cull_mode = render::NGFX_CULL_MODE_NONE;
    pipeline_info.depth_test_enabled = false;
    pipeline_info.depth_write_enabled = false;
    pipeline = render::CreatePipeline(pipeline_info);

    command_pool = render::CreateCommandPool();

    image_acquisition_semaphore = render::CreateSemaphore();
    render_completion_semaphore = render::CreateSemaphore();
    fence = render::CreateFence(render::fence::INITIALIZE_SIGNALED);
}
void Finalize() {
    render::DestroyFence(fence);
    render::DestroyCommandPool(command_pool);

    render::DestroyFramebuffer(framebuffer);
    render::DestroyRenderpass(renderpass);

    render::DestroySwapchain(swapchain);
    render::DestroyContext(render::context);
    RENDER_LOG_FINALIZE

    core::DestroyWindow(window);

    core::Finalize();
}

int main(int argc, char** argv) {
    Initialize();

    auto command_buffer = render::command_pool::BorrowCommandBuffer(command_pool);
    bool running = true;
    while (running) {
        render::fence::Await(fence);
        render::fence::Reset(fence);

        render::command_pool::ResetCommandBuffer(command_pool, command_buffer);

        SDL_Event e{};
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_WINDOWEVENT) {
                if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    render::swapchain::Recreate(swapchain);
                }
            }
            if (e.type == SDL_QUIT) {
                running = false;
            }
        }
        uint32_t image_index = 0;
        render::swapchain::AcquireImage(swapchain, &image_index, image_acquisition_semaphore.vk_semaphore);
        render::command_pool::RecordAsync(command_pool, [command_buffer, image_index]() {
            render::command::BeginCommandBuffer(command_pool, command_buffer);
            VkRenderPassBeginInfo begin_info{};
            begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            begin_info.pNext = nullptr;

            begin_info.renderPass = renderpass->vk_render_pass;
            begin_info.framebuffer = framebuffer->vk_framebuffer[image_index];
            begin_info.renderArea = {
                0,
                0,
                swapchain->extent.x,
                swapchain->extent.y,
            };

            VkClearValue clear_value[1] = {};
            clear_value[0] = {1.0f, 0.0f};
            begin_info.clearValueCount = 1;
            begin_info.pClearValues = clear_value;

            vkCmdBeginRenderPass(command_buffer->vk_command_buffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);

            render::command::BindPipeline(command_buffer, pipeline);
            VkViewport viewport{};
            viewport.width = swapchain->extent.x;
            viewport.height = swapchain->extent.y;
            viewport.x = 0;
            viewport.y = 0;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(command_buffer->vk_command_buffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent = {swapchain->extent.x, swapchain->extent.y};
            vkCmdSetScissor(command_buffer->vk_command_buffer, 0, 1, &scissor);
            vkCmdDraw(command_buffer->vk_command_buffer, 3, 1, 0, 0);

            vkCmdEndRenderPass(command_buffer->vk_command_buffer);
            render::command::EndCommandBuffer(command_pool, command_buffer);
        });

        auto submit_info = render::SubmitInfo{};
        submit_info.wait_semaphores = {image_acquisition_semaphore};
        submit_info.wait_stage_flags = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submit_info.signal_semaphores = {render_completion_semaphore};
        submit_info.fence = fence;
        submit_info.command_pool = command_pool;
        submit_info.command_buffer = command_buffer;
        render::SubmitUniversalAsync(submit_info);

        auto present_info = render::PresentInfo{{render_completion_semaphore}, {swapchain}, {image_index}};
        render::SubmitPresentAsync(present_info);
    }
    render::fence::Await(fence);
    render::fence::Reset(fence);

    render::command_pool::ReturnCommandBuffer(command_pool, command_buffer);
    Finalize();
}
/*render::Swapchain* swapchain = render::CreateSwapchain(window);

render::RenderpassInfo info{};
info.attachments = {
    { render::INITIAL_LAYOUT, render::FINAL_LAYOUT, render::COLOR_FORMAT,
render::LOAD_OP_LOAD, render::STORE_OP_STORE },
}
info.subpasses = {
    Subpass {
        { 0, render::SHADER_READ_LAYOUT },
        {},
        nullptr,
    }
}
render::Renderpass  renderpass  = render::CreateRenderpass(info);
render::Framebuffer framebuffer = render::CreateFramebuffer({extent, { swapchain
}});

render::Shader* shader_vertex   = asset::GetShader(render::SHADER_STAGE_VERTEX,
"vert.glsl"); render::Shader* shader_fragment =
asset::GetShader(render::SHADER_STAGE_FRAGMENT, "frag.glsl");

render::PipelineInfo info{};
info.push_constant_ranges = { render::Camera::PUSH_CONSTANT_RANGE(0) };
info.descriptor_set_layouts = { set_layout, };
info.vertex_attributes = {
    MVS_ATTRIBUTE_POSITION(Vertex, 0, 0),
    MVS_ATTRIBUTE_TEXTURE_COORDINATE_2D(Vertex, 1, 0),
};
info.vertex_bindings = {
    MVS_BINDING(Vertex, 0),
};
info.renderpass = renderpass;
info.shaders = { shader_vertex, shader_fragment };
info.front_face = render::FRONT_FACE_CLOCKWISE;
info.cull_mode  = render::CULL_MODE_BACK_FACE;
info.depth_test_enabled  = true;
info.depth_write_enabled = true;
render::Pipeline* pipeline = render::CompilePipelineAsync(pipeline_info);

render::Pipeline* pipeline_asset =
asset::GetPipeline("asset/pipeline.pipeline");
// Both Loading Pipeline as an Asset and Compile Pipeline Async
// call compile pipeline an inline function which compiles a pipeline which
notifies a cond var
// so AwaitPipelineComp will work with either function
render::Mesh<Vertex>* mesh = asset::GetMesh<Vertex>();
render::Image* image   = asset::GetImage("backpack/diffuse.jpg");

render::AwaitPipelineCompilation(pipeline);


FIF_ARRAY(render::Fence*, fence);
FIF_ARRAY(render::Semaphore, render_finished_semaphore);
for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
    fence[i] = render::CreateFence();
    render_finished_semaphore[i] = render::CreateSemaphore();
}

auto command_buffer = command_manager.RecordAsync([](CommandBuffer*
command_buffer){ render::BeginRenderpass(command_buffer, renderpass,
framebuffer); render::BindPipeline(command_buffer, pipeline);

    render::BindVertexBuffer(command_buffer, render::gpu_buffer, 0);
    render::BindIndexBuffer(command_buffer, render::gpu_buffer, 0);

    render::PushConstant(command_buffer, pipeline, sizeof(glm::mat4), 0,
(void*)&view_projection); render::BindDescriptorSet(command_buffer,
descriptor_set, 0);

    render::Viewport viewport;
    viewport.width  = swapchain->extent.width;
    viewport.height = swapchain->extent.height;
    viewport.x = 0;
    viewport.y = 0;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    render::SetViewport(command_buffer, 0, 1, &viewport);

    render::Scissor scissor;
    scissor.offset = {0, 0};
    scissor.extent = swapchain->extent_;
    render::SetScissor(command_buffer, 0, 1, &scissor);

    render::DrawMesh<Vertex>(mesh);

    render::EndRenderpass(command_buffer);
});

render::SubmitInfo submit_info{};
submit_info.fence             = fence[frame];
submit_info.wait_semaphores   = { swapchain_semaphore[frame] };
submit_info.signal_semaphores = { render_finished_semaphore[frame] };
submit_info.wait_stage_flags  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
render::submit_manager.SubmitAsync(submit_info, command_buffer);

render::PresentInfo present_info{};
present_info.wait_semaphores = { render_finished_semaphore[frame] };
present_info.swapchains      = { swapchain };
present_info.image_indices   = { image_index };
render::submit_manager.PresentAsync(present_info);
}*/
