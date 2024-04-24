#include "window.h"

namespace core {
void Initialize() {
    SDL_Init(SDL_INIT_EVERYTHING);
}
void Finalize() {
    SDL_Quit();
}

Window CreateWindow(WindowInfo info) {
    return {SDL_CreateWindow(info.name, info.offset.x, info.offset.y, info.extent.x, info.extent.y, info.flags)};
}
void DestroyWindow(Window window) {
    SDL_DestroyWindow(window.sdl_window);
}

void GetInstanceExtensions(Window window, unsigned int* p_count, const char** p_names) {
    SDL_Vulkan_GetInstanceExtensions(window.sdl_window, p_count, p_names);
}
} // namespace core