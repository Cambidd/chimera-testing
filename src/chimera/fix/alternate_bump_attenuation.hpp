// SPDX-License-Identifier: GPL-3.0-only

#ifndef CHIMERA_ALTERNATE_BUMP_ATTENUATION
#define CHIMERA_ALTERNATE_BUMP_ATTENUATION

namespace Chimera {
    /**
     * Enables support for the "use alternate bump attenuation" flag.
     */
    void set_up_alternate_bump_attenuation_support() noexcept;

    /**
     * Always use the alternate bump attenuation behaviour.
     */
    void enable_forced_alternate_bump_attenuation() noexcept;

    /**
     * Only use alternate bump attenuation if the "use alternate bump attenuation" flag is set.
     */
    void disable_forced_alternate_bump_attenuation() noexcept;
}

#endif
