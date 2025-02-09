// SPDX-License-Identifier: GPL-3.0-only

#include <d3d9.h>
#include <windows.h>
#include <cstdint>
#include <cmath>
#include "shader_transparent_generic.hpp"
#include "../../chimera.hpp"
#include "../../signature/hook.hpp"
#include "../../signature/signature.hpp"
#include "../../event/map_load.hpp"

#include <impl/rasterizer/rasterizer_shader_transparent_generic.h>

namespace Chimera {
    extern "C" {
        const void *switch_jmp_ret;
        void *switch_jmp = nullptr;
        void *switch_default = nullptr;
        void draw_shader_transparent_generic_hook() noexcept;
    }

    void set_up_shader_transparent_generic_impl() noexcept {
        static Hook hook;
        switch_jmp = get_chimera().get_signature("rasterizer_shader_switch_entry").data();
        switch_default = get_chimera().get_signature("rasterider_shader_switch_default_case").data();
        // Why do I have to do this?
        const short mod[] = {0x90,0x90,0x90,0x90,0x90,0x90};
        write_code_s(get_chimera().get_signature("rasterizer_shader_switch_cmp").data(), mod);

        // Enable ringworld shader_transparent_generic support.
        write_function_override(get_chimera().get_signature("rasterizer_shader_switch_cmp").data(), hook, reinterpret_cast<const void *>(draw_shader_transparent_generic_hook), &switch_jmp_ret);
        add_map_load_event(rasterizer_shader_transparent_generic_update_instances_for_current_map);
    }
}
