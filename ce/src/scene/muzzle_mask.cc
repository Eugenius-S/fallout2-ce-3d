#include "muzzle_mask.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace fallout { namespace scene {
namespace {
bool valid(const MuzzleImage& im)
{
    return im.width > 0 && im.height > 0 && im.width <= 1024 && im.height <= 1024
        && im.indices.size() == static_cast<size_t>(im.width) * im.height;
}
int light(unsigned char index, const unsigned char* p)
{
    return p[index * 3] + p[index * 3 + 1] + p[index * 3 + 2];
}
std::pair<int, int> foot(const MuzzleImage& im, const unsigned char* p)
{
    int bottom = -1;
    std::vector<int> xs;
    // Long flame/smoke sheets can extend below the feet: restrict registration
    // to the native anchor and reject warm flame rather than tracking the tail.
    for (int y = 0; y < im.height; ++y) {
        for (int x = 0; x < im.width; ++x) {
            const auto c = im.indices[y * im.width + x];
            const int r = p[c * 3], g = p[c * 3 + 1], b = p[c * 3 + 2];
            if (c == 0 || light(c, p) <= 18 || std::abs(x - im.anchorX) > 32
                || std::abs(y - im.anchorY) > 12 || (r > 30 && r > b * 1.5 && r >= g)) continue;
            bottom = std::max(bottom, y);
        }
    }
    if (bottom < 0) return { im.anchorX, im.anchorY };
    for (int y = std::max(0, bottom - 4); y <= bottom; ++y) {
        for (int x = std::max(0, im.anchorX - 32); x < std::min(im.width, im.anchorX + 33); ++x) {
            const auto c = im.indices[y * im.width + x];
            const int r = p[c * 3], g = p[c * 3 + 1], b = p[c * 3 + 2];
            if (c != 0 && light(c, p) > 18 && !(r > 30 && r > b * 1.5 && r >= g)) xs.push_back(x);
        }
    }
    if (xs.empty()) return { im.anchorX, bottom };
    std::sort(xs.begin(), xs.end());
    return { (xs[(xs.size() - 1) / 2] + xs[xs.size() / 2]) / 2, bottom };
}
}

std::vector<MuzzleMask> detectMuzzleMasks(const std::vector<MuzzleImage>& frames,
    const MuzzleImage& aim, int rotation, int actionFrame, bool single, bool continuous,
    const unsigned char* palette)
{
    std::vector<MuzzleMask> result(frames.size());
    if (palette == nullptr || !valid(aim) || frames.size() > 64 || rotation < 0 || rotation >= 6) return result;
    const int sign = rotation < 3 ? 1 : -1;
    const auto base = foot(aim, palette);
    int front = -1024;
    for (int y = 0; y < aim.height; ++y) for (int x = 0; x < aim.width; ++x) {
        const auto c = aim.indices[y * aim.width + x];
        if (c != 0 && light(c, palette) > 18 && y < base.second - 12) front = std::max(front, sign * (x - base.first));
    }
    if (front == -1024) return result;
    std::vector<std::vector<int>> cores(frames.size());
    int peak = 0;
    for (size_t i = 0; i < frames.size(); ++i) {
        const auto& im = frames[i];
        if (!valid(im) || (continuous && im.width * 10 < aim.width * 14)) continue;
        auto& out = result[i];
        out.width = im.width; out.height = im.height;
        out.mask.assign(im.indices.size(), 0);
        const auto current = foot(im, palette);
        std::vector<unsigned char> candidates(im.indices.size(), 0);
        for (int y = 0; y < im.height; ++y) for (int x = 0; x < im.width; ++x) {
            const auto c = im.indices[y * im.width + x];
            if (c == 0 || sign * (x - current.first) < front - 16
                || std::max({palette[c * 3], palette[c * 3 + 1], palette[c * 3 + 2]}) < 47 || light(c, palette) < 108) continue;
            int quiet = 0;
            for (int dy = -2; dy <= 2; ++dy) for (int dx = -2; dx <= 2; ++dx) {
                const int rx = x - current.first + base.first + dx, ry = y - current.second + base.second + dy;
                if (rx >= 0 && rx < aim.width && ry >= 0 && ry < aim.height) quiet = std::max(quiet, light(aim.indices[ry * aim.width + rx], palette));
            }
            if (light(c, palette) > quiet + 20) candidates[y * im.width + x] = 1;
        }
        // Isolated eyes/glints are not a flash. Keep coherent bright clusters.
        for (int at = 0; at < static_cast<int>(candidates.size()); ++at) {
            if (!candidates[at]) continue;
            std::vector<int> group = {at}; candidates[at] = 0;
            for (size_t n = 0; n < group.size(); ++n) {
                const int x = group[n] % im.width, y = group[n] / im.width;
                for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx) {
                    const int nx = x + dx, ny = y + dy;
                    if (nx < 0 || nx >= im.width || ny < 0 || ny >= im.height) continue;
                    const int next = ny * im.width + nx;
                    if (candidates[next]) { candidates[next] = 0; group.push_back(next); }
                }
            }
            if (group.size() >= 2) cores[i].insert(cores[i].end(), group.begin(), group.end());
        }
        if (!continuous && !cores[i].empty()) {
            int edge = -1024;
            for (int at : cores[i]) edge = std::max(edge, sign * (at % im.width - current.first));
            auto& core = cores[i];
            core.erase(std::remove_if(core.begin(), core.end(), [&](int at) { return sign * (at % im.width - current.first) < edge - 5; }), core.end());
        }
        peak = std::max(peak, static_cast<int>(cores[i].size()));
    }
    std::vector<bool> active(frames.size(), false);
    int cue = -1;
    if (single && actionFrame >= 0 && actionFrame < static_cast<int>(frames.size()) && cores[actionFrame].size() >= 2) cue = actionFrame;
    if (single && cue < 0 && peak >= 3) {
        for (size_t i = 0; i < cores.size(); ++i) if (static_cast<int>(cores[i].size()) == peak) { cue = static_cast<int>(i); break; }
    }
    const double threshold = continuous ? 3.0 : (single && cue >= 0 ? std::max(2.0, cores[cue].size() * 0.5) : std::max(3.0, peak * 0.5));
    for (size_t i = 0; i < cores.size(); ++i) active[i] = cores[i].size() >= threshold;
    if (single) {
        std::vector<bool> pulse(frames.size(), false);
        if (cue >= 0) {
            for (int i = cue; i >= 0 && active[i]; --i) pulse[i] = true;
            for (int i = cue; i < static_cast<int>(active.size()) && active[i]; ++i) pulse[i] = true;
        }
        active = std::move(pulse);
    }
    for (size_t i = 0; i < frames.size(); ++i) {
        if (!active[i]) continue;
        auto& out = result[i]; const auto& im = frames[i];
        out.corePixels = static_cast<int>(cores[i].size());
        for (int at : cores[i]) {
            const int x = at % im.width, y = at / im.width;
            out.x += x + 0.5; out.y += y + 0.5; ++out.colors[im.indices[at]];
            out.mask[at] = 1;
            for (int dy = -2; dy <= 2; ++dy) for (int dx = -2; dx <= 2; ++dx) {
                const int nx = x + dx, ny = y + dy;
                if (nx < 0 || nx >= im.width || ny < 0 || ny >= im.height) continue;
                const auto c = im.indices[ny * im.width + nx];
                const int r = palette[c * 3], g = palette[c * 3 + 1], b = palette[c * 3 + 2];
                if (c != 0 && ((r >= 28 && r >= g && r > b * 1.5) || std::min({r,g,b}) >= 45)) out.mask[ny * im.width + nx] = 1;
            }
        }
        out.x /= out.corePixels; out.y /= out.corePixels;
        out.rootX=out.x; out.rootY=out.y;
        if (continuous) {
            const auto current=foot(im,palette);
            int closest=cores[i][0],distance=10000000;
            for (int at:cores[i]) {
                const int dx=sign*(at%im.width-current.first)-front;
                if (std::abs(dx)<distance) {distance=std::abs(dx);closest=at;}
            }
            out.rootX=closest%im.width+0.5; out.rootY=closest/im.width+0.5;
        }
        out.extraMask.assign(out.mask.size(),0);
        for (int y=0;y<im.height;++y) for(int x=0;x<im.width;++x) {
            const double dx=x+0.5-out.rootX,dy=y+0.5-out.rootY;
            if (out.mask[y*im.width+x] && dx*dx+dy*dy<=25.0) out.extraMask[y*im.width+x]=1;
        }
    }
    return result;
}
} }
