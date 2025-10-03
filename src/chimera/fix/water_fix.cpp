// SPDX-License-Identifier: GPL-3.0-only

#include <d3d9.h>

#include "water_fix.hpp"
#include "../chimera.hpp"
#include "../signature/hook.hpp"
#include "../signature/signature.hpp"
#include "../event/frame.hpp"
#include "../event/game_loop.hpp"
#include "../halo_data/shaders/shader_blob.hpp"
#include "../halo_data/shader_effects.hpp"

extern "C" {
    void water_opacity_psh_const_asm() noexcept;
    void water_reflection_psh_const_asm() noexcept;
    void water_bumpmap_memes_asm() noexcept;
    void water_bumpmap_memes_2_asm() noexcept;
    void adjust_water_render_target_mip_asm() noexcept;
    void set_water_bumpmap_sampler_states_asm() noexcept;
    const void *original_opacity_set_vsh = nullptr;
    const void *original_reflection_set_vsh = nullptr;
    std::uint32_t render_target_create_loop_counter = 0;
}

namespace Chimera {

    static IDirect3DDevice9 **global_d3d9_device = nullptr;
    static D3DCAPS9 *d3d9_device_caps = nullptr;
    static std::byte *global_render_targets = nullptr;
    static std::byte *bump_vertices = nullptr;

    extern "C" void set_water_shader_const(std::byte *shader, std::uint32_t start_register) noexcept {
        // Doing fog in the pixel shader so disable this.
        IDirect3DDevice9_SetRenderState(*global_d3d9_device, D3DRS_FOGENABLE, FALSE);

        // The cea water pixel shaders expect an additional constant to handle the atmospheric fog flag.
        auto *flags = reinterpret_cast<std::uint8_t *>(shader + 0x28);
        float psh_constants[4] = {0};
        psh_constants[0] = (*flags >> 2) & 1 ? 1.0f : 0.0f;
        IDirect3DDevice9_SetPixelShaderConstantF(*global_d3d9_device, start_register, psh_constants, 1);
    }

    void initialize_water_hooks() noexcept {
        // This depends on ps_2_0 shaders so check caps for memes.
        if(d3d9_device_caps->PixelShaderVersion < 0xffff0200) {
            return;
        }

        static Hook opacity_set_psh_const;
        static Hook reflection_set_psh_const;

        if(get_chimera().feature_present("client_custom_edition")) {
            auto *opacity_psh = get_chimera().get_signature("water_opacity_set_psh_consts_custom_sig").data();
            auto *reflection_psh = get_chimera().get_signature("water_reflection_set_psh_consts_custom_sig").data();
            write_jmp_call(opacity_psh, opacity_set_psh_const, nullptr, reinterpret_cast<const void *>(water_opacity_psh_const_asm));
            write_jmp_call(reflection_psh, reflection_set_psh_const, nullptr, reinterpret_cast<const void *>(water_reflection_psh_const_asm));
        }
        else if(get_chimera().feature_present("client_retail_demo")) {
            auto *opacity_psh = get_chimera().get_signature("water_opacity_set_psh_consts_retail_sig").data() + 4;
            auto *reflection_psh = get_chimera().get_signature("water_reflection_set_psh_consts_retail_sig").data();
            write_jmp_call(opacity_psh, opacity_set_psh_const, reinterpret_cast<const void *>(water_opacity_psh_const_asm), nullptr);
            write_jmp_call(reflection_psh, reflection_set_psh_const, nullptr, reinterpret_cast<const void *>(water_reflection_psh_const_asm));
        }
    }

    extern "C" void make_water_go_brr(int i) noexcept {
        auto water_render_target_texture = reinterpret_cast<IDirect3DTexture9 **>(global_render_targets + 20 * 8 + 16);
        IDirect3DSurface9 *mipmap_surface = NULL;
        IDirect3DTexture9_GetSurfaceLevel(*water_render_target_texture, i, &mipmap_surface);
        IDirect3DDevice9_SetRenderTarget(*global_d3d9_device, 0, mipmap_surface);
        IDirect3DSurface9_Release(mipmap_surface);
    }

    extern "C" void make_water_go_brr_pt2(int mip_levels) noexcept {
        auto water_render_target_texture = reinterpret_cast<IDirect3DTexture9 **>(global_render_targets + 20 * 8 + 16);
        auto vertices = reinterpret_cast<const void *>(bump_vertices);

        for(int i = mip_levels; i < 4; i++) {
            IDirect3DSurface9 *mipmap_surface = NULL;
            IDirect3DTexture9_GetSurfaceLevel(*water_render_target_texture, i, &mipmap_surface);
            IDirect3DDevice9_SetRenderTarget(*global_d3d9_device, 0, mipmap_surface);
            IDirect3DSurface9_Release(mipmap_surface);
            IDirect3DDevice9_DrawPrimitiveUP(*global_d3d9_device, D3DPT_TRIANGLEFAN, 2, vertices, 24);
        }
    }

    extern "C" void make_water_go_brr_samplers(std::byte *shader) noexcept {
        float detaiL_lod_bias = - (*reinterpret_cast<float *>(shader + 0xE0));
        DWORD *mip_lod_bias = reinterpret_cast<DWORD *>(&detaiL_lod_bias);

        IDirect3DDevice9_SetSamplerState(*global_d3d9_device, 0, D3DSAMP_MIPMAPLODBIAS, *mip_lod_bias);
        IDirect3DDevice9_SetSamplerState(*global_d3d9_device, 1, D3DSAMP_MIPMAPLODBIAS, *mip_lod_bias);
        IDirect3DDevice9_SetSamplerState(*global_d3d9_device, 2, D3DSAMP_MIPMAPLODBIAS, *mip_lod_bias);
        IDirect3DDevice9_SetSamplerState(*global_d3d9_device, 3, D3DSAMP_MIPMAPLODBIAS, *mip_lod_bias);
    }

    void set_up_water_fix() noexcept {
        global_d3d9_device = reinterpret_cast<IDirect3DDevice9 **>(*reinterpret_cast<std::byte **>(get_chimera().get_signature("model_af_set_sampler_states_sig").data() + 1));
        d3d9_device_caps = reinterpret_cast<D3DCAPS9 *>(*reinterpret_cast<std::byte **>(get_chimera().get_signature("d3d9_device_caps_sig").data() + 1));

        // Need to check the device caps for ps_2_0 support so set it up after the d3d9 device has been created.
        add_game_start_event(initialize_water_hooks);

        // Make ripple mipmapping work. This is based on Mango's fix from ringoworld.
        global_render_targets = *reinterpret_cast<std::byte **>(get_chimera().get_signature("render_targets_initialize_sig").data() + 1);
        std::byte *water_bump = nullptr;
        auto *water_bump_2 = get_chimera().get_signature("water_bumpmap_draw_2_sig").data() + 6;;
        static Hook render_target_hook, sampler_hook, water_bump_hook, water_bump_hook_2;

        write_jmp_call(get_chimera().get_signature("render_targets_create_texture_sig").data(), render_target_hook, reinterpret_cast<const void *>(adjust_water_render_target_mip_asm), nullptr);

        // Fixed function check that doesn't even work right. This can go in the bin.
        SigByte mod[] = {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90};
        write_code_s(get_chimera().get_signature("water_bumpmap_update_sig").data(), mod);
        write_jmp_call(get_chimera().get_signature("water_bumpmap_samplers_sig").data(), sampler_hook, nullptr, reinterpret_cast<const void *>(set_water_bumpmap_sampler_states_asm));

        if(get_chimera().feature_present("client_retail_demo")) {
            water_bump = get_chimera().get_signature("water_bumpmap_draw_sig").data();
            bump_vertices = *reinterpret_cast<std::byte **>(get_chimera().get_signature("water_bumpmap_vertices_sig").data() + 3);
        }
        else if(get_chimera().feature_present("client_custom_edition")) {
            water_bump = get_chimera().get_signature("water_bumpmap_draw_ce_sig").data() + 6;
            bump_vertices = *reinterpret_cast<std::byte **>(get_chimera().get_signature("water_bumpmap_vertices_ce_sig").data() + 3);
        }

        write_jmp_call(water_bump, water_bump_hook, reinterpret_cast<const void *>(water_bumpmap_memes_asm), nullptr);
        write_jmp_call(water_bump_2, water_bump_hook_2, reinterpret_cast<const void *>(water_bumpmap_memes_2_asm), nullptr);
    }
}