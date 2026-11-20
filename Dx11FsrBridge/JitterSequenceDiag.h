#pragma once
#include <cstdint>

namespace jitter_sequence_diag
{
#if defined(DX11FSRBRIDGE_ENABLE_DIAGNOSTICS)
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

void observe(const Sample &sample);
#else
struct Sample {};
inline void observe(const Sample &) {}
#endif
}
