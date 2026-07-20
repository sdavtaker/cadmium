// SPDX-License-Identifier: BSD-2-Clause
/**
 * Copyright (c) 2026-present, Damian Vicino
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CADMIUM_MODELING_PACK_HPP
#define CADMIUM_MODELING_PACK_HPP

#include <cstddef>

namespace cadmium::modeling {

    /**
     * Sentinel index marking a coupling endpoint that is a plain (non-pack)
     * submodel in the indexed coupling entry types below.
     */
    inline constexpr std::size_t not_packed = static_cast<std::size_t>(-1);

    /**
     * pack<M, N> — a homogeneous collection of N instances of submodel M
     * usable as ONE entry of a models_tuple.
     *
     * The nested model<TIME> is a structural marker (it produces no behavior
     * itself): it carries the element template and the pack size, and exposes
     * the element's port tuples so coupling-entry concepts can validate ports
     * uniformly. The engine layer maps it to a pack engine holding N element
     * engines (simulator or coordinator per element kind), addressed by
     * (pack, index) coupling entries.
     *
     * Usage:
     *   template <typename TIME> using gens = pack<my_generator, 4>::model<TIME>;
     *   using submodels = cadmium::modeling::models_tuple<gens, processor>;
     *   using ics = std::tuple<
     *       pack_IC<gens, 0, gen_defs::out, processor, not_packed, proc_defs::in>,
     *       ...>;
     */
    template <template <typename T> class M, std::size_t N> struct pack {
        static_assert(N > 0, "pack requires at least one element");

        template <typename TIME> struct model {
            template <typename T> using element    = M<T>;
            using element_model                    = M<TIME>;
            static constexpr std::size_t pack_size = N;

            // Element ports re-exposed so coupling entries addressing
            // (pack, index) validate against the element's port tuples.
            using input_ports  = typename M<TIME>::input_ports;
            using output_ports = typename M<TIME>::output_ports;
        };
    };

    /**
     * EIC from a coupled-model input port to input port of element INDEX of a
     * pack submodel. Mirrors EIC's member layout plus the element index.
     */
    template <typename EXTERNAL_PORT, template <typename TIME> class PACK, std::size_t INDEX,
              typename SUBMODEL_PORT>
    struct pack_EIC {
        using external_input_port               = EXTERNAL_PORT;
        template <typename TIME> using submodel = PACK<TIME>;
        static constexpr std::size_t index      = INDEX;
        using submodel_input_port               = SUBMODEL_PORT;
    };

    /**
     * EOC from an output port of element INDEX of a pack submodel to a
     * coupled-model output port. Mirrors EOC's member layout plus the index.
     */
    template <template <typename TIME> class PACK, std::size_t INDEX, typename SUBMODEL_PORT,
              typename EXTERNAL_PORT>
    struct pack_EOC {
        template <typename TIME> using submodel = PACK<TIME>;
        static constexpr std::size_t index      = INDEX;
        using submodel_output_port              = SUBMODEL_PORT;
        using external_output_port              = EXTERNAL_PORT;
    };

    /**
     * IC between submodels where either endpoint (or both) may be a pack
     * element. A plain endpoint uses not_packed as its index. Same-pack
     * couplings are allowed when the two indices differ.
     */
    template <template <typename TIME> class SUBMODEL_FROM, std::size_t FROM_INDEX,
              typename PORT_FROM, template <typename TIME> class SUBMODEL_TO, std::size_t TO_INDEX,
              typename PORT_TO>
    struct pack_IC {
        template <typename TIME> using from_model = SUBMODEL_FROM<TIME>;
        static constexpr std::size_t from_index   = FROM_INDEX;
        using from_model_output_port              = PORT_FROM;
        template <typename TIME> using to_model   = SUBMODEL_TO<TIME>;
        static constexpr std::size_t to_index     = TO_INDEX;
        using to_model_input_port                 = PORT_TO;
    };

} // namespace cadmium::modeling

#endif // CADMIUM_MODELING_PACK_HPP
