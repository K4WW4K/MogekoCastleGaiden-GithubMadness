#pragma once

#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <vector>
#include <functional>

// Dialogue entry from JSON
struct DialogueEntry {
    std::string text;
    int index = 0;
};

// Dialogue file data
struct DialogueData {
    std::string event;
    int speed = 50; // ms per character
    std::vector<DialogueEntry> dialogue;
};

class DialogueSystem {
public:
    DialogueSystem();
    ~DialogueSystem();

    bool init(SDL_Renderer* renderer, const std::string& fontPath, int fontSize = 32);
    void cleanup();

    // Load dialogue from JSONC file
    bool loadDialogue(const std::string& path);
    
    // Start playing loaded dialogue
    void start();
    void stop();
    
    // Update and render
    void update(Uint32 delta_time);
    void render(SDL_Renderer* renderer, int screen_width, int screen_height);
    
    // Check if dialogue is active/complete
    bool isActive() const { return m_active; }
    bool isComplete() const { return m_complete; }
    
    // Skip current text animation or advance to next
    void advance();
    
    // Set callback for when dialogue completes
    void setOnComplete(std::function<void()> callback) { m_onComplete = callback; }

private:
    // Parse JSONC (JSON with comments)
    bool parseJsonc(const std::string& content);
    
    // Process text for special commands (\p[ms], \n, etc.)
    void processText();
    
    // Render text with proper wrapping
    void renderText(SDL_Renderer* renderer, const std::string& text, int x, int y, int maxWidth);

    TTF_Font* m_font = nullptr;
    DialogueData m_data;
    
    bool m_active = false;
    bool m_complete = false;
    
    int m_currentIndex = 0;
    std::string m_displayText;      // Text currently being displayed
    std::string m_fullText;         // Full text of current entry
    size_t m_charIndex = 0;         // Current character being revealed
    
    Uint32 m_charTimer = 0;         // Timer for character reveal
    Uint32 m_pauseTimer = 0;        // Timer for \p[ms] pauses
    bool m_isPaused = false;
    Uint32 m_pauseDuration = 0;
    
    bool m_textComplete = false;    // Current text fully revealed
    
    std::function<void()> m_onComplete;
    
    // Text color (can be changed with \c[color])
    SDL_Color m_textColor = {255, 255, 255, 255};
};
