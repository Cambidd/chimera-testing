// SPDX-License-Identifier: GPL-3.0-only

#include "xbox_channel_order.hpp"
#include "../chimera.hpp"
#include "../signature/hook.hpp"
#include "../signature/signature.hpp"

namespace Chimera {
    extern "C" void set_psh_const_for_xbox_order_asm() noexcept;
    static float *c_self_illumination_color_w = nullptr;

    extern "C" void set_psh_const_for_xbox_order(std::byte *shader) noexcept {
        auto *model_flags = reinterpret_cast<std::uint16_t *>(shader + 0x28);
        if((*model_flags >> 6) & 1) {
            // gearbox defaults this to 1.0f, which is opposite of MCC. The shader code should be set up accordingly.
            *c_self_illumination_color_w = 0.0f;
        }
    }

    void set_up_xbox_channel_order_support() noexcept {
        auto set_shader_const_ptr = get_chimera().get_signature("use_xbox_channel_order_sig").data();
        
        // The channel order is determined by the alpha channel of this shader constant.
        c_self_illumination_color_w = reinterpret_cast<float *>(*reinterpret_cast<std::byte **>(set_shader_const_ptr + 2));

        static Hook hook;
        write_jmp_call(set_shader_const_ptr + 10, hook, reinterpret_cast<const void*>(set_psh_const_for_xbox_order_asm), nullptr);
    }
}
