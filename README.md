# ACMX2 Graphics Demo v2.0.0 Beta

ACMX2 Graphics Demo is an interactive WebGL 2 visualizer built with C++,
libmx2, Emscripten, and JavaScript. It applies real-time GLSL effects to images,
live cameras, and video files, and includes recording, custom shader editing,
3D models, and multipass shader chains.

![browser](https://github.com/user-attachments/assets/c669d7dc-45ae-4cc9-b9ee-c608101f5756)

<img width="3840" height="2112" alt="ACMX2 Graphics Demo" src="https://github.com/user-attachments/assets/6959b33c-b67d-47a9-af8e-80ecb4abf163">

The project uses [libmx2](https://github.com/lostjared/libmx2). The shader
collection contains original work and shaders adapted from multiple creative and
experimental sources.

## What's New in v2.0.0 Beta

- More shaders, loaded from alphabetized shader indexes.
- Cached WebGL 2 shader conversion for faster startup.
- Two-stage startup screen with byte-level WASM/data download progress followed
  by live shader-compilation output.
- GLSL 3.30-to-GLSL ES 3.00 compatibility conversion.
- Detailed startup shader failure logs.
- Cached shader browser in the custom shader editor.
- Live camera and local video-file input.
- WebM and MP4 recording with quality and frame-rate controls.
- Improved Android save/share support for recordings.
- Portrait and landscape mobile recording support.
- Responsive camera, recording, and landscape controls.
- 2D mouse and touch input through `iMouse`.
- Multipass shader chains with search, reordering, sharing, and backups.
- A v2.0 beta launcher on `visualizer.html` linking to `/visualizer2/`.

## Features

### Shader Library

- More than 1,000 WebGL-compatible cached shaders in the current build.
- Kaleidoscope, fractal, mirror, distortion, liquid, VHS, glitch, color,
  geometric, and other real-time effects.
- Alphabetized shader selectors and source/cache indexes.
- Automatic WebGL 2 compatibility fixes for desktop GLSL sources.
- Shaders requiring unsupported FFT, spectrum, audio-data, or texture-cache
  inputs are skipped silently instead of being compiled.

The current source index contains 2,036 entries. The generated WebGL cache
contains 1,030 compatible external shaders, in addition to the built-in shader
collection.

### Custom Shader Editor

- Edit and compile GLSL ES 3.00 fragment shaders in the browser.
- View compiler output and detailed errors without replacing the active shader.
- Start with basic, ripple, and color-shift templates.
- Browse every cached shader from a combobox, load it into the editor, modify it,
  and apply it as a custom shader.

### Image, Camera, and Video Input

- Load PNG and JPEG images.
- Stream a device camera through the active shader.
- Select front/rear cameras and requested camera resolution.
- Enable microphone input separately when recording is needed.
- Load a local video by file picker or drag and drop.
- Play, seek, loop, and stream the video through the shader pipeline.
- Optionally include the video or camera audio track in recordings.

Camera and microphone features require HTTPS or a secure localhost context.

### 2D and 3D Interaction

- Desktop movement, clicks, and drags update `iMouse` in 2D mode.
- Mobile touches behave like mouse presses, drags, and releases in 2D mode.
- 3D mode retains drag-to-rotate and pinch-to-zoom gestures.
- Choose from the included compressed 3D models.
- Use the uniform controls to adjust color, animation, transform, quality,
  camera, and model rotation values.

### Screenshots

- Save the current shader output as PNG.
- Choose 1x, 2x, 3x, or 4x output scaling.
- Output names include the final dimensions and timestamp, for example:
  `acmx2.visualizer.3840x2160.2026-07-18T12-00-00.png`.
- Android WebView builds can save through the native image bridge.

### Video Recording

- Record at 24, 30, or 60 FPS.
- Choose WebM or MP4 when supported by the browser.
- Balanced, High, and Ultra quality profiles.
- Balanced mobile output uses a 720-pixel short edge, High uses 1080, and Ultra
  uses 1440.
- Mobile recordings use portrait or landscape output based on the content
  orientation when recording begins.
- Rotation during recording preserves the output dimensions and updates the crop.
- Recordings crop to the rendered content instead of including letterboxing.
- The optimized canvas capture path avoids duplicate frames and unnecessary
  full-size copies when possible.

Recent Chrome versions can create MP4 directly when native `MediaRecorder` MP4
support is available. Otherwise, the app can convert WebM with FFmpeg when
`SharedArrayBuffer` is available. The included server supplies the required
cross-origin isolation headers.

On Android Chrome, use **Save MP4 to Phone** or **Share / Save to...**, then
choose Photos, Files, or another destination from the system sheet.

### Multipass Shader Chains

The multipass window builds an ordered processing chain:

1. Open **Multipass**.
2. Type part of a shader name in the search box.
3. Select a matching shader and click **Add**.
4. Move passes up or down, remove passes, and enable the chain.

An empty search box displays the complete shader list. Chains can be:

- Copied to the clipboard as compact JSON.
- Downloaded as a text file.
- Imported from pasted JSON.
- Loaded from a `.txt` or `.json` file.

Chain data uses a compact versioned format similar to:

```json
{"v":1,"e":1,"p":[["example.glsl",42]]}
```

Shader names are stored for stable sharing between builds, with indexes retained
as a fallback where appropriate. Chain serialization uses the browser's native
JSON support, so `json.hpp` is not required for this feature.

### Startup and Loading

Startup is split into two visible phases:

1. `index.html` downloads `MX_app.wasm` and `MX_app.data` in parallel. The
   loading screen displays the combined byte count, total size, and percentage.
2. After both files are available, the screen switches to the shader-compilation
   console and reports shader progress, successes, and failures.

The downloaded buffers are passed directly to Emscripten, avoiding a second
WASM or data-file request. If streaming prefetch is unavailable or fails, the
page falls back to Emscripten's standard loader. Servers that provide
`Content-Length` and support `HEAD` requests allow the progress bar to show an
exact total immediately; otherwise it remains indeterminate until the size is
known.

## Requirements

- A modern browser with WebGL 2 support.
- A Chromium-based browser is recommended for the best experience, including
  Google Chrome, Chromium, Brave, Microsoft Edge, and other Chromium-based
  browsers.
- Emscripten and its SDL2 ports for building.
- An Emscripten build of libmx2.
- Python 3 for the included development server.
- Perl for regenerating the WebGL shader cache.

Chrome and other Chromium-based browsers generally provide the most consistent
WebGL 2, `MediaRecorder`, camera, microphone, and WebAssembly support for the
visualizer. Other modern browsers may work, but recording formats and media
features can vary by browser and operating system.

Firefox is not currently recommended for this application. Video recordings
may appear heavily pixelated, and camera input does not appear to work reliably.
For recording and live-camera use, choose a Chromium-based browser instead.

## Building the Web Version

Install and activate the Emscripten SDK, then build libmx2 for Emscripten as
described in the [libmx2 repository](https://github.com/lostjared/libmx2). For
example, after cloning the SDK to a directory of your choice:

```bash
cd /path/to/emsdk
./emsdk install latest
./emsdk activate latest

# Configure Emscripten in the current shell.
source ./emsdk_env.sh
em++ --version

cd /path/to/MX2.Graphics.Demo
make -f Makefile.em
```

The `emsdk_env.sh` script configures `PATH` and the other Emscripten environment
variables for the current shell. To use it from any directory, store the SDK
location in an environment variable:

```bash
export EMSDK_ROOT=/path/to/emsdk
source "$EMSDK_ROOT/emsdk_env.sh"
make -f Makefile.em
```

Alternatively, if an existing Emscripten installation is not activated in the
current shell, pass the path to its C++ compiler directly:

```bash
make -f Makefile.em CXX=/path/to/emscripten/em++
```

The build generates:

- `MX_app.html`
- `MX_app.js`
- `MX_app.wasm`
- `MX_app.data`

The application UI is served from `index.html`, which preloads the generated
WASM and data files with a shared cache-busting token, displays download
progress, and then starts the generated JavaScript runtime.

## Running Locally

Do not open `index.html` through a `file://` URL. Start the included server:

```bash
python3 server.py
```

The default port is 8080. Open:

```text
http://localhost:8080/index.html
```

To choose another port:

```bash
python3 server.py 3000
```

The server sends COOP and COEP headers so `SharedArrayBuffer` and FFmpeg-based
MP4 conversion can work.

## Controls Quick Reference

| Action | Control |
| --- | --- |
| Next shader | **Next**, Right/Down Arrow, or Space |
| Previous shader | **Prev** or Left/Up Arrow |
| Reset animation | **Reset** |
| Load image | **Load Image** |
| Save screenshot | **Save** or `Z` |
| Start/stop recording | **Record** |
| Live camera | **Camera** |
| Local video input | **Video File** |
| Edit shader | **Custom Shader** |
| Adjust uniforms | **Uniform Controls** |
| Build a shader chain | **Multipass** |
| Interact in 2D | Mouse movement/click/drag or touch/drag |
| Interact in 3D | Drag to rotate; pinch to zoom on touch screens |

The mobile controls provide equivalent actions and can collapse while recording
to avoid covering landscape video content.

## Custom Shader Interface

Custom fragment shaders target GLSL ES 3.00. A minimal shader is:

```glsl
#version 300 es
precision highp float;
precision highp int;

in vec2 TexCoord;
out vec4 color;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;

void main() {
    vec2 uv = TexCoord;
    vec2 mouseUV = iMouse.xy / iResolution;

    if (iMouse.z > 0.5) {
        float distanceFromMouse = distance(uv, mouseUV);
        uv += 0.02 * sin(distanceFromMouse * 40.0 - time_f * 6.0);
    }

    color = texture(textTexture, uv);
}
```

Declare only the uniforms a shader needs.

### Available Uniforms

| Uniform | Type | Meaning |
| --- | --- | --- |
| `textTexture` | `sampler2D` | Current image, camera frame, video frame, or prior pass |
| `time_f`, `iTime` | `float` | Animation time |
| `iTimeDelta` | `float` | Time since the previous rendered frame |
| `iFrame` | `float` | Rendered frame counter |
| `iSeconds`, `iMinutes`, `iHours` | `float` | Cyclic time values |
| `iResolution` | `vec2` | Active rendered content dimensions |
| `iAspectRatio` | `float` | Active content aspect ratio |
| `iMouse` | `vec4` | Canvas-space X/Y and pressed state in Z/W |
| `iMouseNormalized` | `vec2` | Mouse position normalized to the active content |
| `iMouseVelocity` | `vec2` | Mouse/touch movement between frames |
| `iMouseActive`, `iMouseClick` | `float` | `1.0` while pressed, otherwise `0.0` |
| `iSpeed` | `float` | Animation-speed control |
| `iAmplitude` | `float` | User amplitude control |
| `iFrequency` | `float` | User frequency control |
| `iBrightness` | `float` | Brightness control |
| `iContrast` | `float` | Contrast control |
| `iSaturation` | `float` | Saturation control |
| `iHueShift` | `float` | Hue rotation in radians |
| `iZoom` | `float` | Zoom control |
| `iRotation` | `float` | Rotation control in radians |
| `iQuality` | `float` | Shader quality control |
| `iDebugMode` | `float` | Debug toggle represented as `0.0` or `1.0` |
| `iCameraPos` | `vec3` | User-controlled camera position |
| `iBeat`, `iAudioLevel` | `float` | Built-in animated compatibility values |
| `alpha` | `float` | Output opacity compatibility value |
| `amp`, `uamp` | `float` | Legacy amplitude compatibility values |

## Shader Cache and Compatibility Pass

External shader filenames are listed in:

```text
data/shaders/index.txt
```

The build uses the preconverted WebGL cache listed in:

```text
data/shaders/webgl_cache/index.txt
```

Both indexes are alphabetized. Regenerate the cache with:

```bash
perl cache_webgl_shaders.pl
```

The Makefile runs this pass when the script, source index, or shader sources
change. The conversion pass can:

- Replace the source version with `#version 300 es`.
- Add required float and integer precision declarations.
- Convert legacy texture functions to `texture()`.
- Convert supported fragment outputs for WebGL 2.
- Remove illegal uniform initializers by converting constant values.
- Normalize desktop-only integer/float expressions rejected by GLSL ES.
- Add explicit precision to numeric arrays for mobile GLSL compilers.
- Inject the common settings-panel controls into external shaders. Generated
  wrappers apply amplitude, frequency, color, zoom, rotation, quality, and
  debug adjustments while preserving shaders that already implement them.

The source files are not modified by the cache pass. Converted files are written
to `data/shaders/webgl_cache/`, allowing normal startup to avoid repeating the
full compatibility conversion.

### Adding an External Shader

1. Place the `.glsl` file in `data/shaders/`.
2. Add its filename to `data/shaders/index.txt` in alphabetical order.
3. Run `perl cache_webgl_shaders.pl`.
4. Rebuild with `make -f Makefile.em`.

Shaders using unsupported runtime FFT/audio data or texture-cache inputs are
omitted from the generated cache automatically.

### Shader Failure Logs

Startup records the compiler output and converted source for shaders that fail.
The browser console receives the complete report. It is also written to the
Emscripten filesystem as `/shader-failures.log`.

To download the current report from the browser console, run:

```js
downloadShaderFailureLog();
```

If every shader compiles, the report contains `All shaders compiled successfully.`

## Project Structure

```text
.
├── graphics.cpp                 # Renderer, uniforms, shader compilation, input
├── shaders.hpp                  # Built-in shaders and shared uniform helpers
├── mirror_shaders.hpp           # Additional built-in shader collection
├── index.html                   # Main v2.0 visualizer UI
├── visualizer.html              # Landing page and v2.0 beta launcher
├── Makefile.em                  # Emscripten build
├── cache_webgl_shaders.pl       # Offline GLSL ES compatibility/cache pass
├── server.py                    # Local server with COOP/COEP headers
└── data/
    ├── logo.png                 # Default texture
    ├── compressed/              # Compressed 3D models
    └── shaders/
        ├── index.txt            # Alphabetized source shader index
        └── webgl_cache/
            └── index.txt        # Alphabetized compatible shader index
```

## Troubleshooting

### `em++: command not found`

Activate the Emscripten environment or pass the compiler explicitly:

```bash
export EMSDK_ROOT=/path/to/emsdk
source "$EMSDK_ROOT/emsdk_env.sh"
em++ --version
```

or:

```bash
make -f Makefile.em CXX=/path/to/emscripten/em++
```

### A custom shader does not compile

- Target `#version 300 es`.
- Add float and integer precision declarations.
- Use `in vec2 TexCoord` and a fragment `out vec4`.
- Use `texture()`, not `texture2D()`.
- Check the compiler output in the custom shader dialog.

### Camera or microphone access fails

- Use HTTPS or localhost.
- Grant the requested browser permission.
- Confirm another application is not holding the device.
- Select a different camera or resolution.

### MP4 is unavailable

- Native MP4 depends on browser `MediaRecorder` support.
- FFmpeg conversion requires `SharedArrayBuffer` and the provided COOP/COEP
  headers.
- Use WebM when neither MP4 path is available.

### Android saving fails

- Use the system share/save sheet rather than relying on a blob URL download.
- Choose Photos, Files, or another application that accepts video files.
- Keep the page open until recording finalization completes.

### Performance is low

- Use Balanced recording quality.
- Reduce recording FPS to 30 or 24.
- Disable expensive multipass chains.
- Choose a simpler shader or lower custom shader loop counts.
- Close editor and media dialogs while recording.

## Performance Notes

- Rendering uses WebGL 2 hardware acceleration.
- The app targets smooth 60 FPS interaction on modern devices.
- External shader conversion is cached before startup.
- WASM and packaged application data download concurrently and are reused by
  Emscripten without duplicate network transfers.
- Shader program compilation remains the main startup cost.
- Recording performance depends on shader complexity, output size, browser
  encoder support, and device hardware.

## License

This project is part of the ACMX2 Graphics Library collection and is distributed
under GPL v3. See the source headers and repository license for details.

## Credits

- Jared Bruni / LostSideDead Software
- [libmx2](https://github.com/lostjared/libmx2)
- Special Thank you to the Interactive graphics community/researchers and creative shader authors
