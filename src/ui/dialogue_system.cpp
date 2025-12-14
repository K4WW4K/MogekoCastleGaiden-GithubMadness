#include "dialogue_system.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <regex>
#include <cstring>

DialogueSystem::DialogueSystem() = default;

DialogueSystem::~DialogueSystem() {
    cleanup();
}

bool DialogueSystem::init(SDL_Renderer* renderer, const std::string& fontPath, int fontSize) {
    (void)renderer;
    
    // Initialize SDL_ttf
    if (TTF_Init() == -1) {
        std::cerr << "SDL_ttf could not initialize! TTF_Error: " << TTF_GetError() << std::endl;
        return false;
    }

    // Load font
    m_font = TTF_OpenFont(fontPath.c_str(), fontSize);
    if (m_font == nullptr) {
        std::cerr << "Failed to load font '" << fontPath << "'! TTF_Error: " << TTF_GetError() << std::endl;
        return false;
    }

    std::cout << "Loaded font: " << fontPath << std::endl;
    return true;
}

void DialogueSystem::cleanup() {
    if (m_font != nullptr) {
        TTF_CloseFont(m_font);
        m_font = nullptr;
    }
    TTF_Quit();
}

bool DialogueSystem::loadDialogue(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open dialogue file: " << path << std::endl;
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    return parseJsonc(buffer.str());
}

bool DialogueSystem::parseJsonc(const std::string& content) {
    // Simple JSONC parser - remove comments first
    std::string json;
    bool inString = false;
    bool inLineComment = false;
    bool inBlockComment = false;
    
    for (size_t i = 0; i < content.size(); i++) {
        char c = content[i];
        char next = (i + 1 < content.size()) ? content[i + 1] : '\0';
        
        if (inLineComment) {
            if (c == '\n') {
                inLineComment = false;
                json += c;
            }
            continue;
        }
        
        if (inBlockComment) {
            if (c == '*' && next == '/') {
                inBlockComment = false;
                i++;
            }
            continue;
        }
        
        if (c == '"' && (i == 0 || content[i-1] != '\\')) {
            inString = !inString;
        }
        
        if (!inString) {
            if (c == '/' && next == '/') {
                inLineComment = true;
                continue;
            }
            if (c == '/' && next == '*') {
                inBlockComment = true;
                i++;
                continue;
            }
        }
        
        json += c;
    }
    
    // Very simple JSON parser for our specific format
    // Find "event"
    size_t pos = json.find("\"event\"");
    if (pos != std::string::npos) {
        pos = json.find(':', pos);
        pos = json.find('"', pos);
        size_t end = json.find('"', pos + 1);
        m_data.event = json.substr(pos + 1, end - pos - 1);
    }
    
    // Find "speed"
    pos = json.find("\"speed\"");
    if (pos != std::string::npos) {
        pos = json.find(':', pos);
        size_t start = pos + 1;
        while (start < json.size() && (json[start] == ' ' || json[start] == '\t')) start++;
        size_t end = start;
        while (end < json.size() && (json[end] >= '0' && json[end] <= '9')) end++;
        if (end > start) {
            m_data.speed = std::stoi(json.substr(start, end - start));
        }
    }
    
    // Find dialogue array
    pos = json.find("\"dialogue\"");
    if (pos != std::string::npos) {
        pos = json.find('[', pos);
        size_t arrayEnd = json.rfind(']');
        if (pos != std::string::npos && arrayEnd != std::string::npos) {
            std::string arrayContent = json.substr(pos + 1, arrayEnd - pos - 1);
            
            // Find each object in the array
            size_t objStart = 0;
            while ((objStart = arrayContent.find('{', objStart)) != std::string::npos) {
                size_t objEnd = arrayContent.find('}', objStart);
                if (objEnd == std::string::npos) break;
                
                std::string obj = arrayContent.substr(objStart, objEnd - objStart + 1);
                
                DialogueEntry entry;
                
                // Find "text"
                size_t textPos = obj.find("\"text\"");
                if (textPos != std::string::npos) {
                    textPos = obj.find(':', textPos);
                    textPos = obj.find('"', textPos);
                    // Handle escaped quotes
                    std::string text;
                    for (size_t i = textPos + 1; i < obj.size(); i++) {
                        if (obj[i] == '\\' && i + 1 < obj.size()) {
                            char next = obj[i + 1];
                            if (next == '"') { text += '"'; i++; }
                            else if (next == 'n') { text += '\n'; i++; }
                            else if (next == 'p') { text += "\\p"; i++; } // Keep \p for processing
                            else if (next == 'c') { text += "\\c"; i++; } // Keep \c for processing
                            else if (next == '\\') { text += '\\'; i++; }
                            else { text += obj[i]; }
                        } else if (obj[i] == '"') {
                            break;
                        } else {
                            text += obj[i];
                        }
                    }
                    entry.text = text;
                }
                
                // Find "index"
                size_t indexPos = obj.find("\"index\"");
                if (indexPos != std::string::npos) {
                    indexPos = obj.find(':', indexPos);
                    size_t start = indexPos + 1;
                    while (start < obj.size() && (obj[start] == ' ' || obj[start] == '\t')) start++;
                    size_t end = start;
                    while (end < obj.size() && (obj[end] >= '0' && obj[end] <= '9')) end++;
                    if (end > start) {
                        entry.index = std::stoi(obj.substr(start, end - start));
                    }
                }
                
                m_data.dialogue.push_back(entry);
                objStart = objEnd + 1;
            }
        }
    }
    
    std::cout << "Loaded dialogue '" << m_data.event << "' with " << m_data.dialogue.size() << " entries" << std::endl;
    return !m_data.dialogue.empty();
}

void DialogueSystem::start() {
    if (m_data.dialogue.empty()) return;
    
    m_active = true;
    m_complete = false;
    m_currentIndex = 0;
    m_charIndex = 0;
    m_displayText = "";
    m_fullText = m_data.dialogue[0].text;
    m_textComplete = false;
    m_isPaused = false;
    m_charTimer = 0;
}

void DialogueSystem::stop() {
    m_active = false;
    m_complete = true;
}

void DialogueSystem::advance() {
    if (!m_active) return;
    
    if (!m_textComplete) {
        // Reveal all text immediately
        m_displayText = "";
        // Strip pause commands for display
        for (size_t i = 0; i < m_fullText.size(); i++) {
            if (m_fullText[i] == '\\' && i + 1 < m_fullText.size() && m_fullText[i + 1] == 'p') {
                // Skip \p[xxx]
                i += 2;
                if (i < m_fullText.size() && m_fullText[i] == '[') {
                    while (i < m_fullText.size() && m_fullText[i] != ']') i++;
                }
            } else {
                m_displayText += m_fullText[i];
            }
        }
        m_charIndex = m_fullText.size();
        m_textComplete = true;
    } else {
        // Move to next dialogue entry
        m_currentIndex++;
        if (m_currentIndex >= static_cast<int>(m_data.dialogue.size()) || 
            m_data.dialogue[m_currentIndex].text.empty()) {
            // Dialogue complete
            m_active = false;
            m_complete = true;
            if (m_onComplete) m_onComplete();
        } else {
            m_fullText = m_data.dialogue[m_currentIndex].text;
            m_displayText = "";
            m_charIndex = 0;
            m_textComplete = false;
            m_isPaused = false;
        }
    }
}

void DialogueSystem::update(Uint32 delta_time) {
    if (!m_active || m_textComplete) return;
    
    // Handle pause
    if (m_isPaused) {
        m_pauseTimer += delta_time;
        if (m_pauseTimer >= m_pauseDuration) {
            m_isPaused = false;
            m_pauseTimer = 0;
        }
        return;
    }
    
    // Character reveal
    m_charTimer += delta_time;
    int charSpeed = m_data.speed > 0 ? m_data.speed : 50;
    
    while (m_charTimer >= static_cast<Uint32>(charSpeed) && m_charIndex < m_fullText.size()) {
        m_charTimer -= charSpeed;
        
        char c = m_fullText[m_charIndex];
        
        // Check for special commands
        if (c == '\\' && m_charIndex + 1 < m_fullText.size()) {
            char cmd = m_fullText[m_charIndex + 1];
            if (cmd == 'p' && m_charIndex + 2 < m_fullText.size() && m_fullText[m_charIndex + 2] == '[') {
                // Parse pause duration \p[ms]
                size_t closePos = m_fullText.find(']', m_charIndex + 3);
                if (closePos != std::string::npos) {
                    std::string msStr = m_fullText.substr(m_charIndex + 3, closePos - m_charIndex - 3);
                    m_pauseDuration = std::stoi(msStr);
                    m_isPaused = true;
                    m_pauseTimer = 0;
                    m_charIndex = closePos + 1;
                    return;
                }
            }
        }
        
        m_displayText += c;
        m_charIndex++;
    }
    
    if (m_charIndex >= m_fullText.size()) {
        m_textComplete = true;
    }
}

void DialogueSystem::renderText(SDL_Renderer* renderer, const std::string& text, int x, int y, int maxWidth) {
    if (m_font == nullptr || text.empty()) return;
    
    // Split by newlines first
    std::vector<std::string> lines;
    std::string currentLine;
    
    for (size_t i = 0; i < text.size(); i++) {
        if (text[i] == '\n') {
            lines.push_back(currentLine);
            currentLine = "";
        } else {
            currentLine += text[i];
        }
    }
    if (!currentLine.empty()) {
        lines.push_back(currentLine);
    }
    
    int lineHeight = TTF_FontLineSkip(m_font);
    int currentY = y;
    
    for (const auto& line : lines) {
        if (line.empty()) {
            currentY += lineHeight;
            continue;
        }
        
        SDL_Surface* surface = TTF_RenderUTF8_Blended(m_font, line.c_str(), m_textColor);
        if (surface) {
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            if (texture) {
                SDL_Rect destRect = {x, currentY, surface->w, surface->h};
                SDL_RenderCopy(renderer, texture, nullptr, &destRect);
                SDL_DestroyTexture(texture);
            }
            SDL_FreeSurface(surface);
        }
        currentY += lineHeight;
    }
}

void DialogueSystem::render(SDL_Renderer* renderer, int screen_width, int screen_height) {
    if (!m_active || m_font == nullptr) return;
    
    // Render semi-transparent background at bottom
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_Rect bgRect = {0, screen_height - 120, screen_width, 120};
    SDL_RenderFillRect(renderer, &bgRect);
    
    // Render text centered
    int padding = 32;
    renderText(renderer, m_displayText, padding, screen_height - 100, screen_width - padding * 2);
    
    // Render "press to continue" indicator if text complete
    if (m_textComplete) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, static_cast<Uint8>(128 + 127 * sin(SDL_GetTicks() / 200.0)));
        SDL_Rect indicator = {screen_width - 40, screen_height - 30, 10, 10};
        SDL_RenderFillRect(renderer, &indicator);
    }
}
