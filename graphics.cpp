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
#include<fstream>
#include<sstream>
#include<string>

#include"mirror_shaders.hpp"
#include"model.hpp"
#define CHECK_GL_ERROR() \
{ GLenum err = glGetError(); \
if (err != GL_NO_ERROR) \
printf("OpenGL Error: %d at %s:%d\n", err, __FILE__, __LINE__); }

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct ShaderInfo {
    std::string name;
    std::string source;
};

const char *sz3DVertex = R"(#version 300 es
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;

out vec3 vNormal;
out vec2 TexCoord;

uniform mat4 mv_matrix;
uniform mat4 proj_matrix;

void main() {
    gl_Position = proj_matrix * mv_matrix * vec4(position, 1.0);
    vNormal = normal;
    TexCoord = texCoord;
})";


static std::vector<ShaderInfo> shaderSources = {
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
    {"MirrorPutty", szMirrorPutty},
    {"frac_shader02_dmdi6i_zoom_xor", src_frac_shader02_dmdi6i_zoom_xor},
    {"frac_shader02_dmdi6i_zoom_xor_amp", src_frac_shader02_dmdi6i_zoom_xor_amp},
    {"frac_shader02_dmdXi", src_frac_shader02_dmdXi},
    {"frac_shader02_dmdXi_amp", src_frac_shader02_dmdXi_amp},
    {"frac_shader02_dmdXi2", src_frac_shader02_dmdXi2},
    {"frac_shader02_dmdXi3", src_frac_shader02_dmdXi3},
    {"frac_shader02_dmdXi3_drain", src_frac_shader02_dmdXi3_drain},
    {"frac_shader02_dmdXi3_warp", src_frac_shader02_dmdXi3_warp},
    {"frac_shader02_dmdXi4", src_frac_shader02_dmdXi4},
    {"frac_shader02_dmdXi5", src_frac_shader02_dmdXi5},
    {"frac_shader02_prisim", src_frac_shader02_prisim},
    {"frac_shader03_size", src_frac_shader03_size},
    {"frac_shader03_wormhole", src_frac_shader03_wormhole},
    {"frac_shader03_wormhole_amp", src_frac_shader03_wormhole_amp},
    {"frac_shader03_wormhole2", src_frac_shader03_wormhole2},
    {"frac_shader03_wormhole3", src_frac_shader03_wormhole3},
    {"frac_shader03_wormhole4", src_frac_shader03_wormhole4},
    {"frac_shader04_echo", src_frac_shader04_echo},
    {"frac_shader04_echo2", src_frac_shader04_echo2},
    {"frac_shader04_echo3_spin", src_frac_shader04_echo3_spin},
    {"frac_shader04_grid", src_frac_shader04_grid},
    {"frac_shader04_julia", src_frac_shader04_julia},
    {"frac_shader05", src_frac_shader05},
    {"frac_star1", src_frac_star1},
    {"frac_zoom1", src_frac_zoom1},
    {"frac_zoom2", src_frac_zoom2},
    {"frac_zoom3", src_frac_zoom3},
    {"frac_zoom4", src_frac_zoom4},
    {"frac_zoom5", src_frac_zoom5},
    {"frac_zoom6", src_frac_zoom6},
    {"frac_zoom7", src_frac_zoom7},
    {"frac_shader01", src_frac_shader01},
    {"frac_shader01_dark", src_frac_shader01_dark},
    {"frac_shader01_smooth", src_frac_shader01_smooth},
    {"frac_shader01_smooth_neon", src_frac_shader01_smooth_neon},
    {"frac_shader02_dmd", src_frac_shader02_dmd},
    {"frac_shader02_dmd_mandella", src_frac_shader02_dmd_mandella},
    {"frac_shader02_dmd2", src_frac_shader02_dmd2},
    {"frac_shader02_dmd2_amp", src_frac_shader02_dmd2_amp},
    {"frac_shader02_dmd3", src_frac_shader02_dmd3},
    {"frac_shader02_dmd4", src_frac_shader02_dmd4},
    {"frac_shader02_dmd5", src_frac_shader02_dmd5},
    {"frac_shader02_dmd6i", src_frac_shader02_dmd6i},
    {"frac_shader02_dmd6i_air", src_frac_shader02_dmd6i_air},
    {"frac_shader02_dmd6i_air_twist", src_frac_shader02_dmd6i_air_twist},
    {"frac_shader02_dmd6i_amp", src_frac_shader02_dmd6i_amp},
    {"frac_shader02_dmd6i_bowl", src_frac_shader02_dmd6i_bowl},
    {"frac_shader02_dmd6i_bubble", src_frac_shader02_dmd6i_bubble},
    {"frac_shader02_dmd6i_neon", src_frac_shader02_dmd6i_neon},
    {"frac_shader02_dmd6i1", src_frac_shader02_dmd6i1},
    {"frac_shader02_dmd7i", src_frac_shader02_dmd7i},
    {"frac_shader02_dmd8i", src_frac_shader02_dmd8i},
    {"frac_shader02_dmd9i", src_frac_shader02_dmd9i},
    {"frac_shader02_dmdi_radial", src_frac_shader02_dmdi_radial},
    {"frac_shader02_dmdi6i_zoom", src_frac_shader02_dmdi6i_zoom},
    {"color_shader02", color_shader02 },
    {"mirror_ufo_wrap", mirror_ufo_wrap},
    {"mirror_wrap_dmd6i", mirror_wrap_dmd6i},
    {"mirror_zoom", mirror_zoom},
    {"mirror_bubble", mirror_bubble},
    {"mirror_geometric", mirror_geometric},
    {"mirror_prism", mirror_prism},
    {"mirror_airshader1", mirror_airshader1},
    {"mirror_putty", mirror_putty},
    {"mirror_pebble", mirror_pebble},
    {"mirror_bowl_by_time", mirror_bowl_by_time},
    {"mirror_comb_mouse", mirror_comb_mouse},
    {"mirror_grad", mirror_grad},
    {"mirror_mandella", mirror_mandella},
    {"mirror_air_bowl", mirror_air_bowl},
    {"mirror_sin_osc", mirror_sin_osc},
    {"mirror_bubble_zoom_mouse", mirror_bubble_zoom_mouse},
    {"mirror_goofy", mirror_goofy},
    {"mirror_twist_gpt", mirror_twist_gpt},
    {"mirror_psyce_wave_all", mirror_psyce_wave_all},
    {"mirror_color_swirl", mirror_color_swirl},
    {"mirror_color_o1", mirror_color_o1},
    {"mirror_color_mouse", mirror_color_mouse},
    {"mirror_swirly", mirror_swirly},
    {"mirror_pong2", mirror_pong2},
    {"mirror_pong", mirror_pong},
    {"mirror_spiral_aura", mirror_spiral_aura},
    {"mirror_center", mirror_center},
    {"mirror_twisted", mirror_twisted},
    {"mirror_gpt", mirror_gpt},
    {"mirror_bowl", mirror_bowl},
    {"mirror_atan", mirror_atan},
    {"gpt_halluc", gpt_halluc},
    {"psych_block", szBlock}
};

class ShaderLibrary {
public:
    ShaderLibrary() = default;
    std::vector<std::string> tokenize(const std::string &input) {
        std::vector<std::string> values;
        size_t index = 0;
        std::string token;
        while(index <  input.size()) {
            char c = input.at(index);
            if(c == '\n') {
                values.push_back(token);
                token = "";
                index ++;
                continue;
            } else {
                token += input.at(index);
                index++;
            }
        }
        return values;
    }

    void init(gl::GLWindow *win, const std::string &filename) {
        std::string value = mx::readFileToString(filename);
        char c = 0;
        int index = 0;
        std::string token;
        std::vector<std::string> values = tokenize(value);
        for(size_t i = 0; i < values.size(); ++i)  {
            auto shader_contents =  mx::readFileToString(win->util.getFilePath("data/shaders/" + values[i]));
            if(!shader_contents.empty())
                shaders.push_back(std::make_pair(values[i], shader_contents));
        }
    }
    void print() {
    
        for(const auto &i : shaders) {
            std::cout << i.first <<":" << i.second << "\n";
        }
    }
    bool empty() const {
        return (shaders.empty());
    }
    std::string getNameAt(int i) const {
        return shaders.at(i).first;
    }
    std::string getShaderAt(int i) const {
        return shaders.at(i).second;
    }
    size_t getSize() const { return shaders.size(); }
protected:
    std::vector<std::pair<std::string, std::string>> shaders;
};

class About : public gl::GLObject {
    GLuint texture = 0;
    gl::ShaderProgram shader;
    gl::GLSprite sprite;
    float animation = 0.0f;
    std::vector<std::unique_ptr<gl::ShaderProgram>> shaders;
    std::vector<std::unique_ptr<gl::ShaderProgram>> shaders2;
    std::vector<std::string> shader_names;
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
    int loadingShaderIndex = 0;
    bool loadingComplete = false;
    gl::GLWindow* loadingWin = nullptr;
    std::unique_ptr<mx::Model> model;
    bool is3d = false;
    bool is3d_comp = false;
    ShaderLibrary library;
    int currentFileIndex = 0;
public:
    About() = default;
    virtual ~About() override {
        if(texture != 0) {
            glDeleteTextures(1, &texture);
        }
    }

    void set3DMode(bool is3d_m) {
        is3d_comp = is3d_m;
    }
    int getShaderIndex() const { return currentShaderIndex; }
    void setShaderIndex(int index) { currentShaderIndex = index; }
    int getShaderCount() { return static_cast<int>(shader_names.size());  }
    std::string getShaderNameAt(int index) {
        if(index >= 0 && index < shader_names.size())
            return shader_names[index];
        return "";
    }

    GLuint createTexture(SDL_Surface *surface, bool flip) {
        if (!surface) {
            throw mx::Exception("Surface is null: Unable to load PNG file.");
        }
        GLuint texture = 0;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        SDL_Surface *converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
        if(!converted) {
            glDeleteTextures(1, &texture);
            throw mx::Exception("Failed to convert surface.");
        }

        if(flip) {
            SDL_Surface *flipped = mx::Texture::flipSurface(converted);
            if (!flipped) {
                glDeleteTextures(1, &texture);
                glBindTexture(GL_TEXTURE_2D, 0);
                throw mx::Exception("Failed to flip surface.");
            }
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, flipped->w, flipped->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, flipped->pixels);
        } else {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, converted->w, converted->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, converted->pixels);    
        }
        
        SDL_FreeSurface(converted);
        return texture;
    }

    void loadNewTexture(SDL_Surface *surface, gl::GLWindow *win) {
        if(texture != 0) {
            glDeleteTextures(1, &texture);
            texture = 0;
        }
        texture = createTexture(surface, true);
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

    void loadShaderAsync(void* arg) {
        About* self = static_cast<About*>(arg);
        self->loadNextShader();
    }
    
    void loadNextShader() {
        if (loadingShaderIndex < static_cast<int>(shaderSources.size())) {
            const auto& info = shaderSources[loadingShaderIndex];
            
            if(info.name == "block") {
                std::cout << info.source << "\n";
            }

#ifdef __EMSCRIPTEN__
            EM_ASM({
                if (typeof window.addLoadingMessage === 'function') {
                    var name = UTF8ToString($0);
                    var idx = $1;
                    var total = $2;
                    window.addLoadingMessage('[' + idx + '/' + total + '] Compiling: ' + name, 'normal');
                }
            }, info.name.c_str(), loadingShaderIndex + 1, (int)shaderSources.size());
#endif
            
            auto shader = std::make_unique<gl::ShaderProgram>();
            bool success = shader->loadProgramFromText(sz3DVertex, info.source);
            

            if(success) {
                std::cout << "Compiled: " << info.name << " [OK]\n";
            } 

            auto shader2 = std::make_unique<gl::ShaderProgram>();
            bool success2 = shader2->loadProgramFromText(gl::vSource, info.source);
            if(success2) {
                std::cout << "Compiled: " << info.name << " [OK]\n";
            }

#ifdef __EMSCRIPTEN__
            EM_ASM({
                if (typeof window.addLoadingMessage === 'function') {
                    var name = UTF8ToString($0);
                    var ok = $1;
                    var ok2 = $2;
                    if (ok && ok2) {
                        window.addLoadingMessage('    ✓ ' + name + ' - OK', 'success');
                    } else if (ok || ok2) {
                        window.addLoadingMessage('    ⚠ ' + name + ' - Partial', 'info');
                    } else {
                        window.addLoadingMessage('    ✗ ' + name + ' - FAILED', 'error');
                    }
                }
            }, info.name.c_str(), success ? 1 : 0, success2 ? 1 : 0);
#endif

            
            if(success && success2) {
                if (success) {
                    shader->setSilent(true);
                    shaders.push_back(std::move(shader));
                }
                if(success2) {
                    shader2->setSilent(true);
                    shaders2.push_back(std::move(shader2));
                }
                shader_names.push_back(info.name);
            } else {
                std::cout << "Failed: " << info.name << "\n";
            } 
            loadingShaderIndex++;
            
            
#ifdef __EMSCRIPTEN__
            emscripten_async_call([](void* arg) {
                About* self = static_cast<About*>(arg);
                self->loadNextShader();
            }, this, 10);  
#endif
        } else {
            finishLoading();
        }
    }
    
    void finishLoading() {
        if (shaders.empty()) {
            mx::system_err << "No shaders loaded successfully\n";
            return;
        }
        
#ifdef __EMSCRIPTEN__
        EM_ASM({
            if (typeof window.addLoadingMessage === 'function') {
                window.addLoadingMessage(' ', 'normal');
                window.addLoadingMessage('Compiled ' + $0 + ' shaders successfully!', 'success');
                window.addLoadingMessage(' ', 'normal');
                window.addLoadingMessage('Loading texture...', 'info');
            }
            if (typeof window.onAllShadersCompiled === 'function') {
                window.onAllShadersCompiled($0);
            }
        }, (int)shaders.size());
#endif
        
        std::string logoPath = loadingWin->util.getFilePath("data/logo.png");
        SDL_Surface *surface = IMG_Load(logoPath.c_str());
        if (!surface) {
            mx::system_err << "Error loading logo.png\n";
            return;
        }
        
        SDL_Surface *converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
        SDL_FreeSurface(surface);
        
        if (!converted) {
            mx::system_err << "Error converting logo surface\n";
            return;
        }
        
#ifdef __EMSCRIPTEN__
        EM_ASM({
            if (typeof window.addLoadingMessage === 'function') {
                window.addLoadingMessage('Texture loaded: ' + $0 + 'x' + $1, 'success');
                window.addLoadingMessage(' ', 'normal');
                window.addLoadingMessage('Initialization complete!', 'success');
            }
        }, converted->w, converted->h);
#endif
        
        lastUpdateTime = SDL_GetTicks();
        sprite.initSize(canvasWidth, canvasHeight);
        loadNewTexture(converted, loadingWin);
        SDL_FreeSurface(converted);
        
        loadingComplete = true;
        
#ifdef __EMSCRIPTEN__
        EM_ASM({
            setTimeout(function() {
                if (typeof window.hideLoadingScreen === 'function') {
                    window.hideLoadingScreen();
                }
            }, 1000);
        });
#endif
    }

    void load(gl::GLWindow *win) override {
        maxWidth = win->w;
        maxHeight = win->h;
        canvasWidth = win->w;
        canvasHeight = win->h;
        loadingWin = win;
        loadModelFile(win->util.getFilePath("data/compressed/quad.mxmod.z"));
        glViewport(0, 0, canvasWidth, canvasHeight);
        
#ifdef __EMSCRIPTEN__
        EM_ASM({
            if (typeof window.addLoadingMessage === 'function') {
                window.addLoadingMessage('MX2 Graphics Demo', 'info');
                window.addLoadingMessage('OpenGL initialized: ' + $0 + 'x' + $1, 'success');
                window.addLoadingMessage(' ', 'normal');
                window.addLoadingMessage('Compiling ' + $2 + ' shaders...', 'info');
                window.addLoadingMessage(' ', 'normal');
            }
        }, canvasWidth, canvasHeight, (int)shaderSources.size());
#endif
        loadingShaderIndex = 0;
        loadingComplete = false;



#ifdef __EMSCRIPTEN__   
        currentFileIndex = 0;
        library.init(win, win->util.getFilePath("data/shaders/index.txt"));
        for(size_t i = 0; i < library.getSize(); ++i) {
            shaderSources.push_back({library.getNameAt(i), library.getShaderAt(i)});
        }
        emscripten_async_call([](void* arg) {
            About* self = static_cast<About*>(arg);
            self->loadNextShader();
        }, this, 50);  
        
#endif
    }
    float cameraYaw = 270.0f;   
    float cameraPitch = 0.0f; 
    const float cameraRotationSpeed = 5.0f; 
    bool viewRotationActive = false; 
    bool oscillateScale = false;
    float cameraDistance = 0.0f;
    float movementSpeed = 0.01f;

    void loadModelFile(const std::string &m_file_path) {
        if(m_file_path.find("quad") != std::string::npos) {
                is3d = false;
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
                    sprite.initSize(canvasWidth, canvasHeight);
                    if (loadingComplete && loadingWin) {
                        switchShader(currentShaderIndex, loadingWin);
                    }
                }
                return;
        } else {
            is3d = true;
        }
        model.reset(new mx::Model());
        if(!model->openModel(m_file_path)) {
            throw mx::Exception("Could not open model: " + m_file_path);
        }
    }

    void drawModel2D(gl::GLWindow *win) {
        if (currentShaderIndex >= shaders2.size()) {
            printf("Error: shaders2 index %zu out of bounds (size: %zu)\n", currentShaderIndex, shaders2.size());
            return;
        }
        glDisable(GL_DEPTH_TEST);
        gl::ShaderProgram *activeShader = shaders2[currentShaderIndex].get();
        activeShader->setUniform("mv_matrix", glm::mat4(1.0f));
        activeShader->setUniform("proj_matrix", glm::mat4(1.0f));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        sprite.setShader(activeShader);
        sprite.setName("textTexture");
        sprite.draw(texture, displayX, displayY, displayW, displayH);
    }


    void drawModel(gl::GLWindow *win) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);
        glDisable(GL_CULL_FACE);  

        static float rotation = 0.0f;
        rotation = fmod(rotation + 0.5f, 360.0f);
        
        const Uint8* keystate = SDL_GetKeyboardState(NULL);
        if (!oscillateScale) {

            if(keystate[SDL_SCANCODE_B]) {
                movementSpeed += 0.01f;
                mx::system_out << "acmx2: movement increased: " << movementSpeed << "\n";
                fflush(stdout);
            }

            if(keystate[SDL_SCANCODE_N]) {
                movementSpeed -= 0.01f;
                mx::system_out << "acmx2: movement decreased: " << movementSpeed << "\n";
                fflush(stdout);
            }

            if (keystate[SDL_SCANCODE_EQUALS] || keystate[SDL_SCANCODE_KP_PLUS]) {
                cameraDistance += movementSpeed;
                mx::system_out << "acmx2: cameraDistance increased: " << cameraDistance << "\n";
                fflush(stdout);
            }
            if (keystate[SDL_SCANCODE_MINUS] || keystate[SDL_SCANCODE_KP_MINUS]) {
                cameraDistance -= movementSpeed;
                mx::system_out << "acmx2: cameraDistance decreased: " << cameraDistance << "\n";
                fflush(stdout);
            }
        }
        static float t = 0.0f;
        float oscOffset = 0.0f;
        if (oscillateScale) {
            t += 0.016f;
            oscOffset = 0.3f * std::sin(t);
        }

        glm::mat4 modelMatrix = glm::mat4(1.0f);
        glm::vec3 cameraPosBase = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 lookDirection;
    
        if (!viewRotationActive) {
            if (keystate[SDL_SCANCODE_W]) {
                cameraPitch += cameraRotationSpeed * 0.3f;
                if (cameraPitch > 89.0f) cameraPitch = 89.0f;
            }
            if (keystate[SDL_SCANCODE_S]) {
                cameraPitch -= cameraRotationSpeed * 0.33f;
                if (cameraPitch < -89.0f) cameraPitch = -89.0;
            }
            if (keystate[SDL_SCANCODE_A]) {
                cameraYaw -= cameraRotationSpeed * 0.3f;
                cameraYaw = fmod(cameraYaw + 360.0f, 360.0f);
            }
            if (keystate[SDL_SCANCODE_D]) {
                cameraYaw += cameraRotationSpeed * 0.3f;
                cameraYaw = fmod(cameraYaw, 360.0f);
            }
        }
        if (viewRotationActive) {
            static float viewRotation = 0.0f;
            viewRotation = fmod(viewRotation + 0.3f, 360.0f);
            float lookX = 0.48f * sin(glm::radians(viewRotation));
            float lookY = 0.48f * sin(glm::radians(viewRotation * 0.7f));
            float lookZ = 0.48f * cos(glm::radians(viewRotation));
            lookDirection = glm::vec3(lookX, lookY, lookZ);
        } else {
            lookDirection.x = cos(glm::radians(cameraPitch)) * cos(glm::radians(cameraYaw));
            lookDirection.y = sin(glm::radians(cameraPitch));
            lookDirection.z = cos(glm::radians(cameraPitch)) * sin(glm::radians(cameraYaw));
            lookDirection = glm::normalize(lookDirection) * 0.48f;
        }

        float finalOffset = oscillateScale ? oscOffset : cameraDistance;
        glm::vec3 cameraPos = cameraPosBase - glm::normalize(lookDirection) * finalOffset;
        glm::vec3 cameraTarget = cameraPos + lookDirection;
        glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::mat4 viewMatrix = glm::lookAt(cameraPos, cameraTarget, cameraUp);
        glm::mat4 projectionMatrix = glm::perspective(
            glm::radians(120.0f),
            static_cast<float>(win->w) / static_cast<float>(win->h),
            0.01f,
            1000.0f
        );
        glm::mat4 mvMatrix = viewMatrix * modelMatrix;
        gl::ShaderProgram *activeShader;
        activeShader = shaders[currentShaderIndex].get();
        activeShader->setUniform("mv_matrix", mvMatrix);
        activeShader->setUniform("proj_matrix", projectionMatrix);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glUniform1i(glGetUniformLocation(activeShader->id(), "textTexture"), 0);
        model->setShaderProgram(activeShader);    
        for(auto &m : model->meshes) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);    
            m.draw();
        }
        glFrontFace(GL_CCW);
    }
  
    void switchShader(size_t index, gl::GLWindow *win) {
        if(index < shaders.size()) {
            currentShaderIndex = index;
            if(is3d) {
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
                model->setShaderProgram(shaders[currentShaderIndex].get());
                forceTextureRebind();
                sprite.initWithTexture(shaders[currentShaderIndex].get(), texture, displayX, displayY, displayW, displayH);
            } else  {
                shaders2[currentShaderIndex]->useProgram();
                shaders2[currentShaderIndex]->setUniform("time_f", animation);
                shaders2[currentShaderIndex]->setUniform("iTime", animation);
                shaders2[currentShaderIndex]->setUniform("iTimeDelta", 0.0f);
                shaders2[currentShaderIndex]->setUniform("iFrame", static_cast<float>(frameCount));
                shaders2[currentShaderIndex]->setUniform("iSeconds", iSeconds);
                shaders2[currentShaderIndex]->setUniform("iMinutes", iMinutes);
                shaders2[currentShaderIndex]->setUniform("iHours", iHours);
                shaders2[currentShaderIndex]->setUniform("iResolution", glm::vec2(displayW, displayH));
                glm::vec4 adjMouse = mouse;
                adjMouse.x -= displayX;
                adjMouse.y -= displayY;
                shaders2[currentShaderIndex]->setUniform("iMouse", adjMouse);
                shaders2[currentShaderIndex]->setUniform("iMouseNormalized", glm::vec2(adjMouse.x / displayW, 1.0f - adjMouse.y / displayH));
                shaders2[currentShaderIndex]->setUniform("iMouseActive", mouse.z > 0.5f ? 1.0f : 0.0f);
                shaders2[currentShaderIndex]->setUniform("iMouseVelocity", iMouseVelocity);
                shaders2[currentShaderIndex]->setUniform("iMouseClick", iMouseClick);
                shaders2[currentShaderIndex]->setUniform("iAspectRatio", static_cast<float>(displayW) / static_cast<float>(displayH));
                shaders2[currentShaderIndex]->setUniform("iSpeed", iSpeed);
                shaders2[currentShaderIndex]->setUniform("iFrequency", iFrequency);
                shaders2[currentShaderIndex]->setUniform("iAmplitude", iAmplitude);
                shaders2[currentShaderIndex]->setUniform("iHueShift", iHueShift);
                shaders2[currentShaderIndex]->setUniform("iSaturation", iSaturation);
                shaders2[currentShaderIndex]->setUniform("iBrightness", iBrightness);
                shaders2[currentShaderIndex]->setUniform("iContrast", iContrast);
                shaders2[currentShaderIndex]->setUniform("iZoom", iZoom);
                shaders2[currentShaderIndex]->setUniform("iRotation", iRotation);
                shaders2[currentShaderIndex]->setUniform("iCameraPos", iCameraPos);
                shaders2[currentShaderIndex]->setUniform("iBeat", beatValue);
                shaders2[currentShaderIndex]->setUniform("iAudioLevel", audioLevel);
                shaders2[currentShaderIndex]->setUniform("iDebugMode", iDebugMode);
                shaders2[currentShaderIndex]->setUniform("iQuality", iQuality);
                shaders2[currentShaderIndex]->setUniform("alpha", 1.0f);
                shaders2[currentShaderIndex]->setUniform("amp", 0.5f);
                shaders2[currentShaderIndex]->setUniform("uamp", 0.5f);
                shaders2[currentShaderIndex]->setUniform("textTexture", 0);
                glActiveTexture(GL_TEXTURE0);
                sprite.initWithTexture(shaders2[currentShaderIndex].get(), texture, displayX, displayY, displayW, displayH);
            }
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
        if (!loadingComplete) {
            return;
        }
        if (texture != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
        
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
        if(is3d) {
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
        }  else {
            shaders2[currentShaderIndex]->useProgram();
            shaders2[currentShaderIndex]->setUniform("time_f", animation);
            shaders2[currentShaderIndex]->setUniform("iTime", animation);
            shaders2[currentShaderIndex]->setUniform("iTimeDelta", deltaTime);
            shaders2[currentShaderIndex]->setUniform("iFrame", static_cast<float>(frameCount));
            shaders2[currentShaderIndex]->setUniform("iSeconds", iSeconds);
            shaders2[currentShaderIndex]->setUniform("iMinutes", iMinutes);
            shaders2[currentShaderIndex]->setUniform("iHours", iHours);
            shaders2[currentShaderIndex]->setUniform("iResolution", glm::vec2(displayW, displayH));
            glm::vec4 adjMouse = mouse;
            adjMouse.x -= displayX;
            adjMouse.y -= displayY;
            shaders2[currentShaderIndex]->setUniform("iMouse", adjMouse);
            shaders2[currentShaderIndex]->setUniform("iMouseNormalized", glm::vec2(adjMouse.x / displayW, 1.0f - adjMouse.y / displayH));
            shaders2[currentShaderIndex]->setUniform("iMouseActive", iMouseClick);
            shaders2[currentShaderIndex]->setUniform("iMouseVelocity", iMouseVelocity);
            shaders2[currentShaderIndex]->setUniform("iMouseClick", iMouseClick);
            shaders2[currentShaderIndex]->setUniform("iAspectRatio", static_cast<float>(displayW) / static_cast<float>(displayH));
            shaders2[currentShaderIndex]->setUniform("iSpeed", iSpeed);
            shaders2[currentShaderIndex]->setUniform("iFrequency", iFrequency);
            shaders2[currentShaderIndex]->setUniform("iAmplitude", iAmplitude);
            shaders2[currentShaderIndex]->setUniform("iHueShift", iHueShift);
            shaders2[currentShaderIndex]->setUniform("iSaturation", iSaturation);
            shaders2[currentShaderIndex]->setUniform("iBrightness", iBrightness);
            shaders2[currentShaderIndex]->setUniform("iContrast", iContrast);
            shaders2[currentShaderIndex]->setUniform("iZoom", iZoom);
            shaders2[currentShaderIndex]->setUniform("iRotation", iRotation);
            shaders2[currentShaderIndex]->setUniform("iCameraPos", iCameraPos);
            shaders2[currentShaderIndex]->setUniform("iBeat", beatValue);
            shaders2[currentShaderIndex]->setUniform("iAudioLevel", audioLevel);
            shaders2[currentShaderIndex]->setUniform("iDebugMode", iDebugMode);
            shaders2[currentShaderIndex]->setUniform("iQuality", iQuality);
            shaders2[currentShaderIndex]->setUniform("alpha", 1.0f);
            shaders2[currentShaderIndex]->setUniform("amp", 0.5f);
            shaders2[currentShaderIndex]->setUniform("uamp", 0.5f);
            shaders2[currentShaderIndex]->setUniform("textTexture", 0);
        }
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        update(deltaTime);
        sprite.initSize(canvasWidth, canvasHeight);
        if(is3d)
            drawModel(win);
        else
            drawModel2D(win);

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

    void forceTextureRebind() {

        if (texture != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            printf("Texture parameters reapplied\n");
        }
    }

    void resize(gl::GLWindow *win) {
        maxWidth = win->w;
        maxHeight = win->h;
        canvasWidth = win->w;
        canvasHeight = win->h;
        
        glViewport(0, 0, canvasWidth, canvasHeight);
        forceTextureRebind();
        
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
            forceTextureRebind();
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
        auto customShader1 = std::make_unique<gl::ShaderProgram>();
        auto customShader2 = std::make_unique<gl::ShaderProgram>();
        if(!customShader1->loadProgramFromText(sz3DVertex, fragmentSource.c_str())) {
            return "ERROR: Failed to create shader program.";
        }
        if(!customShader2->loadProgramFromText(gl::vSource, fragmentSource.c_str())) {
            return "ERROR: Failed to create shader program.";
        }
        
        customShader1->setSilent(true);
        customShader2->setSilent(true);
        static bool hasCustomShader = false;
        if(hasCustomShader && !shaders.empty() && !shaders2.empty()) {
            shaders.back() = std::move(customShader1);
            shaders2.back() = std::move(customShader2);
        } else {
            shaders.push_back(std::move(customShader1));
            shaders2.push_back(std::move(customShader2));
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
   
        if(is3d) {
            cropX = 0;
            cropY = 0;
            cropW = canvasWidth;
            cropH = canvasHeight;
        }
        
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
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("ACMX2 - Interactive Visualizer", tw, th) {
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

    void setShaderIndex(int index) {
        if(about_ptr) {
            about_ptr->setShaderIndex(index);
        }
    }

    void set3D(bool value) {
        if(about_ptr) {
            about_ptr->set3DMode(value);
        }
    }

    void loadModel(const std::string &info) {
        if(about_ptr) {
            about_ptr->loadModelFile("data/compressed/"+info);
        }
    }

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

    void forceTextureRebindWeb() {
        if (about_ptr) {
            about_ptr->forceTextureRebind();
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

    std::string getShaderNameAt(int i) {
        if(about_ptr) {
            return about_ptr->getShaderNameAt(i);
        }
        return "";
    }

    int getShaderCount() {
        if(about_ptr) {
            return about_ptr->getShaderCount();
        }
        return 0;
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

    int getIndex() {
        if(about_ptr) return about_ptr->getShaderIndex();
        return 0;
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
        emscripten::function("forceTextureRebind", &forceTextureRebindWeb);
        emscripten::function("getIndex", &getIndex);
        emscripten::function("loadModel", &loadModel);
        emscripten::function("setShader3DMode", &set3D);
        emscripten::function("setShaderIndex", &setShaderIndex);
        emscripten::function("getShaderCount", &getShaderCount);
        emscripten::function("getShaderNameAt", &getShaderNameAt);
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