#pragma once
#include <cstdint>

namespace jitter_sequence_diag
{
struct Sample
{
    std::uint64_t instance = 0;
    std::uint32_t frame_index = 0;
    std::uint32_t render_w = 0;
    std::uint32_t render_h = 0;
    float raw_x = 0.0f;
    float raw_y = 0.0f;
    float bridge_x = 0.0f;
    float bridge_y = 0.0f;
    int reset = 0;
};

#if defined(DX11FSRBRIDGE_ENABLE_DIAGNOSTICS)
void observe(const Sample &sample);
#else
inline void observe(const Sample &) {}
#endif
}
