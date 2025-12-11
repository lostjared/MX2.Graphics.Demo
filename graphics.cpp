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
        sprite.initSize(win->w, win->h);
        switchShader(currentShaderIndex, win);
    }

    void load(gl::GLWindow *win) override {
        texture = gl::loadTexture(win->util.getFilePath("data/logo.png"));
        if(texture == 0) {
            throw mx::Exception("Error loading texture");
        }
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
        sprite.initSize(win->w, win->h);
        lastUpdateTime = SDL_GetTicks();
        switchShader(0, win);
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
            shaders[currentShaderIndex]->setUniform("iResolution", glm::vec2(win->w, win->h));
            shaders[currentShaderIndex]->setUniform("iMouse", mouse);
            shaders[currentShaderIndex]->setUniform("iMouseNormalized", glm::vec2(mouse.x / win->w, 1.0f - mouse.y / win->h));
            shaders[currentShaderIndex]->setUniform("iMouseActive", mouse.z > 0.5f ? 1.0f : 0.0f);
            shaders[currentShaderIndex]->setUniform("iMouseVelocity", iMouseVelocity);
            shaders[currentShaderIndex]->setUniform("iMouseClick", iMouseClick);
            shaders[currentShaderIndex]->setUniform("iAspectRatio", static_cast<float>(win->w) / static_cast<float>(win->h));
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
            sprite.initSize(win->w, win->h);
            sprite.initWithTexture(shaders[currentShaderIndex].get(), texture, 0, 0, win->w, win->h);
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
        shaders[currentShaderIndex]->setUniform("iResolution", glm::vec2(win->w, win->h));
        shaders[currentShaderIndex]->setUniform("iMouse", mouse);
        shaders[currentShaderIndex]->setUniform("iMouseNormalized", glm::vec2(mouse.x / win->w, 1.0f - mouse.y / win->h));
        shaders[currentShaderIndex]->setUniform("iMouseActive", iMouseClick);
        shaders[currentShaderIndex]->setUniform("iMouseVelocity", iMouseVelocity);
        shaders[currentShaderIndex]->setUniform("iMouseClick", iMouseClick);
        shaders[currentShaderIndex]->setUniform("iAspectRatio", static_cast<float>(win->w) / static_cast<float>(win->h));
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
        sprite.draw();
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
    
private:
    gl::GLSprite sprite;
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
    };

#endif

void eventProc() {
    main_w->proc();
}

int main(int argc, char **argv) {
#ifdef __EMSCRIPTEN__
    try {
        MainWindow main_window("/", 1280, 720);
        main_w =&main_window;
        about_ptr = dynamic_cast<About *>(main_w->object.get());
        emscripten_set_main_loop(eventProc, 0, 1);
    } catch (mx::Exception &e) {
        std::cerr << e.text() << "\n";
        return EXIT_FAILURE;
    }
#else
    Argz<std::string> parser(argc, argv);
    parser.addOptionSingle('h', "Display help message")
        .addOptionSingleValue('p', "assets path")
        .addOptionDoubleValue('P', "path", "assets path")
        .addOptionSingleValue('r',"Resolution WidthxHeight")
        .addOptionDoubleValue('R',"resolution", "Resolution WidthxHeight");
    Argument<std::string> arg;
    std::string path;
    int value = 0;
    int tw = 960, th = 720;
    try {
        while((value = parser.proc(arg)) != -1) {
            switch(value) {
                case 'h':
                case 'v':
                    parser.help(std::cout);
                    exit(EXIT_SUCCESS);
                    break;
                case 'p':
                case 'P':
                    path = arg.arg_value;
                    break;
                case 'r':
                case 'R': {
                    auto pos = arg.arg_value.find("x");
                    if(pos == std::string::npos)  {
                        mx::system_err << "Error invalid resolution use WidthxHeight\n";
                        mx::system_err.flush();
                        exit(EXIT_FAILURE);
                    }
                    std::string left, right;
                    left = arg.arg_value.substr(0, pos);
                    right = arg.arg_value.substr(pos+1);
                    tw = atoi(left.c_str());
                    th = atoi(right.c_str());
                }
                    break;
            }
        }
    } catch (const ArgException<std::string>& e) {
        mx::system_err << e.text() << "\n";
    }
    if(path.empty()) {
        mx::system_out << "mx: No path provided trying default current directory.\n";
        path = ".";
    }
    try {
        MainWindow main_window(path, tw, th);
        main_window.loop();
    } catch(const mx::Exception &e) {
        mx::system_err << "mx: Exception: " << e.text() << "\n";
        mx::system_err.flush();
        exit(EXIT_FAILURE);
    }
#endif
    return 0;
}