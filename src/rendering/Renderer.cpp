#include "rendering/Renderer.h"
#include <SDL2/SDL.h>
#include <cstring>
#include <cmath>

namespace LightsOut {

// ─── Sprint-2-inspired 7×7 arcade bitmap font ────────────────────────────────
// ASCII 32–95 plus { | } (123–125).  Lowercase is mapped to uppercase by the
// caller before indexing.
//
// Encoding: each row uses the TOP 7 BITS of a uint8_t.
//   bit 7 = left-most pixel (col 0)
//   bit 1 = right-most pixel (col 6)
//   bit 0 = unused / always 0
//
// To check column j (0=left): (row_byte >> (7 - j)) & 1
//
static const uint8_t FONT_DATA[][7] = {
    // 32 ' '
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    // 33 '!'
    {0x18,0x18,0x18,0x18,0x00,0x18,0x00},
    // 34 '"'
    {0x6C,0x6C,0x00,0x00,0x00,0x00,0x00},
    // 35 '#'
    {0x6C,0xFE,0x6C,0xFE,0x6C,0x00,0x00},
    // 36 '$'
    {0x18,0x7E,0xD0,0x7C,0x16,0xFC,0x18},
    // 37 '%'
    {0xC6,0xC6,0x0C,0x18,0x30,0x66,0x66},
    // 38 '&'
    {0x38,0x6C,0x38,0x70,0xDE,0xCC,0x76},
    // 39 '\''
    {0x18,0x18,0x00,0x00,0x00,0x00,0x00},
    // 40 '('
    {0x1C,0x38,0x60,0x60,0x60,0x38,0x1C},
    // 41 ')'
    {0x70,0x38,0x1C,0x1C,0x1C,0x38,0x70},
    // 42 '*'
    {0x00,0x6C,0x38,0xFE,0x38,0x6C,0x00},
    // 43 '+'
    {0x00,0x18,0x18,0xFE,0x18,0x18,0x00},
    // 44 ','
    {0x00,0x00,0x00,0x00,0x18,0x18,0x30},
    // 45 '-'
    {0x00,0x00,0x00,0xFE,0x00,0x00,0x00},
    // 46 '.'
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18},
    // 47 '/'
    {0x06,0x06,0x0C,0x18,0x30,0x60,0x60},
    // 48 '0'
    {0x7C,0xC6,0xCE,0xD6,0xE6,0xC6,0x7C},
    // 49 '1'
    {0x18,0x38,0x18,0x18,0x18,0x18,0x7E},
    // 50 '2'
    {0x7C,0xC6,0x06,0x1C,0x30,0x60,0xFE},
    // 51 '3'
    {0x7C,0xC6,0x06,0x3C,0x06,0xC6,0x7C},
    // 52 '4'
    {0x1C,0x3C,0x6C,0xCC,0xFE,0x0C,0x0C},
    // 53 '5'
    {0xFE,0xC0,0xC0,0xFC,0x06,0x06,0xFC},
    // 54 '6'
    {0x3C,0x60,0xC0,0xFC,0xC6,0xC6,0x7C},
    // 55 '7'
    {0xFE,0x06,0x0C,0x18,0x30,0x30,0x30},
    // 56 '8'
    {0x7C,0xC6,0xC6,0x7C,0xC6,0xC6,0x7C},
    // 57 '9'
    {0x7C,0xC6,0xC6,0x7E,0x06,0x0C,0x78},
    // 58 ':'
    {0x00,0x18,0x18,0x00,0x18,0x18,0x00},
    // 59 ';'
    {0x00,0x18,0x18,0x00,0x18,0x18,0x30},
    // 60 '<'
    {0x00,0x1C,0x38,0x70,0x38,0x1C,0x00},
    // 61 '='
    {0x00,0x00,0xFE,0x00,0xFE,0x00,0x00},
    // 62 '>'
    {0x00,0x70,0x38,0x1C,0x38,0x70,0x00},
    // 63 '?'
    {0x7C,0xC6,0x06,0x1C,0x18,0x00,0x18},
    // 64 '@'
    {0x7C,0xC6,0xDE,0xDE,0xDE,0xC0,0x7C},
    // 65 'A'     ...#...  ..###..  .##.##.  ##...##  #######  ##...##  ##...##
    {0x10,0x38,0x6C,0xC6,0xFE,0xC6,0xC6},
    // 66 'B'     ######.  ##...##  ##...##  ######.  ##...##  ##...##  ######.
    {0xFC,0xC6,0xC6,0xFC,0xC6,0xC6,0xFC},
    // 67 'C'     .######  ##.....  ##.....  ##.....  ##.....  ##.....  .######
    {0x7E,0xC0,0xC0,0xC0,0xC0,0xC0,0x7E},
    // 68 'D'     ######.  ##...##  ##...##  ##...##  ##...##  ##...##  ######.
    {0xFC,0xC6,0xC6,0xC6,0xC6,0xC6,0xFC},
    // 69 'E'     #######  ##.....  ##.....  #####..  ##.....  ##.....  #######
    {0xFE,0xC0,0xC0,0xF8,0xC0,0xC0,0xFE},
    // 70 'F'     #######  ##.....  ##.....  #####..  ##.....  ##.....  ##.....
    {0xFE,0xC0,0xC0,0xF8,0xC0,0xC0,0xC0},
    // 71 'G'     .######  ##.....  ##.....  ##..###  ##...##  ##...##  .######
    {0x7E,0xC0,0xC0,0xCE,0xC6,0xC6,0x7E},
    // 72 'H'     ##...##  ##...##  ##...##  #######  ##...##  ##...##  ##...##
    {0xC6,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6},
    // 73 'I'     #######  ...##..  ...##..  ...##..  ...##..  ...##..  #######
    {0xFE,0x18,0x18,0x18,0x18,0x18,0xFE},
    // 74 'J'     ..#####  ....##.  ....##.  ....##.  ##...##  ##...##  .#####.
    {0x3E,0x06,0x06,0x06,0xC6,0xC6,0x7C},
    // 75 'K'     ##...##  ##..##.  ##.##..  ####...  ##.##..  ##..##.  ##...##
    {0xC6,0xCC,0xD8,0xF0,0xD8,0xCC,0xC6},
    // 76 'L'     ##.....  ##.....  ##.....  ##.....  ##.....  ##.....  #######
    {0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xFE},
    // 77 'M'     ##...##  ###.###  ##.#.##  ##...##  ##...##  ##...##  ##...##
    {0xC6,0xEE,0xD6,0xC6,0xC6,0xC6,0xC6},
    // 78 'N'     ##...##  ###..##  ###..##  ##.####  ##..###  ##...##  ##...##
    {0xC6,0xE6,0xE6,0xF6,0xDE,0xCE,0xC6},
    // 79 'O'     .#####.  ##...##  ##...##  ##...##  ##...##  ##...##  .#####.
    {0x7C,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C},
    // 80 'P'     ######.  ##...##  ##...##  ######.  ##.....  ##.....  ##.....
    {0xFC,0xC6,0xC6,0xFC,0xC0,0xC0,0xC0},
    // 81 'Q'     .#####.  ##...##  ##...##  ##...##  ##..###  ##...##  .####.#
    {0x7C,0xC6,0xC6,0xC6,0xCE,0xC6,0x7A},
    // 82 'R'     ######.  ##...##  ##...##  ######.  ##.##..  ##..##.  ##...##
    {0xFC,0xC6,0xC6,0xFC,0xD8,0xCC,0xC6},
    // 83 'S'     .######  ##.....  ##.....  .#####.  .....##  .....##  ######.
    {0x7E,0xC0,0xC0,0x7C,0x06,0x06,0xFC},
    // 84 'T'     #######  ...##..  ...##..  ...##..  ...##..  ...##..  ...##..
    {0xFE,0x18,0x18,0x18,0x18,0x18,0x18},
    // 85 'U'     ##...##  ##...##  ##...##  ##...##  ##...##  ##...##  .#####.
    {0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C},
    // 86 'V'     ##...##  ##...##  ##...##  ##...##  .#####.  ..###..  ...#...
    {0xC6,0xC6,0xC6,0xC6,0x7C,0x38,0x10},
    // 87 'W'     ##...##  ##...##  ##...##  ##.#.##  ###.###  ###.###  ##...##
    {0xC6,0xC6,0xC6,0xD6,0xEE,0xEE,0xC6},
    // 88 'X'     ##...##  ##...##  .#####.  ...#...  .#####.  ##...##  ##...##
    {0xC6,0xC6,0x7C,0x10,0x7C,0xC6,0xC6},
    // 89 'Y'     ##...##  ##...##  .#####.  ...#...  ...#...  ...#...  ...#...
    {0xC6,0xC6,0x7C,0x10,0x10,0x10,0x10},
    // 90 'Z'     #######  .....##  ....##.  ...##..  ..##...  .##....  #######
    {0xFE,0x06,0x0C,0x18,0x30,0x60,0xFE},
    // 91 '['     .####..  .##....  .##....  .##....  .##....  .##....  .####..
    {0x78,0x60,0x60,0x60,0x60,0x60,0x78},
    // 92 '\'     ##.....  ##.....  .##....  ..##...  ...##..  ....##.  ....###
    {0xC0,0xC0,0x60,0x30,0x18,0x0C,0x0E},
    // 93 ']'     ..####.  ....##.  ....##.  ....##.  ....##.  ....##.  ..####.
    {0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C},
    // 94 '^'     ...#...  ..###..  .##.##.  ##...##  .......  .......  .......
    {0x10,0x38,0x6C,0xC6,0x00,0x00,0x00},
    // 95 '_'     .......  .......  .......  .......  .......  .......  #######
    {0x00,0x00,0x00,0x00,0x00,0x00,0xFE},
    // 64: '{' mapped here  ..###..  .##....  .##....  ###....  .##....  .##....  ..###..
    {0x38,0x60,0x60,0xE0,0x60,0x60,0x38},
    // 65: '|' mapped here  ...##..  ...##..  ...##..  ...##..  ...##..  ...##..  ...##..
    {0x18,0x18,0x18,0x18,0x18,0x18,0x18},
    // 66: '}' mapped here  ..###..  ....##.  ....##.  .....##  ....##.  ....##.  ..###..
    {0x38,0x0C,0x0C,0x0E,0x0C,0x0C,0x38},
};

static int fontIndex(char c) {
    int i = static_cast<int>(static_cast<unsigned char>(c));
    if (i >= 32 && i <= 95) return i - 32;   // indices 0–63
    if (i == '{') return 64;
    if (i == '|') return 65;
    if (i == '}') return 66;
    return 0;  // space
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

// Draw a single 7×7 character at pixel position (x, y) in render-space.
// Each font pixel is rendered as a 2×2 block so glyphs appear 14×14 on screen.
void Renderer::drawChar(char c, int x, int y, Color col) {
    int idx = fontIndex(c);
    SDL_SetRenderDrawColor(m_renderer, col.r, col.g, col.b, col.a);
    for (int row = 0; row < 7; ++row) {
        uint8_t bits = FONT_DATA[idx][row];
        for (int col_ = 0; col_ < 7; ++col_) {
            if (bits & (0x80 >> col_)) {
                SDL_Rect px = {x + col_ * 2, y + row * 2, 2, 2};
                SDL_RenderFillRect(m_renderer, &px);
            }
        }
    }
}

// Draw a string. Each glyph is 14px wide (7 pixels × 2) + 2px gap = 16px per char.
int Renderer::drawText(const std::string& text, Vec2 pos, Color c,
                        int /*fontSize*/, bool center) {
    constexpr int charW = 16;  // 14px glyph + 2px gap
    int totalW = static_cast<int>(text.size()) * charW;
    float startX = pos.x;
    if (center) startX -= totalW / 2.0f;

    for (size_t i = 0; i < text.size(); ++i) {
        char ch = text[i];
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
