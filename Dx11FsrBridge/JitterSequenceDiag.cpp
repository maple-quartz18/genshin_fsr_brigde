#include "JitterSequenceDiag.h"

#include <cmath>
#include <string>

#include "BridgeLogger.h"

namespace jitter_sequence_diag
{
#if defined(DX11FSRBRIDGE_ENABLE_DIAGNOSTICS)
namespace
{
constexpr unsigned kHistory = 512;
constexpr unsigned kMaxPeriod = 256;

struct State
{
    Sample samples[kHistory] {};
    unsigned head = 0;
    unsigned count = 0;
    std::uint32_t last_w = 0;
    std::uint32_t last_h = 0;
    std::uint64_t last_instance = 0;
    unsigned raw_period = 0;
    unsigned bridge_period = 0;
};

State g_state;

bool close(float a, float b)
{
    return std::isfinite(a) && std::isfinite(b) && std::fabs(a - b) < 1.0e-6f;
}
#endif

unsigned detect(const Sample *items, unsigned count, bool bridge)
{
    if (count < 2)
        return 0;
    const unsigned last = count - 1;
    unsigned limit = count / 2;
    if (limit > kMaxPeriod)
        limit = kMaxPeriod;
    for (unsigned p = 1; p <= limit; ++p)
    {
        auto x = [bridge](const Sample &s) { return bridge ? s.bridge_x : s.raw_x; };
        auto y = [bridge](const Sample &s) { return bridge ? s.bridge_y : s.raw_y; };
        if (!close(x(items[last]), x(items[last - p])) ||
            !close(y(items[last]), y(items[last - p])))
            continue;
        unsigned k = 1;
        for (; k < p; ++k)
        {
            if (!close(x(items[last - k]), x(items[last - k - p])) ||
                !close(y(items[last - k]), y(items[last - k - p])))
                break;
        }
        if (k == p)
            return p;
    }
    return 0;
}
}

void observe(const Sample &sample)
{
    const bool size_changed = g_state.last_w != 0 &&
        (g_state.last_w != sample.render_w || g_state.last_h != sample.render_h);
    const bool instance_changed = g_state.last_instance != 0 &&
        g_state.last_instance != sample.instance;
    if (size_changed || instance_changed || sample.reset != 0)
    {
        g_state = State {};
        LOG_INFO(blog::cat::upscale,
                 "jitter_sequence window_reset frame=" + std::to_string(sample.frame_index) +
                 " reason=" + (size_changed ? std::string("render_size") :
                                (instance_changed ? std::string("instance") : std::string("ngx_reset"))) +
                 " render=" + std::to_string(sample.render_w) + "x" +
                 std::to_string(sample.render_h));
    }

    g_state.samples[g_state.head] = sample;
    g_state.head = (g_state.head + 1) % kHistory;
    if (g_state.count < kHistory)
        ++g_state.count;

    Sample ordered[kHistory] {};
    for (unsigned i = 0; i < g_state.count; ++i)
        ordered[i] = g_state.samples[(g_state.head + kHistory - g_state.count + i) % kHistory];
    g_state.raw_period = detect(ordered, g_state.count, false);
    g_state.bridge_period = detect(ordered, g_state.count, true);
    g_state.last_w = sample.render_w;
    g_state.last_h = sample.render_h;
    g_state.last_instance = sample.instance;

    static std::uint64_t count = 0;
    ++count;
    static unsigned last_logged_raw = 0;
    static unsigned last_logged_bridge = 0;
    const bool period_changed = g_state.raw_period != last_logged_raw ||
                                g_state.bridge_period != last_logged_bridge;
    if (count <= 8 || count % 1021 == 0 || size_changed || instance_changed ||
        sample.reset != 0 || period_changed)
    {
        LOG_INFO(blog::cat::upscale,
                 "jitter_sequence sample frame=" + std::to_string(sample.frame_index) +
                 " inst=" + std::to_string(sample.instance) +
                 " raw=" + std::to_string(sample.raw_x) + "," + std::to_string(sample.raw_y) +
                 " bridge=" + std::to_string(sample.bridge_x) + "," + std::to_string(sample.bridge_y) +
                 " reset=" + std::to_string(sample.reset) +
                 " render=" + std::to_string(sample.render_w) + "x" +
                 std::to_string(sample.render_h) +
                 " raw_period=" + std::to_string(g_state.raw_period) +
                 " bridge_period=" + std::to_string(g_state.bridge_period));
        last_logged_raw = g_state.raw_period;
        last_logged_bridge = g_state.bridge_period;
    }
}
}
