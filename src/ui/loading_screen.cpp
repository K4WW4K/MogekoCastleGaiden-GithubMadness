#include "loading_screen.hpp"
#include <iostream>
#include <cstdlib>
#include <ctime>

// Simple 5x7 bitmap font for "NOW LOADING"
// Each character is stored as 5 columns of 7 bits
static const uint8_t FONT_DATA[][5] = {
    // Space
    {0x00, 0x00, 0x00, 0x00, 0x00},
    // A
    {0x7E, 0x11, 0x11, 0x11, 0x7E},
    // D
    {0x7F, 0x41, 0x41, 0x41, 0x3E},
    // G
    {0x3E, 0x41, 0x49, 0x49, 0x7A},
    // I
    {0x00, 0x41, 0x7F, 0x41, 0x00},
    // L
    {0x7F, 0x40, 0x40, 0x40, 0x40},
    // N
    {0x7F, 0x02, 0x0C, 0x10, 0x7F},
    // O
    {0x3E, 0x41, 0x41, 0x41, 0x3E},
    // W
    {0x3F, 0x40, 0x38, 0x40, 0x3F},
    // .
    {0x00, 0x60, 0x60, 0x00, 0x00},
};

// Character to font index mapping
static int charToIndex(char c) {
    switch (c) {
        case ' ': return 0;
        case 'A': return 1;
        case 'D': return 2;
        case 'G': return 3;
        case 'I': return 4;
        case 'L': return 5;
        case 'N': return 6;
        case 'O': return 7;
        case 'W': return 8;
        case '.': return 9;
        default: return 0;
    }
}

LoadingScreen::LoadingScreen() = default;

LoadingScreen::~LoadingScreen() {
    cleanup();
}

bool LoadingScreen::init(SDL_Renderer* renderer) {
    // Initialize SDL_image
    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        std::cerr << "SDL_image could not initialize! IMG_Error: " << IMG_GetError() << std::endl;
        return false;
    }

    // Load the loading GIF
    if (!loadGif(renderer, "assets/sprites/loading.gif")) {
        std::cerr << "Failed to load loading.gif, using fallback" << std::endl;
        // Create a simple fallback texture
        SDL_Surface* surface = SDL_CreateRGBSurface(0, 64, 64, 32, 0, 0, 0, 0);
        if (surface) {
            SDL_FillRect(surface, nullptr, SDL_MapRGB(surface->format, 255, 220, 100));
            GifFrame frame;
            frame.texture = SDL_CreateTextureFromSurface(renderer, surface);
            frame.delay_ms = 100;
            m_frames.push_back(frame);
            SDL_FreeSurface(surface);
        }
    }
    // Set random loading duration (5-15 seconds)
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    m_loadingDuration = 5000 + (std::rand() % 10001); // 5000-15000ms
    std::cout << "Loading duration: " << m_loadingDuration << "ms" << std::endl;

    return true;
}

bool LoadingScreen::loadGif(SDL_Renderer* renderer, const std::string& path) {
    // SDL_image can load GIFs but doesn't animate them directly
    // We'll load it as a single frame for now
    // For full GIF animation, you'd need a library like gif_lib or stb_image
    
    IMG_Animation* anim = IMG_LoadAnimation(path.c_str());
    if (anim) {
        // Load all frames from the animation
        for (int i = 0; i < anim->count; i++) {
            GifFrame frame;
            frame.texture = SDL_CreateTextureFromSurface(renderer, anim->frames[i]);
            frame.delay_ms = anim->delays[i] > 0 ? anim->delays[i] : 100;
            if (frame.texture) {
                m_frames.push_back(frame);
            }
        }
        IMG_FreeAnimation(anim);
        return !m_frames.empty();
    }
    
    // Fallback: try loading as static image
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (surface) {
        GifFrame frame;
        frame.texture = SDL_CreateTextureFromSurface(renderer, surface);
        frame.delay_ms = 100;
        SDL_FreeSurface(surface);
        if (frame.texture) {
            m_frames.push_back(frame);
            return true;
        }
    }
    
    return false;
}

void LoadingScreen::cleanup() {
    for (auto& frame : m_frames) {
        if (frame.texture) {
            SDL_DestroyTexture(frame.texture);
            frame.texture = nullptr;
        }
    }
    m_frames.clear();
    IMG_Quit();
}

void LoadingScreen::update(Uint32 delta_time) {
    // Update GIF animation
    if (!m_frames.empty()) {
        m_frame_timer += delta_time;
        if (m_frame_timer >= static_cast<Uint32>(m_frames[m_current_frame].delay_ms)) {
            m_frame_timer = 0;
            m_current_frame = (m_current_frame + 1) % m_frames.size();
        }
    }
    
    // Update loading dots animation
    m_dot_timer += delta_time;
    if (m_dot_timer >= 500) { // Update dots every 500ms
        m_dot_timer = 0;
        m_dot_count = (m_dot_count + 1) % 4;
    }
    
    // Update loading timer
    m_loadingTimer += delta_time;
    if (m_loadingTimer >= m_loadingDuration) {
        m_complete = true;
    }
}

void LoadingScreen::renderText(SDL_Renderer* renderer, const char* text, int x, int y, int scale) {
    int cursor_x = x;
    
    for (const char* p = text; *p; p++) {
        int idx = charToIndex(*p);
        const uint8_t* glyph = FONT_DATA[idx];
        
        for (int col = 0; col < 5; col++) {
            for (int row = 0; row < 7; row++) {
                if (glyph[col] & (1 << row)) {
                    SDL_Rect pixel = {
                        cursor_x + col * scale,
                        y + row * scale,
                        scale,
                        scale
                    };
                    SDL_RenderFillRect(renderer, &pixel);
                }
            }
        }
        cursor_x += 6 * scale; // 5 pixels + 1 spacing
    }
}

void LoadingScreen::render(SDL_Renderer* renderer, int screen_width, int screen_height) {
    // Clear with black
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    
    // Render loading GIF in bottom left
    if (!m_frames.empty() && m_frames[m_current_frame].texture) {
        int gif_w, gif_h;
        SDL_QueryTexture(m_frames[m_current_frame].texture, nullptr, nullptr, &gif_w, &gif_h);
        
        // Scale to reasonable size (max 96px)
        int max_size = 96;
        float scale = 1.0f;
        if (gif_w > max_size || gif_h > max_size) {
            scale = static_cast<float>(max_size) / std::max(gif_w, gif_h);
        }
        
        int dest_w = static_cast<int>(gif_w * scale);
        int dest_h = static_cast<int>(gif_h * scale);
        
        SDL_Rect dest = {
            16, // Padding from left
            screen_height - dest_h - 16, // Padding from bottom
            dest_w,
            dest_h
        };
        
        SDL_RenderCopy(renderer, m_frames[m_current_frame].texture, nullptr, &dest);
    }
    
    // Render "NOW LOADING" text in bottom right (Sonic 06 style - white text)
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    
    // Build the text with animated dots
    char loading_text[32] = "NOW LOADING";
    for (int i = 0; i < m_dot_count; i++) {
        strcat(loading_text, ".");
    }
    
    int text_scale = 3;
    int text_width = static_cast<int>(strlen(loading_text)) * 6 * text_scale;
    int text_x = screen_width - text_width - 16;
    int text_y = screen_height - 7 * text_scale - 16;
    
    renderText(renderer, loading_text, text_x, text_y, text_scale);
}
