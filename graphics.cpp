/**
 * @file graphics.cpp
 * @brief Application and WebAssembly entry points for the MX2 graphics demo.
 *
 * The demo loads built-in or external fragment shaders, renders them over an
 * image or 3D model, and exposes its controls to JavaScript when compiled with
 * Emscripten.
 *
 * @author Jared Bruni
 * @copyright GPL-3.0
 */

#include "argz.hpp"
#include "mx.hpp"
#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#include <emscripten/bind.h>
#include <emscripten/emscripten.h>
#endif
#include "gl.hpp"
#include "loadpng.hpp"
#include <SDL2/SDL_image.h>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "mirror_shaders.hpp"
#include "model.hpp"
/** @brief Reports the current OpenGL error together with its source location. */
#define CHECK_GL_ERROR()                                                    \
    {                                                                       \
        GLenum err = glGetError();                                          \
        if (err != GL_NO_ERROR)                                             \
            printf("OpenGL Error: %d at %s:%d\n", err, __FILE__, __LINE__); \
    }

#ifndef M_PI
/** @brief Fallback value of pi for platforms that do not define `M_PI`. */
#define M_PI 3.14159265358979323846
#endif

/** @brief Associates a display name with a fragment-shader source string. */
struct ShaderInfo {
    std::string name;   ///< Name shown in the shader selector.
    std::string source; ///< Complete GLSL fragment-shader source.
};

namespace {

    /**
     * @brief Removes leading and trailing ASCII whitespace.
     * @param value Text to trim.
     * @return A trimmed copy of @p value.
     */
    std::string trim(const std::string &value) {
        const size_t first = value.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) {
            return "";
        }
        const size_t last = value.find_last_not_of(" \t\r\n");
        return value.substr(first, last - first + 1);
    }

    /**
     * @brief Replaces every non-overlapping occurrence of a substring.
     * @param[in,out] source Text to modify.
     * @param from Substring to find.
     * @param to Replacement text.
     */
    void replaceAll(std::string &source, const std::string &from, const std::string &to) {
        size_t position = 0;
        while ((position = source.find(from, position)) != std::string::npos) {
            source.replace(position, from.size(), to);
            position += to.size();
        }
    }

    /**
     * @brief Adds explicit high-precision qualifiers to numeric GLSL arrays.
     * @param[in,out] source Fragment-shader source to normalize.
     */
    void qualifyArrayPrecisionForWebGL(std::string &source) {
        // Some mobile GLSL ES compilers do not apply the default precision to
        // array variables. Give numeric arrays an explicit qualifier while
        // preserving declarations that already specify one.
        static const std::regex arrayPattern(
            R"(\b(?:float|int|uint|vec[234]|ivec[234]|uvec[234]|mat[234](?:x[234])?)\s+[A-Za-z_][A-Za-z0-9_]*\s*\[[^\]\r\n]*\])");
        std::vector<size_t> insertions;
        for (std::sregex_iterator it(source.begin(), source.end(), arrayPattern), end; it != end; ++it) {
            const size_t start = static_cast<size_t>((*it).position());
            size_t cursor = start;
            while (cursor > 0 && std::isspace(static_cast<unsigned char>(source[cursor - 1])))
                --cursor;
            const size_t wordEnd = cursor;
            while (cursor > 0 && (std::isalnum(static_cast<unsigned char>(source[cursor - 1])) ||
                                  source[cursor - 1] == '_'))
                --cursor;
            const std::string previousWord = source.substr(cursor, wordEnd - cursor);
            if (previousWord != "lowp" && previousWord != "mediump" && previousWord != "highp")
                insertions.push_back(start);
        }
        for (auto it = insertions.rbegin(); it != insertions.rend(); ++it)
            source.insert(*it, "highp ");
    }

    /**
     * @brief Converts integer literals used in floating-point GLSL expressions.
     * @param[in,out] source Fragment-shader source to normalize.
     *
     * WebGL's GLSL ES compiler does not permit several implicit integer-to-float
     * conversions accepted by desktop drivers.
     */
    void normalizeFloatLiteralOperands(std::string &source) {
        const auto isIdentifierCharacter = [](char c) {
            return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
        };
        const auto skipSpaces = [&source](size_t &position) {
            while (position < source.size() && std::isspace(static_cast<unsigned char>(source[position]))) {
                ++position;
            }
        };
        const auto hasBareIntegerAfter = [&](const std::string &name) {
            size_t position = 0;
            while ((position = source.find(name, position)) != std::string::npos) {
                const size_t nameEnd = position + name.size();
                if ((position > 0 && isIdentifierCharacter(source[position - 1])) ||
                    (nameEnd < source.size() && isIdentifierCharacter(source[nameEnd]))) {
                    position = nameEnd;
                    continue;
                }
                size_t cursor = nameEnd;
                skipSpaces(cursor);
                if (cursor < source.size() && source[cursor] == '.') {
                    ++cursor;
                    while (cursor < source.size() && isIdentifierCharacter(source[cursor]))
                        ++cursor;
                } else if (cursor < source.size() && source[cursor] == '[') {
                    const size_t close = source.find(']', cursor + 1);
                    if (close == std::string::npos)
                        return false;
                    cursor = close + 1;
                }
                skipSpaces(cursor);
                if (cursor >= source.size() || std::string("+-*/<>=!").find(source[cursor]) == std::string::npos) {
                    position = nameEnd;
                    continue;
                }
                ++cursor;
                if (cursor < source.size() && source[cursor] == '=')
                    ++cursor;
                skipSpaces(cursor);
                if (cursor < source.size() && source[cursor] == '-')
                    ++cursor;
                const size_t numberStart = cursor;
                while (cursor < source.size() && std::isdigit(static_cast<unsigned char>(source[cursor])))
                    ++cursor;
                if (cursor > numberStart && (cursor >= source.size() || source[cursor] != '.'))
                    return true;
                position = nameEnd;
            }
            return false;
        };
        const auto hasBareIntegerBefore = [&](const std::string &name) {
            size_t position = 0;
            while ((position = source.find(name, position)) != std::string::npos) {
                const size_t nameEnd = position + name.size();
                if ((position > 0 && isIdentifierCharacter(source[position - 1])) ||
                    (nameEnd < source.size() && isIdentifierCharacter(source[nameEnd]))) {
                    position = nameEnd;
                    continue;
                }
                size_t cursor = position;
                while (cursor > 0 && std::isspace(static_cast<unsigned char>(source[cursor - 1])))
                    --cursor;
                if (cursor == 0 || std::string("+-*/<>=!").find(source[cursor - 1]) == std::string::npos) {
                    position = nameEnd;
                    continue;
                }
                --cursor;
                if (cursor > 0 && source[cursor - 1] == '=')
                    --cursor;
                while (cursor > 0 && std::isspace(static_cast<unsigned char>(source[cursor - 1])))
                    --cursor;
                const size_t numberEnd = cursor;
                while (cursor > 0 && std::isdigit(static_cast<unsigned char>(source[cursor - 1])))
                    --cursor;
                if (cursor < numberEnd &&
                    (cursor == 0 || (source[cursor - 1] != '.' && !isIdentifierCharacter(source[cursor - 1]))))
                    return true;
                position = nameEnd;
            }
            return false;
        };

        static const std::regex floatHelperPattern(
            R"(\b(mod|pingPong)\s*\(([^,\r\n]+),\s*(-?[0-9]+)\s*\))");
        bool needsConversion = std::regex_search(source, floatHelperPattern);
        for (const char *name : {"tc", "time_f", "iTime", "iResolution", "iMouse", "color"}) {
            needsConversion = needsConversion || hasBareIntegerAfter(name) || hasBareIntegerBefore(name);
        }
        if (!needsConversion) {
            return;
        }

        std::set<std::string> floatNames = {
            "tc", "TexCoord", "time_f", "iTime", "iResolution", "iMouse", "mxOutputColor"};
        static const std::regex declarationPattern(
            R"(\b(?:float|vec[234])\s+([A-Za-z_][A-Za-z0-9_]*)\b(?!\s*\())");
        for (std::sregex_iterator it(source.begin(), source.end(), declarationPattern), end; it != end; ++it) {
            floatNames.insert((*it)[1].str());
        }

        struct LiteralReplacement {
            size_t position;
            size_t length;
            std::string value;
        };
        std::vector<LiteralReplacement> replacements;
        static const std::regex floatOnLeftPattern(
            R"(\b([A-Za-z_][A-Za-z0-9_]*)(?:\s*(?:\.[A-Za-z]+|\[[^\]\r\n]+\]))?\s*(?:\*=|/=|\+=|-=|[+*/<>]=?|==|!=|-)\s*(-?[0-9]+)(?![.A-Za-z0-9_]))");
        for (std::sregex_iterator it(source.begin(), source.end(), floatOnLeftPattern), end; it != end; ++it) {
            if (floatNames.contains((*it)[1].str())) {
                replacements.push_back({static_cast<size_t>((*it).position(2)),
                                        static_cast<size_t>((*it).length(2)), (*it)[2].str() + ".0"});
            }
        }

        static const std::regex floatOnRightPattern(
            R"((^|[^A-Za-z0-9_.])(-?[0-9]+)\s*(?:[+*/<>]=?|==|!=|-)\s*([A-Za-z_][A-Za-z0-9_]*)(?:\s*(?:\.[A-Za-z]+|\[[^\]\r\n]+\]))?)");
        for (std::sregex_iterator it(source.begin(), source.end(), floatOnRightPattern), end; it != end; ++it) {
            if (floatNames.contains((*it)[3].str())) {
                replacements.push_back({static_cast<size_t>((*it).position(2)),
                                        static_cast<size_t>((*it).length(2)), (*it)[2].str() + ".0"});
            }
        }
        std::sort(replacements.begin(), replacements.end(),
                  [](const LiteralReplacement &left, const LiteralReplacement &right) {
                      return left.position > right.position;
                  });
        size_t lastPosition = std::string::npos;
        for (const auto &replacement : replacements) {
            if (replacement.position != lastPosition) {
                source.replace(replacement.position, replacement.length, replacement.value);
                lastPosition = replacement.position;
            }
        }

        // These helpers take floats, but many desktop shaders pass bare integer literals.
        source = std::regex_replace(
            source, floatHelperPattern, "$1($2, $3.0)");
        static const std::regex constructorArithmeticPattern(
            R"(((?:float|vec[234])\s*\([^;\r\n]+\))\s*([+*/-])\s*(-?[0-9]+)(?![.A-Za-z0-9_]))");
        source = std::regex_replace(
            source, constructorArithmeticPattern,
            "$1 $2 $3.0");
    }

    /**
     * @brief Rewrites dynamically indexed fragment outputs for WebGL.
     * @param[in,out] source Fragment-shader source to inspect and rewrite.
     */
    void makeFragmentOutputWebGLSafe(std::string &source) {
        static const std::regex outputPattern(R"(\bout\s+vec4\s+([A-Za-z_][A-Za-z0-9_]*)\s*;)");
        std::smatch outputMatch;
        if (!std::regex_search(source, outputMatch, outputPattern)) {
            return;
        }

        const std::string outputName = outputMatch[1].str();
        bool hasDynamicOutputIndex = false;
        size_t indexPosition = 0;
        while ((indexPosition = source.find(outputName, indexPosition)) != std::string::npos) {
            size_t cursor = indexPosition + outputName.size();
            while (cursor < source.size() && std::isspace(static_cast<unsigned char>(source[cursor])))
                ++cursor;
            if (cursor < source.size() && source[cursor] == '[') {
                ++cursor;
                while (cursor < source.size() && std::isspace(static_cast<unsigned char>(source[cursor])))
                    ++cursor;
                if (cursor < source.size() && (std::isalpha(static_cast<unsigned char>(source[cursor])) ||
                                               source[cursor] == '_')) {
                    hasDynamicOutputIndex = true;
                    break;
                }
            }
            indexPosition += outputName.size();
        }
        if (!hasDynamicOutputIndex) {
            return;
        }
        source.replace(static_cast<size_t>(outputMatch.position()), static_cast<size_t>(outputMatch.length()),
                       "out vec4 mxWebGLFragColor;\nvec4 mxOutputColor;");
        source = std::regex_replace(source, std::regex("\\b" + outputName + "\\b"), "mxOutputColor");
        // The replacement above cannot match outputName inside the generated names
        // because both adjacent characters are identifier characters.
        source = std::regex_replace(source,
                                    std::regex(R"(\bvoid\s+main\s*\(\s*(?:void)?\s*\))"),
                                    "void mxShaderMain()");
        source += "\nvoid main() {\n    mxShaderMain();\n    mxWebGLFragColor = mxOutputColor;\n}\n";
    }

    /**
     * @brief Converts a desktop fragment shader to GLSL ES 3.00 syntax.
     * @param source Original shader source.
     * @return WebGL-compatible shader source.
     */
    std::string convertFragmentShaderToWebGL(std::string source) {
        if (source.size() >= 3 && static_cast<unsigned char>(source[0]) == 0xef &&
            static_cast<unsigned char>(source[1]) == 0xbb && static_cast<unsigned char>(source[2]) == 0xbf) {
            source.erase(0, 3);
        }

        const size_t versionStart = source.find("#version");
        if (versionStart == std::string::npos) {
            source.insert(0, "#version 300 es\n");
        } else {
            if (versionStart > 0) {
                source.erase(0, versionStart);
            }
            const size_t versionEnd = source.find_first_of("\r\n");
            source.replace(0,
                           versionEnd == std::string::npos ? source.size() : versionEnd,
                           "#version 300 es");
        }

        const size_t headerEnd = source.find('\n', source.find("#version"));
        if (source.find("precision ") == std::string::npos) {
            source.insert(headerEnd == std::string::npos ? source.size() : headerEnd + 1,
                          "precision highp float;\nprecision highp int;\n");
        }

        source = std::regex_replace(source, std::regex(R"(\bin\s+vec2\s+tc\s*;)"),
                                    "in vec2 TexCoord;\n#define tc TexCoord");
        replaceAll(source, "texture2D(", "texture(");
        replaceAll(source, "texture1D(", "texture(");

        // Uniform initializers are illegal in GLSL ES. These values were being
        // used as constants by the desktop shaders and are not app-controlled.
        source = std::regex_replace(
            source,
            std::regex(R"(\buniform\s+(float|vec[234])\s+([A-Za-z_][A-Za-z0-9_]*)\s*=\s*([^;]+);)"),
            "const $1 $2 = $3;");

        if (source.find("gl_FragColor") != std::string::npos) {
            source.insert(source.find('\n', source.find("#version")) + 1, "out vec4 mxFragColor;\n");
            replaceAll(source, "gl_FragColor", "mxFragColor");
        }
        qualifyArrayPrecisionForWebGL(source);
        makeFragmentOutputWebGLSafe(source);
        normalizeFloatLiteralOperands(source);
        return source;
    }

    /**
     * @brief Detects shaders that require unavailable audio or cached inputs.
     * @param filename Shader filename used for heuristic checks.
     * @param source Shader source used for uniform inspection.
     * @return `true` when the browser demo cannot provide the required inputs.
     */
    bool usesUnsupportedRuntimeInputs(const std::string &filename, const std::string &source) {
        if (filename.find("cache") != std::string::npos || filename.find("audio") != std::string::npos ||
            source.find("sampler1D") != std::string::npos || source.find("spectrum") != std::string::npos ||
            source.find("audioData") != std::string::npos || source.find("audio_data") != std::string::npos ||
            source.find("fft") != std::string::npos || source.find("FFT") != std::string::npos) {
            return true;
        }

        static const std::regex audioUniformPattern(
            R"(\buniform\s+float\s+[^;]*\b(?:amp|amp_peak|amp_rms|amp_smooth|amp_low|amp_mid|amp_high|iamp)\b[^;]*;)");
        static const std::regex cachedTexturePattern(
            R"(\buniform\s+sampler2D\s+[^;]*(?:\bsamp[1-9][0-9]*\b|\btextures\s*\[)[^;]*;)");
        return std::regex_search(source, audioUniformPattern) || std::regex_search(source, cachedTexturePattern);
    }

} // namespace

/** @brief Vertex shader used by the textured 3D-model render path. */
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

/** @brief Ordered catalog of built-in shaders exposed by the application. */
static std::vector<ShaderInfo> shaderSources = {
    {"Bubble", srcShader1},
    {"Abilify", srcShaderAbilify},
    {"Pastel", srcShaderPinkClouds},
    {"KaleidoCloud", srcShaderKaleidoCloud},
    {"KaleidoFromTex", srcShaderKaleidoFromTex},
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
    {"color_shader02", color_shader02},
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
    {"psych_block", szBlock},
    {"psychedelic_energy", psychedelic_energy}};

/**
 * @brief Loads and stores external fragment shaders listed in an index file.
 *
 * Cached WebGL shaders are preferred. Desktop GLSL sources are converted on
 * demand when no cache is available.
 */
class ShaderLibrary {
  public:
    ShaderLibrary() = default;
    /**
     * @brief Parses a newline-delimited shader index.
     * @param input Index contents; blank lines and lines beginning with `#` are ignored.
     * @return Shader filenames in their original order.
     */
    std::vector<std::string> tokenize(const std::string &input) {
        std::vector<std::string> values;
        std::istringstream stream(input);
        std::string line;
        while (std::getline(stream, line)) {
            line = trim(line);
            if (!line.empty() && line.front() != '#') {
                values.push_back(line);
            }
        }
        return values;
    }

    /**
     * @brief Loads indexed shader sources from the application's data directory.
     * @param win Window used to resolve packaged resource paths.
     * @param filename Fallback index path when no WebGL cache exists.
     */
    void init(gl::GLWindow *win, const std::string &filename) {
        const std::string cacheIndex = win->util.getFilePath("data/shaders/webgl_cache/index.txt");
        std::string value = mx::readFileToString(cacheIndex);
        const bool useCache = !value.empty();
        if (!useCache) {
            value = mx::readFileToString(filename);
        }
        std::vector<std::string> values = tokenize(value);
        for (size_t i = 0; i < values.size(); ++i) {
            const std::string directory = useCache ? "data/shaders/webgl_cache/" : "data/shaders/";
            auto shader_contents = mx::readFileToString(win->util.getFilePath(directory + values[i]));
            if (shader_contents.empty()) {
                continue;
            }
            if (useCache) {
                shaders.push_back(std::make_pair(values[i], shader_contents));
            } else if (!usesUnsupportedRuntimeInputs(values[i], shader_contents)) {
                shaders.push_back(std::make_pair(values[i], convertFragmentShaderToWebGL(shader_contents)));
            }
        }
    }
    /** @brief Writes every loaded shader name and source to standard output. */
    void print() {

        for (const auto &i : shaders) {
            std::cout << i.first << ":" << i.second << "\n";
        }
    }
    /** @return `true` when no external shaders have been loaded. */
    bool empty() const {
        return (shaders.empty());
    }
    /** @brief Returns the name at @p i. @throws std::out_of_range for an invalid index. */
    std::string getNameAt(int i) const {
        return shaders.at(i).first;
    }
    /** @brief Returns the source at @p i. @throws std::out_of_range for an invalid index. */
    std::string getShaderAt(int i) const {
        return shaders.at(i).second;
    }
    /** @return Number of loaded external shaders. */
    size_t getSize() const { return shaders.size(); }

  protected:
    /** @brief Loaded shader names paired with their GLSL source. */
    std::vector<std::pair<std::string, std::string>> shaders;
};

/**
 * @brief Owns the demo's render state, shaders, texture, model, and input state.
 *
 * The object supports both a 2D sprite path and a textured 3D-model path. It
 * also manages optional ping-pong framebuffers for multipass effects.
 */
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
    gl::GLWindow *loadingWin = nullptr;
    std::unique_ptr<mx::Model> model;
    bool is3d = false;
    ShaderLibrary library;
    int currentFileIndex = 0;
    GLuint passFBO[2] = {0, 0};
    GLuint passTexture[2] = {0, 0};
    int passFBOWidth = 0, passFBOHeight = 0;
    bool multipassEnabled = false;
    std::vector<int> shaderPassList;

  public:
    /** @brief Constructs an empty render controller. */
    About() = default;
    /** @brief Releases the image texture and multipass framebuffer resources. */
    virtual ~About() override {
        if (texture != 0) {
            glDeleteTextures(1, &texture);
        }
        for (int i = 0; i < 2; ++i) {
            if (passFBO[i] != 0) {
                glDeleteFramebuffers(1, &passFBO[i]);
            }
            if (passTexture[i] != 0) {
                glDeleteTextures(1, &passTexture[i]);
            }
        }
    }

    /** @brief Selects textured-model rendering instead of the 2D sprite path. */
    void set3DMode(bool is3d_m) {
        is3d = is3d_m;
    }

    /**
     * @brief Creates or resizes the ping-pong framebuffer pair.
     * @param w Render-target width in pixels.
     * @param h Render-target height in pixels.
     */
    void initPassFBOs(int w, int h) {
        if (passFBO[0] != 0 && passFBOWidth == w && passFBOHeight == h) {
            return;
        }

        for (int i = 0; i < 2; ++i) {
            if (passFBO[i] != 0) {
                glDeleteFramebuffers(1, &passFBO[i]);
                passFBO[i] = 0;
            }
            if (passTexture[i] != 0) {
                glDeleteTextures(1, &passTexture[i]);
                passTexture[i] = 0;
            }
        }

        passFBOWidth = w;
        passFBOHeight = h;

        for (int i = 0; i < 2; ++i) {
            glGenFramebuffers(1, &passFBO[i]);
            glGenTextures(1, &passTexture[i]);
            glBindTexture(GL_TEXTURE_2D, passTexture[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glBindFramebuffer(GL_FRAMEBUFFER, passFBO[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, passTexture[i], 0);
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                printf("Error: Pass FBO %d is not complete!\n", i);
            }
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        printf("Multipass FBOs initialized: %dx%d\n", w, h);
    }

    /** @brief Enables or disables the configured multipass shader chain. */
    void enableMultipass(bool enable) {
        multipassEnabled = enable;
        printf("Multipass %s\n", enable ? "enabled" : "disabled");
    }

    /** @return Whether multipass rendering is enabled. */
    bool isMultipassEnabled() const {
        return multipassEnabled;
    }

    /** @brief Removes all shaders from the multipass chain. */
    void clearShaderPasses() {
        shaderPassList.clear();
        printf("Shader pass list cleared\n");
    }

    /** @brief Appends a valid 2D shader index to the multipass chain. */
    void addShaderPass(int shaderIndex) {
        if (shaderIndex >= 0 && shaderIndex < static_cast<int>(shaders2.size())) {
            shaderPassList.push_back(shaderIndex);
            printf("Added shader pass: %d (%s)\n", shaderIndex,
                   shaderIndex < static_cast<int>(shader_names.size()) ? shader_names[shaderIndex].c_str() : "unknown");
        }
    }

    /** @brief Replaces the multipass chain with @p passes. */
    void setShaderPasses(const std::vector<int> &passes) {
        shaderPassList = passes;
        printf("Set %zu shader passes\n", passes.size());
    }

    /** @return A copy of the configured shader-index chain. */
    std::vector<int> getShaderPasses() const {
        return shaderPassList;
    }

    /**
     * @brief Uploads the current application state to a shader program.
     * @param shader Program that receives the common uniforms.
     * @param deltaTime Seconds elapsed since the previous frame.
     */
    void updateShaderUniforms(gl::ShaderProgram *shader, float deltaTime) {
        shader->setUniform("time_f", animation);
        shader->setUniform("iTime", animation);
        shader->setUniform("iTimeDelta", deltaTime);
        shader->setUniform("iFrame", static_cast<float>(frameCount));
        shader->setUniform("iSeconds", iSeconds);
        shader->setUniform("iMinutes", iMinutes);
        shader->setUniform("iHours", iHours);
        shader->setUniform("iResolution", glm::vec2(canvasWidth, canvasHeight));
        glm::vec4 adjMouse = mouse;
        adjMouse.x -= displayX;
        adjMouse.y -= displayY;
        shader->setUniform("iMouse", adjMouse);
        shader->setUniform("iMouseNormalized", glm::vec2(adjMouse.x / displayW, 1.0f - adjMouse.y / displayH));
        shader->setUniform("iMouseActive", iMouseClick);
        shader->setUniform("iMouseVelocity", iMouseVelocity);
        shader->setUniform("iMouseClick", iMouseClick);
        shader->setUniform("iAspectRatio", static_cast<float>(canvasWidth) / static_cast<float>(canvasHeight));
        shader->setUniform("iSpeed", iSpeed);
        shader->setUniform("iFrequency", iFrequency);
        shader->setUniform("iAmplitude", iAmplitude);
        shader->setUniform("iHueShift", iHueShift);
        shader->setUniform("iSaturation", iSaturation);
        shader->setUniform("iBrightness", iBrightness);
        shader->setUniform("iContrast", iContrast);
        shader->setUniform("iZoom", iZoom);
        shader->setUniform("iRotation", iRotation);
        shader->setUniform("iCameraPos", iCameraPos);
        shader->setUniform("iBeat", beatValue);
        shader->setUniform("iAudioLevel", audioLevel);
        shader->setUniform("iDebugMode", iDebugMode);
        shader->setUniform("iQuality", iQuality);
        shader->setUniform("alpha", 1.0f);
        updateCompatibilityUniforms(shader);
        shader->setUniform("mv_matrix", glm::mat4(1.0f));
        shader->setUniform("proj_matrix", glm::mat4(1.0f));
    }

    /**
     * @brief Supplies legacy sampler and synthesized audio uniforms.
     * @param shader Program that receives the compatibility values.
     */
    void updateCompatibilityUniforms(gl::ShaderProgram *shader) {
        shader->setUniform("textTexture", 0);
        shader->setUniform("samp", 0);
        shader->setUniform("mat_samp", 0);

        shader->setUniform("amp", audioLevel);
        shader->setUniform("amp_peak", std::max(audioLevel, beatValue));
        shader->setUniform("amp_rms", audioLevel);
        shader->setUniform("amp_smooth", audioLevel);
        shader->setUniform("amp_low", beatValue);
        shader->setUniform("amp_mid", (audioLevel + beatValue) * 0.5f);
        shader->setUniform("amp_high", audioLevel * (1.0f - 0.35f * beatValue));
        shader->setUniform("iamp", audioLevel);
        shader->setUniform("uamp", audioLevel);
    }

    /** @return Index of the active shader. */
    int getShaderIndex() const { return currentShaderIndex; }
    /** @brief Selects @p index when it refers to a compiled shader. */
    void setShaderIndex(int index) {
        if (index >= 0 && index < static_cast<int>(shaders.size())) {
            currentShaderIndex = static_cast<size_t>(index);
        }
    }
    /** @return Number of successfully compiled shader pairs. */
    int getShaderCount() { return static_cast<int>(shaders.size()); }
    /** @return Display name for @p index, or an empty string when invalid. */
    std::string getShaderNameAt(int index) {
        if (index >= 0 && index < static_cast<int>(shader_names.size()))
            return shader_names[index];
        return "";
    }

    /**
     * @brief Uploads an SDL surface as an RGBA OpenGL texture.
     * @param surface Source pixel surface; must not be null.
     * @param flip Whether to vertically flip the pixels before upload.
     * @return Newly allocated OpenGL texture identifier.
     * @throws mx::Exception if conversion or flipping fails.
     */
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
        if (!converted) {
            glDeleteTextures(1, &texture);
            throw mx::Exception("Failed to convert surface.");
        }

        if (flip) {
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

    /**
     * @brief Replaces the source image and recomputes its aspect-fit rectangle.
     * @param surface Image pixels to upload.
     * @param win Window whose viewport and dimensions are updated.
     */
    void loadNewTexture(SDL_Surface *surface, gl::GLWindow *win) {
        if (texture != 0) {
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

    /** @brief Emscripten-compatible callback that compiles the next shader. */
    void loadShaderAsync(void *arg) {
        About *self = static_cast<About *>(arg);
        self->loadNextShader();
    }

    /** @brief Compiles one built-in shader for both 2D and 3D rendering. */
    void loadNextShader() {
        if (loadingShaderIndex < static_cast<int>(shaderSources.size())) {
            const auto &info = shaderSources[loadingShaderIndex];

            if (info.name == "block") {
                std::cout << info.source << "\n";
            }

#ifdef __EMSCRIPTEN__
            EM_ASM({
                if (typeof window.beginShaderCompilation === 'function') {
                    window.beginShaderCompilation(UTF8ToString($0));
                }
                if (typeof window.addLoadingMessage === 'function') {
                    var name = UTF8ToString($0);
                    var idx = $1;
                    var total = $2;
                    window.addLoadingMessage('[' + idx + '/' + total + '] Compiling: ' + name, 'normal');
                } }, info.name.c_str(), loadingShaderIndex + 1, (int)shaderSources.size());
#endif

            auto shader = std::make_unique<gl::ShaderProgram>();
            bool success = shader->loadProgramFromText(sz3DVertex, info.source);

            if (success) {
                std::cout << "Compiled: " << info.name << " [OK]\n";
            }

            auto shader2 = std::make_unique<gl::ShaderProgram>();
            bool success2 = shader2->loadProgramFromText(gl::vSource, info.source);
            if (success2) {
                std::cout << "Compiled: " << info.name << " [OK]\n";
            }

#ifdef __EMSCRIPTEN__
            EM_ASM({
                if (typeof window.finishShaderCompilation === 'function') {
                    window.finishShaderCompilation(UTF8ToString($0), UTF8ToString($3), $1 !== 0, $2 !== 0);
                }
                if (typeof window.addLoadingMessage === 'function') {
                    var name = UTF8ToString($0);
                    var ok = $1;
                    var ok2 = $2;
                    if (ok && ok2) {
                        window.addLoadingMessage('     ' + name + ' - OK', 'success');
                    } else if (ok || ok2) {
                        window.addLoadingMessage('     ' + name + ' - Partial', 'info');
                    } else {
                        window.addLoadingMessage('     ' + name + ' - FAILED', 'error');
                    }
                } }, info.name.c_str(), success ? 1 : 0, success2 ? 1 : 0, info.source.c_str());
#endif

            if (success && success2) {
                if (success) {
                    shader->setSilent(true);
                    shaders.push_back(std::move(shader));
                }
                if (success2) {
                    shader2->setSilent(true);
                    shaders2.push_back(std::move(shader2));
                }
                shader_names.push_back(info.name);
            } else {
                std::cout << "Failed: " << info.name << "\n";
            }
            loadingShaderIndex++;

#ifdef __EMSCRIPTEN__
            emscripten_async_call([](void *arg) {
                About *self = static_cast<About *>(arg);
                self->loadNextShader();
            },
                                  this, 10);
#endif
        } else {
            finishLoading();
        }
    }

    /** @brief Finalizes shader loading and activates the first available shader. */
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
            if (typeof window.finalizeShaderFailureLog === 'function') {
                window.finalizeShaderFailureLog();
            } }, (int)shaders.size());
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
            } }, converted->w, converted->h);
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
                } }, 1000);
        });
#endif
    }

    /**
     * @brief Initializes textures, shader loading, and initial viewport state.
     * @param win Owning OpenGL window.
     */
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
            } }, canvasWidth, canvasHeight, (int)shaderSources.size());
#endif
        loadingShaderIndex = 0;
        loadingComplete = false;

#ifdef __EMSCRIPTEN__
        currentFileIndex = 0;
        library.init(win, win->util.getFilePath("data/shaders/index.txt"));
        for (size_t i = 0; i < library.getSize(); ++i) {
            shaderSources.push_back({library.getNameAt(i), library.getShaderAt(i)});
        }
        emscripten_async_call([](void *arg) {
            About *self = static_cast<About *>(arg);
            self->loadNextShader();
        },
                              this, 50);

#endif
    }
    float cameraYaw = 270.0f;                  ///< Camera yaw in degrees.
    float cameraPitch = 0.0f;                 ///< Camera pitch in degrees.
    float cameraRoll = 0.0f;                  ///< Camera roll in degrees.
    const float cameraRotationSpeed = 5.0f;   ///< Keyboard rotation step in degrees.
    bool viewRotationActive = false;           ///< Whether automatic view rotation is active.
    bool oscillateScale = false;               ///< Whether model scale oscillation is active.
    float cameraDistance = 0.0f;               ///< Distance from the camera to the model center.
    float movementSpeed = 0.01f;               ///< Per-frame model movement step.
    float modelSize = 1.0f;                    ///< Largest dimension of the model bounds.
    float modelScale = 1.0f;                   ///< Scale that normalizes the loaded model.
    glm::vec3 modelCenter{0.0f};                ///< Center of the loaded model bounds.
    static constexpr float TARGET_MODEL_SIZE = 1.0f; ///< Desired normalized model size.
    float cameraOffsetX = 0.0f;                ///< Horizontal camera translation.
    float cameraOffsetY = 0.0f;                ///< Vertical camera translation.
    float cameraOffsetZ = 0.0f;                ///< Depth camera translation.
    /** @brief Rotates the model camera horizontally by @p delta degrees. */
    void adjustCameraYaw(float delta) {
        cameraYaw += delta;
        cameraYaw = fmod(cameraYaw + 360.0f, 360.0f);
    }

    /** @brief Rotates the model camera vertically, clamped away from the poles. */
    void adjustCameraPitch(float delta) {
        cameraPitch += delta;
        if (cameraPitch > 89.0f)
            cameraPitch = 89.0f;
        if (cameraPitch < -89.0f)
            cameraPitch = -89.0f;
    }

    /** @brief Changes the model-camera distance by a model-relative amount. */
    void adjustCameraDistance(float delta) {
        cameraDistance += delta * modelSize * 0.1f;
    }

    /** @name Normalized model-camera translation */
    ///@{
    /** @brief Sets the horizontal camera offset in model-relative units. */
    void setCameraX(float x) { cameraOffsetX = x * modelSize * 0.5f; }
    /** @brief Sets the vertical camera offset in model-relative units. */
    void setCameraY(float y) { cameraOffsetY = y * modelSize * 0.5f; }
    /** @brief Sets the depth camera offset in model-relative units. */
    void setCameraZ(float z) { cameraOffsetZ = z * modelSize * 0.5f; }
    /** @return Horizontal camera offset in model-relative units. */
    float getCameraX() const { return modelSize > 0.001f ? cameraOffsetX / (modelSize * 0.5f) : 0.0f; }
    /** @return Vertical camera offset in model-relative units. */
    float getCameraY() const { return modelSize > 0.001f ? cameraOffsetY / (modelSize * 0.5f) : 0.0f; }
    /** @return Depth camera offset in model-relative units. */
    float getCameraZ() const { return modelSize > 0.001f ? cameraOffsetZ / (modelSize * 0.5f) : 0.0f; }
    /** @return Largest dimension of the normalized model bounds. */
    float getModelSize() const { return modelSize; }
    ///@}

    /** @name Normalized model rotation */
    ///@{
    /** @brief Sets normalized pitch, where `-1` and `1` approach the poles. */
    void setRotationX(float x) { cameraPitch = x * 89.0f; }
    /** @brief Sets normalized yaw in the range `[-1, 1]`. */
    void setRotationY(float y) { cameraYaw = (y + 1.0f) * 180.0f; }
    /** @brief Sets normalized roll in the range `[-1, 1]`. */
    void setRotationZ(float z) { cameraRoll = z * 180.0f; }
    /** @return Normalized camera pitch. */
    float getRotationX() const { return cameraPitch / 89.0f; }
    /** @return Normalized camera yaw. */
    float getRotationY() const { return (cameraYaw / 180.0f) - 1.0f; }
    /** @return Normalized camera roll. */
    float getRotationZ() const { return cameraRoll / 180.0f; }
    ///@}
    /**
     * @brief Loads a model and derives camera defaults from its bounding box.
     * @param m_file_path Resource path of the model file.
     */
    void loadModelFile(const std::string &m_file_path) {
        if (m_file_path.find("quad") != std::string::npos) {
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
        if (!model->openModel(m_file_path)) {
            throw mx::Exception("Could not open model: " + m_file_path);
        }

        if (!model->meshes.empty()) {
            float minX = std::numeric_limits<float>::max();
            float minY = std::numeric_limits<float>::max();
            float minZ = std::numeric_limits<float>::max();
            float maxX = std::numeric_limits<float>::lowest();
            float maxY = std::numeric_limits<float>::lowest();
            float maxZ = std::numeric_limits<float>::lowest();
            for (const auto &mesh : model->meshes) {
                for (size_t i = 0; i + 2 < mesh.vert.size(); i += 3) {
                    float x = mesh.vert[i];
                    float y = mesh.vert[i + 1];
                    float z = mesh.vert[i + 2];
                    minX = std::min(minX, x);
                    minY = std::min(minY, y);
                    minZ = std::min(minZ, z);
                    maxX = std::max(maxX, x);
                    maxY = std::max(maxY, y);
                    maxZ = std::max(maxZ, z);
                }
            }
            float dx = maxX - minX;
            float dy = maxY - minY;
            float dz = maxZ - minZ;
            float rawSize = std::sqrt(dx * dx + dy * dy + dz * dz);
            if (rawSize < 0.001f)
                rawSize = 1.0f;
            modelCenter = glm::vec3((minX + maxX) * 0.5f, (minY + maxY) * 0.5f, (minZ + maxZ) * 0.5f);
            modelScale = TARGET_MODEL_SIZE / rawSize;
            modelSize = TARGET_MODEL_SIZE;
            printf("Model bounding diagonal: %f, scale: %f, center: (%f, %f, %f)\n",
                   rawSize, modelScale, modelCenter.x, modelCenter.y, modelCenter.z);
        }
    }

    /** @brief Draws the current texture through the active 2D shader. */
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

    /**
     * @brief Draws the loaded 3D model with the active shader.
     * @param win Window providing the projection dimensions.
     * @param textureToUse Optional multipass result; zero selects the source texture.
     */
    void drawModel(gl::GLWindow *win, GLuint textureToUse = 0) {
        GLuint meshTexture = (textureToUse != 0) ? textureToUse : texture;
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);
        glDisable(GL_CULL_FACE);

        static float rotation = 0.0f;
        rotation = fmod(rotation + 0.5f, 360.0f);

        const Uint8 *keystate = SDL_GetKeyboardState(NULL);
        if (!oscillateScale) {

            if (keystate[SDL_SCANCODE_B]) {
                movementSpeed += 0.01f;
                mx::system_out << "acmx2: movement increased: " << movementSpeed << "\n";
                fflush(stdout);
            }

            if (keystate[SDL_SCANCODE_N]) {
                movementSpeed -= 0.01f;
                mx::system_out << "acmx2: movement decreased: " << movementSpeed << "\n";
                fflush(stdout);
            }

            if (keystate[SDL_SCANCODE_EQUALS] || keystate[SDL_SCANCODE_KP_PLUS]) {
                cameraDistance += movementSpeed * modelSize * 0.1f;
                mx::system_out << "acmx2: cameraDistance increased: " << cameraDistance << "\n";
                fflush(stdout);
            }
            if (keystate[SDL_SCANCODE_MINUS] || keystate[SDL_SCANCODE_KP_MINUS]) {
                cameraDistance -= movementSpeed * modelSize * 0.1f;
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
        modelMatrix = glm::scale(modelMatrix, glm::vec3(modelScale));
        modelMatrix = glm::translate(modelMatrix, -modelCenter);
        glm::vec3 cameraPosBase = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 lookDirection;

        if (!viewRotationActive) {
            if (keystate[SDL_SCANCODE_W]) {
                cameraPitch += cameraRotationSpeed * 0.3f;
                if (cameraPitch > 89.0f)
                    cameraPitch = 89.0f;
            }
            if (keystate[SDL_SCANCODE_S]) {
                cameraPitch -= cameraRotationSpeed * 0.33f;
                if (cameraPitch < -89.0f)
                    cameraPitch = -89.0;
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
        cameraPos += glm::vec3(cameraOffsetX, cameraOffsetY, cameraOffsetZ);
        glm::vec3 cameraTarget = cameraPos + lookDirection;
        glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
        if (std::abs(cameraRoll) > 0.001f) {
            float rollRad = glm::radians(cameraRoll);
            glm::vec3 forward = glm::normalize(lookDirection);
            glm::mat4 rollMatrix = glm::rotate(glm::mat4(1.0f), rollRad, forward);
            cameraUp = glm::vec3(rollMatrix * glm::vec4(cameraUp, 0.0f));
        }
        glm::mat4 viewMatrix = glm::lookAt(cameraPos, cameraTarget, cameraUp);
        glm::mat4 projectionMatrix = glm::perspective(
            glm::radians(120.0f),
            static_cast<float>(win->w) / static_cast<float>(win->h),
            0.01f,
            1000.0f);
        glm::mat4 mvMatrix = viewMatrix * modelMatrix;
        gl::ShaderProgram *activeShader;
        activeShader = shaders[currentShaderIndex].get();
        activeShader->useProgram();
        activeShader->setUniform("mv_matrix", mvMatrix);
        activeShader->setUniform("proj_matrix", projectionMatrix);
        activeShader->setUniform("time_f", animation);
        activeShader->setUniform("iTime", animation);
        activeShader->setUniform("iResolution", glm::vec2(canvasWidth, canvasHeight));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, meshTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glUniform1i(glGetUniformLocation(activeShader->id(), "textTexture"), 0);
        model->setShaderProgram(activeShader);
        for (auto &m : model->meshes) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, meshTexture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            m.draw();
        }
        glFrontFace(GL_CCW);
    }

    /**
     * @brief Activates a compiled shader and reconnects it to render objects.
     * @param index Shader index to activate.
     * @param win Window whose render state is refreshed.
     */
    void switchShader(size_t index, gl::GLWindow *win) {
        if (index < shaders.size()) {
            currentShaderIndex = index;
            if (is3d) {
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
            } else {
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
    /** @brief Advances to the next shader, wrapping at the end. */
    void nextShader(gl::GLWindow *win) {
        currentShaderIndex = (currentShaderIndex + 1) % shaders.size();
        switchShader(currentShaderIndex, win);
        mx::system_out << "Switched to shader: " << getShaderNameAt(static_cast<int>(currentShaderIndex)) << "\n";
    }

    /** @brief Selects the previous shader, wrapping at the beginning. */
    void prevShader(gl::GLWindow *win) {
        currentShaderIndex = (currentShaderIndex == 0) ? shaders.size() - 1 : currentShaderIndex - 1;
        switchShader(currentShaderIndex, win);
        mx::system_out << "Switched to shader: " << getShaderNameAt(static_cast<int>(currentShaderIndex)) << "\n";
    }

    /**
     * @brief Updates animation state and renders one frame.
     * @param win Owning OpenGL window.
     */
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
        if (is3d) {
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
        } else {
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
        updateCompatibilityUniforms(is3d ? shaders[currentShaderIndex].get() : shaders2[currentShaderIndex].get());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        update(deltaTime);
        sprite.initSize(canvasWidth, canvasHeight);

        if (multipassEnabled && !shaderPassList.empty()) {
            initPassFBOs(canvasWidth, canvasHeight);
            GLuint inputTex = texture;
            int pingpong = 0;

            glDisable(GL_DEPTH_TEST);
            for (size_t i = 0; i < shaderPassList.size(); ++i) {
                int shaderIdx = shaderPassList[i];
                if (shaderIdx >= 0 && shaderIdx < static_cast<int>(shaders2.size())) {
                    gl::ShaderProgram *passShader = shaders2[shaderIdx].get();
                    if (passShader) {
                        glBindFramebuffer(GL_FRAMEBUFFER, passFBO[pingpong]);
                        glViewport(0, 0, canvasWidth, canvasHeight);
                        glClear(GL_COLOR_BUFFER_BIT);

                        passShader->useProgram();
                        updateShaderUniforms(passShader, deltaTime);
                        passShader->setUniform("textTexture", 0);
                        passShader->setUniform("mv_matrix", glm::mat4(1.0f));
                        passShader->setUniform("proj_matrix", glm::mat4(1.0f));

                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, inputTex);

                        sprite.setShader(passShader);
                        sprite.setName("textTexture");
                        sprite.draw(inputTex, 0, 0, canvasWidth, canvasHeight);

                        inputTex = passTexture[pingpong];
                        pingpong = 1 - pingpong;
                    }
                }
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, canvasWidth, canvasHeight);

            if (is3d) {
                glEnable(GL_DEPTH_TEST);
                glDepthFunc(GL_LESS);
                glDepthMask(GL_TRUE);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                drawModel(win, inputTex);
            } else {

                gl::ShaderProgram *finalShader = shaders2[currentShaderIndex].get();
                finalShader->useProgram();
                updateShaderUniforms(finalShader, deltaTime);
                finalShader->setUniform("textTexture", 0);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, inputTex);

                sprite.setShader(finalShader);
                sprite.setName("textTexture");
                sprite.draw(inputTex, displayX, displayY, displayW, displayH);
            }
        } else {
            if (is3d)
                drawModel(win);
            else
                drawModel2D(win);
        }

        if (captureNextFrame) {
            captureNextFrame = false;
            captureFrame();
        }
    }

    bool mouseDown = false;                 ///< Whether the primary pointer is pressed.
    glm::vec4 mouse = glm::vec4(0.0f);      ///< Shader-style pointer position and click state.

    /**
     * @brief Updates pointer coordinates and button state used by shader uniforms.
     * @param x Canvas-space horizontal coordinate.
     * @param y Canvas-space vertical coordinate.
     * @param pressed Whether the primary pointer is down.
     */
    void setPointerState(float x, float y, bool pressed) {
        mouse.x = std::clamp(x, 0.0f, static_cast<float>(canvasWidth));
        mouse.y = std::clamp(y, 0.0f, static_cast<float>(canvasHeight));
        mouse.z = pressed ? 1.0f : 0.0f;
        mouse.w = pressed ? 1.0f : 0.0f;
        mouseDown = pressed;
    }

    /** @brief Resets shader animation time to zero. */
    void reset() {
        animation = 0.0f;
    }

    /** @name Shader controls */
    ///@{
    /** @brief Sets the animation-speed multiplier. */
    void setSpeed(float value) { iSpeed = value; }
    /** @brief Sets the effect amplitude. */
    void setAmplitude(float value) { iAmplitude = value; }
    /** @brief Sets the effect frequency multiplier. */
    void setFrequency(float value) { iFrequency = value; }
    /** @brief Sets the output brightness multiplier. */
    void setBrightness(float value) { iBrightness = value; }
    /** @brief Sets the output contrast multiplier. */
    void setContrast(float value) { iContrast = value; }
    /** @brief Sets the output saturation multiplier. */
    void setSaturation(float value) { iSaturation = value; }
    /** @brief Sets the hue rotation in radians. */
    void setHueShift(float value) { iHueShift = value; }
    /** @brief Sets the texture-coordinate zoom factor. */
    void setZoom(float value) { iZoom = value; }
    /** @brief Sets the texture-coordinate rotation in radians. */
    void setRotation(float value) { iRotation = value; }
    /** @brief Sets the shader quality hint. */
    void setQuality(float value) { iQuality = value; }
    /** @brief Enables or disables shader debug visualization. */
    void setDebugMode(bool value) { iDebugMode = value ? 1.0f : 0.0f; }

    /** @return Animation-speed multiplier. */
    float getSpeed() const { return iSpeed; }
    /** @return Effect amplitude. */
    float getAmplitude() const { return iAmplitude; }
    /** @return Effect frequency multiplier. */
    float getFrequency() const { return iFrequency; }
    /** @return Output brightness multiplier. */
    float getBrightness() const { return iBrightness; }
    /** @return Output contrast multiplier. */
    float getContrast() const { return iContrast; }
    /** @return Output saturation multiplier. */
    float getSaturation() const { return iSaturation; }
    /** @return Hue rotation in radians. */
    float getHueShift() const { return iHueShift; }
    /** @return Texture-coordinate zoom factor. */
    float getZoom() const { return iZoom; }
    /** @return Texture-coordinate rotation in radians. */
    float getRotation() const { return iRotation; }
    /** @return Shader quality hint. */
    float getQuality() const { return iQuality; }
    /** @return Whether shader debug visualization is enabled. */
    bool getDebugMode() const { return iDebugMode > 0.5f; }
    ///@}

    /** @name Render geometry */
    ///@{
    /** @return Canvas width in pixels. */
    int getCanvasWidth() const { return canvasWidth; }
    /** @return Canvas height in pixels. */
    int getCanvasHeight() const { return canvasHeight; }
    /** @return Horizontal origin of the aspect-fit image rectangle. */
    int getDisplayX() const { return displayX; }
    /** @return Vertical origin of the aspect-fit image rectangle. */
    int getDisplayY() const { return displayY; }
    /** @return Width of the aspect-fit image rectangle. */
    int getDisplayWidth() const { return displayW; }
    /** @return Height of the aspect-fit image rectangle. */
    int getDisplayHeight() const { return displayH; }
    ///@}
    /** @brief Rebinds the source texture and reapplies its sampling parameters. */
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

    /**
     * @brief Updates viewport dimensions and recomputes the aspect-fit rectangle.
     * @param win Window containing the new canvas dimensions.
     */
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

    /** @brief Handles pointer, keyboard, and window events for the demo. */
    void event(gl::GLWindow *win, SDL_Event &e) override {
        switch (e.type) {
        case SDL_FINGERDOWN: {
            Uint32 currentTime = SDL_GetTicks();
            if (firstTapRegistered) {
                Uint32 timeSinceFirstTap = currentTime - firstTapTime;
                if (timeSinceFirstTap >= DOUBLE_TAP_MIN_TIME &&
                    timeSinceFirstTap <= DOUBLE_TAP_MAX_TIME) {
                    nextShader(win);
                    firstTapRegistered = false;
                    firstTapTime = 0;
                } else if (timeSinceFirstTap > DOUBLE_TAP_MAX_TIME) {
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
            if (e.button.button == SDL_BUTTON_LEFT) {
                mouseDown = true;
                mouse.x = static_cast<float>(e.button.x);
                mouse.y = static_cast<float>(win->h - e.button.y);
                mouse.z = 1.0f;
                mouse.w = 1.0f;
            }
            break;
        case SDL_MOUSEBUTTONUP:
            if (e.button.button == SDL_BUTTON_LEFT) {
                mouseDown = false;
                mouse.z = 0.0f;
                mouse.w = 0.0f;
            }
            break;
        case SDL_MOUSEMOTION:
            mouse.x = static_cast<float>(e.motion.x);
            mouse.y = static_cast<float>(win->h - e.motion.y);
            if (mouseDown) {
                mouse.z = 1.0f;
            }
            break;
        case SDL_KEYDOWN:
            if (e.key.keysym.sym == SDLK_DOWN || e.key.keysym.sym == SDLK_SPACE) {
                nextShader(win);
            } else if (e.key.keysym.sym == SDLK_UP || e.key.keysym.sym == SDLK_BACKSPACE) {
                prevShader(win);
            }
            break;
        }
    }
    /** @brief Advances model animation state by @p deltaTime seconds. */
    void update(float deltaTime) {
        if (firstTapRegistered) {
            Uint32 currentTime = SDL_GetTicks();
            if ((currentTime - firstTapTime) > DOUBLE_TAP_MAX_TIME) {
                firstTapRegistered = false;
                firstTapTime = 0;
            }
        }
    }

    /**
     * @brief Compiles and activates a user-provided fragment shader.
     * @param fragmentSource GLSL ES fragment-shader source.
     * @param win Window whose render objects receive the new program.
     * @return Empty text on success, otherwise a human-readable error message.
     */
    std::string compileCustomShader(const std::string &fragmentSource, gl::GLWindow *win) {
        if (usesUnsupportedRuntimeInputs("custom", fragmentSource)) {
            return "ERROR: Shader uses unsupported audio, FFT, spectrum, or texture-cache inputs.";
        }
        const std::string convertedSource = convertFragmentShaderToWebGL(fragmentSource);
        auto customShader1 = std::make_unique<gl::ShaderProgram>();
        auto customShader2 = std::make_unique<gl::ShaderProgram>();
        if (!customShader1->loadProgramFromText(sz3DVertex, convertedSource.c_str())) {
            return "ERROR: Failed to create shader program.";
        }
        if (!customShader2->loadProgramFromText(gl::vSource, convertedSource.c_str())) {
            return "ERROR: Failed to create shader program.";
        }

        customShader1->setSilent(true);
        customShader2->setSilent(true);
        static bool hasCustomShader = false;
        static const std::string customShaderName = "Custom Shader";
        if (hasCustomShader && !shaders.empty() && !shaders2.empty()) {
            shaders.back() = std::move(customShader1);
            shaders2.back() = std::move(customShader2);
            if (!shader_names.empty()) {
                shader_names.back() = customShaderName;
            } else {
                shader_names.push_back(customShaderName);
            }
        } else {
            shaders.push_back(std::move(customShader1));
            shaders2.push_back(std::move(customShader2));
            shader_names.push_back(customShaderName);
            hasCustomShader = true;
        }
        currentShaderIndex = shaders.size() - 1;
        switchShader(currentShaderIndex, win);
        return "SUCCESS: Shader compiled and applied successfully!";
    }
    /** @brief Restores the first built-in shader. */
    void resetToDefaultShader(gl::GLWindow *win) {
        if (!shaders.empty()) {
            currentShaderIndex = 0;
            switchShader(0, win);
        }
    }

    /** @brief Schedules a frame capture at integer output @p scale. */
    void saveImage(int scale) {
        captureNextFrame = true;
        captureScale = scale;
    }

    /** @brief Reads the current framebuffer and emits a PNG capture. */
    void captureFrame() {
        int readW = canvasWidth;
        int readH = canvasHeight;

        printf("Capturing: canvas=%dx%d, display=(%d,%d) %dx%d, tex=%dx%d, scale=%d, is3d=%d\n",
               readW, readH, displayX, displayY, displayW, displayH, texWidth, texHeight, captureScale, is3d ? 1 : 0);

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
            uint8_t *top = pixels.data() + y * stride;
            uint8_t *bot = pixels.data() + (readH - 1 - y) * stride;
            memcpy(row.data(), top, stride);
            memcpy(top, bot, stride);
            memcpy(bot, row.data(), stride);
        }

        int cropX, cropY, cropW, cropH;
        int finalW, finalH;

        if (is3d) {
            cropX = 0;
            cropY = 0;
            cropW = canvasWidth;
            cropH = canvasHeight;
            finalW = cropW * captureScale;
            finalH = cropH * captureScale;
        } else {
            cropX = displayX;
            cropY = canvasHeight - displayY - displayH;
            cropW = displayW;
            cropH = displayH;
            finalW = texWidth * captureScale;
            finalH = texHeight * captureScale;
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
                var timestamp = now.getFullYear() + '-' + 
                    String(now.getMonth() + 1).padStart(2, '0') + '-' + 
                    String(now.getDate()).padStart(2, '0') + 'T' +
                    String(now.getHours()).padStart(2, '0') + '-' + 
                    String(now.getMinutes()).padStart(2, '0') + '-' + 
                    String(now.getSeconds()).padStart(2, '0');

                var dataUrl = outCanvas.toDataURL('image/png');
                var filename = 'acmx2.visualizer.' + finalW + 'x' + finalH + '.' + timestamp + '.png';

                if (typeof window.AndroidInterface !== 'undefined' &&
                    typeof window.AndroidInterface.saveImage === 'function') {
                    window.AndroidInterface.saveImage(dataUrl);
                } else {
                    var link = document.createElement('a');
                    link.download = filename;
                    link.href = dataUrl;
                    document.body.appendChild(link);
                    link.click();
                    document.body.removeChild(link);
                }
                
                console.log('Image saved:', finalW, 'x', finalH);
            } catch (e) {
                console.error('Save error:', e);
            } }, cropW, cropH, croppedPixels.data(), finalW, finalH);
#endif
        printf("Frame captured and saved\n");
    }
};

/** @brief Top-level OpenGL window that forwards events and drawing to About. */
class MainWindow : public gl::GLWindow {
  public:
    /** @brief Creates the window and initializes its render controller. */
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("ACMX2 - Interactive Visualizer", tw, th) {
        setPath(path);
        setObject(new About());
        object->load(this);
    }

    /** @brief Destroys the application window. */
    ~MainWindow() override {}

    /** @brief Forwards an SDL event to the active render object. */
    virtual void event(SDL_Event &e) override {
        object->event(this, e);
    }

    /** @brief Clears, renders, presents, and paces one frame. */
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

MainWindow *main_w = nullptr; ///< Active application window used by web callbacks.
About *about_ptr = nullptr;   ///< Active render controller used by web callbacks.

#ifdef __EMSCRIPTEN__

/** @defgroup web_api JavaScript API
 *  @brief Functions exported to JavaScript through Emscripten bindings.
 *  @{
 */

/** @brief Activates a built-in shader by index. */
void setShaderIndex(int index) {
    if (about_ptr && main_w && index >= 0) {
        about_ptr->switchShader(static_cast<size_t>(index), main_w);
    }
}

/** @brief Enables or disables textured 3D-model rendering. */
void set3D(bool value) {
    if (about_ptr) {
        about_ptr->set3DMode(value);
    }
}

/** @brief Supplies canvas-space pointer state from JavaScript. */
void setMouseInput(float x, float y, bool pressed) {
    if (about_ptr) {
        about_ptr->setPointerState(x, y, pressed);
    }
}

/** @brief Loads a packaged compressed model named by @p info. */
void loadModel(const std::string &info) {
    if (about_ptr) {
        about_ptr->loadModelFile("data/compressed/" + info);
    }
}

/** @brief Synchronizes native render dimensions with the HTML canvas. */
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

/** @brief Reapplies texture bindings after a browser context-state change. */
void forceTextureRebindWeb() {
    if (about_ptr) {
        about_ptr->forceTextureRebind();
    }
}

/** @brief Requests a PNG capture at integer output @p scale. */
void saveImageWeb(int scale) {
    if (about_ptr) {
        about_ptr->saveImage(scale);
    }
}

/** @brief Selects the next built-in shader. */
void nextShaderWeb() {
    if (about_ptr) {
        about_ptr->nextShader(main_w);
    }
}

/** @brief Selects the previous built-in shader. */
void prevShaderWeb() {
    if (about_ptr) {
        about_ptr->prevShader(main_w);
    }
}

/** @brief Decodes PNG bytes and replaces the source texture. */
void loadImagePNG(const std::vector<uint8_t> &imageData) {
    std::ofstream outFile("image.png", std::ios::binary);
    outFile.write(reinterpret_cast<const char *>(imageData.data()), imageData.size());
    outFile.close();

    SDL_Surface *surface = png::LoadPNG("image.png");
    if (!surface) {
        mx::system_err << "Failed to load PNG image\n";
        return;
    }

    SDL_Surface *converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(surface);

    if (converted && main_w && main_w->object) {
        About *about = dynamic_cast<About *>(main_w->object.get());
        if (about) {
            about->loadNewTexture(converted, main_w);
        }
    }
    if (converted) {
        SDL_FreeSurface(converted);
    }
}

/** @brief Decodes JPEG bytes and replaces the source texture. */
void loadImageJPG(const std::vector<uint8_t> &imageData) {
    std::ofstream outFile("image.jpg", std::ios::binary);
    outFile.write(reinterpret_cast<const char *>(imageData.data()), imageData.size());
    outFile.close();
    SDL_Surface *surface = IMG_Load("image.jpg");
    if (!surface) {
        mx::system_err << "Failed to load JPG image: " << IMG_GetError() << "\n";
        return;
    }

    SDL_Surface *converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(surface);

    if (converted && main_w && main_w->object) {
        About *about = dynamic_cast<About *>(main_w->object.get());
        if (about) {
            about->loadNewTexture(converted, main_w);
        }
    }
    if (converted) {
        SDL_FreeSurface(converted);
    }
}

/**
 * @brief Replaces the source texture from tightly packed RGBA bytes.
 * @param rgbaData Pixel buffer containing exactly `width * height * 4` bytes.
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 */
void loadImageRGBA(const std::vector<uint8_t> &rgbaData, int width, int height) {
    if (rgbaData.size() != static_cast<size_t>(width * height * 4)) {
        mx::system_err << "Invalid RGBA data size. Expected " << (width * height * 4) << " but got " << rgbaData.size() << "\n";
        return;
    }

    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormatFrom(
        const_cast<uint8_t *>(rgbaData.data()),
        width, height,
        32,
        width * 4,
        SDL_PIXELFORMAT_RGBA32);

    if (!surface) {
        mx::system_err << "Failed to create surface from RGBA data: " << SDL_GetError() << "\n";
        return;
    }

    SDL_Surface *converted = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA32);
    if (!converted) {
        mx::system_err << "Failed to create converted surface: " << SDL_GetError() << "\n";
        SDL_FreeSurface(surface);
        return;
    }

    SDL_BlitSurface(surface, NULL, converted, NULL);
    SDL_FreeSurface(surface);

    if (converted && main_w && main_w->object) {
        About *about = dynamic_cast<About *>(main_w->object.get());
        if (about) {
            about->loadNewTexture(converted, main_w);
        }
    }
    if (converted) {
        SDL_FreeSurface(converted);
    }
}

/**
 * @brief Replaces the source texture from an RGBA buffer in WebAssembly memory.
 * @param dataPtr Address of the first pixel byte.
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 */
void loadImageRGBAPtr(uintptr_t dataPtr, int width, int height) {
    uint8_t *rgbaData = reinterpret_cast<uint8_t *>(dataPtr);
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormatFrom(
        rgbaData,
        width, height,
        32,
        width * 4,
        SDL_PIXELFORMAT_RGBA32);

    if (!surface) {
        mx::system_err << "Failed to create surface from RGBA data: " << SDL_GetError() << "\n";
        return;
    }

    if (main_w && main_w->object) {
        About *about = dynamic_cast<About *>(main_w->object.get());
        if (about) {
            about->loadNewTexture(surface, main_w);
        }
    }

    SDL_FreeSurface(surface);
}

/** @return Empty text on successful custom-shader compilation, or an error message. */
std::string compileCustomShaderWeb(const std::string &fragmentSource) {
    if (about_ptr && main_w) {
        return about_ptr->compileCustomShader(fragmentSource, main_w);
    }
    return "ERROR: Application not initialized";
}

/** @return Display name for shader @p i, or an empty string when unavailable. */
std::string getShaderNameAt(int i) {
    if (about_ptr) {
        return about_ptr->getShaderNameAt(i);
    }
    return "";
}

/** @return Number of compiled built-in shaders. */
int getShaderCount() {
    if (about_ptr) {
        return about_ptr->getShaderCount();
    }
    return 0;
}

/** @brief Restores the first built-in shader. */
void resetToDefaultShaderWeb() {
    if (about_ptr && main_w) {
        about_ptr->resetToDefaultShader(main_w);
    }
}

/** @brief Resets shader animation time. */
void reset_time() {
    if (about_ptr && main_w) {
        about_ptr->reset();
    }
}

/** @brief Sets the animation-speed uniform. */
void setUniformSpeed(float value) {
    if (about_ptr)
        about_ptr->setSpeed(value);
}

/** @brief Sets the effect-amplitude uniform. */
void setUniformAmplitude(float value) {
    if (about_ptr)
        about_ptr->setAmplitude(value);
}

/** @brief Sets the effect-frequency uniform. */
void setUniformFrequency(float value) {
    if (about_ptr)
        about_ptr->setFrequency(value);
}

/** @brief Sets the brightness uniform. */
void setUniformBrightness(float value) {
    if (about_ptr)
        about_ptr->setBrightness(value);
}

/** @brief Sets the contrast uniform. */
void setUniformContrast(float value) {
    if (about_ptr)
        about_ptr->setContrast(value);
}

/** @brief Sets the saturation uniform. */
void setUniformSaturation(float value) {
    if (about_ptr)
        about_ptr->setSaturation(value);
}

/** @brief Sets the hue-shift uniform in radians. */
void setUniformHueShift(float value) {
    if (about_ptr)
        about_ptr->setHueShift(value);
}

/** @brief Sets the texture-coordinate zoom uniform. */
void setUniformZoom(float value) {
    if (about_ptr)
        about_ptr->setZoom(value);
}

/** @brief Sets the texture-coordinate rotation uniform in radians. */
void setUniformRotation(float value) {
    if (about_ptr)
        about_ptr->setRotation(value);
}

/** @brief Sets the shader quality hint. */
void setUniformQuality(float value) {
    if (about_ptr)
        about_ptr->setQuality(value);
}

/** @brief Enables or disables shader debug visualization. */
void setUniformDebugMode(bool value) {
    if (about_ptr)
        about_ptr->setDebugMode(value);
}

/** @return Current animation-speed uniform. */
float getUniformSpeed() {
    if (about_ptr)
        return about_ptr->getSpeed();
    return 1.0f;
}

/** @return Current effect-amplitude uniform. */
float getUniformAmplitude() {
    if (about_ptr)
        return about_ptr->getAmplitude();
    return 1.0f;
}

/** @return Current effect-frequency uniform. */
float getUniformFrequency() {
    if (about_ptr)
        return about_ptr->getFrequency();
    return 1.0f;
}

/** @return Current brightness uniform. */
float getUniformBrightness() {
    if (about_ptr)
        return about_ptr->getBrightness();
    return 1.0f;
}

/** @return Current contrast uniform. */
float getUniformContrast() {
    if (about_ptr)
        return about_ptr->getContrast();
    return 1.0f;
}

/** @return Current saturation uniform. */
float getUniformSaturation() {
    if (about_ptr)
        return about_ptr->getSaturation();
    return 1.0f;
}

/** @return Current hue-shift uniform in radians. */
float getUniformHueShift() {
    if (about_ptr)
        return about_ptr->getHueShift();
    return 0.0f;
}

/** @return Current texture-coordinate zoom uniform. */
float getUniformZoom() {
    if (about_ptr)
        return about_ptr->getZoom();
    return 1.0f;
}

/** @return Current texture-coordinate rotation uniform in radians. */
float getUniformRotation() {
    if (about_ptr)
        return about_ptr->getRotation();
    return 0.0f;
}

/** @return Current shader quality hint. */
float getUniformQuality() {
    if (about_ptr)
        return about_ptr->getQuality();
    return 1.0f;
}

/** @return Whether shader debug visualization is enabled. */
bool getUniformDebugMode() {
    if (about_ptr)
        return about_ptr->getDebugMode();
    return false;
}

/** @return Canvas width in pixels. */
int getCanvasWidthWeb() {
    if (about_ptr)
        return about_ptr->getCanvasWidth();
    return 1280;
}

/** @return Canvas height in pixels. */
int getCanvasHeightWeb() {
    if (about_ptr)
        return about_ptr->getCanvasHeight();
    return 720;
}

/** @return Horizontal origin of the aspect-fit display rectangle. */
int getDisplayXWeb() {
    if (about_ptr)
        return about_ptr->getDisplayX();
    return 0;
}

/** @return Vertical origin of the aspect-fit display rectangle. */
int getDisplayYWeb() {
    if (about_ptr)
        return about_ptr->getDisplayY();
    return 0;
}

/** @return Width of the aspect-fit display rectangle. */
int getDisplayWidthWeb() {
    if (about_ptr)
        return about_ptr->getDisplayWidth();
    return 1280;
}

/** @return Height of the aspect-fit display rectangle. */
int getDisplayHeightWeb() {
    if (about_ptr)
        return about_ptr->getDisplayHeight();
    return 720;
}

/** @return Index of the active shader. */
int getIndex() {
    if (about_ptr)
        return about_ptr->getShaderIndex();
    return 0;
}

/** @brief Applies an incremental model-camera pitch gesture. */
void touchRotateX(float delta) {
    if (about_ptr)
        about_ptr->adjustCameraPitch(delta);
}

/** @brief Applies an incremental model-camera yaw gesture. */
void touchRotateY(float delta) {
    if (about_ptr)
        about_ptr->adjustCameraYaw(delta);
}

/** @brief Applies an incremental model-camera zoom gesture. */
void touchZoom(float delta) {
    if (about_ptr)
        about_ptr->adjustCameraDistance(delta);
}

/** @brief Sets normalized horizontal model-camera translation. */
void setCameraXWeb(float x) {
    if (about_ptr)
        about_ptr->setCameraX(x);
}
/** @brief Sets normalized vertical model-camera translation. */
void setCameraYWeb(float y) {
    if (about_ptr)
        about_ptr->setCameraY(y);
}
/** @brief Sets normalized depth model-camera translation. */
void setCameraZWeb(float z) {
    if (about_ptr)
        about_ptr->setCameraZ(z);
}
/** @return Normalized horizontal model-camera translation. */
float getCameraXWeb() {
    if (about_ptr)
        return about_ptr->getCameraX();
    return 0.0f;
}
/** @return Normalized vertical model-camera translation. */
float getCameraYWeb() {
    if (about_ptr)
        return about_ptr->getCameraY();
    return 0.0f;
}
/** @return Normalized depth model-camera translation. */
float getCameraZWeb() {
    if (about_ptr)
        return about_ptr->getCameraZ();
    return 0.0f;
}
/** @return Largest dimension of the loaded model. */
float getModelSizeWeb() {
    if (about_ptr)
        return about_ptr->getModelSize();
    return 1.0f;
}

/** @brief Sets normalized model pitch. */
void setRotationXWeb(float x) {
    if (about_ptr)
        about_ptr->setRotationX(x);
}
/** @brief Sets normalized model yaw. */
void setRotationYWeb(float y) {
    if (about_ptr)
        about_ptr->setRotationY(y);
}
/** @brief Sets normalized model roll. */
void setRotationZWeb(float z) {
    if (about_ptr)
        about_ptr->setRotationZ(z);
}
/** @return Normalized model pitch. */
float getRotationXWeb() {
    if (about_ptr)
        return about_ptr->getRotationX();
    return 0.0f;
}
/** @return Normalized model yaw. */
float getRotationYWeb() {
    if (about_ptr)
        return about_ptr->getRotationY();
    return 0.0f;
}
/** @return Normalized model roll. */
float getRotationZWeb() {
    if (about_ptr)
        return about_ptr->getRotationZ();
    return 0.0f;
}

/** @brief Enables or disables multipass rendering. */
void enableMultipassWeb(bool enable) {
    if (about_ptr)
        about_ptr->enableMultipass(enable);
}

/** @return Whether multipass rendering is enabled. */
bool isMultipassEnabledWeb() {
    if (about_ptr)
        return about_ptr->isMultipassEnabled();
    return false;
}

/** @brief Clears the multipass shader chain. */
void clearShaderPassesWeb() {
    if (about_ptr)
        about_ptr->clearShaderPasses();
}

/** @brief Appends @p shaderIndex to the multipass chain when valid. */
void addShaderPassWeb(int shaderIndex) {
    if (about_ptr)
        about_ptr->addShaderPass(shaderIndex);
}

/** @brief Replaces the multipass chain with @p passes. */
void setShaderPassesWeb(std::vector<int> passes) {
    if (about_ptr)
        about_ptr->setShaderPasses(passes);
}

/** @return A copy of the multipass shader-index chain. */
std::vector<int> getShaderPassesWeb() {
    if (about_ptr)
        return about_ptr->getShaderPasses();
    return std::vector<int>();
}

EMSCRIPTEN_BINDINGS(image_loader) {
    emscripten::function("nextShaderWeb", &nextShaderWeb);
    emscripten::function("prevShaderWeb", &prevShaderWeb);
    emscripten::function("compileCustomShader", &compileCustomShaderWeb);
    emscripten::function("resetToDefaultShader", &resetToDefaultShaderWeb);
    emscripten::register_vector<uint8_t>("VectorU8");
    emscripten::function("loadImagePNG", &loadImagePNG);
    emscripten::function("loadImageJPG", &loadImageJPG);
    emscripten::function("loadImageRGBA", &loadImageRGBA);
    emscripten::function("loadImageRGBAPtr", &loadImageRGBAPtr);
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
    emscripten::function("touchRotateX", &touchRotateX);
    emscripten::function("touchRotateY", &touchRotateY);
    emscripten::function("touchZoom", &touchZoom);
    emscripten::function("setCameraX", &setCameraXWeb);
    emscripten::function("setCameraY", &setCameraYWeb);
    emscripten::function("setCameraZ", &setCameraZWeb);
    emscripten::function("getCameraX", &getCameraXWeb);
    emscripten::function("getCameraY", &getCameraYWeb);
    emscripten::function("getCameraZ", &getCameraZWeb);
    emscripten::function("getModelSize", &getModelSizeWeb);
    emscripten::function("setRotationX", &setRotationXWeb);
    emscripten::function("setRotationY", &setRotationYWeb);
    emscripten::function("setRotationZ", &setRotationZWeb);
    emscripten::function("getRotationX", &getRotationXWeb);
    emscripten::function("getRotationY", &getRotationYWeb);
    emscripten::function("getRotationZ", &getRotationZWeb);
    emscripten::function("getIndex", &getIndex);
    emscripten::function("loadModel", &loadModel);
    emscripten::function("setShader3DMode", &set3D);
    emscripten::function("setMouseInput", &setMouseInput);
    emscripten::function("setShaderIndex", &setShaderIndex);
    emscripten::function("getShaderCount", &getShaderCount);
    emscripten::function("getShaderNameAt", &getShaderNameAt);
    emscripten::function("enableMultipass", &enableMultipassWeb);
    emscripten::function("isMultipassEnabled", &isMultipassEnabledWeb);
    emscripten::function("clearShaderPasses", &clearShaderPassesWeb);
    emscripten::function("addShaderPass", &addShaderPassWeb);
    emscripten::function("setShaderPasses", &setShaderPassesWeb);
    emscripten::function("getShaderPasses", &getShaderPassesWeb);
    emscripten::register_vector<int>("VectorInt");
};

/** @} */

#endif

/** @brief Runs one iteration of the Emscripten application loop. */
void eventProc() {
    main_w->proc();
}

/**
 * @brief Application entry point.
 * @return `EXIT_FAILURE` when initialization throws; otherwise zero.
 */
int main(int argc, char **argv) {
#ifdef __EMSCRIPTEN__
    try {
        MainWindow main_window("/", 1920, 1080);
        main_w = &main_window;
        about_ptr = dynamic_cast<About *>(main_w->object.get());
        emscripten_set_main_loop(eventProc, 0, 1);
    } catch (mx::Exception &e) {
        std::cerr << e.text() << "\n";
        return EXIT_FAILURE;
    }
#endif
    return 0;
}
