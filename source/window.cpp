#include "window.h"

namespace core {
void Initialize() { SDL_Init(SDL_INIT_EVERYTHING); }
void Finalize() { SDL_Quit(); }

Window CreateWindow(WindowInfo info) {
    return Window{SDL_CreateWindow(info.name, info.offset.x, info.offset.y, info.extent.x, info.extent.y, info.flags)};
}
void DestroyWindow(Window window) { SDL_DestroyWindow(window.sdl_window); }

namespace window {
Extent2D GetFramebufferExtent(Window window) {
    int width;
    int height;
    SDL_GetWindowSize(window.sdl_window, &width, &height);
    return Extent2D{
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
    };
}

void GetRequiredVkInstanceExtensions(Window window, unsigned int* p_count, const char** p_names) {
    SDL_Vulkan_GetInstanceExtensions(window.sdl_window, p_count, p_names);
}
void CreateVkSurface(Window window, void* vk_instance, void* vk_surface) {
    SDL_Vulkan_CreateSurface(window.sdl_window, *(VkInstance*)vk_instance, (VkSurfaceKHR*)vk_surface);
}
} // namespace window
} // namespace core