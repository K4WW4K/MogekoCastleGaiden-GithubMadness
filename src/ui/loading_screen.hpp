#pragma once

#include <SDL.h>
#include <SDL_image.h>
#include <string>
#include <vector>

// Animated GIF frame
struct GifFrame {
    SDL_Texture* texture = nullptr;
    int delay_ms = 100; // Frame delay in milliseconds
};

class LoadingScreen {
public:
    LoadingScreen();
    ~LoadingScreen();

    bool init(SDL_Renderer* renderer);
    void cleanup();
    
    void update(Uint32 delta_time);
    void render(SDL_Renderer* renderer, int screen_width, int screen_height);
    
    bool isComplete() const { return m_complete; }
    void setComplete(bool complete) { m_complete = complete; }

private:
    bool loadGif(SDL_Renderer* renderer, const std::string& path);
    void renderText(SDL_Renderer* renderer, const char* text, int x, int y, int scale);
    
    std::vector<GifFrame> m_frames;
    int m_current_frame = 0;
    Uint32 m_frame_timer = 0;
    
    bool m_complete = false;
    int m_dot_count = 0;
    Uint32 m_dot_timer = 0;
    
    // Loading duration (5-15 seconds)
    Uint32 m_loadingTimer = 0;
    Uint32 m_loadingDuration = 0; // Set randomly on init
};
