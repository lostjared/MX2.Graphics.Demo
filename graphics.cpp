/* 

MX2 Graphics Demo
Coded by Jared Bruni
https://lostsidedead.biz
GPL v3

*/


#include"mx.hpp"
#include"argz.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/bind.h>
#include <GLES3/gl3.h>
#endif

#include<SDL2/SDL_image.h>

#include"gl.hpp"
#include"loadpng.hpp"
#include<iostream>
#include<vector>
#include<cstdint>
#include<algorithm>
#include"shaders.hpp"
#define CHECK_GL_ERROR() \
{ GLenum err = glGetError(); \
if (err != GL_NO_ERROR) \
printf("OpenGL Error: %d at %s:%d\n", err, __FILE__, __LINE__); }

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct ShaderInfo {
    const char* name;
    const char* source;
};

static const std::vector<ShaderInfo> shaderSources = {
    {"Bubble", srcShader1},
    {"Kaleidoscope", srcShader2},
    {"KaleidoscopeAlt", srcShader3},
    {"Scramble", srcShader4},
    {"Swirl", srcShader5},
    {"Mirror", szShader},
    {"VHS", szShader2},
    {"Twist", szTwist},
    {"Time", szTime},
    {"Bend", szBend},
    {"Pong", szPong},
    {"PsychWave", psychWave},
    {"CrystalBall", crystalBall},
    {"ZoomMouse", szZoomMouse},
    {"Drain", szDrain},
    {"Ufo", szUfo},
    {"Wave", szWave},
    {"UfoWarp", szUfoWarp},
    {"ByMouse", szByMouse},
    {"Deform", szDeform},
    {"Geo", szGeo},
    {"Smooth", szSmooth},
    {"Hue", szHue},
    {"kMouse", szkMouse},
    {"Drum", szDrum},
    {"ColorSwirl", szColorSwirl},
    {"MouseZoom", szMouseZoom},
    {"Rev2", szRev2},
    {"Fish", szFish},
    {"RipplePrism", szRipplePrism},
    {"MirrorMouse", szMirrorMouse},
    {"FracMouse", szFracMouse},
    {"Cats", szCats},
    {"ColorMouse", szColorMouse},
    {"TwistFull", szTwistFull},
    {"BowlByTime", szBowlByTime},
    {"Pebble", szPebble},
    {"MirrorPutty", szMirrorPutty}
};

class About : public gl::GLObject {
    GLuint texture = 0;
    gl::ShaderProgram shader;
    gl::GLSprite sprite;
    float animation = 0.0f;
    std::vector<std::unique_ptr<gl::ShaderProgram>> shaders;
    size_t currentShaderIndex = 0;
    Uint32 firstTapTime = 0;
    bool firstTapRegistered = false;
    static const Uint32 DOUBLE_TAP_MIN_TIME = 80;   
    static const Uint32 DOUBLE_TAP_MAX_TIME = 400;  
    Uint32 lastUpdateTime = 0;
    int texWidth = 0, texHeight = 0;
    uint64_t frameCount = 0;
    float beatValue = 0.0f;
    float audioLevel = 0.0f;
    float iSpeed = 1.0f;
    float iFrequency = 1.0f;
    float iAmplitude = 1.0f;
    float iHueShift = 0.0f;
    float iSaturation = 1.0f;
    float iBrightness = 1.0f;
    float iContrast = 1.0f;
    float iZoom = 1.0f;
    float iRotation = 0.0f;
    glm::vec3 iCameraPos = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec2 iMouseVelocity = glm::vec2(0.0f);
    float iMouseClick = 0.0f;
    float iSeconds = 0.0f;
    float iMinutes = 0.0f;
    float iHours = 0.0f;
    float iDebugMode = 0.0f;
    float iQuality = 1.0f;
    glm::vec2 prevMousePos = glm::vec2(0.0f);
    int maxWidth = 1920, maxHeight = 1080;  
    int canvasWidth = 1920, canvasHeight = 1080;
    int displayX = 0, displayY = 0;
    int displayW = 1920, displayH = 1080;
    bool captureNextFrame = false;
    int captureScale = 1;
public:
    About() = default;
    virtual ~About() override {
        if(texture != 0) {
            glDeleteTextures(1, &texture);
        }
    }

      void loadNewTexture(SDL_Surface *surface, gl::GLWindow *win) {
        if(texture != 0) {
            glDeleteTextures(1, &texture);
            texture = 0;
        }
        texture = gl::createTexture(surface, true);
        texWidth = surface->w;
        texHeight = surface->h;
        
        
        canvasWidth = maxWidth; 
        canvasHeight = maxHeight;
        
        printf("loadNewTexture: tex=%dx%d, canvas=%dx%d, max=%dx%d\n",
               texWidth, texHeight, canvasWidth, canvasHeight, maxWidth, maxHeight);

        float imgAspect = static_cast<float>(texWidth) / static_cast<float>(texHeight);
        float canvasAspect = static_cast<float>(canvasWidth) / static_cast<float>(canvasHeight);
        
        if (imgAspect > canvasAspect) {
            
            displayW = canvasWidth;
            displayH = static_cast<int>(canvasWidth / imgAspect);
            displayX = 0;
            displayY = (canvasHeight - displayH) / 2;
        } else {
            
            displayH = canvasHeight;
            displayW = static_cast<int>(canvasHeight * imgAspect);
            displayX = (canvasWidth - displayW) / 2;
            displayY = 0;
        }
        
        printf("Display: %d,%d %dx%d\n", displayX, displayY, displayW, displayH);
        
#ifdef __EMSCRIPTEN__
        emscripten_set_canvas_element_size("#canvas", canvasWidth, canvasHeight);
#endif
        win->w = canvasWidth;
        win->h = canvasHeight;
        glViewport(0, 0, canvasWidth, canvasHeight);
        
        sprite.initSize(canvasWidth, canvasHeight);
        switchShader(currentShaderIndex, win);
    }

    void load(gl::GLWindow *win) override {
        
        maxWidth = win->w;
        maxHeight = win->h;
        canvasWidth = win->w;
        canvasHeight = win->h;
        
        glViewport(0, 0, canvasWidth, canvasHeight);
        
        
        for(const auto& info : shaderSources) {
            auto shader = std::make_unique<gl::ShaderProgram>();
            if(!shader->loadProgramFromText(gl::vSource, info.source)) {
                mx::system_err << "Failed to load shader: " << info.name << "\n";
                continue;
            }
            shader->setSilent(true);
            shaders.push_back(std::move(shader));
        }
        
        if(shaders.empty()) {
            throw mx::Exception("No shaders loaded successfully");
        }
        
        
        std::string logoPath = win->util.getFilePath("data/logo.png");
        SDL_Surface *surface = IMG_Load(logoPath.c_str());
        if(!surface) {
            throw mx::Exception("Error loading logo.png");
        }
        
        SDL_Surface *converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
        SDL_FreeSurface(surface);
        
        if(!converted) {
            throw mx::Exception("Error converting logo surface");
        }
        
        lastUpdateTime = SDL_GetTicks();
        sprite.initSize(canvasWidth, canvasHeight);
        loadNewTexture(converted, win);
        SDL_FreeSurface(converted);
    }
  
    void switchShader(size_t index, gl::GLWindow *win) {
        if(index < shaders.size()) {
            currentShaderIndex = index;
            shaders[currentShaderIndex]->useProgram();
            shaders[currentShaderIndex]->setUniform("time_f", animation);
            shaders[currentShaderIndex]->setUniform("iTime", animation);
            shaders[currentShaderIndex]->setUniform("iTimeDelta", 0.0f);
            shaders[currentShaderIndex]->setUniform("iFrame", static_cast<float>(frameCount));
            shaders[currentShaderIndex]->setUniform("iSeconds", iSeconds);
            shaders[currentShaderIndex]->setUniform("iMinutes", iMinutes);
            shaders[currentShaderIndex]->setUniform("iHours", iHours);
            shaders[currentShaderIndex]->setUniform("iResolution", glm::vec2(displayW, displayH));
            glm::vec4 adjMouse = mouse;
            adjMouse.x -= displayX;
            adjMouse.y -= displayY;
            shaders[currentShaderIndex]->setUniform("iMouse", adjMouse);
            shaders[currentShaderIndex]->setUniform("iMouseNormalized", glm::vec2(adjMouse.x / displayW, 1.0f - adjMouse.y / displayH));
            
            shaders[currentShaderIndex]->setUniform("iMouseActive", mouse.z > 0.5f ? 1.0f : 0.0f);
            shaders[currentShaderIndex]->setUniform("iMouseVelocity", iMouseVelocity);
            shaders[currentShaderIndex]->setUniform("iMouseClick", iMouseClick);
            shaders[currentShaderIndex]->setUniform("iAspectRatio", static_cast<float>(displayW) / static_cast<float>(displayH));
            shaders[currentShaderIndex]->setUniform("iSpeed", iSpeed);
            shaders[currentShaderIndex]->setUniform("iFrequency", iFrequency);
            shaders[currentShaderIndex]->setUniform("iAmplitude", iAmplitude);
            shaders[currentShaderIndex]->setUniform("iHueShift", iHueShift);
            shaders[currentShaderIndex]->setUniform("iSaturation", iSaturation);
            shaders[currentShaderIndex]->setUniform("iBrightness", iBrightness);
            shaders[currentShaderIndex]->setUniform("iContrast", iContrast);
            shaders[currentShaderIndex]->setUniform("iZoom", iZoom);
            shaders[currentShaderIndex]->setUniform("iRotation", iRotation);
            shaders[currentShaderIndex]->setUniform("iCameraPos", iCameraPos);
            shaders[currentShaderIndex]->setUniform("iBeat", beatValue);
            shaders[currentShaderIndex]->setUniform("iAudioLevel", audioLevel);
            shaders[currentShaderIndex]->setUniform("iDebugMode", iDebugMode);
            shaders[currentShaderIndex]->setUniform("iQuality", iQuality);
            shaders[currentShaderIndex]->setUniform("alpha", 1.0f);
            shaders[currentShaderIndex]->setUniform("amp", 0.5f);
            shaders[currentShaderIndex]->setUniform("uamp", 0.5f);
            shaders[currentShaderIndex]->setUniform("textTexture", 0);
            glActiveTexture(GL_TEXTURE0);
            sprite.initWithTexture(shaders[currentShaderIndex].get(), texture, displayX, displayY, displayW, displayH);
        }
    }
    void nextShader(gl::GLWindow *win) {
        currentShaderIndex = (currentShaderIndex + 1) % shaders.size();
        switchShader(currentShaderIndex, win);
        mx::system_out << "Switched to shader: " << shaderSources[currentShaderIndex].name << "\n";
    }
    
    void prevShader(gl::GLWindow *win) {
        currentShaderIndex = (currentShaderIndex == 0) ? shaders.size() - 1 : currentShaderIndex - 1;
        switchShader(currentShaderIndex, win);
        mx::system_out << "Switched to shader: " << shaderSources[currentShaderIndex].name << "\n";
    }

    void draw(gl::GLWindow *win) override {
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f;
        lastUpdateTime = currentTime;
        animation += deltaTime * iSpeed;
        frameCount++;
        beatValue = 0.5f + 0.5f * sinf(animation * 2.0f * M_PI);
        audioLevel = 0.3f + 0.7f * (0.5f + 0.5f * sinf(animation * 0.5f));
        iSeconds = fmodf(animation, 60.0f);
        iMinutes = fmodf(animation / 60.0f, 60.0f);
        iHours = fmodf(animation / 3600.0f, 24.0f);
        glm::vec2 currentMousePos(mouse.x, mouse.y);
        iMouseVelocity = currentMousePos - prevMousePos;
        prevMousePos = currentMousePos;
        iMouseClick = mouse.z > 0.5f ? 1.0f : 0.0f;
        shaders[currentShaderIndex]->useProgram();
        shaders[currentShaderIndex]->setUniform("time_f", animation);
        shaders[currentShaderIndex]->setUniform("iTime", animation);
        shaders[currentShaderIndex]->setUniform("iTimeDelta", deltaTime);
        shaders[currentShaderIndex]->setUniform("iFrame", static_cast<float>(frameCount));
        shaders[currentShaderIndex]->setUniform("iSeconds", iSeconds);
        shaders[currentShaderIndex]->setUniform("iMinutes", iMinutes);
        shaders[currentShaderIndex]->setUniform("iHours", iHours);
        shaders[currentShaderIndex]->setUniform("iResolution", glm::vec2(displayW, displayH));
        glm::vec4 adjMouse = mouse;
        adjMouse.x -= displayX;
        adjMouse.y -= displayY;
        shaders[currentShaderIndex]->setUniform("iMouse", adjMouse);
        shaders[currentShaderIndex]->setUniform("iMouseNormalized", glm::vec2(adjMouse.x / displayW, 1.0f - adjMouse.y / displayH));
        
        shaders[currentShaderIndex]->setUniform("iMouseActive", iMouseClick);
        shaders[currentShaderIndex]->setUniform("iMouseVelocity", iMouseVelocity);
        shaders[currentShaderIndex]->setUniform("iMouseClick", iMouseClick);
        shaders[currentShaderIndex]->setUniform("iAspectRatio", static_cast<float>(displayW) / static_cast<float>(displayH));
        shaders[currentShaderIndex]->setUniform("iSpeed", iSpeed);
        shaders[currentShaderIndex]->setUniform("iFrequency", iFrequency);
        shaders[currentShaderIndex]->setUniform("iAmplitude", iAmplitude);
        shaders[currentShaderIndex]->setUniform("iHueShift", iHueShift);
        shaders[currentShaderIndex]->setUniform("iSaturation", iSaturation);
        shaders[currentShaderIndex]->setUniform("iBrightness", iBrightness);
        shaders[currentShaderIndex]->setUniform("iContrast", iContrast);
        shaders[currentShaderIndex]->setUniform("iZoom", iZoom);
        shaders[currentShaderIndex]->setUniform("iRotation", iRotation);
        shaders[currentShaderIndex]->setUniform("iCameraPos", iCameraPos);
        shaders[currentShaderIndex]->setUniform("iBeat", beatValue);
        shaders[currentShaderIndex]->setUniform("iAudioLevel", audioLevel);
        shaders[currentShaderIndex]->setUniform("iDebugMode", iDebugMode);
        shaders[currentShaderIndex]->setUniform("iQuality", iQuality);
        
        shaders[currentShaderIndex]->setUniform("alpha", 1.0f);
        shaders[currentShaderIndex]->setUniform("amp", 0.5f);
        shaders[currentShaderIndex]->setUniform("uamp", 0.5f);
        shaders[currentShaderIndex]->setUniform("textTexture", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        update(deltaTime);
        sprite.initSize(canvasWidth, canvasHeight);
        sprite.draw();
        if (captureNextFrame) {
            captureNextFrame = false;
            captureFrame();
        }
    }

    bool mouseDown = false;
    glm::vec4 mouse = glm::vec4(0.0f);

    void reset() {
        animation = 0.0f;
    }

    void setSpeed(float value) { iSpeed = value; }
    void setAmplitude(float value) { iAmplitude = value; }
    void setFrequency(float value) { iFrequency = value; }
    void setBrightness(float value) { iBrightness = value; }
    void setContrast(float value) { iContrast = value; }
    void setSaturation(float value) { iSaturation = value; }
    void setHueShift(float value) { iHueShift = value; }
    void setZoom(float value) { iZoom = value; }
    void setRotation(float value) { iRotation = value; }
    void setQuality(float value) { iQuality = value; }
    void setDebugMode(bool value) { iDebugMode = value ? 1.0f : 0.0f; }
    
    float getSpeed() const { return iSpeed; }
    float getAmplitude() const { return iAmplitude; }
    float getFrequency() const { return iFrequency; }
    float getBrightness() const { return iBrightness; }
    float getContrast() const { return iContrast; }
    float getSaturation() const { return iSaturation; }
    float getHueShift() const { return iHueShift; }
    float getZoom() const { return iZoom; }
    float getRotation() const { return iRotation; }
    float getQuality() const { return iQuality; }
    bool getDebugMode() const { return iDebugMode > 0.5f; }
    int getCanvasWidth() const { return canvasWidth; }
    int getCanvasHeight() const { return canvasHeight; }
    int getDisplayX() const { return displayX; }
    int getDisplayY() const { return displayY; }
    int getDisplayWidth() const { return displayW; }
    int getDisplayHeight() const { return displayH; }

    

    void resize(gl::GLWindow *win) {
        maxWidth = win->w;
        maxHeight = win->h;
        canvasWidth = win->w;
        canvasHeight = win->h;
        
        glViewport(0, 0, canvasWidth, canvasHeight);
        
        
        if (texWidth > 0 && texHeight > 0) {
            float imgAspect = static_cast<float>(texWidth) / static_cast<float>(texHeight);
            float canvasAspect = static_cast<float>(canvasWidth) / static_cast<float>(canvasHeight);
            
            if (imgAspect > canvasAspect) {
                displayW = canvasWidth;
                displayH = static_cast<int>(canvasWidth / imgAspect);
                displayX = 0;
                displayY = (canvasHeight - displayH) / 2;
            } else {
                displayH = canvasHeight;
                displayW = static_cast<int>(canvasHeight * imgAspect);
                displayX = (canvasWidth - displayW) / 2;
                displayY = 0;
            }
            
            printf("resize: canvas=%dx%d, display=(%d,%d) %dx%d\n",
                   canvasWidth, canvasHeight, displayX, displayY, displayW, displayH);
            
            sprite.initSize(canvasWidth, canvasHeight);
            switchShader(currentShaderIndex, win);
        }
    }

    void event(gl::GLWindow *win, SDL_Event &e) override {
        switch(e.type) {
            case SDL_FINGERDOWN: {
                Uint32 currentTime = SDL_GetTicks();
                if(firstTapRegistered) {
                    Uint32 timeSinceFirstTap = currentTime - firstTapTime;
                    if(timeSinceFirstTap >= DOUBLE_TAP_MIN_TIME && 
                       timeSinceFirstTap <= DOUBLE_TAP_MAX_TIME) {
                        nextShader(win);
                        firstTapRegistered = false;
                        firstTapTime = 0;
                    } else if(timeSinceFirstTap > DOUBLE_TAP_MAX_TIME) {
                        firstTapTime = currentTime;
                        firstTapRegistered = true;
                    }
                } else {
                    firstTapTime = currentTime;
                    firstTapRegistered = true;
                }
                mouse.x = e.tfinger.x * static_cast<float>(win->w);
                mouse.y = (1.0f - e.tfinger.y) * static_cast<float>(win->h);
                mouse.z = 1.0f;
                mouse.w = 1.0f;
                break;
            }
            case SDL_FINGERUP: {
                mouse.z = 0.0f;
                mouse.w = 0.0f;
                break;
            }
            case SDL_FINGERMOTION: {
                mouse.x = e.tfinger.x * win->w;
                mouse.y = (1.0f - e.tfinger.y) * win->h;
                mouse.z = 1.0f;
                break;
            }
            case SDL_MOUSEBUTTONDOWN:
                if(e.button.button == SDL_BUTTON_LEFT) {
                    mouseDown = true;
                    mouse.x = static_cast<float>(e.button.x);
                    mouse.y = static_cast<float>(win->h - e.button.y);
                    mouse.z = 1.0f;  
                    mouse.w = 1.0f;  
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if(e.button.button == SDL_BUTTON_LEFT) {
                    mouseDown = false;
                    mouse.z = 0.0f;  
                    mouse.w = 0.0f;
                }
                break;
            case SDL_MOUSEMOTION:
                mouse.x = static_cast<float>(e.motion.x);
                mouse.y = static_cast<float>(win->h - e.motion.y);
                if(mouseDown) {
                    mouse.z = 1.0f;
                }
                break;
        case SDL_KEYDOWN:
            if(e.key.keysym.sym == SDLK_DOWN || e.key.keysym.sym == SDLK_SPACE) {
                nextShader(win);
            }
            else if(e.key.keysym.sym == SDLK_UP || e.key.keysym.sym == SDLK_BACKSPACE) {
                prevShader(win);
            }
            break;
        }
    }
    void update(float deltaTime) {
        if(firstTapRegistered) {
            Uint32 currentTime = SDL_GetTicks();
            if((currentTime - firstTapTime) > DOUBLE_TAP_MAX_TIME) {
                firstTapRegistered = false;
                firstTapTime = 0;
            }
        }
    }


    std::string compileCustomShader(const std::string &fragmentSource, gl::GLWindow *win) {
        GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
        const char *src = fragmentSource.c_str();
        glShaderSource(fragShader, 1, &src, nullptr);
        glCompileShader(fragShader);
        
        GLint success;
        glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
        
        if (!success) {
            GLchar infoLog[1024];
            glGetShaderInfoLog(fragShader, 1024, nullptr, infoLog);
            glDeleteShader(fragShader);
            return std::string("ERROR: Fragment shader compilation failed:\n") + infoLog;
        }
        
        GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertShader, 1, &gl::vSource, nullptr);
        glCompileShader(vertShader);
        
        glGetShaderiv(vertShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            GLchar infoLog[1024];
            glGetShaderInfoLog(vertShader, 1024, nullptr, infoLog);
            glDeleteShader(fragShader);
            glDeleteShader(vertShader);
            return std::string("ERROR: Vertex shader compilation failed:\n") + infoLog;
        }
        
        GLuint program = glCreateProgram();
        glAttachShader(program, vertShader);
        glAttachShader(program, fragShader);
        glLinkProgram(program);
        
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            GLchar infoLog[1024];
            glGetProgramInfoLog(program, 1024, nullptr, infoLog);
            glDeleteShader(fragShader);
            glDeleteShader(vertShader);
            glDeleteProgram(program);
            return std::string("ERROR: Shader program linking failed:\n") + infoLog;
        }
        
        glDeleteShader(fragShader);
        glDeleteShader(vertShader);
        glDeleteProgram(program);
        
        auto customShader = std::make_unique<gl::ShaderProgram>();
        if(!customShader->loadProgramFromText(gl::vSource, fragmentSource.c_str())) {
            return "ERROR: Failed to create shader program.";
        }
        customShader->setSilent(true);
        
        static bool hasCustomShader = false;
        if(hasCustomShader && !shaders.empty()) {
            shaders.back() = std::move(customShader);
        } else {
            shaders.push_back(std::move(customShader));
            hasCustomShader = true;
        }   
        currentShaderIndex = shaders.size() - 1;
        switchShader(currentShaderIndex, win);
        return "SUCCESS: Shader compiled and applied successfully!";
    }
    void resetToDefaultShader(gl::GLWindow *win) {
        if(!shaders.empty()) {
            currentShaderIndex = 0;
            switchShader(0, win);
        }
    }

    void saveImage(int scale) {
        captureNextFrame = true;
        captureScale = scale;
    }


    

    void captureFrame() {
        int readW = canvasWidth;
        int readH = canvasHeight;
        
        printf("Capturing: canvas=%dx%d, display=(%d,%d) %dx%d, scale=%d\n", 
               readW, readH, displayX, displayY, displayW, displayH, captureScale);
        
        if (readW <= 0 || readH <= 0 || displayW <= 0 || displayH <= 0) {
            printf("Invalid dimensions\n");
            return;
        }
        
        
        std::vector<uint8_t> pixels(readW * readH * 4);
        glReadPixels(0, 0, readW, readH, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            printf("glReadPixels error: %d\n", err);
            return;
        }
        
        
        int stride = readW * 4;
        std::vector<uint8_t> row(stride);
        for (int y = 0; y < readH / 2; ++y) {
            uint8_t* top = pixels.data() + y * stride;
            uint8_t* bot = pixels.data() + (readH - 1 - y) * stride;
            memcpy(row.data(), top, stride);
            memcpy(top, bot, stride);
            memcpy(bot, row.data(), stride);
        }
        
        
        int cropX = displayX;
        int cropY = 0; 
        int cropW = displayW;
        int cropH = displayH;
        
        printf("Crop: x=%d, y=%d, w=%d, h=%d\n", cropX, cropY, cropW, cropH);
        
        if (cropW <= 0 || cropH <= 0) {
            printf("Invalid crop dimensions\n");
            return;
        }
        
        
        std::vector<uint8_t> croppedPixels(cropW * cropH * 4);
        
        for (int y = 0; y < cropH; ++y) {
            int srcY = cropY + y;
            int srcOffset = (srcY * readW + cropX) * 4;
            int dstOffset = y * cropW * 4;
            memcpy(croppedPixels.data() + dstOffset, pixels.data() + srcOffset, cropW * 4);
        }
        
        
        int finalW = cropW * captureScale;
        int finalH = cropH * captureScale;
        
        printf("Output: %dx%d (scale %d)\n", finalW, finalH, captureScale);
        
#ifdef __EMSCRIPTEN__
        EM_ASM({
            var cropW = $0;
            var cropH = $1;
            var dataPtr = $2;
            var finalW = $3;
            var finalH = $4;
            
            try {
                var srcCanvas = document.createElement('canvas');
                srcCanvas.width = cropW;
                srcCanvas.height = cropH;
                var srcCtx = srcCanvas.getContext('2d');
                
                var pixelArray = new Uint8ClampedArray(cropW * cropH * 4);
                for (var i = 0; i < cropW * cropH * 4; i++) {
                    pixelArray[i] = Module.HEAPU8[dataPtr + i];
                }
                
                var imageData = new ImageData(pixelArray, cropW, cropH);
                srcCtx.putImageData(imageData, 0, 0);
                
                var outCanvas = document.createElement('canvas');
                outCanvas.width = finalW;
                outCanvas.height = finalH;
                var outCtx = outCanvas.getContext('2d');
                
                outCtx.imageSmoothingEnabled = true;
                outCtx.imageSmoothingQuality = 'high';
                
                outCtx.drawImage(srcCanvas, 0, 0, finalW, finalH);
                
                var now = new Date();
                var timestamp = now.getFullYear() + 
                    String(now.getMonth() + 1).padStart(2, '0') + 
                    String(now.getDate()).padStart(2, '0') + '_' +
                    String(now.getHours()).padStart(2, '0') + 
                    String(now.getMinutes()).padStart(2, '0') + 
                    String(now.getSeconds()).padStart(2, '0');
                
                var link = document.createElement('a');
                link.download = 'mx2_shader_' + finalW + 'x' + finalH + '_' + timestamp + '.png';
                link.href = outCanvas.toDataURL('image/png');
                document.body.appendChild(link);
                link.click();
                document.body.removeChild(link);
                
                console.log('Image saved:', finalW, 'x', finalH);
            } catch (e) {
                console.error('Save error:', e);
            }
        }, cropW, cropH, croppedPixels.data(), finalW, finalH);
#endif
        printf("Frame captured and saved\n");
    }
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("MX2 - Interactive Graphics Demo", tw, th) {
        setPath(path);
        setObject(new About());
        object->load(this);
    }
    
    ~MainWindow() override {}
    
    virtual void event(SDL_Event &e) override {
        object->event(this, e);
    }
    
    virtual void draw() override {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glViewport(0, 0, w, h);
        object->draw(this);
        swap();
        delay();
    }
};

MainWindow *main_w = nullptr;
About *about_ptr = nullptr;

#ifdef __EMSCRIPTEN__

    void resizeWeb() {
        if (about_ptr && main_w) {
            int w, h;
            emscripten_get_canvas_element_size("#canvas", &w, &h);
            printf("resizeWeb: new canvas size %dx%d\n", w, h);
            main_w->w = w;
            main_w->h = h;
            about_ptr->resize(main_w);
        }
    }

    void saveImageWeb(int scale) {
        if (about_ptr) {
            about_ptr->saveImage(scale);
        }
    }

    void nextShaderWeb() {
        if(about_ptr) {
            about_ptr->nextShader(main_w);
        }
    }

    void prevShaderWeb() {
        if(about_ptr) {
            about_ptr->prevShader(main_w);
        }
    }

    void loadImagePNG(const std::vector<uint8_t>& imageData) {
        std::ofstream outFile("image.png", std::ios::binary);
        outFile.write(reinterpret_cast<const char*>(imageData.data()), imageData.size());
        outFile.close();
        
        SDL_Surface *surface = png::LoadPNG("image.png");
        if(!surface) {
            mx::system_err << "Failed to load PNG image\n";
            return;
        }
        
        SDL_Surface *converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
        SDL_FreeSurface(surface);
        
        if(converted && main_w && main_w->object) {
            About *about = dynamic_cast<About*>(main_w->object.get());
            if(about) {
                about->loadNewTexture(converted, main_w);
            }
        }
        if(converted) {
            SDL_FreeSurface(converted);
        }
    }

    void loadImageJPG(const std::vector<uint8_t>& imageData) {
        std::ofstream outFile("image.jpg", std::ios::binary);
        outFile.write(reinterpret_cast<const char*>(imageData.data()), imageData.size());
        outFile.close();
        SDL_Surface *surface = IMG_Load("image.jpg");
        if(!surface) {
            mx::system_err << "Failed to load JPG image: " << IMG_GetError() << "\n";
            return;
        }
    
        SDL_Surface *converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
        SDL_FreeSurface(surface);
        
        if(converted && main_w && main_w->object) {
            About *about = dynamic_cast<About*>(main_w->object.get());
            if(about) {
                about->loadNewTexture(converted, main_w);
            }
        }
        if(converted) {
            SDL_FreeSurface(converted);
        }
    }

     std::string compileCustomShaderWeb(const std::string &fragmentSource) {
        if(about_ptr && main_w) {
            return about_ptr->compileCustomShader(fragmentSource, main_w);
        }
        return "ERROR: Application not initialized";
    }

    void resetToDefaultShaderWeb() {
        if(about_ptr && main_w) {
            about_ptr->resetToDefaultShader(main_w);
        }
    }

    void reset_time() {
        if(about_ptr && main_w) {
            about_ptr->reset();
        }
    }

    void setUniformSpeed(float value) {
        if(about_ptr) about_ptr->setSpeed(value);
    }
    
    void setUniformAmplitude(float value) {
        if(about_ptr) about_ptr->setAmplitude(value);
    }
    
    void setUniformFrequency(float value) {
        if(about_ptr) about_ptr->setFrequency(value);
    }
    
    void setUniformBrightness(float value) {
        if(about_ptr) about_ptr->setBrightness(value);
    }
    
    void setUniformContrast(float value) {
        if(about_ptr) about_ptr->setContrast(value);
    }
    
    void setUniformSaturation(float value) {
        if(about_ptr) about_ptr->setSaturation(value);
    }
    
    void setUniformHueShift(float value) {
        if(about_ptr) about_ptr->setHueShift(value);
    }
    
    void setUniformZoom(float value) {
        if(about_ptr) about_ptr->setZoom(value);
    }
    
    void setUniformRotation(float value) {
        if(about_ptr) about_ptr->setRotation(value);
    }
    
    void setUniformQuality(float value) {
        if(about_ptr) about_ptr->setQuality(value);
    }
    
    void setUniformDebugMode(bool value) {
        if(about_ptr) about_ptr->setDebugMode(value);
    }


    float getUniformSpeed() {
        if(about_ptr) return about_ptr->getSpeed();
        return 1.0f;
    }
    
    float getUniformAmplitude() {
        if(about_ptr) return about_ptr->getAmplitude();
        return 1.0f;
    }
    
    float getUniformFrequency() {
        if(about_ptr) return about_ptr->getFrequency();
        return 1.0f;
    }
    
    float getUniformBrightness() {
        if(about_ptr) return about_ptr->getBrightness();
        return 1.0f;
    }
    
    float getUniformContrast() {
        if(about_ptr) return about_ptr->getContrast();
        return 1.0f;
    }
    
    float getUniformSaturation() {
        if(about_ptr) return about_ptr->getSaturation();
        return 1.0f;
    }
    
    float getUniformHueShift() {
        if(about_ptr) return about_ptr->getHueShift();
        return 0.0f;
    }
    
    float getUniformZoom() {
        if(about_ptr) return about_ptr->getZoom();
        return 1.0f;
    }
    
    float getUniformRotation() {
        if(about_ptr) return about_ptr->getRotation();
        return 0.0f;
    }
    
    float getUniformQuality() {
        if(about_ptr) return about_ptr->getQuality();
        return 1.0f;
    }
    
    bool getUniformDebugMode() {
        if(about_ptr) return about_ptr->getDebugMode();
        return false;
    }
    
    int getCanvasWidthWeb() {
        if(about_ptr) return about_ptr->getCanvasWidth();
        return 1280;
    }
    
    int getCanvasHeightWeb() {
        if(about_ptr) return about_ptr->getCanvasHeight();
        return 720;
    }

    
    int getDisplayXWeb() {
        if(about_ptr) return about_ptr->getDisplayX();
        return 0;
    }
    
    int getDisplayYWeb() {
        if(about_ptr) return about_ptr->getDisplayY();
        return 0;
    }
    
    int getDisplayWidthWeb() {
        if(about_ptr) return about_ptr->getDisplayWidth();
        return 1280;
    }
    
    int getDisplayHeightWeb() {
        if(about_ptr) return about_ptr->getDisplayHeight();
        return 720;
    }

    EMSCRIPTEN_BINDINGS(image_loader) {
        emscripten::function("nextShaderWeb", &nextShaderWeb);
        emscripten::function("prevShaderWeb", &prevShaderWeb);
        emscripten::function("compileCustomShader", &compileCustomShaderWeb);
        emscripten::function("resetToDefaultShader", &resetToDefaultShaderWeb);
        emscripten::register_vector<uint8_t>("VectorU8");
        emscripten::function("loadImagePNG", &loadImagePNG);
        emscripten::function("loadImageJPG", &loadImageJPG);
        emscripten::function("reset_time", &reset_time);
        emscripten::function("setUniformSpeed", &setUniformSpeed);
        emscripten::function("setUniformAmplitude", &setUniformAmplitude);
        emscripten::function("setUniformFrequency", &setUniformFrequency);
        emscripten::function("setUniformBrightness", &setUniformBrightness);
        emscripten::function("setUniformContrast", &setUniformContrast);
        emscripten::function("setUniformSaturation", &setUniformSaturation);
        emscripten::function("setUniformHueShift", &setUniformHueShift);
        emscripten::function("setUniformZoom", &setUniformZoom);
        emscripten::function("setUniformRotation", &setUniformRotation);
        emscripten::function("setUniformQuality", &setUniformQuality);
        emscripten::function("setUniformDebugMode", &setUniformDebugMode);
        emscripten::function("getUniformSpeed", &getUniformSpeed);
        emscripten::function("getUniformAmplitude", &getUniformAmplitude);
        emscripten::function("getUniformFrequency", &getUniformFrequency);
        emscripten::function("getUniformBrightness", &getUniformBrightness);
        emscripten::function("getUniformContrast", &getUniformContrast);
        emscripten::function("getUniformSaturation", &getUniformSaturation);
        emscripten::function("getUniformHueShift", &getUniformHueShift);
        emscripten::function("getUniformZoom", &getUniformZoom);
        emscripten::function("getUniformRotation", &getUniformRotation);
        emscripten::function("getUniformQuality", &getUniformQuality);
        emscripten::function("getUniformDebugMode", &getUniformDebugMode);
        emscripten::function("getCanvasWidth", &getCanvasWidthWeb);
        emscripten::function("getCanvasHeight", &getCanvasHeightWeb);
        emscripten::function("getDisplayX", &getDisplayXWeb);
        emscripten::function("getDisplayY", &getDisplayYWeb);
        emscripten::function("getDisplayWidth", &getDisplayWidthWeb);
        emscripten::function("getDisplayHeight", &getDisplayHeightWeb);
        emscripten::function("saveImage", &saveImageWeb);
        emscripten::function("resize", &resizeWeb);
    };

#endif

void eventProc() {
    main_w->proc();
}

int main(int argc, char **argv) {
#ifdef __EMSCRIPTEN__
    try {
        MainWindow main_window("/", 1920, 1080);
        main_w =&main_window;
        about_ptr = dynamic_cast<About *>(main_w->object.get());
        emscripten_set_main_loop(eventProc, 0, 1);
    } catch (mx::Exception &e) {
        std::cerr << e.text() << "\n";
        return EXIT_FAILURE;
    }  
#endif
    return 0;
}