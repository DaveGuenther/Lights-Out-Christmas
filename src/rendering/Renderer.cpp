#include "rendering/Renderer.h"
#include <SDL2/SDL.h>
#include <cstring>
#include <cmath>

namespace LightsOut {

// 5×7 pixel bitmap font for ASCII 32 (' ') through 90 ('Z') + lowercase
// Each entry: 5 rows × 4 bits (MSB = leftmost pixel), padded to uint8_t
static const uint8_t FONT5x7[][5] = {
    // 32 ' '
    {0x00,0x00,0x00,0x00,0x00},
    // 33 '!'
    {0x40,0x40,0x40,0x00,0x40},
    // 34 '"'
    {0xA0,0xA0,0x00,0x00,0x00},
    // 35 '#'
    {0xA0,0xE0,0xA0,0xE0,0xA0},
    // 36 '$'  (skip to 48 '0' for brevity in placeholder)
    {0x60,0xC0,0x60,0x30,0xC0},
    // 37-47: generic placeholder
    {0xF0,0x00,0x00,0x00,0x00},
    {0x80,0x40,0x20,0x10,0x08},
    {0x60,0xA0,0x60,0xA0,0x60},
    {0xC0,0x40,0x40,0x40,0xE0},
    {0xE0,0x20,0x60,0x80,0xE0},
    {0xE0,0x20,0x60,0x20,0xE0},
    {0xA0,0xA0,0xE0,0x20,0x20},
    {0xC0,0x80,0xE0,0x20,0xC0},
    {0x60,0x80,0xE0,0xA0,0x60},
    {0xE0,0x20,0x40,0x40,0x40},
    {0xE0,0xA0,0xE0,0xA0,0xE0},
    // 48 '0'
    {0x60,0xA0,0xA0,0xA0,0x60},
    // 49 '1'
    {0x40,0xC0,0x40,0x40,0xE0},
    // 50 '2'
    {0xE0,0x20,0x60,0x80,0xE0},
    // 51 '3'
    {0xE0,0x20,0x60,0x20,0xE0},
    // 52 '4'
    {0xA0,0xA0,0xE0,0x20,0x20},
    // 53 '5'
    {0xE0,0x80,0xE0,0x20,0xE0},
    // 54 '6'
    {0x60,0x80,0xE0,0xA0,0x60},
    // 55 '7'
    {0xE0,0x20,0x20,0x40,0x40},
    // 56 '8'
    {0x60,0xA0,0x60,0xA0,0x60},
    // 57 '9'
    {0x60,0xA0,0x60,0x20,0x40},
    // 58 ':'
    {0x00,0x40,0x00,0x40,0x00},
    // 59 ';'
    {0x00,0x40,0x00,0x40,0x80},
    // 60 '<'
    {0x20,0x40,0x80,0x40,0x20},
    // 61 '='
    {0x00,0xE0,0x00,0xE0,0x00},
    // 62 '>'
    {0x80,0x40,0x20,0x40,0x80},
    // 63 '?'
    {0x60,0x20,0x40,0x00,0x40},
    // 64 '@'
    {0x60,0xA0,0xE0,0x80,0x60},
    // 65 'A'
    {0x60,0xA0,0xE0,0xA0,0xA0},
    // 66 'B'
    {0xC0,0xA0,0xC0,0xA0,0xC0},
    // 67 'C'
    {0x60,0x80,0x80,0x80,0x60},
    // 68 'D'
    {0xC0,0xA0,0xA0,0xA0,0xC0},
    // 69 'E'
    {0xE0,0x80,0xE0,0x80,0xE0},
    // 70 'F'
    {0xE0,0x80,0xE0,0x80,0x80},
    // 71 'G'
    {0x60,0x80,0xA0,0xA0,0x60},
    // 72 'H'
    {0xA0,0xA0,0xE0,0xA0,0xA0},
    // 73 'I'
    {0xE0,0x40,0x40,0x40,0xE0},
    // 74 'J'
    {0x60,0x20,0x20,0xA0,0x60},
    // 75 'K'
    {0xA0,0xA0,0xC0,0xA0,0xA0},
    // 76 'L'
    {0x80,0x80,0x80,0x80,0xE0},
    // 77 'M'
    {0xA0,0xE0,0xE0,0xA0,0xA0},
    // 78 'N'
    {0xA0,0xE0,0xE0,0xC0,0xA0},
    // 79 'O'
    {0x60,0xA0,0xA0,0xA0,0x60},
    // 80 'P'
    {0xC0,0xA0,0xC0,0x80,0x80},
    // 81 'Q'
    {0x60,0xA0,0xA0,0xE0,0x70},
    // 82 'R'
    {0xC0,0xA0,0xC0,0xA0,0xA0},
    // 83 'S'
    {0x60,0x80,0x60,0x20,0xC0},
    // 84 'T'
    {0xE0,0x40,0x40,0x40,0x40},
    // 85 'U'
    {0xA0,0xA0,0xA0,0xA0,0x60},
    // 86 'V'
    {0xA0,0xA0,0xA0,0xA0,0x40},
    // 87 'W'
    {0xA0,0xA0,0xE0,0xE0,0xA0},
    // 88 'X'
    {0xA0,0xA0,0x40,0xA0,0xA0},
    // 89 'Y'
    {0xA0,0xA0,0x40,0x40,0x40},
    // 90 'Z'
    {0xE0,0x20,0x40,0x80,0xE0},
};

static int fontIndex(char c) {
    int i = static_cast<int>(static_cast<unsigned char>(c));
    if (i < 32 || i > 90) return 0;  // space
    return i - 32;
}

// ─────────────────────────────────────────────────────────────────────────────

Renderer::~Renderer() {
    shutdown();
}

bool Renderer::init(SDL_Window* window) {
    m_renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!m_renderer) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "CreateRenderer failed: %s", SDL_GetError());
        return false;
    }

    m_renderTarget = SDL_CreateTexture(m_renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        RENDER_WIDTH, RENDER_HEIGHT);
    if (!m_renderTarget) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "CreateTexture (render target) failed: %s", SDL_GetError());
        return false;
    }

    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetTextureBlendMode(m_renderTarget, SDL_BLENDMODE_BLEND);
    SDL_RenderSetLogicalSize(m_renderer, SCREEN_WIDTH, SCREEN_HEIGHT);

    m_initialized = true;
    return true;
}

void Renderer::shutdown() {
    if (m_renderTarget) { SDL_DestroyTexture(m_renderTarget); m_renderTarget = nullptr; }
    if (m_renderer)     { SDL_DestroyRenderer(m_renderer);    m_renderer     = nullptr; }
    m_initialized = false;
}

void Renderer::beginFrame() {
    SDL_SetRenderTarget(m_renderer, m_renderTarget);
    SDL_SetRenderDrawColor(m_renderer, 8, 12, 35, 255);
    SDL_RenderClear(m_renderer);
}

void Renderer::endFrame() {
    // Blit render target to screen at PIXEL_SCALE × scale
    SDL_SetRenderTarget(m_renderer, nullptr);
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
    SDL_RenderClear(m_renderer);

    SDL_Rect dst = {0, 0, RENDER_WIDTH * PIXEL_SCALE, RENDER_HEIGHT * PIXEL_SCALE};
    // Center in window
    int wx, wy;
    SDL_GetRendererOutputSize(m_renderer, &wx, &wy);
    dst.x = (wx - dst.w) / 2;
    dst.y = (wy - dst.h) / 2;

    SDL_RenderCopy(m_renderer, m_renderTarget, nullptr, &dst);
    SDL_RenderPresent(m_renderer);
}

void Renderer::setColor(Color c) {
    SDL_SetRenderDrawColor(m_renderer, c.r, c.g, c.b, c.a);
}

void Renderer::setColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    SDL_SetRenderDrawColor(m_renderer, r, g, b, a);
}

void Renderer::clear(Color c) {
    setColor(c);
    SDL_RenderClear(m_renderer);
}

void Renderer::drawRect(const Rect& r) {
    SDL_FRect fr = {r.x, r.y, r.w, r.h};
    SDL_RenderDrawRectF(m_renderer, &fr);
}

void Renderer::fillRect(const Rect& r) {
    SDL_FRect fr = {r.x, r.y, r.w, r.h};
    SDL_RenderFillRectF(m_renderer, &fr);
}

void Renderer::drawLine(Vec2 a, Vec2 b) {
    SDL_RenderDrawLineF(m_renderer, a.x, a.y, b.x, b.y);
}

void Renderer::drawPoint(Vec2 p) {
    SDL_RenderDrawPointF(m_renderer, p.x, p.y);
}

void Renderer::fillCircle(Vec2 center, float radius) {
    int r = static_cast<int>(radius);
    int cx = static_cast<int>(center.x);
    int cy = static_cast<int>(center.y);
    for (int dy = -r; dy <= r; ++dy) {
        int dx = static_cast<int>(std::sqrt(static_cast<float>(r * r - dy * dy)));
        SDL_RenderDrawLine(m_renderer, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

void Renderer::drawChar(char c, int x, int y, Color col) {
    int idx = fontIndex(c);
    SDL_SetRenderDrawColor(m_renderer, col.r, col.g, col.b, col.a);
    for (int row = 0; row < 5; ++row) {
        uint8_t bits = FONT5x7[idx][row];
        for (int col_ = 0; col_ < 4; ++col_) {
            if (bits & (0x80 >> col_)) {
                SDL_RenderDrawPoint(m_renderer, x + col_, y + row);
            }
        }
    }
}

int Renderer::drawText(const std::string& text, Vec2 pos, Color c,
                        int /*fontSize*/, bool center) {
    int charW = 5;  // 4px char + 1px gap
    int totalW = static_cast<int>(text.size()) * charW;
    float startX = pos.x;
    if (center) startX -= totalW / 2.0f;

    for (size_t i = 0; i < text.size(); ++i) {
        char ch = text[i];
        // Map lowercase to uppercase for our simple font
        if (ch >= 'a' && ch <= 'z') ch = ch - 'a' + 'A';
        drawChar(ch, static_cast<int>(startX) + static_cast<int>(i) * charW,
                 static_cast<int>(pos.y), c);
    }
    return totalW;
}

void Renderer::loadBitmapFont() {
    // Font is drawn procedurally using drawChar() — no texture needed
}

void Renderer::drawTexture(SDL_Texture* tex, const Rect& src, const Rect& dst) {
    SDL_Rect s = {static_cast<int>(src.x), static_cast<int>(src.y),
                  static_cast<int>(src.w), static_cast<int>(src.h)};
    SDL_FRect d = {dst.x, dst.y, dst.w, dst.h};
    SDL_RenderCopyF(m_renderer, tex, &s, &d);
}

}  // namespace LightsOut
