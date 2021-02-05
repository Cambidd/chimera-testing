// SPDX-License-Identifier: GPL-3.0-only

#include "leak_descriptors.hpp"
#include "../chimera.hpp"
#include "../signature/signature.hpp"
#include "../signature/hook.hpp"

namespace Chimera {
    void set_up_fix_leaking_descriptors() noexcept {
        static std::uint32_t fix = 0x7F000000;
        auto &chimera = get_chimera();
        
        if(chimera.feature_present("core_file_descriptors")) {
            auto *addr1 = chimera.get_signature("leak_file_descriptors_1_sig").data();
            overwrite(addr1 + 1, fix);

            auto *addr2 = chimera.get_signature("leak_file_descriptors_2_sig").data();
            overwrite(addr2 + 2, fix);

            auto *addr3 = chimera.get_signature("leak_file_descriptors_3_sig").data();
            overwrite(addr3 + 1, fix);
        }
        
        if(chimera.feature_present("core_file_size_demo")) {
            auto *addr4 = chimera.get_signature("max_map_size_demo_1_sig").data();
            overwrite(addr4 + 1, fix);
            
            auto *addr5 = chimera.get_signature("max_map_size_demo_2_sig").data();
            overwrite(addr5 + 1, fix);
        }
    }
}
