#pragma once
#include <fstream>
#include <stdio.h>

#include <string>

#include <deque>
#include <functional>
#include <mutex>
#include <thread>

#include "glm.hpp"

#include "thread_pool.h"

#include "render/context.h"
#include "render/descriptor.h"
#include "render/render_buffer.h"

namespace render {
enum ShaderStage {
    SHADER_STAGE_VERTEX = 0x00000001,
    SHADER_STAGE_FRAGMENT = 0x00000010,
};
enum ShaderFormat {
    SHADER_FORMAT_GLSL,
    SHADER_FORMAT_SPIRV,
};
struct ShaderInfo {
    ShaderStage shader_stage;
    ShaderFormat shader_code_format;
    std::string filepath;
    size_t buffer_size;
    char* buffer;
};
struct Shader {
    ShaderStage shader_stage;
    VkShaderModule vk_shader_module;
};
namespace shader {
CompileGLSL(size_t buffer_size, char* buffer);
CompileSPIRV(size_t buffer_size, char* buffer);
} // namespace shader

struct VertexBinding {
    uint32_t binding;
    uint32_t stride;
    VkVertexInputRate input_rate = VK_VERTEX_INPUT_RATE_VERTEX;
};
enum class AttributeFormat {
    Float,
    Float1,
    Float2,
    Float3,
    Float4,

    Int,
    Int1,
    Int2,
    Int3,
    Int4,

    Uint,
    Uint1,
    Uint2,
    Uint3,
    Uint4,
};
struct VertexAttribute {
    uint32_t location;
    uint32_t binding;
    VkFormat format;
    uint32_t offset;
};
enum NGFX_FrontFace {
    NGFX_FRONT_FACE_CCW = 0,
    FRONT_FACE_CW = 1,
};
enum NGFX_CullMode {
    NGFX_CULL_MODE_NONE = 0,
    NGFX_CULL_MODE_FRONT_FACE = 1,
    CULL_MODE_BACK_FACE = 2,
    NGFX_CULL_MODE_FRONT_AND_BACK_FACE = 3,
};
struct PushConstantRange {
    ShaderStage stageFlags;
    uint32_t offset;
    uint32_t size;
};
struct PipelineInfo {
    std::vector<PushConstantRange> push_constant_ranges;
    std::vector<DescriptorSetLayout> descriptor_set_layouts;

    std::vector<VertexBinding> vertex_bindings;
    std::vector<VertexAttribute> vertex_attributes;

    RenderBuffer* render_buffer;
    std::vector<ShaderInfo> shaders;

    NGFX_FrontFace front_face = FRONT_FACE_CW;
    NGFX_CullMode cull_mode = NGFX_CULL_MODE_NONE;

    bool depth_test_enabled = false;
    bool depth_write_enabled = false;
};
struct Pipeline {
    VkPipelineLayout vk_pipeline_layout;
    VkPipeline vk_pipeline;
};
namespace pipeline {
void Initialize(Pipeline* pointer, PipelineInfo info);
void Terminate(Pipeline* pointer);
} // namespace pipeline
Pipeline* CreatePipeline();
void DestroyPipeline(Pipeline* pointer);

} // namespace render
