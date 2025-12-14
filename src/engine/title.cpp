#include "title.hpp"
#include <iostream>
#include <cmath>

TitleScreen::TitleScreen() = default;

TitleScreen::~TitleScreen() {
    cleanup();
}

bool TitleScreen::init(SDL_Renderer* renderer) {
    // Load background image
    SDL_Surface* bgSurface = IMG_Load("assets/sprites/bg/sample1.png");
    if (bgSurface) {
        m_background = SDL_CreateTextureFromSurface(renderer, bgSurface);
        SDL_FreeSurface(bgSurface);
        if (m_background) {
            std::cout << "Title background loaded: assets/sprites/bg/sample1.png" << std::endl;
        }
    } else {
        std::cerr << "Failed to load title background: " << IMG_GetError() << std::endl;
    }

    // Load font
    m_font = TTF_OpenFont("assets/fonts/determination.ttf", 24);
    if (!m_font) {
        std::cerr << "Failed to load title font: " << TTF_GetError() << std::endl;
    } else {
        std::cout << "Title font loaded: determination.ttf" << std::endl;
    }

#ifndef NO_AUDIO
    // Load sound effects
    m_selectSound = Mix_LoadWAV("assets/sounds/select.wav");
    if (!m_selectSound) {
        std::cerr << "Failed to load select.wav: " << Mix_GetError() << std::endl;
    }
    
    m_backSound = Mix_LoadWAV("assets/sounds/back.wav");
    if (!m_backSound) {
        std::cerr << "Failed to load back.wav: " << Mix_GetError() << std::endl;
    }
#endif

    std::cout << "Title screen initialized" << std::endl;
    return true;
}

void TitleScreen::cleanup() {
    if (m_background) {
        SDL_DestroyTexture(m_background);
        m_background = nullptr;
    }
    
    if (m_font) {
        TTF_CloseFont(m_font);
        m_font = nullptr;
    }
    
#ifndef NO_AUDIO
    if (m_selectSound) {
        Mix_FreeChunk(m_selectSound);
        m_selectSound = nullptr;
    }
    if (m_backSound) {
        Mix_FreeChunk(m_backSound);
        m_backSound = nullptr;
    }
#endif
}

void TitleScreen::handleKeyDown(SDL_Keycode key) {
    switch (key) {
        case SDLK_UP:
            m_selectedOption = (m_selectedOption - 1 + 3) % 3;
#ifndef NO_AUDIO
            if (m_selectSound) Mix_PlayChannel(-1, m_selectSound, 0);
#endif
            break;
        case SDLK_DOWN:
            m_selectedOption = (m_selectedOption + 1) % 3;
#ifndef NO_AUDIO
            if (m_selectSound) Mix_PlayChannel(-1, m_selectSound, 0);
#endif
            break;
        case SDLK_RETURN:
        case SDLK_SPACE:
        case SDLK_z:
            if (m_selectedOption == 0) {
                m_startGame = true;
#ifndef NO_AUDIO
                if (m_selectSound) Mix_PlayChannel(-1, m_selectSound, 0);
#endif
            }
            break;
        case SDLK_ESCAPE:
        case SDLK_x:
#ifndef NO_AUDIO
            if (m_backSound) Mix_PlayChannel(-1, m_backSound, 0);
#endif
            break;
    }
}

void TitleScreen::update(Uint32 delta_time) {
    m_animTimer += delta_time;
}

void TitleScreen::render(SDL_Renderer* renderer, int screen_width, int screen_height) {
    // Draw background image (or fallback to dark purple)
    if (m_background) {
        SDL_RenderCopy(renderer, m_background, nullptr, nullptr);
    } else {
        SDL_SetRenderDrawColor(renderer, 32, 16, 48, 255);
        SDL_RenderClear(renderer);
    }

    // Menu positioned in bottom-left corner
    const char* options[] = {"START", "LOAD", "EXIT"};
    int menuX = 20;
    int menuStartY = screen_height - 120;
    int lineHeight = 32;
    
    for (int i = 0; i < 3; i++) {
        int optionY = menuStartY + i * lineHeight;
        
        // Draw bullet point for selected option
        if (i == m_selectedOption) {
            // Pulsing bullet
            float pulse = 0.7f + 0.3f * static_cast<float>(sin(m_animTimer / 150.0));
            Uint8 brightness = static_cast<Uint8>(255 * pulse);
            SDL_SetRenderDrawColor(renderer, brightness, brightness, brightness, 255);
            
            // Diamond/bullet shape
            SDL_Rect bullet = {
                menuX,
                optionY + 8,
                8,
                8
            };
            SDL_RenderFillRect(renderer, &bullet);
        }
        
        // Render text
        if (m_font) {
            SDL_Color textColor;
            if (i == m_selectedOption) {
                textColor = {255, 255, 255, 255};
            } else {
                textColor = {180, 180, 200, 255};
            }
            
            SDL_Surface* textSurface = TTF_RenderText_Blended(m_font, options[i], textColor);
            if (textSurface) {
                SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                if (textTexture) {
                    SDL_Rect textRect = {
                        menuX + 16,
                        optionY,
                        textSurface->w,
                        textSurface->h
                    };
                    SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);
                    SDL_DestroyTexture(textTexture);
                }
                SDL_FreeSurface(textSurface);
            }
        } else {
            // Fallback: draw placeholder rectangles if font failed
            if (i == m_selectedOption) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 200, 200, 220, 255);
            }
            
            int textWidth = (i == 0) ? 60 : (i == 1) ? 50 : 40;
            SDL_Rect textRect = {
                menuX + 16,
                optionY + 4,
                textWidth,
                16
            };
            SDL_RenderFillRect(renderer, &textRect);
        }
    }
}
