#pragma once

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#ifndef NO_AUDIO
#include <SDL_mixer_ext.h>
#endif

class TitleScreen {
public:
    TitleScreen();
    ~TitleScreen();

    bool init(SDL_Renderer* renderer);
    void cleanup();
    
    void update(Uint32 delta_time);
    void render(SDL_Renderer* renderer, int screen_width, int screen_height);
    
    // Check if user selected start game
    bool shouldStartGame() const { return m_startGame; }
    void reset() { m_startGame = false; }

    // Handle input
    void handleKeyDown(SDL_Keycode key);

private:
    bool m_startGame = false;
    int m_selectedOption = 0;
    
    // Animation
    Uint32 m_animTimer = 0;
    
    // Background
    SDL_Texture* m_background = nullptr;
    
    // Sound effects
#ifndef NO_AUDIO
    Mix_Chunk* m_selectSound = nullptr;
    Mix_Chunk* m_backSound = nullptr;
#endif

    // Font
    TTF_Font* m_font = nullptr;
};
