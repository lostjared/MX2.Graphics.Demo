# MX2.Graphics.Demo
MX2 Web Assembly Graphics Demo
# MX2 - Interactive Graphics Demo

A powerful web-based and desktop graphics demonstration application featuring real-time shader effects, interactive controls, and custom shader compilation.

## Features

### 🎨 38+ Built-in Shaders
Explore a diverse collection of visually stunning shader effects including:
- **Kaleidoscope effects** - Symmetrical fractal patterns with real-time transformations
- **Distortion effects** - Wave, ripple, twist, and bend distortions
- **Psychedelic effects** - Color shifts, rainbow effects, and chromatic aberration
- ** 3D transformations** - Perspective warping, 3D rotations, and projections
- **Noise & chaos** - Scrambling, VHS effects, and digital artifacts
- **Interactive effects** - Mouse-controlled swirls, zooms, and fractal folding

### ✏️ Custom Shader Editor
Write and compile your own GLSL ES 3.0 fragment shaders with:
- Real-time shader code editing
- Instant compilation feedback with detailed error messages
- Pre-built shader templates to get started
- Live preview of your custom effects
- Shader compilation validation before applying

### 🖼️ Image Loading
- Load PNG and JPG images directly into the application
- Apply any shader effect to your own images
- Supports drag-and-drop or file selection
- Dynamic texture replacement at runtime

### 💾 Screenshot Capture
- Save the current rendered output as PNG
- Automatically timestamped filenames
- High-quality WebGL canvas capture

### ⌨️ Keyboard & Mouse Controls
- **Arrow Keys / Spacebar** - Cycle through shaders
- **Mouse Movement** - Control interactive shader parameters
- **Mouse Click & Drag** - Manipulate focal points in mouse-aware effects
- **Custom Shader Panel** - Press `✏️ Custom Shader` button to edit

## Installation

### Web Version (Emscripten)

```bash
# Build with Emscripten
emcripten++ about.cpp -o MX2_app.js \
  -s WASM=1 \
  -s USE_SDL=2 \
  -s USE_SDL_IMAGE=2 \
  -s SDL2_IMAGE_FORMATS='["png","jpg"]' \
  -s USE_LIBJPEG=1 \
  -s GL_PREINITIALIZED_CONTEXT=1 \
  -s GL_ENABLE_GET_PROC_ADDRESS=1 \
  --bind \
  -O3
```

### Desktop Version

```bash
# Requires: SDL2, SDL2_image, OpenGL 3.0+, libpng

# Compile
g++ -std=c++17 about.cpp -o mx2_demo \
  -lSDL2 -lSDL2_image -lGL -lpng

# Run
./mx2_demo -p /path/to/assets -R 1280x720
```

## Usage

### Starting the Application

**Web:**
Open `index.html` in a modern web browser (Chrome, Firefox, Edge)

**Desktop:**
```bash
./mx2_demo -p ./assets -R 1280x720
```

### Navigation

1. **Browsing Shaders:**
   - Click `◀ Prev` and `Next ▶` buttons
   - Use arrow keys (↑↓) or spacebar
   - Each shader is instantly applied

2. **Loading Custom Images:**
   - Click `🖼️ Load Image` button
   - Select a PNG or JPG file
   - The image is loaded into the current shader
   - Try different shaders on your image

3. **Creating Custom Shaders:**
   - Click `✏️ Custom Shader` button
   - The shader editor modal opens
   - Choose a template or start from scratch
   - Edit the GLSL code
   - Click `▶ Compile & Apply` to test
   - Compilation errors appear in red in the log
   - Reset to defaults with `🔄 Reset`

4. **Saving Screenshots:**
   - Click `💾 Save Image` button
   - Or press `S` key
   - PNG saves with timestamp: `mx2_shader_YYYYMMDD_HHMMSS.png`

## Shader Uniforms Available

When writing custom shaders, you have access to:

```glsl
uniform sampler2D textTexture;  // Input texture/image
uniform float time_f;           // Animation time in seconds
uniform vec2 iResolution;       // Screen resolution (width, height)
uniform vec4 iMouse;            // Mouse state (x, y, button_pressed, dragging)
uniform float alpha;            // Opacity (0.0 - 1.0)
uniform float amp;              // Amplitude parameter (0.0 - 1.0)
uniform float uamp;             // User amplitude (0.0 - 1.0)
```

## Example Custom Shader

```glsl
#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;
uniform vec4 iMouse;

void main(void) {
    vec2 tc = TexCoord;
    vec2 center = iMouse.xy / iResolution;
    vec2 dist = tc - center;
    float r = length(dist);
    
    // Ripple effect from mouse
    float ripple = sin(r * 30.0 - time_f * 5.0) * 0.02;
    vec2 displaced = tc + normalize(dist + 0.001) * ripple;
    
    color = texture(textTexture, displaced);
}
```

## Built-in Shader List

1. Bubble - Bubble distortion effect
2. Kaleidoscope - Symmetrical fractal patterns
3. KaleidoscopeAlt - Alternative kaleidoscope
4. Scramble - Digital scrambling
5. Swirl - Smooth swirl distortion
6. Mirror - Mirrored reflection
7. VHS - VCR/VHS tape effect
8. Twist - Twist and rotation
9. Time - Time-based morphing
10. Bend - Bending distortion
11. Pong - Logarithmic spiral animation
12. PsychWave - Psychedelic wave patterns
13. CrystalBall - Crystal ball effect
14. ZoomMouse - Mouse-controlled zoom
15. Drain - Spiral drain effect
16. Ufo - UFO distortion
17. Wave - Diagonal wave patterns
18. UfoWarp - UFO with warping
19. ByMouse - Mouse interaction
20. Deform - Organic deformation
21. Geo - Geometric patterns
22. Smooth - Smooth blur and color
23. Hue - Hue shifting
24. kMouse - Kaleidoscope with mouse
25. Drum - Drum ripple effect
26. ColorSwirl - Color swirling
27. MouseZoom - Mouse-based zoom
28. Rev2 - Mirrored reflection
29. Fish - Fisheye lens effect
30. RipplePrism - Ripple with color separation
31. MirrorMouse - Mouse-controlled mirror
32. FracMouse - Fractal folding with mouse
33. Cats - Psychedelic XOR effects
34. ColorMouse - Color manipulation with mouse
35. TwistFull - Full-screen twist
36. BowlByTime - Time-based bowl effect
37. Pebble - Ripple from pebble
38. MirrorPutty - Putty-like mirroring

## Controls Quick Reference

| Action | Web | Desktop |
|--------|-----|---------|
| Next Shader | `Next ▶` button or `↓` | `↓` or `Space` |
| Prev Shader | `◀ Prev` button or `↑` | `↑` or `Backspace` |
| Load Image | `🖼️ Load Image` button | - |
| Save Screenshot | `💾 Save Image` or `S` | - |
| Open Shader Editor | `✏️ Custom Shader` button | - |
| Close Editor | `✕ Close` or `Esc` | - |
| Interact with Effect | Mouse movement/click | Mouse movement/click |

## Command Line Options (Desktop)

```
-p, --path PATH          Path to assets directory
-r, -R WIDTHxHEIGHT     Set resolution (e.g., 1920x1080)
-h                       Show help message

...

## Troubleshooting

### Shader won't compile
- Check GLSL syntax against ES 3.0 specification
- Ensure all variable types match (vec2, vec3, vec4, float, int)
- Look at the compilation log for specific errors
- Try a template as a starting point

### Image won't load
- Ensure file is PNG or JPG format
- Check file size isn't too large
- Try a different image file
- Check browser console for errors

### Custom shader not showing
- Click `▶ Compile & Apply` button
- Check compilation log for errors
- Ensure the shader has a `main()` function
- Verify all uniforms are declared

### Poor performance
- Close the shader editor modal when not in use
- Reduce shader complexity (fewer loops, simpler math)
- Lower canvas resolution on older devices

## Development

### Project Structure
```
libmx2/gl_about/
├── graphics.cpp           # Main application and shader definitions
├── index.html          # Web interface and controls
├── Makefile.em         # Emscripten build configuration
└── data/
    └── logo.png        # Default texture
```

### Adding New Shaders

1. Create shader source string:
```cpp
const char *szNewShader = R"(#version 300 es
// ... shader code ...
)";
```

2. Add to `shaderSources` vector:
```cpp
{"ShaderName", szNewShader}
```

### Building Custom Versions

Modify shader uniforms, add new features, or integrate with other GL applications by linking against the library components.

## License

Part of the MX2 Graphics Library collection.

## Credits

Shader effects created with inspiration from:
- Shadertoy community
- Demoscene artists
- Interactive graphics researchers

## Performance Notes

- Web version runs in WebGL 2.0 (hardware-accelerated)
- Desktop version uses OpenGL 3.0+
- 60 FPS target on modern hardware
- Shader compilation happens in real-time
- Image loading supports up to 4K resolution

---

