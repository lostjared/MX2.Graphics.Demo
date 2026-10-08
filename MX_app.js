// include: shell.js
// include: minimum_runtime_check.js
(function() {
  // "30.0.0" -> 300000
  function humanReadableVersionToPacked(str) {
    str = str.split('-')[0]; // Remove any trailing part from e.g. "12.53.3-alpha"
    var vers = str.split('.').slice(0, 3);
    while(vers.length < 3) vers.push('00');
    vers = vers.map((n, i, arr) => n.padStart(2, '0'));
    return vers.join('');
  }
  // 300000 -> "30.0.0"
  var packedVersionToHumanReadable = n => [n / 10000 | 0, (n / 100 | 0) % 100, n % 100].join('.');

  var TARGET_NOT_SUPPORTED = 2147483647;

  // Note: We use a typeof check here instead of optional chaining using
  // globalThis because older browsers might not have globalThis defined.

  // We skip the node version checking when running on Bun/Deno since the node
  // version they report doesn't seem to be useful.
  if (typeof process !== 'undefined' && !process.versions?.bun && typeof Deno == "undefined") {
    var currentNodeVersion = process.versions?.node ? humanReadableVersionToPacked(process.versions.node) : TARGET_NOT_SUPPORTED;
    if (currentNodeVersion < TARGET_NOT_SUPPORTED) {
      throw new Error('not compiled for this environment (did you build to HTML and try to run it not on the web, or set ENVIRONMENT to something - like node - and run it someplace else - like on the web?)');
    }
    if (currentNodeVersion < 2147483647) {
      throw new Error(`This emscripten-generated code requires node v${ packedVersionToHumanReadable(2147483647) } (detected v${packedVersionToHumanReadable(currentNodeVersion)})`);
    }
  }

  var userAgent = typeof navigator !== 'undefined' && navigator.userAgent;
  if (!userAgent) {
    return;
  }

  var currentSafariVersion = userAgent.includes("Safari/") && !userAgent.includes("Chrome/") && userAgent.match(/Version\/(\d+\.?\d*\.?\d*)/) ? humanReadableVersionToPacked(userAgent.match(/Version\/(\d+\.?\d*\.?\d*)/)[1]) : TARGET_NOT_SUPPORTED;
  if (currentSafariVersion < 150000) {
    throw new Error(`This emscripten-generated code requires Safari v${ packedVersionToHumanReadable(150000) } (detected v${currentSafariVersion})`);
  }

  var currentFirefoxVersion = userAgent.match(/Firefox\/(\d+(?:\.\d+)?)/) ? parseFloat(userAgent.match(/Firefox\/(\d+(?:\.\d+)?)/)[1]) : TARGET_NOT_SUPPORTED;
  if (currentFirefoxVersion < 79) {
    throw new Error(`This emscripten-generated code requires Firefox v79 (detected v${currentFirefoxVersion})`);
  }

  var currentChromeVersion = userAgent.match(/Chrome\/(\d+(?:\.\d+)?)/) ? parseFloat(userAgent.match(/Chrome\/(\d+(?:\.\d+)?)/)[1]) : TARGET_NOT_SUPPORTED;
  if (currentChromeVersion < 85) {
    throw new Error(`This emscripten-generated code requires Chrome v85 (detected v${currentChromeVersion})`);
  }
})();

// end include: minimum_runtime_check.js
// The Module object: Our interface to the outside world. We import
// and export values on it. There are various ways Module can be used:
// 1. Not defined. We create it here
// 2. A function parameter, function(moduleArg) => Promise<Module>
// 3. pre-run appended it, var Module = {}; ..generated code..
// 4. External script tag defines var Module.
// We need to check if Module already exists (e.g. case 3 above).
// Substitution will be replaced with actual code on later stage of the build,
// this way Closure Compiler will not mangle it (e.g. case 4. above).
// Note that if you want to run closure, and also to use Module
// after the generated code, you will need to define   var Module = {};
// before the code. Then that object will be used in the code, and you
// can continue to use Module afterwards as well.
var Module = typeof Module != 'undefined' ? Module : {};

// Determine the runtime environment we are in. You can customize this by
// setting the ENVIRONMENT setting at compile time (see settings.js).

// Attempt to auto-detect the environment
var ENVIRONMENT_IS_WEB = !!globalThis.window;
var ENVIRONMENT_IS_WORKER = !!globalThis.WorkerGlobalScope;
// N.b. Electron.js environment is simultaneously a NODE-environment, but
// also a web environment.
var ENVIRONMENT_IS_NODE = globalThis.process?.versions?.node && globalThis.process?.type != 'renderer';
var ENVIRONMENT_IS_SHELL = !ENVIRONMENT_IS_WEB && !ENVIRONMENT_IS_NODE && !ENVIRONMENT_IS_WORKER;

// --pre-jses are emitted after the Module integration code, so that they can
// refer to Module (if they choose; they can also define Module)
// include: /tmp/tmp63tejlnq.js

  if (!Module['expectedDataFileDownloads']) Module['expectedDataFileDownloads'] = 0;
  Module['expectedDataFileDownloads']++;
  (() => {
    // Do not attempt to redownload the virtual filesystem data when in a pthread or a Wasm Worker context.
    var isPthread = typeof ENVIRONMENT_IS_PTHREAD != 'undefined' && ENVIRONMENT_IS_PTHREAD;
    var isWasmWorker = typeof ENVIRONMENT_IS_WASM_WORKER != 'undefined' && ENVIRONMENT_IS_WASM_WORKER;
    if (isPthread || isWasmWorker) return;
    async function loadPackage(metadata) {

      var PACKAGE_PATH = '';
      if (typeof window === 'object') {
        PACKAGE_PATH = window['encodeURIComponent'](window.location.pathname.substring(0, window.location.pathname.lastIndexOf('/')) + '/');
      } else if (typeof process === 'undefined' && typeof location !== 'undefined') {
        // web worker
        PACKAGE_PATH = encodeURIComponent(location.pathname.substring(0, location.pathname.lastIndexOf('/')) + '/');
      }
      var PACKAGE_NAME = 'MX_app.data';
      var REMOTE_PACKAGE_BASE = 'MX_app.data';
      var REMOTE_PACKAGE_NAME = Module['locateFile'] ? Module['locateFile'](REMOTE_PACKAGE_BASE, '') : REMOTE_PACKAGE_BASE;
      var REMOTE_PACKAGE_SIZE = metadata['remote_package_size'];

      async function fetchRemotePackage(packageName, packageSize) {
        
        if (!Module['dataFileDownloads']) Module['dataFileDownloads'] = {};
        try {
          var response = await fetch(packageName);
        } catch (e) {
          throw new Error(`Network Error: ${packageName}`, {e});
        }
        if (!response.ok) {
          throw new Error(`${response.status}: ${response.url}`);
        }

        const chunks = [];
        const headers = response.headers;
        const total = Number(headers.get('Content-Length') || packageSize);
        let loaded = 0;

        Module['setStatus'] && Module['setStatus']('Downloading data...');
        const reader = response.body.getReader();

        while (1) {
          var {done, value} = await reader.read();
          if (done) break;
          chunks.push(value);
          loaded += value.length;
          Module['dataFileDownloads'][packageName] = {loaded, total};

          let totalLoaded = 0;
          let totalSize = 0;

          for (const download of Object.values(Module['dataFileDownloads'])) {
            totalLoaded += download.loaded;
            totalSize += download.total;
          }

          Module['setStatus'] && Module['setStatus'](`Downloading data... (${totalLoaded}/${totalSize})`);
        }

        const packageData = new Uint8Array(chunks.map((c) => c.length).reduce((a, b) => a + b, 0));
        let offset = 0;
        for (const chunk of chunks) {
          packageData.set(chunk, offset);
          offset += chunk.length;
        }
        return packageData.buffer;
      }

      var fetchPromise;
      var fetched = Module['getPreloadedPackage'] && Module['getPreloadedPackage'](REMOTE_PACKAGE_NAME, REMOTE_PACKAGE_SIZE);

      if (!fetched) {
        // Note that we don't use await here because we want to execute the
        // the rest of this function immediately.
        fetchPromise = fetchRemotePackage(REMOTE_PACKAGE_NAME, REMOTE_PACKAGE_SIZE);
      }

    async function runWithFS(Module) {

      function assert(check, msg) {
        if (!check) throw new Error(msg);
      }
Module['FS_createPath']("/", "data", true, true);
Module['FS_createPath']("/data", "compressed", true, true);
Module['FS_createPath']("/data", "shaders", true, true);
Module['FS_createPath']("/data/shaders", "webgl_cache", true, true);

      async function processPackageData(arrayBuffer) {
        assert(arrayBuffer, 'Loading data file failed.');
        assert(arrayBuffer.constructor.name === ArrayBuffer.name, 'bad input to processPackageData ' + arrayBuffer.constructor.name);
        var byteArray = new Uint8Array(arrayBuffer);
        var curr;
        // Reuse the bytearray from the XHR as the source for file reads.
          for (var file of metadata['files']) {
            var name = file['filename'];
            var data = byteArray.subarray(file['start'], file['end']);
            // canOwn this data in the filesystem, it is a slice into the heap that will never change
        Module['FS_createDataFile'](name, null, data, true, true, true);
          }
          Module['removeRunDependency']('datafile_MX_app.data');
      }
      Module['addRunDependency']('datafile_MX_app.data');

      if (!Module['preloadResults']) Module['preloadResults'] = {};

      Module['preloadResults'][PACKAGE_NAME] = {fromCache: false};
      if (!fetched) {
        fetched = await fetchPromise;
      }
      await processPackageData(fetched);

    }
    // Detect whether the module JS file has already been loaded.
    if (Module['FS_createPath']) {
      runWithFS(Module);
    } else {
      if (!Module['preRun']) Module['preRun'] = [];
      Module['preRun'].push(runWithFS); // FS is not initialized yet, wait for it
    }

    }
    loadPackage({"files": [{"filename": "/data/compressed/3dplus.mxmod.z", "start": 0, "end": 569}, {"filename": "/data/compressed/3dstar.mxmod.z", "start": 569, "end": 871}, {"filename": "/data/compressed/UFO3.mxmod.z", "start": 871, "end": 2705}, {"filename": "/data/compressed/bird.mxmod.z", "start": 2705, "end": 3587}, {"filename": "/data/compressed/cigar_tube.mxmod.z", "start": 3587, "end": 5688}, {"filename": "/data/compressed/cigar_tunnel.mxmod.z", "start": 5688, "end": 6297}, {"filename": "/data/compressed/cone.mxmod.z", "start": 6297, "end": 37943}, {"filename": "/data/compressed/crystal_cave.mxmod.z", "start": 37943, "end": 39200}, {"filename": "/data/compressed/cube.mxmod.z", "start": 39200, "end": 39370}, {"filename": "/data/compressed/cylinder.mxmod.z", "start": 39370, "end": 40408}, {"filename": "/data/compressed/diamond.mxmod.z", "start": 40408, "end": 40617}, {"filename": "/data/compressed/dome.mxmod.z", "start": 40617, "end": 58710}, {"filename": "/data/compressed/fruit_of_life.mxmod.z", "start": 58710, "end": 314677}, {"filename": "/data/compressed/g_ufo.mxmod.z", "start": 314677, "end": 315202}, {"filename": "/data/compressed/geosphere.mxmod.z", "start": 315202, "end": 342471}, {"filename": "/data/compressed/glob.mxmod.z", "start": 342471, "end": 531548}, {"filename": "/data/compressed/globe.mxmod.z", "start": 531548, "end": 720625}, {"filename": "/data/compressed/hour_glass.mxmod.z", "start": 720625, "end": 813611}, {"filename": "/data/compressed/hourglass.mxmod.z", "start": 813611, "end": 866425}, {"filename": "/data/compressed/hourglass_pyramid.mxmod.z", "start": 866425, "end": 919480}, {"filename": "/data/compressed/icosahedron.mxmod.z", "start": 919480, "end": 919808}, {"filename": "/data/compressed/kaleidoscope.mxmod.z", "start": 919808, "end": 920154}, {"filename": "/data/compressed/large_cylinder.mxmod.z", "start": 920154, "end": 921193}, {"filename": "/data/compressed/large_hex.mxmod.z", "start": 921193, "end": 921600}, {"filename": "/data/compressed/large_sphere.mxmod.z", "start": 921600, "end": 994621}, {"filename": "/data/compressed/list.txt", "start": 994621, "end": 995791}, {"filename": "/data/compressed/menger_lattice.mxmod.z", "start": 995791, "end": 1023113}, {"filename": "/data/compressed/menger_spong.mxmod.z", "start": 1023113, "end": 1552218}, {"filename": "/data/compressed/merkaba.mxmod.z", "start": 1552218, "end": 1567186}, {"filename": "/data/compressed/moon.mxmod.z", "start": 1567186, "end": 1609249}, {"filename": "/data/compressed/new_cube6x.mxmod.z", "start": 1609249, "end": 1609417}, {"filename": "/data/compressed/new_diamond.mxmod.z", "start": 1609417, "end": 1609603}, {"filename": "/data/compressed/new_sphere.mxmod.z", "start": 1609603, "end": 1683573}, {"filename": "/data/compressed/object.mxmod.z", "start": 1683573, "end": 1683698}, {"filename": "/data/compressed/octahedron.mxmod.z", "start": 1683698, "end": 1683863}, {"filename": "/data/compressed/octopus.mxmod.z", "start": 1683863, "end": 1875779}, {"filename": "/data/compressed/pentagon.mxmod.z", "start": 1875779, "end": 1876128}, {"filename": "/data/compressed/pillar.mxmod.z", "start": 1876128, "end": 1878105}, {"filename": "/data/compressed/prism.mxmod.z", "start": 1878105, "end": 1878336}, {"filename": "/data/compressed/prism_hex.mxmod.z", "start": 1878336, "end": 1878679}, {"filename": "/data/compressed/pyramid.mxmod.z", "start": 1878679, "end": 1878841}, {"filename": "/data/compressed/sacred_sphere_opt.mxmod.z", "start": 1878841, "end": 1913410}, {"filename": "/data/compressed/saturn.mxmod.z", "start": 1913410, "end": 2005644}, {"filename": "/data/compressed/scube.mxmod.z", "start": 2005644, "end": 2005882}, {"filename": "/data/compressed/ship.mxmod.z", "start": 2005882, "end": 2006043}, {"filename": "/data/compressed/ship1.mxmod.z", "start": 2006043, "end": 2015050}, {"filename": "/data/compressed/ship2.mxmod.z", "start": 2015050, "end": 2015216}, {"filename": "/data/compressed/ship3.mxmod.z", "start": 2015216, "end": 2015730}, {"filename": "/data/compressed/shipshooter.mxmod.z", "start": 2015730, "end": 2015950}, {"filename": "/data/compressed/sky.mxmod.z", "start": 2015950, "end": 2016159}, {"filename": "/data/compressed/skybox1.mxmod.z", "start": 2016159, "end": 2016368}, {"filename": "/data/compressed/skybox_cross.mxmod.z", "start": 2016368, "end": 2016956}, {"filename": "/data/compressed/skybox_hex.mxmod.z", "start": 2016956, "end": 2017310}, {"filename": "/data/compressed/skybox_pyramid_top.mxmod.z", "start": 2017310, "end": 2017565}, {"filename": "/data/compressed/skybox_star.mxmod.z", "start": 2017565, "end": 2018175}, {"filename": "/data/compressed/smooth.mxmod.z", "start": 2018175, "end": 2021768}, {"filename": "/data/compressed/sphere.mxmod.z", "start": 2021768, "end": 2022501}, {"filename": "/data/compressed/sphere_pillar.mxmod.z", "start": 2022501, "end": 2042466}, {"filename": "/data/compressed/spiked_skybox.mxmod.z", "start": 2042466, "end": 2137746}, {"filename": "/data/compressed/star.mxmod.z", "start": 2137746, "end": 2138091}, {"filename": "/data/compressed/star_skybox.mxmod.z", "start": 2138091, "end": 2230125}, {"filename": "/data/compressed/torus.mxmod.z", "start": 2230125, "end": 2273109}, {"filename": "/data/compressed/trapezoid.mxmod.z", "start": 2273109, "end": 2273459}, {"filename": "/data/compressed/tree.mxmod.z", "start": 2273459, "end": 2273840}, {"filename": "/data/compressed/trefoil_knot.mxmod.z", "start": 2273840, "end": 2409199}, {"filename": "/data/compressed/triforce.mxmod.z", "start": 2409199, "end": 2417101}, {"filename": "/data/compressed/tube.mxmod.z", "start": 2417101, "end": 2418134}, {"filename": "/data/compressed/tunnel.mxmod.z", "start": 2418134, "end": 2420360}, {"filename": "/data/compressed/twisted_torus.mxmod.z", "start": 2420360, "end": 2501608}, {"filename": "/data/compressed/ufo.mxmod.z", "start": 2501608, "end": 2501803}, {"filename": "/data/compressed/ufo_model.mxmod.z", "start": 2501803, "end": 2504554}, {"filename": "/data/compressed/ufox.mxmod.z", "start": 2504554, "end": 2522604}, {"filename": "/data/compressed/uv_sphere.mxmod.z", "start": 2522604, "end": 2540371}, {"filename": "/data/logo.png", "start": 2540371, "end": 7892015}, {"filename": "/data/shaders/3high.glsl", "start": 7892015, "end": 7894492}, {"filename": "/data/shaders/4ac_rand.glsl", "start": 7894492, "end": 7895697}, {"filename": "/data/shaders/4rand.glsl", "start": 7895697, "end": 7897479}, {"filename": "/data/shaders/8bit-norm.glsl", "start": 7897479, "end": 7898348}, {"filename": "/data/shaders/8bit.glsl", "start": 7898348, "end": 7899329}, {"filename": "/data/shaders/8huri.glsl", "start": 7899329, "end": 7900735}, {"filename": "/data/shaders/BubbleMouse.glsl", "start": 7900735, "end": 7902488}, {"filename": "/data/shaders/CursorMove.glsl", "start": 7902488, "end": 7903911}, {"filename": "/data/shaders/CursorWarp2.glsl", "start": 7903911, "end": 7905494}, {"filename": "/data/shaders/DigitalLight.glsl", "start": 7905494, "end": 7907498}, {"filename": "/data/shaders/DigitalLightStorm.glsl", "start": 7907498, "end": 7912687}, {"filename": "/data/shaders/Dispersion.glsl", "start": 7912687, "end": 7917014}, {"filename": "/data/shaders/DispersionRotate.glsl", "start": 7917014, "end": 7921532}, {"filename": "/data/shaders/DispersionX.glsl", "start": 7921532, "end": 7924344}, {"filename": "/data/shaders/DispersionX_Audio.glsl", "start": 7924344, "end": 7928410}, {"filename": "/data/shaders/DispersionY.glsl", "start": 7928410, "end": 7931222}, {"filename": "/data/shaders/DispersionZ.glsl", "start": 7931222, "end": 7934034}, {"filename": "/data/shaders/ElectricMirror.glsl", "start": 7934034, "end": 7943868}, {"filename": "/data/shaders/Electric_Fold.glsl", "start": 7943868, "end": 7950382}, {"filename": "/data/shaders/Electric_Fold_Texture.glsl", "start": 7950382, "end": 7956222}, {"filename": "/data/shaders/Fractal1.glsl", "start": 7956222, "end": 7957425}, {"filename": "/data/shaders/HyperFcousAmp.glsl", "start": 7957425, "end": 7962177}, {"filename": "/data/shaders/HyperFocusTrails.glsl", "start": 7962177, "end": 7966034}, {"filename": "/data/shaders/HyperVortex.glsl", "start": 7966034, "end": 7970119}, {"filename": "/data/shaders/Light_Rainbow_Swirl.glsl", "start": 7970119, "end": 7975277}, {"filename": "/data/shaders/Liquid_Censorship.glsl", "start": 7975277, "end": 7976825}, {"filename": "/data/shaders/Liquid_Crystal.glsl", "start": 7976825, "end": 7982920}, {"filename": "/data/shaders/Liquid_Crystal_2.glsl", "start": 7982920, "end": 7989052}, {"filename": "/data/shaders/Liquid_Crystal_Rainbow1.glsl", "start": 7989052, "end": 7994820}, {"filename": "/data/shaders/Liquid_Fractal_Tunnel.glsl", "start": 7994820, "end": 7999329}, {"filename": "/data/shaders/Liquid_Heat.glsl", "start": 7999329, "end": 8006257}, {"filename": "/data/shaders/Liquid_Heat_blend.glsl", "start": 8006257, "end": 8013224}, {"filename": "/data/shaders/Liquid_Light_Rainbow_Blend.glsl", "start": 8013224, "end": 8018993}, {"filename": "/data/shaders/Liquid_Light_Time.glsl", "start": 8018993, "end": 8025593}, {"filename": "/data/shaders/Liquid_Rainbow.glsl", "start": 8025593, "end": 8030812}, {"filename": "/data/shaders/Liquid_Rainbow_Wave.glsl", "start": 8030812, "end": 8036922}, {"filename": "/data/shaders/PI_x1.mp4.glsl", "start": 8036922, "end": 8040559}, {"filename": "/data/shaders/README.md", "start": 8040559, "end": 8043189}, {"filename": "/data/shaders/RGBColorTrails_cache.glsl", "start": 8043189, "end": 8045232}, {"filename": "/data/shaders/ResizeVertical_cache.glsl", "start": 8045232, "end": 8047422}, {"filename": "/data/shaders/Temporal.glsl", "start": 8047422, "end": 8049594}, {"filename": "/data/shaders/XorKern.glsl", "start": 8049594, "end": 8053149}, {"filename": "/data/shaders/XorKernBrighter.glsl", "start": 8053149, "end": 8056815}, {"filename": "/data/shaders/abc.glsl", "start": 8056815, "end": 8057634}, {"filename": "/data/shaders/abc123.glsl", "start": 8057634, "end": 8058427}, {"filename": "/data/shaders/acid_color2.glsl", "start": 8058427, "end": 8059312}, {"filename": "/data/shaders/acidcam.glsl", "start": 8059312, "end": 8059436}, {"filename": "/data/shaders/acidcolor.glsl", "start": 8059436, "end": 8060240}, {"filename": "/data/shaders/acidcolorrand.glsl", "start": 8060240, "end": 8061297}, {"filename": "/data/shaders/addup_blend.glsl", "start": 8061297, "end": 8061979}, {"filename": "/data/shaders/addup_cos.glsl", "start": 8061979, "end": 8062661}, {"filename": "/data/shaders/af_scale.glsl", "start": 8062661, "end": 8064790}, {"filename": "/data/shaders/af_scale2.glsl", "start": 8064790, "end": 8066970}, {"filename": "/data/shaders/af_scale2_react.glsl", "start": 8066970, "end": 8069770}, {"filename": "/data/shaders/af_scale3.glsl", "start": 8069770, "end": 8071968}, {"filename": "/data/shaders/af_scale_pulse.glsl", "start": 8071968, "end": 8074169}, {"filename": "/data/shaders/af_scale_puple.glsl", "start": 8074169, "end": 8077142}, {"filename": "/data/shaders/af_scale_spectrum.glsl", "start": 8077142, "end": 8079507}, {"filename": "/data/shaders/air-bowl.glsl", "start": 8079507, "end": 8083611}, {"filename": "/data/shaders/air.glsl", "start": 8083611, "end": 8084267}, {"filename": "/data/shaders/air_full.glsl", "start": 8084267, "end": 8084884}, {"filename": "/data/shaders/air_full_mouse.glsl", "start": 8084884, "end": 8085846}, {"filename": "/data/shaders/air_mouse.glsl", "start": 8085846, "end": 8087617}, {"filename": "/data/shaders/airshader1.glsl", "start": 8087617, "end": 8090267}, {"filename": "/data/shaders/all_colors.glsl", "start": 8090267, "end": 8093017}, {"filename": "/data/shaders/alpha_diamond.glsl", "start": 8093017, "end": 8094268}, {"filename": "/data/shaders/alpha_sin_os.glsl", "start": 8094268, "end": 8094778}, {"filename": "/data/shaders/alpha_xor.glsl", "start": 8094778, "end": 8095719}, {"filename": "/data/shaders/analog.glsl", "start": 8095719, "end": 8096465}, {"filename": "/data/shaders/and_smooth.glsl", "start": 8096465, "end": 8098628}, {"filename": "/data/shaders/animation.glsl", "start": 8098628, "end": 8102100}, {"filename": "/data/shaders/ant_cache_spectrum8_acid_rain.glsl", "start": 8102100, "end": 8105212}, {"filename": "/data/shaders/ant_cache_spectrum8_caustic_storm.glsl", "start": 8105212, "end": 8108409}, {"filename": "/data/shaders/ant_cache_spectrum8_chromatic_pulse.glsl", "start": 8108409, "end": 8111615}, {"filename": "/data/shaders/ant_cache_spectrum8_cosmic_web.glsl", "start": 8111615, "end": 8115424}, {"filename": "/data/shaders/ant_cache_spectrum8_echo_quad_mirror.glsl", "start": 8115424, "end": 8118775}, {"filename": "/data/shaders/ant_cache_spectrum8_fractal.glsl", "start": 8118775, "end": 8121834}, {"filename": "/data/shaders/ant_cache_spectrum8_fractal_xor_fold.glsl", "start": 8121834, "end": 8125710}, {"filename": "/data/shaders/ant_cache_spectrum8_galaxy_swirl.glsl", "start": 8125710, "end": 8129005}, {"filename": "/data/shaders/ant_cache_spectrum8_geometric_polar.glsl", "start": 8129005, "end": 8132337}, {"filename": "/data/shaders/ant_cache_spectrum8_glitch_boil.glsl", "start": 8132337, "end": 8136027}, {"filename": "/data/shaders/ant_cache_spectrum8_glitch_storm.glsl", "start": 8136027, "end": 8139145}, {"filename": "/data/shaders/ant_cache_spectrum8_hex_grid.glsl", "start": 8139145, "end": 8142273}, {"filename": "/data/shaders/ant_cache_spectrum8_holographic.glsl", "start": 8142273, "end": 8145534}, {"filename": "/data/shaders/ant_cache_spectrum8_kaleido_sin_osc.glsl", "start": 8145534, "end": 8149178}, {"filename": "/data/shaders/ant_cache_spectrum8_kaleidoscope.glsl", "start": 8149178, "end": 8152457}, {"filename": "/data/shaders/ant_cache_spectrum8_lava_flow.glsl", "start": 8152457, "end": 8155495}, {"filename": "/data/shaders/ant_cache_spectrum8_lightning.glsl", "start": 8155495, "end": 8158691}, {"filename": "/data/shaders/ant_cache_spectrum8_liquid_light.glsl", "start": 8158691, "end": 8162563}, {"filename": "/data/shaders/ant_cache_spectrum8_mandala.glsl", "start": 8162563, "end": 8165756}, {"filename": "/data/shaders/ant_cache_spectrum8_mirror_tile_rotor.glsl", "start": 8165756, "end": 8169164}, {"filename": "/data/shaders/ant_cache_spectrum8_neon_grid.glsl", "start": 8169164, "end": 8172309}, {"filename": "/data/shaders/ant_cache_spectrum8_neural.glsl", "start": 8172309, "end": 8175724}, {"filename": "/data/shaders/ant_cache_spectrum8_oilslick.glsl", "start": 8175724, "end": 8178882}, {"filename": "/data/shaders/ant_cache_spectrum8_plasma.glsl", "start": 8178882, "end": 8181969}, {"filename": "/data/shaders/ant_cache_spectrum8_radial_echo.glsl", "start": 8181969, "end": 8185111}, {"filename": "/data/shaders/ant_cache_spectrum8_spectrum_rings.glsl", "start": 8185111, "end": 8188524}, {"filename": "/data/shaders/ant_cache_spectrum8_starburst.glsl", "start": 8188524, "end": 8191651}, {"filename": "/data/shaders/ant_cache_spectrum8_strobe_tunnel.glsl", "start": 8191651, "end": 8194907}, {"filename": "/data/shaders/ant_cache_spectrum8_tremor_storm.glsl", "start": 8194907, "end": 8198220}, {"filename": "/data/shaders/ant_cache_spectrum8_tunnel.glsl", "start": 8198220, "end": 8201476}, {"filename": "/data/shaders/ant_cache_spectrum8_vignette_calm.glsl", "start": 8201476, "end": 8204535}, {"filename": "/data/shaders/ant_cache_spectrum8_voronoi_pulse.glsl", "start": 8204535, "end": 8207990}, {"filename": "/data/shaders/ant_cache_spectrum8_vortex.glsl", "start": 8207990, "end": 8211111}, {"filename": "/data/shaders/ant_cache_spectrum8_warp_drive.glsl", "start": 8211111, "end": 8214356}, {"filename": "/data/shaders/ant_cache_spectrum8_zebra_wave.glsl", "start": 8214356, "end": 8217514}, {"filename": "/data/shaders/ant_gem_aurora_tunnel.glsl", "start": 8217514, "end": 8220796}, {"filename": "/data/shaders/ant_gem_chrome_wave.glsl", "start": 8220796, "end": 8223645}, {"filename": "/data/shaders/ant_gem_cosmic_web.glsl", "start": 8223645, "end": 8227100}, {"filename": "/data/shaders/ant_gem_crystal_pulse.glsl", "start": 8227100, "end": 8229341}, {"filename": "/data/shaders/ant_gem_deep_bloom.glsl", "start": 8229341, "end": 8231961}, {"filename": "/data/shaders/ant_gem_diamond_storm.glsl", "start": 8231961, "end": 8234395}, {"filename": "/data/shaders/ant_gem_fire_spoke.glsl", "start": 8234395, "end": 8236937}, {"filename": "/data/shaders/ant_gem_fractal_ocean.glsl", "start": 8236937, "end": 8239688}, {"filename": "/data/shaders/ant_gem_fractal_ocean_repeat.glsl", "start": 8239688, "end": 8243868}, {"filename": "/data/shaders/ant_gem_glass_mandala.glsl", "start": 8243868, "end": 8246796}, {"filename": "/data/shaders/ant_gem_hypno_lens.glsl", "start": 8246796, "end": 8249493}, {"filename": "/data/shaders/ant_gem_ice_ripple.glsl", "start": 8249493, "end": 8252264}, {"filename": "/data/shaders/ant_gem_liquid_mirror.glsl", "start": 8252264, "end": 8255006}, {"filename": "/data/shaders/ant_gem_mercury_bloom.glsl", "start": 8255006, "end": 8258156}, {"filename": "/data/shaders/ant_gem_metal_aurora.glsl", "start": 8258156, "end": 8260824}, {"filename": "/data/shaders/ant_gem_metal_cascade.glsl", "start": 8260824, "end": 8263193}, {"filename": "/data/shaders/ant_gem_metal_chrome.glsl", "start": 8263193, "end": 8265428}, {"filename": "/data/shaders/ant_gem_metal_coil.glsl", "start": 8265428, "end": 8268192}, {"filename": "/data/shaders/ant_gem_metal_crystal.glsl", "start": 8268192, "end": 8270589}, {"filename": "/data/shaders/ant_gem_metal_ember.glsl", "start": 8270589, "end": 8273569}, {"filename": "/data/shaders/ant_gem_metal_flux.glsl", "start": 8273569, "end": 8276156}, {"filename": "/data/shaders/ant_gem_metal_forge.glsl", "start": 8276156, "end": 8279081}, {"filename": "/data/shaders/ant_gem_metal_fracture.glsl", "start": 8279081, "end": 8282134}, {"filename": "/data/shaders/ant_gem_metal_glacier.glsl", "start": 8282134, "end": 8284898}, {"filename": "/data/shaders/ant_gem_metal_helix.glsl", "start": 8284898, "end": 8287753}, {"filename": "/data/shaders/ant_gem_metal_inferno.glsl", "start": 8287753, "end": 8290872}, {"filename": "/data/shaders/ant_gem_metal_lattice.glsl", "start": 8290872, "end": 8293646}, {"filename": "/data/shaders/ant_gem_metal_nebula.glsl", "start": 8293646, "end": 8296780}, {"filename": "/data/shaders/ant_gem_metal_opal.glsl", "start": 8296780, "end": 8299971}, {"filename": "/data/shaders/ant_gem_metal_orbital.glsl", "start": 8299971, "end": 8302522}, {"filename": "/data/shaders/ant_gem_metal_prism.glsl", "start": 8302522, "end": 8305531}, {"filename": "/data/shaders/ant_gem_metal_pulse.glsl", "start": 8305531, "end": 8308210}, {"filename": "/data/shaders/ant_gem_metal_pulse_mouse.glsl", "start": 8308210, "end": 8311478}, {"filename": "/data/shaders/ant_gem_metal_ripple.glsl", "start": 8311478, "end": 8314752}, {"filename": "/data/shaders/ant_gem_metal_shard.glsl", "start": 8314752, "end": 8317914}, {"filename": "/data/shaders/ant_gem_metal_storm.glsl", "start": 8317914, "end": 8321515}, {"filename": "/data/shaders/ant_gem_metal_tessera.glsl", "start": 8321515, "end": 8324441}, {"filename": "/data/shaders/ant_gem_metal_vortex.glsl", "start": 8324441, "end": 8327087}, {"filename": "/data/shaders/ant_gem_metal_weave.glsl", "start": 8327087, "end": 8330291}, {"filename": "/data/shaders/ant_gem_molten_web.glsl", "start": 8330291, "end": 8333198}, {"filename": "/data/shaders/ant_gem_nebula_fold.glsl", "start": 8333198, "end": 8336134}, {"filename": "/data/shaders/ant_gem_neon_silk.glsl", "start": 8336134, "end": 8338884}, {"filename": "/data/shaders/ant_gem_plasma_facets.glsl", "start": 8338884, "end": 8341817}, {"filename": "/data/shaders/ant_gem_prism_vortex.glsl", "start": 8341817, "end": 8344288}, {"filename": "/data/shaders/ant_gem_rainbow_fracture.glsl", "start": 8344288, "end": 8346916}, {"filename": "/data/shaders/ant_gem_sketch_warp.glsl", "start": 8346916, "end": 8349726}, {"filename": "/data/shaders/ant_gem_spiral_shatter.glsl", "start": 8349726, "end": 8353087}, {"filename": "/data/shaders/ant_gem_void_pulse.glsl", "start": 8353087, "end": 8355667}, {"filename": "/data/shaders/ant_light_color_acid_ripple.glsl", "start": 8355667, "end": 8358194}, {"filename": "/data/shaders/ant_light_color_aurora_veil.glsl", "start": 8358194, "end": 8360548}, {"filename": "/data/shaders/ant_light_color_aurora_weave.glsl", "start": 8360548, "end": 8363173}, {"filename": "/data/shaders/ant_light_color_chromatic_worm.glsl", "start": 8363173, "end": 8365557}, {"filename": "/data/shaders/ant_light_color_cosmic_ripple.glsl", "start": 8365557, "end": 8367701}, {"filename": "/data/shaders/ant_light_color_crystal_cave.glsl", "start": 8367701, "end": 8370481}, {"filename": "/data/shaders/ant_light_color_cyber_grid.glsl", "start": 8370481, "end": 8372933}, {"filename": "/data/shaders/ant_light_color_deep_fractal_glow.glsl", "start": 8372933, "end": 8375017}, {"filename": "/data/shaders/ant_light_color_diamond_rain.glsl", "start": 8375017, "end": 8377461}, {"filename": "/data/shaders/ant_light_color_electric_web.glsl", "start": 8377461, "end": 8379632}, {"filename": "/data/shaders/ant_light_color_ember_cascade.glsl", "start": 8379632, "end": 8382401}, {"filename": "/data/shaders/ant_light_color_flicker_gem.glsl", "start": 8382401, "end": 8384722}, {"filename": "/data/shaders/ant_light_color_fractal_fire.glsl", "start": 8384722, "end": 8387580}, {"filename": "/data/shaders/ant_light_color_fractal_lantern.glsl", "start": 8387580, "end": 8389548}, {"filename": "/data/shaders/ant_light_color_frozen_lightning.glsl", "start": 8389548, "end": 8392367}, {"filename": "/data/shaders/ant_light_color_galaxy_swirl.glsl", "start": 8392367, "end": 8395094}, {"filename": "/data/shaders/ant_light_color_glitch_rainbow.glsl", "start": 8395094, "end": 8397306}, {"filename": "/data/shaders/ant_light_color_helix_aurora.glsl", "start": 8397306, "end": 8399959}, {"filename": "/data/shaders/ant_light_color_hologram_pulse.glsl", "start": 8399959, "end": 8402113}, {"filename": "/data/shaders/ant_light_color_hypnotic_rings.glsl", "start": 8402113, "end": 8404426}, {"filename": "/data/shaders/ant_light_color_infinity_loop.glsl", "start": 8404426, "end": 8406983}, {"filename": "/data/shaders/ant_light_color_kaleidoscope_blaze.glsl", "start": 8406983, "end": 8409297}, {"filename": "/data/shaders/ant_light_color_kaleidovoid.glsl", "start": 8409297, "end": 8411444}, {"filename": "/data/shaders/ant_light_color_liquid_prism.glsl", "start": 8411444, "end": 8413989}, {"filename": "/data/shaders/ant_light_color_mirror_inferno.glsl", "start": 8413989, "end": 8416549}, {"filename": "/data/shaders/ant_light_color_molten_mirror.glsl", "start": 8416549, "end": 8418992}, {"filename": "/data/shaders/ant_light_color_neon_mandala.glsl", "start": 8418992, "end": 8421311}, {"filename": "/data/shaders/ant_light_color_neon_rain.glsl", "start": 8421311, "end": 8423900}, {"filename": "/data/shaders/ant_light_color_neon_shatter.glsl", "start": 8423900, "end": 8426211}, {"filename": "/data/shaders/ant_light_color_nova_burst.glsl", "start": 8426211, "end": 8428640}, {"filename": "/data/shaders/ant_light_color_phase_shift.glsl", "start": 8428640, "end": 8430927}, {"filename": "/data/shaders/ant_light_color_plasma_helix.glsl", "start": 8430927, "end": 8433325}, {"filename": "/data/shaders/ant_light_color_plasma_ocean.glsl", "start": 8433325, "end": 8436072}, {"filename": "/data/shaders/ant_light_color_prism_bloom_echo.glsl", "start": 8436072, "end": 8438375}, {"filename": "/data/shaders/ant_light_color_prism_tornado.glsl", "start": 8438375, "end": 8440357}, {"filename": "/data/shaders/ant_light_color_psyche_tunnel.glsl", "start": 8440357, "end": 8443098}, {"filename": "/data/shaders/ant_light_color_pulse_matrix.glsl", "start": 8443098, "end": 8445529}, {"filename": "/data/shaders/ant_light_color_quantum_fold.glsl", "start": 8445529, "end": 8447836}, {"filename": "/data/shaders/ant_light_color_radiant_mosaic.glsl", "start": 8447836, "end": 8450445}, {"filename": "/data/shaders/ant_light_color_shockwave_prism.glsl", "start": 8450445, "end": 8453035}, {"filename": "/data/shaders/ant_light_color_solar_corona.glsl", "start": 8453035, "end": 8456205}, {"filename": "/data/shaders/ant_light_color_solar_flare.glsl", "start": 8456205, "end": 8459016}, {"filename": "/data/shaders/ant_light_color_spectral_drain.glsl", "start": 8459016, "end": 8461692}, {"filename": "/data/shaders/ant_light_color_spiral_nebula.glsl", "start": 8461692, "end": 8464765}, {"filename": "/data/shaders/ant_light_color_stardust_spiral.glsl", "start": 8464765, "end": 8467369}, {"filename": "/data/shaders/ant_light_color_strobe_fracture.glsl", "start": 8467369, "end": 8469579}, {"filename": "/data/shaders/ant_light_color_tesseract_fold.glsl", "start": 8469579, "end": 8471736}, {"filename": "/data/shaders/ant_light_color_vortex_bloom.glsl", "start": 8471736, "end": 8473933}, {"filename": "/data/shaders/ant_light_color_warp_cathedral.glsl", "start": 8473933, "end": 8476356}, {"filename": "/data/shaders/ant_light_color_wave_collapse.glsl", "start": 8476356, "end": 8478498}, {"filename": "/data/shaders/ant_medianblend_cache.glsl", "start": 8478498, "end": 8482328}, {"filename": "/data/shaders/ant_new_chrome_tunnel.glsl", "start": 8482328, "end": 8485104}, {"filename": "/data/shaders/ant_new_flame_vortex.glsl", "start": 8485104, "end": 8487893}, {"filename": "/data/shaders/ant_new_helix_mirror.glsl", "start": 8487893, "end": 8491065}, {"filename": "/data/shaders/ant_new_infinity_plasma.glsl", "start": 8491065, "end": 8493906}, {"filename": "/data/shaders/ant_new_kaleido_fractal.glsl", "start": 8493906, "end": 8496536}, {"filename": "/data/shaders/ant_new_liquid_helix.glsl", "start": 8496536, "end": 8499447}, {"filename": "/data/shaders/ant_new_ocean_kaleido.glsl", "start": 8499447, "end": 8502503}, {"filename": "/data/shaders/ant_new_plasma_ocean.glsl", "start": 8502503, "end": 8504985}, {"filename": "/data/shaders/ant_new_prism_flame.glsl", "start": 8504985, "end": 8507914}, {"filename": "/data/shaders/ant_new_vortex_helix.glsl", "start": 8507914, "end": 8510338}, {"filename": "/data/shaders/ant_peak_inversion_cache_spectrum_time_acid_rain_flood.glsl", "start": 8510338, "end": 8513865}, {"filename": "/data/shaders/ant_peak_inversion_cache_spectrum_time_caustic_drowning.glsl", "start": 8513865, "end": 8517496}, {"filename": "/data/shaders/ant_peak_inversion_cache_spectrum_time_chromatic_quake.glsl", "start": 8517496, "end": 8521183}, {"filename": "/data/shaders/ant_peak_inversion_cache_spectrum_time_fractal_inferno.glsl", "start": 8521183, "end": 8524699}, {"filename": "/data/shaders/ant_peak_inversion_cache_spectrum_time_galaxy_devourer.glsl", "start": 8524699, "end": 8528433}, {"filename": "/data/shaders/ant_peak_inversion_cache_spectrum_time_glitch_apocalypse.glsl", "start": 8528433, "end": 8531994}, {"filename": "/data/shaders/ant_peak_inversion_cache_spectrum_time_hex_seizure.glsl", "start": 8531994, "end": 8535572}, {"filename": "/data/shaders/ant_peak_inversion_cache_spectrum_time_hologram_collapse.glsl", "start": 8535572, "end": 8539278}, {"filename": "/data/shaders/ant_peak_inversion_cache_spectrum_time_hyperspace_tunnel.glsl", "start": 8539278, "end": 8542890}, {"filename": "/data/shaders/ant_peak_inversion_cache_spectrum_time_kaleidoscope_storm.glsl", "start": 8542890, "end": 8546730}, {"filename": "/data/shaders/ant_peak_inversion_cache_spectrum_time_lightning_god.glsl", "start": 8546730, "end": 8550361}, {"filename": "/data/shaders/ant_peak_inversion_cache_spectrum_time_magma_eruption.glsl", "start": 8550361, "end": 8553886}, {"filename": "/data/shaders/ant_peak_inversion_cache_spectrum_time_mandala_pulse.glsl", "start": 8553886, "end": 8557579}, {"filename": "/data/shaders/ant_peak_inversion_cache_spectrum_time_neon_grid_inferno.glsl", "start": 8557579, "end": 8561196}, {"filename": "/data/shaders/ant_peak_inversion_cache_spectrum_time_neural_overload.glsl", "start": 8561196, "end": 8564977}, {"filename": "/data/shaders/ant_peak_inversion_cache_spectrum_time_oilslick_meltdown.glsl", "start": 8564977, "end": 8568565}, {"filename": "/data/shaders/ant_peak_inversion_cache_spectrum_time_plasma_furnace.glsl", "start": 8568565, "end": 8572141}, {"filename": "/data/shaders/ant_peak_inversion_cache_spectrum_time_radial_echo_chamber.glsl", "start": 8572141, "end": 8575699}, {"filename": "/data/shaders/ant_peak_inversion_cache_spectrum_time_spectrum_visualizer.glsl", "start": 8575699, "end": 8579535}, {"filename": "/data/shaders/ant_peak_inversion_cache_spectrum_time_starburst_supernova.glsl", "start": 8579535, "end": 8583095}, {"filename": "/data/shaders/ant_peak_inversion_cache_spectrum_time_strobe_void.glsl", "start": 8583095, "end": 8586756}, {"filename": "/data/shaders/ant_peak_inversion_cache_spectrum_time_voronoi_seizure.glsl", "start": 8586756, "end": 8590644}, {"filename": "/data/shaders/ant_peak_inversion_cache_spectrum_time_vortex_singularity.glsl", "start": 8590644, "end": 8594201}, {"filename": "/data/shaders/ant_peak_inversion_cache_spectrum_time_warp_drive_overload.glsl", "start": 8594201, "end": 8597892}, {"filename": "/data/shaders/ant_peak_inversion_cache_spectrum_time_zebra_riot.glsl", "start": 8597892, "end": 8601487}, {"filename": "/data/shaders/ant_spectrum_aurora_fold.glsl", "start": 8601487, "end": 8604129}, {"filename": "/data/shaders/ant_spectrum_chromatic_lotus.glsl", "start": 8604129, "end": 8607014}, {"filename": "/data/shaders/ant_spectrum_chromatic_pulse.glsl", "start": 8607014, "end": 8609884}, {"filename": "/data/shaders/ant_spectrum_chromatic_web.glsl", "start": 8609884, "end": 8612770}, {"filename": "/data/shaders/ant_spectrum_crystal_cathedral.glsl", "start": 8612770, "end": 8615513}, {"filename": "/data/shaders/ant_spectrum_crystal_echo.glsl", "start": 8615513, "end": 8618160}, {"filename": "/data/shaders/ant_spectrum_crystal_gradient.glsl", "start": 8618160, "end": 8620894}, {"filename": "/data/shaders/ant_spectrum_diamond_echo.glsl", "start": 8620894, "end": 8623278}, {"filename": "/data/shaders/ant_spectrum_echo_bloom.glsl", "start": 8623278, "end": 8626003}, {"filename": "/data/shaders/ant_spectrum_echo_chamber.glsl", "start": 8626003, "end": 8628293}, {"filename": "/data/shaders/ant_spectrum_echo_diamond.glsl", "start": 8628293, "end": 8630958}, {"filename": "/data/shaders/ant_spectrum_echo_prism.glsl", "start": 8630958, "end": 8633778}, {"filename": "/data/shaders/ant_spectrum_echo_spiral.glsl", "start": 8633778, "end": 8636106}, {"filename": "/data/shaders/ant_spectrum_echo_vortex.glsl", "start": 8636106, "end": 8639064}, {"filename": "/data/shaders/ant_spectrum_fractal_mirror.glsl", "start": 8639064, "end": 8641315}, {"filename": "/data/shaders/ant_spectrum_gradient_bloom.glsl", "start": 8641315, "end": 8644004}, {"filename": "/data/shaders/ant_spectrum_gradient_kaleidoscope.glsl", "start": 8644004, "end": 8646643}, {"filename": "/data/shaders/ant_spectrum_gradient_mirror.glsl", "start": 8646643, "end": 8648959}, {"filename": "/data/shaders/ant_spectrum_gradient_storm.glsl", "start": 8648959, "end": 8651868}, {"filename": "/data/shaders/ant_spectrum_kaleido_flame.glsl", "start": 8651868, "end": 8655046}, {"filename": "/data/shaders/ant_spectrum_kaleido_pulse.glsl", "start": 8655046, "end": 8657583}, {"filename": "/data/shaders/ant_spectrum_kaleido_rings.glsl", "start": 8657583, "end": 8660753}, {"filename": "/data/shaders/ant_spectrum_kaleido_storm.glsl", "start": 8660753, "end": 8663875}, {"filename": "/data/shaders/ant_spectrum_kaleido_wave.glsl", "start": 8663875, "end": 8666503}, {"filename": "/data/shaders/ant_spectrum_kaleidoscope_dream.glsl", "start": 8666503, "end": 8669087}, {"filename": "/data/shaders/ant_spectrum_mirror_bloom.glsl", "start": 8669087, "end": 8671442}, {"filename": "/data/shaders/ant_spectrum_mirror_cascade.glsl", "start": 8671442, "end": 8673993}, {"filename": "/data/shaders/ant_spectrum_mirror_fractal.glsl", "start": 8673993, "end": 8676309}, {"filename": "/data/shaders/ant_spectrum_mirror_infinity.glsl", "start": 8676309, "end": 8679163}, {"filename": "/data/shaders/ant_spectrum_mirror_kaleidoscope.glsl", "start": 8679163, "end": 8682077}, {"filename": "/data/shaders/ant_spectrum_mirror_nebula.glsl", "start": 8682077, "end": 8685031}, {"filename": "/data/shaders/ant_spectrum_mirror_spiral.glsl", "start": 8685031, "end": 8687900}, {"filename": "/data/shaders/ant_spectrum_mirror_wave.glsl", "start": 8687900, "end": 8690100}, {"filename": "/data/shaders/ant_spectrum_neon_butterfly.glsl", "start": 8690100, "end": 8692995}, {"filename": "/data/shaders/ant_spectrum_neon_echo.glsl", "start": 8692995, "end": 8695757}, {"filename": "/data/shaders/ant_spectrum_neon_mandala.glsl", "start": 8695757, "end": 8698534}, {"filename": "/data/shaders/ant_spectrum_neon_mirror.glsl", "start": 8698534, "end": 8701098}, {"filename": "/data/shaders/ant_spectrum_prism_cascade.glsl", "start": 8701098, "end": 8703482}, {"filename": "/data/shaders/ant_spectrum_prism_fold.glsl", "start": 8703482, "end": 8706288}, {"filename": "/data/shaders/ant_spectrum_prism_mirror.glsl", "start": 8706288, "end": 8708934}, {"filename": "/data/shaders/ant_spectrum_prismatic_bloom.glsl", "start": 8708934, "end": 8711521}, {"filename": "/data/shaders/ant_spectrum_prismatic_tunnel.glsl", "start": 8711521, "end": 8714195}, {"filename": "/data/shaders/ant_spectrum_rainbow_cathedral.glsl", "start": 8714195, "end": 8717336}, {"filename": "/data/shaders/ant_spectrum_rainbow_echo.glsl", "start": 8717336, "end": 8720027}, {"filename": "/data/shaders/ant_spectrum_rainbow_fold.glsl", "start": 8720027, "end": 8722530}, {"filename": "/data/shaders/ant_spectrum_rainbow_fractal.glsl", "start": 8722530, "end": 8725030}, {"filename": "/data/shaders/ant_spectrum_rainbow_helix.glsl", "start": 8725030, "end": 8727574}, {"filename": "/data/shaders/ant_spectrum_rainbow_pulse.glsl", "start": 8727574, "end": 8729826}, {"filename": "/data/shaders/ant_spectrum_rainbow_shatter.glsl", "start": 8729826, "end": 8732685}, {"filename": "/data/shaders/ant_spectrum_rainbow_vortex.glsl", "start": 8732685, "end": 8735008}, {"filename": "/data/shaders/ant_texture_cache_spectrum_scale_chromatic.glsl", "start": 8735008, "end": 8738685}, {"filename": "/data/shaders/ant_texture_cache_spectrum_scale_comet.glsl", "start": 8738685, "end": 8742299}, {"filename": "/data/shaders/ant_texture_cache_spectrum_scale_cosmic_web.glsl", "start": 8742299, "end": 8745908}, {"filename": "/data/shaders/ant_texture_cache_spectrum_scale_ember.glsl", "start": 8745908, "end": 8749688}, {"filename": "/data/shaders/ant_texture_cache_spectrum_scale_fracture.glsl", "start": 8749688, "end": 8753310}, {"filename": "/data/shaders/ant_texture_cache_spectrum_scale_ghost.glsl", "start": 8753310, "end": 8756960}, {"filename": "/data/shaders/ant_texture_cache_spectrum_scale_glow_pulse.glsl", "start": 8756960, "end": 8760618}, {"filename": "/data/shaders/ant_texture_cache_spectrum_scale_holo_grid.glsl", "start": 8760618, "end": 8764382}, {"filename": "/data/shaders/ant_texture_cache_spectrum_scale_inkdrop.glsl", "start": 8764382, "end": 8767991}, {"filename": "/data/shaders/ant_texture_cache_spectrum_scale_kaleido.glsl", "start": 8767991, "end": 8771666}, {"filename": "/data/shaders/ant_texture_cache_spectrum_scale_lava.glsl", "start": 8771666, "end": 8775333}, {"filename": "/data/shaders/ant_texture_cache_spectrum_scale_lightning.glsl", "start": 8775333, "end": 8779047}, {"filename": "/data/shaders/ant_texture_cache_spectrum_scale_mirror_drift.glsl", "start": 8779047, "end": 8782644}, {"filename": "/data/shaders/ant_texture_cache_spectrum_scale_mosaic.glsl", "start": 8782644, "end": 8786260}, {"filename": "/data/shaders/ant_texture_cache_spectrum_scale_neon_trail.glsl", "start": 8786260, "end": 8789970}, {"filename": "/data/shaders/ant_texture_cache_spectrum_scale_plasma.glsl", "start": 8789970, "end": 8793712}, {"filename": "/data/shaders/ant_texture_cache_spectrum_scale_polar_echo.glsl", "start": 8793712, "end": 8797333}, {"filename": "/data/shaders/ant_texture_cache_spectrum_scale_ripple.glsl", "start": 8797333, "end": 8800957}, {"filename": "/data/shaders/ant_texture_cache_spectrum_scale_shockwave.glsl", "start": 8800957, "end": 8804640}, {"filename": "/data/shaders/ant_texture_cache_spectrum_scale_silk.glsl", "start": 8804640, "end": 8808380}, {"filename": "/data/shaders/ant_texture_cache_spectrum_scale_starfield.glsl", "start": 8808380, "end": 8811943}, {"filename": "/data/shaders/ant_texture_cache_spectrum_scale_swirl.glsl", "start": 8811943, "end": 8815591}, {"filename": "/data/shaders/ant_texture_cache_spectrum_scale_tunnel.glsl", "start": 8815591, "end": 8819170}, {"filename": "/data/shaders/ant_texture_cache_spectrum_scale_vortex.glsl", "start": 8819170, "end": 8822820}, {"filename": "/data/shaders/ant_texture_cache_spectrum_scale_wave_warp.glsl", "start": 8822820, "end": 8826432}, {"filename": "/data/shaders/ant_time_f_color_acid_smear.glsl", "start": 8826432, "end": 8829776}, {"filename": "/data/shaders/ant_time_f_color_afterimage_burn.glsl", "start": 8829776, "end": 8833047}, {"filename": "/data/shaders/ant_time_f_color_bass_pulse_rings.glsl", "start": 8833047, "end": 8836192}, {"filename": "/data/shaders/ant_time_f_color_crosshatch_recall.glsl", "start": 8836192, "end": 8839566}, {"filename": "/data/shaders/ant_time_f_color_data_stream.glsl", "start": 8839566, "end": 8842879}, {"filename": "/data/shaders/ant_time_f_color_echo_chamber.glsl", "start": 8842879, "end": 8846086}, {"filename": "/data/shaders/ant_time_f_color_fft_aurora.glsl", "start": 8846086, "end": 8849401}, {"filename": "/data/shaders/ant_time_f_color_ghost_train.glsl", "start": 8849401, "end": 8852679}, {"filename": "/data/shaders/ant_time_f_color_glitch_archive.glsl", "start": 8852679, "end": 8855930}, {"filename": "/data/shaders/ant_time_f_color_holo_replay.glsl", "start": 8855930, "end": 8859700}, {"filename": "/data/shaders/ant_time_f_color_light_painting.glsl", "start": 8859700, "end": 8862945}, {"filename": "/data/shaders/ant_time_f_color_memory_kaleidoscope.glsl", "start": 8862945, "end": 8866295}, {"filename": "/data/shaders/ant_time_f_color_mirror_corridor.glsl", "start": 8866295, "end": 8869598}, {"filename": "/data/shaders/ant_time_f_color_parallax_layers.glsl", "start": 8869598, "end": 8872838}, {"filename": "/data/shaders/ant_time_f_color_polar_history.glsl", "start": 8872838, "end": 8876228}, {"filename": "/data/shaders/ant_time_f_color_quantum_freeze.glsl", "start": 8876228, "end": 8879479}, {"filename": "/data/shaders/ant_time_f_color_radial_smear.glsl", "start": 8879479, "end": 8882772}, {"filename": "/data/shaders/ant_time_f_color_ringbuffer_spiral.glsl", "start": 8882772, "end": 8886294}, {"filename": "/data/shaders/ant_time_f_color_shutter_burst.glsl", "start": 8886294, "end": 8889635}, {"filename": "/data/shaders/ant_time_f_color_spectrogram_paint.glsl", "start": 8889635, "end": 8892958}, {"filename": "/data/shaders/ant_time_f_color_spectrum_waterfall.glsl", "start": 8892958, "end": 8896324}, {"filename": "/data/shaders/ant_time_f_color_timeline_strips.glsl", "start": 8896324, "end": 8899629}, {"filename": "/data/shaders/ant_time_f_color_timeline_swirl.glsl", "start": 8899629, "end": 8903072}, {"filename": "/data/shaders/ant_time_f_color_trail_blaze.glsl", "start": 8903072, "end": 8906303}, {"filename": "/data/shaders/ant_time_f_color_tunnel_recall.glsl", "start": 8906303, "end": 8909630}, {"filename": "/data/shaders/apart.glsl", "start": 8909630, "end": 8910455}, {"filename": "/data/shaders/apart_mouse.glsl", "start": 8910455, "end": 8911916}, {"filename": "/data/shaders/atan-bowl.glsl", "start": 8911916, "end": 8913263}, {"filename": "/data/shaders/atan-glitch.glsl", "start": 8913263, "end": 8915560}, {"filename": "/data/shaders/audio_mouse.glsl", "start": 8915560, "end": 8916330}, {"filename": "/data/shaders/audiocolor.glsl", "start": 8916330, "end": 8919590}, {"filename": "/data/shaders/aura.glsl", "start": 8919590, "end": 8920638}, {"filename": "/data/shaders/aura2.glsl", "start": 8920638, "end": 8926600}, {"filename": "/data/shaders/aura3.glsl", "start": 8926600, "end": 8927450}, {"filename": "/data/shaders/aura4.glsl", "start": 8927450, "end": 8929320}, {"filename": "/data/shaders/aura5.glsl", "start": 8929320, "end": 8937796}, {"filename": "/data/shaders/aura6.glsl", "start": 8937796, "end": 8942477}, {"filename": "/data/shaders/aura7.glsl", "start": 8942477, "end": 8950246}, {"filename": "/data/shaders/aura8.glsl", "start": 8950246, "end": 8957027}, {"filename": "/data/shaders/aura9.glsl", "start": 8957027, "end": 8964548}, {"filename": "/data/shaders/auraXi1.glsl", "start": 8964548, "end": 8968523}, {"filename": "/data/shaders/auraXi2.glsl", "start": 8968523, "end": 8972931}, {"filename": "/data/shaders/auraXi3.glsl", "start": 8972931, "end": 8977442}, {"filename": "/data/shaders/average_pixels.glsl", "start": 8977442, "end": 8978215}, {"filename": "/data/shaders/balloon.glsl", "start": 8978215, "end": 8979043}, {"filename": "/data/shaders/bend.glsl", "start": 8979043, "end": 8981012}, {"filename": "/data/shaders/bend2.glsl", "start": 8981012, "end": 8981763}, {"filename": "/data/shaders/bend2mouse.glsl", "start": 8981763, "end": 8983549}, {"filename": "/data/shaders/bend_dir.glsl", "start": 8983549, "end": 8984827}, {"filename": "/data/shaders/bend_rev.glsl", "start": 8984827, "end": 8985557}, {"filename": "/data/shaders/bend_twist.glsl", "start": 8985557, "end": 8986601}, {"filename": "/data/shaders/bi_cycle.glsl", "start": 8986601, "end": 8987327}, {"filename": "/data/shaders/bit_rainbow.glsl", "start": 8987327, "end": 8988422}, {"filename": "/data/shaders/black_rings.glsl", "start": 8988422, "end": 8993686}, {"filename": "/data/shaders/blackhole.glsl", "start": 8993686, "end": 8994247}, {"filename": "/data/shaders/blank.glsl", "start": 8994247, "end": 8996655}, {"filename": "/data/shaders/blend_orig_10_cache.glsl", "start": 8996655, "end": 8998398}, {"filename": "/data/shaders/blend_orig_25_cache.glsl", "start": 8998398, "end": 9000141}, {"filename": "/data/shaders/blend_orig_50_cache.glsl", "start": 9000141, "end": 9001883}, {"filename": "/data/shaders/blend_orig_75_cache.glsl", "start": 9001883, "end": 9003626}, {"filename": "/data/shaders/block_pixels.glsl", "start": 9003626, "end": 9003977}, {"filename": "/data/shaders/bloom.glsl", "start": 9003977, "end": 9007246}, {"filename": "/data/shaders/blue.glsl", "start": 9007246, "end": 9008257}, {"filename": "/data/shaders/blue_2.glsl", "start": 9008257, "end": 9009429}, {"filename": "/data/shaders/blue_3.glsl", "start": 9009429, "end": 9010626}, {"filename": "/data/shaders/blue_4.glsl", "start": 9010626, "end": 9011829}, {"filename": "/data/shaders/blue_shade.glsl", "start": 9011829, "end": 9012983}, {"filename": "/data/shaders/blue_strobe.glsl", "start": 9012983, "end": 9014030}, {"filename": "/data/shaders/blue_wave.glsl", "start": 9014030, "end": 9014983}, {"filename": "/data/shaders/blur_offset.glsl", "start": 9014983, "end": 9015889}, {"filename": "/data/shaders/blur_offset_color_fade.glsl", "start": 9015889, "end": 9017404}, {"filename": "/data/shaders/blurry.glsl", "start": 9017404, "end": 9018109}, {"filename": "/data/shaders/bowl-by_time.glsl", "start": 9018109, "end": 9019279}, {"filename": "/data/shaders/bowl.glsl", "start": 9019279, "end": 9020478}, {"filename": "/data/shaders/breathe.glsl", "start": 9020478, "end": 9021606}, {"filename": "/data/shaders/bright.glsl", "start": 9021606, "end": 9022469}, {"filename": "/data/shaders/bright_rainbow.glsl", "start": 9022469, "end": 9023165}, {"filename": "/data/shaders/brighten.glsl", "start": 9023165, "end": 9023740}, {"filename": "/data/shaders/brighten_rev.glsl", "start": 9023740, "end": 9024333}, {"filename": "/data/shaders/brightness_increase.glsl", "start": 9024333, "end": 9025968}, {"filename": "/data/shaders/broadcast.glsl", "start": 9025968, "end": 9028251}, {"filename": "/data/shaders/broadcast_sync_lost.glsl", "start": 9028251, "end": 9031151}, {"filename": "/data/shaders/brokenglass.glsl", "start": 9031151, "end": 9032519}, {"filename": "/data/shaders/brot-zoom-mouse.glsl", "start": 9032519, "end": 9033937}, {"filename": "/data/shaders/bubble-2.glsl", "start": 9033937, "end": 9034644}, {"filename": "/data/shaders/bubble-move.glsl", "start": 9034644, "end": 9035685}, {"filename": "/data/shaders/bubble-zoom-mouse.glsl", "start": 9035685, "end": 9036882}, {"filename": "/data/shaders/bubble.glsl", "start": 9036882, "end": 9037313}, {"filename": "/data/shaders/bubbleGL.glsl", "start": 9037313, "end": 9038368}, {"filename": "/data/shaders/bubble_amp.glsl", "start": 9038368, "end": 9039229}, {"filename": "/data/shaders/c_ripple.glsl", "start": 9039229, "end": 9039768}, {"filename": "/data/shaders/cane.glsl", "start": 9039768, "end": 9040609}, {"filename": "/data/shaders/cd.glsl", "start": 9040609, "end": 9041063}, {"filename": "/data/shaders/cd_zoom.glsl", "start": 9041063, "end": 9041631}, {"filename": "/data/shaders/cd_zoom_out.glsl", "start": 9041631, "end": 9042174}, {"filename": "/data/shaders/chue.glsl", "start": 9042174, "end": 9043007}, {"filename": "/data/shaders/cmod.glsl", "start": 9043007, "end": 9045072}, {"filename": "/data/shaders/code-frac-aurora-vault.glsl", "start": 9045072, "end": 9050813}, {"filename": "/data/shaders/code-frac-bass-singularity.glsl", "start": 9050813, "end": 9056559}, {"filename": "/data/shaders/code-frac-chromatic-rift.glsl", "start": 9056559, "end": 9062303}, {"filename": "/data/shaders/code-frac-cosmic-iris.glsl", "start": 9062303, "end": 9068045}, {"filename": "/data/shaders/code-frac-crystal-tunnel.glsl", "start": 9068045, "end": 9073788}, {"filename": "/data/shaders/code-frac-diamond-echo.glsl", "start": 9073788, "end": 9079529}, {"filename": "/data/shaders/code-frac-electric-reef.glsl", "start": 9079529, "end": 9085272}, {"filename": "/data/shaders/code-frac-fracture-garden.glsl", "start": 9085272, "end": 9091018}, {"filename": "/data/shaders/code-frac-glitch-cathedral.glsl", "start": 9091018, "end": 9096764}, {"filename": "/data/shaders/code-frac-infinite-stair.glsl", "start": 9096764, "end": 9102508}, {"filename": "/data/shaders/code-frac-laser-orchid.glsl", "start": 9102508, "end": 9108251}, {"filename": "/data/shaders/code-frac-liquid-mirror.glsl", "start": 9108251, "end": 9113993}, {"filename": "/data/shaders/code-frac-mandala-pulse.glsl", "start": 9113993, "end": 9119737}, {"filename": "/data/shaders/code-frac-neon-petals.glsl", "start": 9119737, "end": 9125478}, {"filename": "/data/shaders/code-frac-ocean-kaleido.glsl", "start": 9125478, "end": 9131221}, {"filename": "/data/shaders/code-frac-plasma-orbit.glsl", "start": 9131221, "end": 9136962}, {"filename": "/data/shaders/code-frac-prism-wormhole.glsl", "start": 9136962, "end": 9142705}, {"filename": "/data/shaders/code-frac-quartz-ripples.glsl", "start": 9142705, "end": 9148449}, {"filename": "/data/shaders/code-frac-ruby-hyperspace.glsl", "start": 9148449, "end": 9154194}, {"filename": "/data/shaders/code-frac-solar-lattice.glsl", "start": 9154194, "end": 9159936}, {"filename": "/data/shaders/code-frac-spectral-hive.glsl", "start": 9159936, "end": 9165680}, {"filename": "/data/shaders/code-frac-spiral-crown.glsl", "start": 9165680, "end": 9171423}, {"filename": "/data/shaders/code-frac-velvet-recursion.glsl", "start": 9171423, "end": 9177169}, {"filename": "/data/shaders/code-frac-vortex-bloom.glsl", "start": 9177169, "end": 9182911}, {"filename": "/data/shaders/code-frac-x-astral-dream-engine.glsl", "start": 9182911, "end": 9193485}, {"filename": "/data/shaders/code-frac-x-aurora-maelstrom.glsl", "start": 9193485, "end": 9204037}, {"filename": "/data/shaders/code-frac-x-aurora-sanctum.glsl", "start": 9204037, "end": 9214617}, {"filename": "/data/shaders/code-frac-x-chromatic-constellation.glsl", "start": 9214617, "end": 9225893}, {"filename": "/data/shaders/code-frac-x-chrono-tunnel.glsl", "start": 9225893, "end": 9236402}, {"filename": "/data/shaders/code-frac-x-cosmic-filigree.glsl", "start": 9236402, "end": 9247042}, {"filename": "/data/shaders/code-frac-x-diamond-supernova.glsl", "start": 9247042, "end": 9257593}, {"filename": "/data/shaders/code-frac-x-event-horizon-bloom.glsl", "start": 9257593, "end": 9268178}, {"filename": "/data/shaders/code-frac-x-fractured-singularity.glsl", "start": 9268178, "end": 9279393}, {"filename": "/data/shaders/code-frac-x-holographic-hive.glsl", "start": 9279393, "end": 9290572}, {"filename": "/data/shaders/code-frac-x-hyperion-cathedral.glsl", "start": 9290572, "end": 9301162}, {"filename": "/data/shaders/code-frac-x-infinite-rose-window.glsl", "start": 9301162, "end": 9311685}, {"filename": "/data/shaders/code-frac-x-julia-wormhole.glsl", "start": 9311685, "end": 9322697}, {"filename": "/data/shaders/code-frac-x-liquid-julia-dream.glsl", "start": 9322697, "end": 9333690}, {"filename": "/data/shaders/code-frac-x-molten-ripples.glsl", "start": 9333690, "end": 9344177}, {"filename": "/data/shaders/code-frac-x-nebula-vortex.glsl", "start": 9344177, "end": 9354750}, {"filename": "/data/shaders/code-frac-x-oceanic-shatter.glsl", "start": 9354750, "end": 9365937}, {"filename": "/data/shaders/code-frac-x-orbital-julia.glsl", "start": 9365937, "end": 9377007}, {"filename": "/data/shaders/code-frac-x-plasma-chrysalis.glsl", "start": 9377007, "end": 9387565}, {"filename": "/data/shaders/code-frac-x-prismatic-basilica.glsl", "start": 9387565, "end": 9398785}, {"filename": "/data/shaders/code-frac-x-quantum-crystal.glsl", "start": 9398785, "end": 9409768}, {"filename": "/data/shaders/code-frac-x-spectral-icefield.glsl", "start": 9409768, "end": 9420313}, {"filename": "/data/shaders/code-frac-x-stained-glass-julia.glsl", "start": 9420313, "end": 9431337}, {"filename": "/data/shaders/code-frac-x-tesseract-lattice.glsl", "start": 9431337, "end": 9441820}, {"filename": "/data/shaders/code-frac-x-velvet-recursion.glsl", "start": 9441820, "end": 9452455}, {"filename": "/data/shaders/code-frac-zero-gravity-fold.glsl", "start": 9452455, "end": 9458202}, {"filename": "/data/shaders/code-mirror-aurora-corridor.glsl", "start": 9458202, "end": 9466284}, {"filename": "/data/shaders/code-mirror-bass-observatory.glsl", "start": 9466284, "end": 9474368}, {"filename": "/data/shaders/code-mirror-celestial-palace.glsl", "start": 9474368, "end": 9482451}, {"filename": "/data/shaders/code-mirror-chromatic-monolith.glsl", "start": 9482451, "end": 9490538}, {"filename": "/data/shaders/code-mirror-crystal-ballroom.glsl", "start": 9490538, "end": 9498622}, {"filename": "/data/shaders/code-mirror-diamond-horizon.glsl", "start": 9498622, "end": 9506704}, {"filename": "/data/shaders/code-mirror-echo-temple.glsl", "start": 9506704, "end": 9514783}, {"filename": "/data/shaders/code-mirror-electric-mandala.glsl", "start": 9514783, "end": 9522868}, {"filename": "/data/shaders/code-mirror-fractured-opera.glsl", "start": 9522868, "end": 9530952}, {"filename": "/data/shaders/code-mirror-glass-tesseract.glsl", "start": 9530952, "end": 9539035}, {"filename": "/data/shaders/code-mirror-infinity-rose.glsl", "start": 9539035, "end": 9547116}, {"filename": "/data/shaders/code-mirror-laser-pagoda.glsl", "start": 9547116, "end": 9555197}, {"filename": "/data/shaders/code-mirror-liquid-chandelier.glsl", "start": 9555197, "end": 9563281}, {"filename": "/data/shaders/code-mirror-nebula-lens.glsl", "start": 9563281, "end": 9571360}, {"filename": "/data/shaders/code-mirror-neon-reliquary.glsl", "start": 9571360, "end": 9579442}, {"filename": "/data/shaders/code-mirror-ocean-origami.glsl", "start": 9579442, "end": 9587523}, {"filename": "/data/shaders/code-mirror-opal-wormhole.glsl", "start": 9587523, "end": 9595604}, {"filename": "/data/shaders/code-mirror-plasma-catacombs.glsl", "start": 9595604, "end": 9603689}, {"filename": "/data/shaders/code-mirror-prismatic-sanctum.glsl", "start": 9603689, "end": 9611774}, {"filename": "/data/shaders/code-mirror-ripple-citadel.glsl", "start": 9611774, "end": 9619856}, {"filename": "/data/shaders/code-mirror-solar-mosaic.glsl", "start": 9619856, "end": 9627936}, {"filename": "/data/shaders/code-mirror-spectral-basilica.glsl", "start": 9627936, "end": 9636021}, {"filename": "/data/shaders/code-mirror-velvet-kaleidoscope.glsl", "start": 9636021, "end": 9644109}, {"filename": "/data/shaders/code-mirror-vortex-gallery.glsl", "start": 9644109, "end": 9652190}, {"filename": "/data/shaders/code-mirror-zero-gravity-mirage.glsl", "start": 9652190, "end": 9660278}, {"filename": "/data/shaders/code-update-aurora-cathedral.glsl", "start": 9660278, "end": 9662363}, {"filename": "/data/shaders/code-update-celestial-circuit.glsl", "start": 9662363, "end": 9664024}, {"filename": "/data/shaders/code-update-chromatic-gravity-lens.glsl", "start": 9664024, "end": 9665657}, {"filename": "/data/shaders/code-update-cosmic-loom.glsl", "start": 9665657, "end": 9667300}, {"filename": "/data/shaders/code-update-cyber-lotus.glsl", "start": 9667300, "end": 9668908}, {"filename": "/data/shaders/code-update-diamond-supernova.glsl", "start": 9668908, "end": 9670559}, {"filename": "/data/shaders/code-update-electric-reef.glsl", "start": 9670559, "end": 9672157}, {"filename": "/data/shaders/code-update-event-horizon-crown.glsl", "start": 9672157, "end": 9673981}, {"filename": "/data/shaders/code-update-fractal-eclipse.glsl", "start": 9673981, "end": 9675701}, {"filename": "/data/shaders/code-update-holographic-monolith.glsl", "start": 9675701, "end": 9677323}, {"filename": "/data/shaders/code-update-iridescent-turbine.glsl", "start": 9677323, "end": 9678835}, {"filename": "/data/shaders/code-update-laser-basilica.glsl", "start": 9678835, "end": 9680395}, {"filename": "/data/shaders/code-update-liquid-stained-glass.glsl", "start": 9680395, "end": 9682344}, {"filename": "/data/shaders/code-update-mercury-rose.glsl", "start": 9682344, "end": 9683982}, {"filename": "/data/shaders/code-update-nebula-origami.glsl", "start": 9683982, "end": 9685648}, {"filename": "/data/shaders/code-update-neon-mycelium.glsl", "start": 9685648, "end": 9687348}, {"filename": "/data/shaders/code-update-oceanic-star-forge.glsl", "start": 9687348, "end": 9688875}, {"filename": "/data/shaders/code-update-opal-ferrofluid.glsl", "start": 9688875, "end": 9690783}, {"filename": "/data/shaders/code-update-plasma-chrysalis.glsl", "start": 9690783, "end": 9692401}, {"filename": "/data/shaders/code-update-quantum-ribbon-vault.glsl", "start": 9692401, "end": 9694008}, {"filename": "/data/shaders/code-update-quartz-tsunami.glsl", "start": 9694008, "end": 9695542}, {"filename": "/data/shaders/code-update-radiant-dream-engine.glsl", "start": 9695542, "end": 9697441}, {"filename": "/data/shaders/code-update-spectral-wormhole.glsl", "start": 9697441, "end": 9698965}, {"filename": "/data/shaders/code-update-temporal-prism-orbit.glsl", "start": 9698965, "end": 9700962}, {"filename": "/data/shaders/code-update-velvet-inferno.glsl", "start": 9700962, "end": 9702747}, {"filename": "/data/shaders/code_flux_mouse.glsl", "start": 9702747, "end": 9704227}, {"filename": "/data/shaders/code_rev3.glsl", "start": 9704227, "end": 9704853}, {"filename": "/data/shaders/code_wave.glsl", "start": 9704853, "end": 9705185}, {"filename": "/data/shaders/code_wave_rev.glsl", "start": 9705185, "end": 9705529}, {"filename": "/data/shaders/code_wave_side.glsl", "start": 9705529, "end": 9705867}, {"filename": "/data/shaders/codex_aurora_vortex.glsl", "start": 9705867, "end": 9707176}, {"filename": "/data/shaders/codex_cartoon.glsl", "start": 9707176, "end": 9710106}, {"filename": "/data/shaders/codex_cartoon_color.glsl", "start": 9710106, "end": 9713526}, {"filename": "/data/shaders/codex_cartoon_saturday.glsl", "start": 9713526, "end": 9717714}, {"filename": "/data/shaders/codex_cartoon_vhs.glsl", "start": 9717714, "end": 9723441}, {"filename": "/data/shaders/codex_chrome_spiderweb.glsl", "start": 9723441, "end": 9724642}, {"filename": "/data/shaders/codex_complex_aura.glsl", "start": 9724642, "end": 9728549}, {"filename": "/data/shaders/codex_fractal_orchid.glsl", "start": 9728549, "end": 9729799}, {"filename": "/data/shaders/codex_glass_mosaic_flow.glsl", "start": 9729799, "end": 9731050}, {"filename": "/data/shaders/codex_glitch_bloom_wormhole.glsl", "start": 9731050, "end": 9732560}, {"filename": "/data/shaders/codex_glitch_folded_scanburn.glsl", "start": 9732560, "end": 9733877}, {"filename": "/data/shaders/codex_glitch_fractal_datamosh.glsl", "start": 9733877, "end": 9735316}, {"filename": "/data/shaders/codex_glitch_kaleido_tear.glsl", "start": 9735316, "end": 9736767}, {"filename": "/data/shaders/codex_glitch_logpolar_rift.glsl", "start": 9736767, "end": 9737973}, {"filename": "/data/shaders/codex_glitch_mandel_wound.glsl", "start": 9737973, "end": 9739350}, {"filename": "/data/shaders/codex_glitch_neon_escape.glsl", "start": 9739350, "end": 9740613}, {"filename": "/data/shaders/codex_glitch_psychic_tiles.glsl", "start": 9740613, "end": 9741913}, {"filename": "/data/shaders/codex_glitch_recursive_xor.glsl", "start": 9741913, "end": 9743214}, {"filename": "/data/shaders/codex_glitch_shard_stutter.glsl", "start": 9743214, "end": 9744572}, {"filename": "/data/shaders/codex_grad_fractal_aurora_mandala.glsl", "start": 9744572, "end": 9745818}, {"filename": "/data/shaders/codex_grad_fractal_crystal_bloom.glsl", "start": 9745818, "end": 9747012}, {"filename": "/data/shaders/codex_grad_fractal_electric_garden.glsl", "start": 9747012, "end": 9748469}, {"filename": "/data/shaders/codex_grad_fractal_hypernova_eye.glsl", "start": 9748469, "end": 9749828}, {"filename": "/data/shaders/codex_grad_fractal_inferno_lattice.glsl", "start": 9749828, "end": 9751142}, {"filename": "/data/shaders/codex_grad_fractal_nebula_gate.glsl", "start": 9751142, "end": 9752423}, {"filename": "/data/shaders/codex_grad_fractal_oceanic_void.glsl", "start": 9752423, "end": 9753609}, {"filename": "/data/shaders/codex_grad_fractal_prism_storm.glsl", "start": 9753609, "end": 9755000}, {"filename": "/data/shaders/codex_grad_fractal_royal_shatter.glsl", "start": 9755000, "end": 9756450}, {"filename": "/data/shaders/codex_grad_fractal_solar_crown.glsl", "start": 9756450, "end": 9757731}, {"filename": "/data/shaders/codex_liquid_tessellation.glsl", "start": 9757731, "end": 9759381}, {"filename": "/data/shaders/codex_neon_topography.glsl", "start": 9759381, "end": 9760520}, {"filename": "/data/shaders/codex_polar_echo_lens.glsl", "start": 9760520, "end": 9761830}, {"filename": "/data/shaders/codex_prism_feedback.glsl", "start": 9761830, "end": 9763265}, {"filename": "/data/shaders/codex_quantum_bloom.glsl", "start": 9763265, "end": 9764517}, {"filename": "/data/shaders/codex_signal_decay.glsl", "start": 9764517, "end": 9765747}, {"filename": "/data/shaders/codex_vhs_chroma_bleed.glsl", "start": 9765747, "end": 9766724}, {"filename": "/data/shaders/codex_vhs_crt_combo.glsl", "start": 9766724, "end": 9767978}, {"filename": "/data/shaders/codex_vhs_ghost_luma.glsl", "start": 9767978, "end": 9768987}, {"filename": "/data/shaders/codex_vhs_head_switch.glsl", "start": 9768987, "end": 9770040}, {"filename": "/data/shaders/codex_vhs_magnetic_dropouts.glsl", "start": 9770040, "end": 9771083}, {"filename": "/data/shaders/codex_vhs_midnight_dub.glsl", "start": 9771083, "end": 9772341}, {"filename": "/data/shaders/codex_vhs_pause_jitter.glsl", "start": 9772341, "end": 9773326}, {"filename": "/data/shaders/codex_vhs_rainbow_noise.glsl", "start": 9773326, "end": 9774245}, {"filename": "/data/shaders/codex_vhs_tape_warp.glsl", "start": 9774245, "end": 9775252}, {"filename": "/data/shaders/codex_vhs_tracking_roll.glsl", "start": 9775252, "end": 9776491}, {"filename": "/data/shaders/color-bloom.glsl", "start": 9776491, "end": 9777651}, {"filename": "/data/shaders/color-f-diag.glsl", "start": 9777651, "end": 9778272}, {"filename": "/data/shaders/color-f.glsl", "start": 9778272, "end": 9778873}, {"filename": "/data/shaders/color-f2.glsl", "start": 9778873, "end": 9779624}, {"filename": "/data/shaders/color-max.glsl", "start": 9779624, "end": 9783288}, {"filename": "/data/shaders/color-mod-dark.glsl", "start": 9783288, "end": 9784141}, {"filename": "/data/shaders/color-mod.glsl", "start": 9784141, "end": 9784910}, {"filename": "/data/shaders/color-o.glsl", "start": 9784910, "end": 9785500}, {"filename": "/data/shaders/color-o1.glsl", "start": 9785500, "end": 9785500}, {"filename": "/data/shaders/color_bars.glsl", "start": 9785500, "end": 9786505}, {"filename": "/data/shaders/color_effect.glsl", "start": 9786505, "end": 9787776}, {"filename": "/data/shaders/color_effect_xor.glsl", "start": 9787776, "end": 9789079}, {"filename": "/data/shaders/color_flux.glsl", "start": 9789079, "end": 9790515}, {"filename": "/data/shaders/color_flux_whirl.glsl", "start": 9790515, "end": 9791837}, {"filename": "/data/shaders/color_g.glsl", "start": 9791837, "end": 9793337}, {"filename": "/data/shaders/color_glitch_dark.glsl", "start": 9793337, "end": 9795195}, {"filename": "/data/shaders/color_grad1.glsl", "start": 9795195, "end": 9795835}, {"filename": "/data/shaders/color_grad_rainbow.glsl", "start": 9795835, "end": 9797787}, {"filename": "/data/shaders/color_increase.glsl", "start": 9797787, "end": 9798312}, {"filename": "/data/shaders/color_infinity_loop_single.glsl", "start": 9798312, "end": 9800994}, {"filename": "/data/shaders/color_l.glsl", "start": 9800994, "end": 9802610}, {"filename": "/data/shaders/color_pal.glsl", "start": 9802610, "end": 9808024}, {"filename": "/data/shaders/color_peak_inversion.glsl", "start": 9808024, "end": 9811971}, {"filename": "/data/shaders/color_peak_inversion_cache.glsl", "start": 9811971, "end": 9817320}, {"filename": "/data/shaders/color_peak_inversion_cache_spectrum8.glsl", "start": 9817320, "end": 9823755}, {"filename": "/data/shaders/color_peak_inversion_cache_spectrum_time.glsl", "start": 9823755, "end": 9830207}, {"filename": "/data/shaders/color_pool.glsl", "start": 9830207, "end": 9831095}, {"filename": "/data/shaders/color_scale.glsl", "start": 9831095, "end": 9832027}, {"filename": "/data/shaders/color_shift_fade.glsl", "start": 9832027, "end": 9833061}, {"filename": "/data/shaders/color_swirl-o1.glsl", "start": 9833061, "end": 9834028}, {"filename": "/data/shaders/color_swirl_beautiful.glsl", "start": 9834028, "end": 9835383}, {"filename": "/data/shaders/color_swirl_c.glsl", "start": 9835383, "end": 9836753}, {"filename": "/data/shaders/color_swirl_g.glsl", "start": 9836753, "end": 9838556}, {"filename": "/data/shaders/color_swirl_o1_mouse.glsl", "start": 9838556, "end": 9839613}, {"filename": "/data/shaders/color_swirl_sin.glsl", "start": 9839613, "end": 9840612}, {"filename": "/data/shaders/color_swirl_sin_mouse.glsl", "start": 9840612, "end": 9841717}, {"filename": "/data/shaders/color_time.glsl", "start": 9841717, "end": 9842380}, {"filename": "/data/shaders/color_timeval.glsl", "start": 9842380, "end": 9843612}, {"filename": "/data/shaders/color_trails_cache_spectrum.glsl", "start": 9843612, "end": 9847428}, {"filename": "/data/shaders/color_trails_cache_spectrum_jet_ember.glsl", "start": 9847428, "end": 9849971}, {"filename": "/data/shaders/color_trails_cache_spectrum_jet_glacier.glsl", "start": 9849971, "end": 9852388}, {"filename": "/data/shaders/color_trails_cache_spectrum_jet_nebula.glsl", "start": 9852388, "end": 9854846}, {"filename": "/data/shaders/color_trails_cache_spectrum_jet_photon.glsl", "start": 9854846, "end": 9857454}, {"filename": "/data/shaders/color_trails_cache_spectrum_jet_storm.glsl", "start": 9857454, "end": 9859900}, {"filename": "/data/shaders/color_trails_cache_spectrum_knot_braid.glsl", "start": 9859900, "end": 9862432}, {"filename": "/data/shaders/color_trails_cache_spectrum_knot_helix.glsl", "start": 9862432, "end": 9864901}, {"filename": "/data/shaders/color_trails_cache_spectrum_knot_spindle.glsl", "start": 9864901, "end": 9867448}, {"filename": "/data/shaders/color_trails_cache_spectrum_knot_tangle.glsl", "start": 9867448, "end": 9869991}, {"filename": "/data/shaders/color_trails_cache_spectrum_knot_velvet.glsl", "start": 9869991, "end": 9872447}, {"filename": "/data/shaders/color_trails_cache_spectrum_orbit_gyro.glsl", "start": 9872447, "end": 9875089}, {"filename": "/data/shaders/color_trails_cache_spectrum_orbit_halo.glsl", "start": 9875089, "end": 9877698}, {"filename": "/data/shaders/color_trails_cache_spectrum_orbit_lantern.glsl", "start": 9877698, "end": 9880269}, {"filename": "/data/shaders/color_trails_cache_spectrum_orbit_prism.glsl", "start": 9880269, "end": 9883173}, {"filename": "/data/shaders/color_trails_cache_spectrum_orbit_vortex.glsl", "start": 9883173, "end": 9885877}, {"filename": "/data/shaders/color_trails_cache_spectrum_ribbon_aurora.glsl", "start": 9885877, "end": 9890031}, {"filename": "/data/shaders/color_trails_cache_spectrum_ribbon_blaze.glsl", "start": 9890031, "end": 9894188}, {"filename": "/data/shaders/color_trails_cache_spectrum_ribbon_comet.glsl", "start": 9894188, "end": 9898728}, {"filename": "/data/shaders/color_trails_cache_spectrum_ribbon_monsoon.glsl", "start": 9898728, "end": 9902805}, {"filename": "/data/shaders/color_trails_cache_spectrum_ribbon_serpent.glsl", "start": 9902805, "end": 9907062}, {"filename": "/data/shaders/color_trails_cache_spectrum_shear_delta.glsl", "start": 9907062, "end": 9909548}, {"filename": "/data/shaders/color_trails_cache_spectrum_shear_magnet.glsl", "start": 9909548, "end": 9912137}, {"filename": "/data/shaders/color_trails_cache_spectrum_shear_mirage.glsl", "start": 9912137, "end": 9914616}, {"filename": "/data/shaders/color_trails_cache_spectrum_shear_quartz.glsl", "start": 9914616, "end": 9917049}, {"filename": "/data/shaders/color_trails_cache_spectrum_shear_razor.glsl", "start": 9917049, "end": 9919589}, {"filename": "/data/shaders/color_w.glsl", "start": 9919589, "end": 9921511}, {"filename": "/data/shaders/color_w2.glsl", "start": 9921511, "end": 9922639}, {"filename": "/data/shaders/color_w_mouse.glsl", "start": 9922639, "end": 9924673}, {"filename": "/data/shaders/color_xor.glsl", "start": 9924673, "end": 9925683}, {"filename": "/data/shaders/colordivflash.glsl", "start": 9925683, "end": 9926516}, {"filename": "/data/shaders/colorsheet.glsl", "start": 9926516, "end": 9927334}, {"filename": "/data/shaders/comb.glsl", "start": 9927334, "end": 9928448}, {"filename": "/data/shaders/comb3-frac-mouse.glsl", "start": 9928448, "end": 9930464}, {"filename": "/data/shaders/comb3-frac-mouse2.glsl", "start": 9930464, "end": 9932480}, {"filename": "/data/shaders/comb3.glsl", "start": 9932480, "end": 9934201}, {"filename": "/data/shaders/comb3_geo_mouse.glsl", "start": 9934201, "end": 9937431}, {"filename": "/data/shaders/comb3_mouse.glsl", "start": 9937431, "end": 9939455}, {"filename": "/data/shaders/comp_inc.glsl", "start": 9939455, "end": 9941883}, {"filename": "/data/shaders/comp_mouse.glsl", "start": 9941883, "end": 9943465}, {"filename": "/data/shaders/comp_quality.glsl", "start": 9943465, "end": 9944961}, {"filename": "/data/shaders/comp_zoom.glsl", "start": 9944961, "end": 9946018}, {"filename": "/data/shaders/compo.glsl", "start": 9946018, "end": 9947673}, {"filename": "/data/shaders/composite-static.glsl", "start": 9947673, "end": 9951754}, {"filename": "/data/shaders/composite.glsl", "start": 9951754, "end": 9952522}, {"filename": "/data/shaders/composite2.glsl", "start": 9952522, "end": 9953908}, {"filename": "/data/shaders/composite3.glsl", "start": 9953908, "end": 9958721}, {"filename": "/data/shaders/composite_crt.glsl", "start": 9958721, "end": 9960531}, {"filename": "/data/shaders/composite_vhs.glsl", "start": 9960531, "end": 9962277}, {"filename": "/data/shaders/composite_vhs_flat.glsl", "start": 9962277, "end": 9963778}, {"filename": "/data/shaders/coord_color.glsl", "start": 9963778, "end": 9964496}, {"filename": "/data/shaders/cos.glsl", "start": 9964496, "end": 9965290}, {"filename": "/data/shaders/cos_sin.glsl", "start": 9965290, "end": 9966087}, {"filename": "/data/shaders/cosine_swirl.glsl", "start": 9966087, "end": 9967261}, {"filename": "/data/shaders/cosine_swirl_two.glsl", "start": 9967261, "end": 9968941}, {"filename": "/data/shaders/cripple.glsl", "start": 9968941, "end": 9969304}, {"filename": "/data/shaders/cripple2.glsl", "start": 9969304, "end": 9969706}, {"filename": "/data/shaders/crt.glsl", "start": 9969706, "end": 9970321}, {"filename": "/data/shaders/crystal-2.glsl", "start": 9970321, "end": 9971249}, {"filename": "/data/shaders/crystal-3.glsl", "start": 9971249, "end": 9972456}, {"filename": "/data/shaders/crystal-4.glsl", "start": 9972456, "end": 9973649}, {"filename": "/data/shaders/crystal.glsl", "start": 9973649, "end": 9974423}, {"filename": "/data/shaders/crystalball.glsl", "start": 9974423, "end": 9975282}, {"filename": "/data/shaders/crystalbend.glsl", "start": 9975282, "end": 9977206}, {"filename": "/data/shaders/crystalblend2.glsl", "start": 9977206, "end": 9979527}, {"filename": "/data/shaders/crystalprism.glsl", "start": 9979527, "end": 9981556}, {"filename": "/data/shaders/cwarp.glsl", "start": 9981556, "end": 9983179}, {"filename": "/data/shaders/cwave.glsl", "start": 9983179, "end": 9983591}, {"filename": "/data/shaders/cwave2.glsl", "start": 9983591, "end": 9984234}, {"filename": "/data/shaders/cxripple.glsl", "start": 9984234, "end": 9984957}, {"filename": "/data/shaders/cyclone.glsl", "start": 9984957, "end": 9986149}, {"filename": "/data/shaders/cyclone_zoom.glsl", "start": 9986149, "end": 9986924}, {"filename": "/data/shaders/cyclone_zoom_mouse.glsl", "start": 9986924, "end": 9987791}, {"filename": "/data/shaders/d1.glsl", "start": 9987791, "end": 9988783}, {"filename": "/data/shaders/damaged_vcr.glsl", "start": 9988783, "end": 9992091}, {"filename": "/data/shaders/dark.glsl", "start": 9992091, "end": 9993348}, {"filename": "/data/shaders/dark_addup_xor.glsl", "start": 9993348, "end": 9994331}, {"filename": "/data/shaders/dark_psych.glsl", "start": 9994331, "end": 9995766}, {"filename": "/data/shaders/dark_psych_wave.glsl", "start": 9995766, "end": 9997575}, {"filename": "/data/shaders/dark_rainbow.glsl", "start": 9997575, "end": 9998285}, {"filename": "/data/shaders/dark_rainbow2.glsl", "start": 9998285, "end": 9999232}, {"filename": "/data/shaders/dark_rainbow_limit.glsl", "start": 9999232, "end": 10000130}, {"filename": "/data/shaders/dark_rainbow_rubber_soul.glsl", "start": 10000130, "end": 10001033}, {"filename": "/data/shaders/dark_square.glsl", "start": 10001033, "end": 10001977}, {"filename": "/data/shaders/darkb.glsl", "start": 10001977, "end": 10002464}, {"filename": "/data/shaders/darkg.glsl", "start": 10002464, "end": 10002951}, {"filename": "/data/shaders/darkr.glsl", "start": 10002951, "end": 10003438}, {"filename": "/data/shaders/darkred.glsl", "start": 10003438, "end": 10004418}, {"filename": "/data/shaders/data.glsl", "start": 10004418, "end": 10006766}, {"filename": "/data/shaders/datam.glsl", "start": 10006766, "end": 10008656}, {"filename": "/data/shaders/datam1.glsl", "start": 10008656, "end": 10013235}, {"filename": "/data/shaders/datam2.glsl", "start": 10013235, "end": 10015161}, {"filename": "/data/shaders/datam3.glsl", "start": 10015161, "end": 10017154}, {"filename": "/data/shaders/datam4.glsl", "start": 10017154, "end": 10019188}, {"filename": "/data/shaders/datam5.glsl", "start": 10019188, "end": 10021228}, {"filename": "/data/shaders/datam_a.glsl", "start": 10021228, "end": 10023331}, {"filename": "/data/shaders/deepcolor.glsl", "start": 10023331, "end": 10025658}, {"filename": "/data/shaders/deeps1.glsl", "start": 10025658, "end": 10028265}, {"filename": "/data/shaders/deeps3.glsl", "start": 10028265, "end": 10029569}, {"filename": "/data/shaders/deepseek3.glsl", "start": 10029569, "end": 10032154}, {"filename": "/data/shaders/deepseek4.glsl", "start": 10032154, "end": 10034632}, {"filename": "/data/shaders/deform.glsl", "start": 10034632, "end": 10035050}, {"filename": "/data/shaders/deform_cos.glsl", "start": 10035050, "end": 10035468}, {"filename": "/data/shaders/deform_tan.glsl", "start": 10035468, "end": 10035886}, {"filename": "/data/shaders/delic.glsl", "start": 10035886, "end": 10036674}, {"filename": "/data/shaders/dhue.glsl", "start": 10036674, "end": 10037804}, {"filename": "/data/shaders/diagsquare.glsl", "start": 10037804, "end": 10038509}, {"filename": "/data/shaders/diagsquarecircle.glsl", "start": 10038509, "end": 10039377}, {"filename": "/data/shaders/diamond.glsl", "start": 10039377, "end": 10040632}, {"filename": "/data/shaders/diamond_pattern_1.glsl", "start": 10040632, "end": 10042362}, {"filename": "/data/shaders/disk.glsl", "start": 10042362, "end": 10043064}, {"filename": "/data/shaders/disk2.glsl", "start": 10043064, "end": 10044205}, {"filename": "/data/shaders/distort-disp.glsl", "start": 10044205, "end": 10044801}, {"filename": "/data/shaders/distort_light.glsl", "start": 10044801, "end": 10046174}, {"filename": "/data/shaders/distortf.glsl", "start": 10046174, "end": 10047218}, {"filename": "/data/shaders/distortf2.glsl", "start": 10047218, "end": 10048453}, {"filename": "/data/shaders/distortf3.glsl", "start": 10048453, "end": 10049282}, {"filename": "/data/shaders/distortf4.glsl", "start": 10049282, "end": 10050142}, {"filename": "/data/shaders/dmd_gpt.glsl", "start": 10050142, "end": 10051007}, {"filename": "/data/shaders/dmdc.glsl", "start": 10051007, "end": 10051901}, {"filename": "/data/shaders/drain.glsl", "start": 10051901, "end": 10053252}, {"filename": "/data/shaders/drain_bend.glsl", "start": 10053252, "end": 10054435}, {"filename": "/data/shaders/drain_mandella.glsl", "start": 10054435, "end": 10056677}, {"filename": "/data/shaders/drain_mirror.glsl", "start": 10056677, "end": 10057484}, {"filename": "/data/shaders/drain_mirror_amp.glsl", "start": 10057484, "end": 10058989}, {"filename": "/data/shaders/drain_mirror_top.glsl", "start": 10058989, "end": 10059797}, {"filename": "/data/shaders/drain_mouse.glsl", "start": 10059797, "end": 10060742}, {"filename": "/data/shaders/drain_rainbow.glsl", "start": 10060742, "end": 10062676}, {"filename": "/data/shaders/drain_reset.glsl", "start": 10062676, "end": 10063447}, {"filename": "/data/shaders/dream-tunnel.glsl", "start": 10063447, "end": 10065753}, {"filename": "/data/shaders/dream-tunnel2.glsl", "start": 10065753, "end": 10068303}, {"filename": "/data/shaders/ds1.glsl", "start": 10068303, "end": 10069773}, {"filename": "/data/shaders/dseek.glsl", "start": 10069773, "end": 10071309}, {"filename": "/data/shaders/dye.glsl", "start": 10071309, "end": 10071309}, {"filename": "/data/shaders/dys.glsl", "start": 10071309, "end": 10072517}, {"filename": "/data/shaders/echo-pingpong.glsl", "start": 10072517, "end": 10073457}, {"filename": "/data/shaders/echo-pingpong_c.glsl", "start": 10073457, "end": 10074486}, {"filename": "/data/shaders/echo.glsl", "start": 10074486, "end": 10075154}, {"filename": "/data/shaders/echo1.glsl", "start": 10075154, "end": 10075481}, {"filename": "/data/shaders/echo2.glsl", "start": 10075481, "end": 10075811}, {"filename": "/data/shaders/echo3.glsl", "start": 10075811, "end": 10076193}, {"filename": "/data/shaders/echo_2.glsl", "start": 10076193, "end": 10076862}, {"filename": "/data/shaders/echo_3.glsl", "start": 10076862, "end": 10077582}, {"filename": "/data/shaders/echo_4.glsl", "start": 10077582, "end": 10078303}, {"filename": "/data/shaders/echo_4_mix.glsl", "start": 10078303, "end": 10078840}, {"filename": "/data/shaders/echo_4_rev.glsl", "start": 10078840, "end": 10079723}, {"filename": "/data/shaders/echo_5_mix_rev.glsl", "start": 10079723, "end": 10080255}, {"filename": "/data/shaders/echo_alpha.glsl", "start": 10080255, "end": 10080952}, {"filename": "/data/shaders/echo_alpha_three.glsl", "start": 10080952, "end": 10081654}, {"filename": "/data/shaders/echo_alpha_three_rgb.glsl", "start": 10081654, "end": 10082441}, {"filename": "/data/shaders/echo_alpha_two.glsl", "start": 10082441, "end": 10083144}, {"filename": "/data/shaders/echo_alpha_two_xor.glsl", "start": 10083144, "end": 10084277}, {"filename": "/data/shaders/echo_and_or_xor.glsl", "start": 10084277, "end": 10085582}, {"filename": "/data/shaders/echo_bgr.glsl", "start": 10085582, "end": 10086416}, {"filename": "/data/shaders/echo_cache.glsl", "start": 10086416, "end": 10087423}, {"filename": "/data/shaders/echo_cf.glsl", "start": 10087423, "end": 10088433}, {"filename": "/data/shaders/echo_color.glsl", "start": 10088433, "end": 10089187}, {"filename": "/data/shaders/echo_color1.glsl", "start": 10089187, "end": 10089878}, {"filename": "/data/shaders/echo_color2.glsl", "start": 10089878, "end": 10090569}, {"filename": "/data/shaders/echo_color3.glsl", "start": 10090569, "end": 10091260}, {"filename": "/data/shaders/echo_color4.glsl", "start": 10091260, "end": 10091969}, {"filename": "/data/shaders/echo_color5.glsl", "start": 10091969, "end": 10092678}, {"filename": "/data/shaders/echo_div2.glsl", "start": 10092678, "end": 10093432}, {"filename": "/data/shaders/echo_div4.glsl", "start": 10093432, "end": 10094186}, {"filename": "/data/shaders/echo_div4_rainbow.glsl", "start": 10094186, "end": 10095629}, {"filename": "/data/shaders/echo_g.glsl", "start": 10095629, "end": 10096543}, {"filename": "/data/shaders/echo_loop.glsl", "start": 10096543, "end": 10097272}, {"filename": "/data/shaders/echo_loop2.glsl", "start": 10097272, "end": 10097988}, {"filename": "/data/shaders/echo_mirror_1.glsl", "start": 10097988, "end": 10098738}, {"filename": "/data/shaders/echo_mirror_2.glsl", "start": 10098738, "end": 10099513}, {"filename": "/data/shaders/echo_mirror_3.glsl", "start": 10099513, "end": 10100287}, {"filename": "/data/shaders/echo_mix.glsl", "start": 10100287, "end": 10101222}, {"filename": "/data/shaders/echo_mix_colors.glsl", "start": 10101222, "end": 10102093}, {"filename": "/data/shaders/echo_mix_colors_rev.glsl", "start": 10102093, "end": 10102964}, {"filename": "/data/shaders/echo_or.glsl", "start": 10102964, "end": 10104308}, {"filename": "/data/shaders/echo_or_and_xor.glsl", "start": 10104308, "end": 10105614}, {"filename": "/data/shaders/echo_rainbow_spin.glsl", "start": 10105614, "end": 10106595}, {"filename": "/data/shaders/echo_rainbow_swirl.glsl", "start": 10106595, "end": 10107795}, {"filename": "/data/shaders/echo_rand.glsl", "start": 10107795, "end": 10108747}, {"filename": "/data/shaders/echo_rand_2.glsl", "start": 10108747, "end": 10109655}, {"filename": "/data/shaders/echo_rev_x1.glsl", "start": 10109655, "end": 10110401}, {"filename": "/data/shaders/echo_rev_x2.glsl", "start": 10110401, "end": 10111146}, {"filename": "/data/shaders/echo_rgb.glsl", "start": 10111146, "end": 10111980}, {"filename": "/data/shaders/echo_rgb1.glsl", "start": 10111980, "end": 10112699}, {"filename": "/data/shaders/echo_rgb_mouse.glsl", "start": 10112699, "end": 10113808}, {"filename": "/data/shaders/echo_s.glsl", "start": 10113808, "end": 10114897}, {"filename": "/data/shaders/echo_s_rgb.glsl", "start": 10114897, "end": 10116125}, {"filename": "/data/shaders/echo_shift.glsl", "start": 10116125, "end": 10117152}, {"filename": "/data/shaders/echo_sin.glsl", "start": 10117152, "end": 10117772}, {"filename": "/data/shaders/echo_sin_particle.glsl", "start": 10117772, "end": 10119446}, {"filename": "/data/shaders/echo_strobe_x1.glsl", "start": 10119446, "end": 10120383}, {"filename": "/data/shaders/echo_two.glsl", "start": 10120383, "end": 10121056}, {"filename": "/data/shaders/echo_x2.glsl", "start": 10121056, "end": 10122178}, {"filename": "/data/shaders/echo_x3.glsl", "start": 10122178, "end": 10123075}, {"filename": "/data/shaders/echo_x4.glsl", "start": 10123075, "end": 10124035}, {"filename": "/data/shaders/echo_x5.glsl", "start": 10124035, "end": 10124789}, {"filename": "/data/shaders/echo_x6.glsl", "start": 10124789, "end": 10125547}, {"filename": "/data/shaders/echo_xor.glsl", "start": 10125547, "end": 10126893}, {"filename": "/data/shaders/echo_xor_color_blend.glsl", "start": 10126893, "end": 10128170}, {"filename": "/data/shaders/echo_xor_or_and.glsl", "start": 10128170, "end": 10129451}, {"filename": "/data/shaders/edge_black.glsl", "start": 10129451, "end": 10130699}, {"filename": "/data/shaders/edge_face.glsl", "start": 10130699, "end": 10132318}, {"filename": "/data/shaders/edge_fill.glsl", "start": 10132318, "end": 10133979}, {"filename": "/data/shaders/edge_pencil.glsl", "start": 10133979, "end": 10135442}, {"filename": "/data/shaders/edge_rainbow.glsl", "start": 10135442, "end": 10136957}, {"filename": "/data/shaders/effect.glsl", "start": 10136957, "end": 10137782}, {"filename": "/data/shaders/elastic.glsl", "start": 10137782, "end": 10138765}, {"filename": "/data/shaders/elastic2.glsl", "start": 10138765, "end": 10142157}, {"filename": "/data/shaders/elastic_mouse.glsl", "start": 10142157, "end": 10144441}, {"filename": "/data/shaders/empty.glsl", "start": 10144441, "end": 10144686}, {"filename": "/data/shaders/empty1.glsl", "start": 10144686, "end": 10144866}, {"filename": "/data/shaders/error.log.txt", "start": 10144866, "end": 10144866}, {"filename": "/data/shaders/es1.glsl", "start": 10144866, "end": 10146814}, {"filename": "/data/shaders/expand_contract_.glsl", "start": 10146814, "end": 10148016}, {"filename": "/data/shaders/expand_mul.glsl", "start": 10148016, "end": 10148426}, {"filename": "/data/shaders/expand_mul_fast.glsl", "start": 10148426, "end": 10148845}, {"filename": "/data/shaders/expand_spot.glsl", "start": 10148845, "end": 10149460}, {"filename": "/data/shaders/eye2.glsl", "start": 10149460, "end": 10153483}, {"filename": "/data/shaders/eyes.glsl", "start": 10153483, "end": 10155619}, {"filename": "/data/shaders/faster.glsl", "start": 10155619, "end": 10158455}, {"filename": "/data/shaders/fat-blue.glsl", "start": 10158455, "end": 10158979}, {"filename": "/data/shaders/fat-green.glsl", "start": 10158979, "end": 10159503}, {"filename": "/data/shaders/fat-red.glsl", "start": 10159503, "end": 10160027}, {"filename": "/data/shaders/fat-rgb.glsl", "start": 10160027, "end": 10160860}, {"filename": "/data/shaders/fat-slow.glsl", "start": 10160860, "end": 10161222}, {"filename": "/data/shaders/fat.glsl", "start": 10161222, "end": 10161537}, {"filename": "/data/shaders/feedback_inf_hands_cache.glsl", "start": 10161537, "end": 10166746}, {"filename": "/data/shaders/feedback_infinite_recursion_fractal_cache.glsl", "start": 10166746, "end": 10171942}, {"filename": "/data/shaders/feedback_infinite_tunnel_cache.glsl", "start": 10171942, "end": 10176440}, {"filename": "/data/shaders/feedback_recursion_cache.glsl", "start": 10176440, "end": 10178988}, {"filename": "/data/shaders/feedback_recursion_fractal_cache.glsl", "start": 10178988, "end": 10184163}, {"filename": "/data/shaders/fill_black.glsl", "start": 10184163, "end": 10184437}, {"filename": "/data/shaders/fill_black_fold.glsl", "start": 10184437, "end": 10186517}, {"filename": "/data/shaders/fill_white.glsl", "start": 10186517, "end": 10187910}, {"filename": "/data/shaders/fish_pos_mouse.glsl", "start": 10187910, "end": 10189011}, {"filename": "/data/shaders/fisheye.glsl", "start": 10189011, "end": 10189572}, {"filename": "/data/shaders/fisheye_mouse.glsl", "start": 10189572, "end": 10190834}, {"filename": "/data/shaders/fisheye_pi.mp4.glsl", "start": 10190834, "end": 10192303}, {"filename": "/data/shaders/fisheye_warp.glsl", "start": 10192303, "end": 10192775}, {"filename": "/data/shaders/flash.glsl", "start": 10192775, "end": 10193725}, {"filename": "/data/shaders/flash_gradient_strobe.glsl", "start": 10193725, "end": 10194430}, {"filename": "/data/shaders/focus.glsl", "start": 10194430, "end": 10194987}, {"filename": "/data/shaders/fold-fov.glsl", "start": 10194987, "end": 10195656}, {"filename": "/data/shaders/fold-mirror.glsl", "start": 10195656, "end": 10195978}, {"filename": "/data/shaders/fold-spin.glsl", "start": 10195978, "end": 10196607}, {"filename": "/data/shaders/fold-water.glsl", "start": 10196607, "end": 10197744}, {"filename": "/data/shaders/fold.glsl", "start": 10197744, "end": 10198056}, {"filename": "/data/shaders/frac-wrap.glsl", "start": 10198056, "end": 10205028}, {"filename": "/data/shaders/frac_cache.glsl", "start": 10205028, "end": 10212980}, {"filename": "/data/shaders/frac_cosine.glsl", "start": 10212980, "end": 10221245}, {"filename": "/data/shaders/frac_mir.glsl", "start": 10221245, "end": 10223762}, {"filename": "/data/shaders/frac_mirror_mouse_zoom.glsl", "start": 10223762, "end": 10226827}, {"filename": "/data/shaders/frac_mouse_z.glsl", "start": 10226827, "end": 10231161}, {"filename": "/data/shaders/frac_mouse_z2.glsl", "start": 10231161, "end": 10235471}, {"filename": "/data/shaders/frac_shade02_dmdXi3_rot.glsl", "start": 10235471, "end": 10240203}, {"filename": "/data/shaders/frac_shader01.glsl", "start": 10240203, "end": 10245268}, {"filename": "/data/shaders/frac_shader01_dark.glsl", "start": 10245268, "end": 10252561}, {"filename": "/data/shaders/frac_shader01_smooth.glsl", "start": 10252561, "end": 10259735}, {"filename": "/data/shaders/frac_shader01_smooth_neon.glsl", "start": 10259735, "end": 10267658}, {"filename": "/data/shaders/frac_shader02_dmd.glsl", "start": 10267658, "end": 10273226}, {"filename": "/data/shaders/frac_shader02_dmd2.glsl", "start": 10273226, "end": 10279925}, {"filename": "/data/shaders/frac_shader02_dmd2_amp.glsl", "start": 10279925, "end": 10288583}, {"filename": "/data/shaders/frac_shader02_dmd3.glsl", "start": 10288583, "end": 10295311}, {"filename": "/data/shaders/frac_shader02_dmd4.glsl", "start": 10295311, "end": 10302100}, {"filename": "/data/shaders/frac_shader02_dmd5.glsl", "start": 10302100, "end": 10308881}, {"filename": "/data/shaders/frac_shader02_dmd6i.glsl", "start": 10308881, "end": 10315777}, {"filename": "/data/shaders/frac_shader02_dmd6i1.glsl", "start": 10315777, "end": 10322778}, {"filename": "/data/shaders/frac_shader02_dmd6iFold.glsl", "start": 10322778, "end": 10326007}, {"filename": "/data/shaders/frac_shader02_dmd6i_air.glsl", "start": 10326007, "end": 10336264}, {"filename": "/data/shaders/frac_shader02_dmd6i_air_twist.glsl", "start": 10336264, "end": 10347506}, {"filename": "/data/shaders/frac_shader02_dmd6i_amp.glsl", "start": 10347506, "end": 10354907}, {"filename": "/data/shaders/frac_shader02_dmd6i_audio.glsl", "start": 10354907, "end": 10362277}, {"filename": "/data/shaders/frac_shader02_dmd6i_bowl.glsl", "start": 10362277, "end": 10373150}, {"filename": "/data/shaders/frac_shader02_dmd6i_bubble.glsl", "start": 10373150, "end": 10381408}, {"filename": "/data/shaders/frac_shader02_dmd6i_neon.glsl", "start": 10381408, "end": 10389081}, {"filename": "/data/shaders/frac_shader02_dmd6i_spectrum16.glsl", "start": 10389081, "end": 10400335}, {"filename": "/data/shaders/frac_shader02_dmd6i_transparent.glsl", "start": 10400335, "end": 10407664}, {"filename": "/data/shaders/frac_shader02_dmd6i_wrap.glsl", "start": 10407664, "end": 10414635}, {"filename": "/data/shaders/frac_shader02_dmd7i.glsl", "start": 10414635, "end": 10421562}, {"filename": "/data/shaders/frac_shader02_dmd8i.glsl", "start": 10421562, "end": 10428492}, {"filename": "/data/shaders/frac_shader02_dmd9i.glsl", "start": 10428492, "end": 10435485}, {"filename": "/data/shaders/frac_shader02_dmdXi.glsl", "start": 10435485, "end": 10440076}, {"filename": "/data/shaders/frac_shader02_dmdXi2.glsl", "start": 10440076, "end": 10444737}, {"filename": "/data/shaders/frac_shader02_dmdXi3.glsl", "start": 10444737, "end": 10449392}, {"filename": "/data/shaders/frac_shader02_dmdXi3_drain.glsl", "start": 10449392, "end": 10455575}, {"filename": "/data/shaders/frac_shader02_dmdXi3_warp.glsl", "start": 10455575, "end": 10461140}, {"filename": "/data/shaders/frac_shader02_dmdXi4.glsl", "start": 10461140, "end": 10466722}, {"filename": "/data/shaders/frac_shader02_dmdXi5.glsl", "start": 10466722, "end": 10476565}, {"filename": "/data/shaders/frac_shader02_dmdXi_amp.glsl", "start": 10476565, "end": 10483449}, {"filename": "/data/shaders/frac_shader02_dmd_mandella.glsl", "start": 10483449, "end": 10496612}, {"filename": "/data/shaders/frac_shader02_dmdi6i_zoom.glsl", "start": 10496612, "end": 10503980}, {"filename": "/data/shaders/frac_shader02_dmdi6i_zoom_xor.glsl", "start": 10503980, "end": 10512801}, {"filename": "/data/shaders/frac_shader02_dmdi6i_zoom_xor_amp.glsl", "start": 10512801, "end": 10522064}, {"filename": "/data/shaders/frac_shader02_dmdi_radial.glsl", "start": 10522064, "end": 10529768}, {"filename": "/data/shaders/frac_shader02_octo.glsl", "start": 10529768, "end": 10536870}, {"filename": "/data/shaders/frac_shader02_octo_amp.glsl", "start": 10536870, "end": 10544776}, {"filename": "/data/shaders/frac_shader02_octo_strobe.glsl", "start": 10544776, "end": 10552945}, {"filename": "/data/shaders/frac_shader02_octo_strobe2.glsl", "start": 10552945, "end": 10561195}, {"filename": "/data/shaders/frac_shader02_prisim.glsl", "start": 10561195, "end": 10568521}, {"filename": "/data/shaders/frac_shader03_size.glsl", "start": 10568521, "end": 10575160}, {"filename": "/data/shaders/frac_shader03_worm4_amp.glsl", "start": 10575160, "end": 10584631}, {"filename": "/data/shaders/frac_shader03_wormhole.glsl", "start": 10584631, "end": 10592671}, {"filename": "/data/shaders/frac_shader03_wormhole2.glsl", "start": 10592671, "end": 10601020}, {"filename": "/data/shaders/frac_shader03_wormhole3.glsl", "start": 10601020, "end": 10608598}, {"filename": "/data/shaders/frac_shader03_wormhole4.glsl", "start": 10608598, "end": 10616932}, {"filename": "/data/shaders/frac_shader03_wormhole_amp.glsl", "start": 10616932, "end": 10626070}, {"filename": "/data/shaders/frac_shader04_echo.glsl", "start": 10626070, "end": 10631254}, {"filename": "/data/shaders/frac_shader04_echo2.glsl", "start": 10631254, "end": 10636610}, {"filename": "/data/shaders/frac_shader04_echo3_spin.glsl", "start": 10636610, "end": 10641890}, {"filename": "/data/shaders/frac_shader04_grid.glsl", "start": 10641890, "end": 10647146}, {"filename": "/data/shaders/frac_shader04_julia.glsl", "start": 10647146, "end": 10653006}, {"filename": "/data/shaders/frac_shader05.glsl", "start": 10653006, "end": 10655818}, {"filename": "/data/shaders/frac_shader_02_dmdXi2.glsl", "start": 10655818, "end": 10655818}, {"filename": "/data/shaders/frac_shader_diamond.glsl", "start": 10655818, "end": 10661291}, {"filename": "/data/shaders/frac_shader_dmd4_glass.glsl", "start": 10661291, "end": 10665546}, {"filename": "/data/shaders/frac_shader_echo4_spin_frac.glsl", "start": 10665546, "end": 10672979}, {"filename": "/data/shaders/frac_shader_echo4_spin_full.glsl", "start": 10672979, "end": 10678253}, {"filename": "/data/shaders/frac_shader_xor.glsl", "start": 10678253, "end": 10685028}, {"filename": "/data/shaders/frac_shader_xor2.glsl", "start": 10685028, "end": 10693038}, {"filename": "/data/shaders/frac_star1.glsl", "start": 10693038, "end": 10698150}, {"filename": "/data/shaders/frac_texture.glsl", "start": 10698150, "end": 10704301}, {"filename": "/data/shaders/frac_zoom1.glsl", "start": 10704301, "end": 10711760}, {"filename": "/data/shaders/frac_zoom2.glsl", "start": 10711760, "end": 10719603}, {"filename": "/data/shaders/frac_zoom3.glsl", "start": 10719603, "end": 10727454}, {"filename": "/data/shaders/frac_zoom4.glsl", "start": 10727454, "end": 10735337}, {"filename": "/data/shaders/frac_zoom5.glsl", "start": 10735337, "end": 10743333}, {"filename": "/data/shaders/frac_zoom6.glsl", "start": 10743333, "end": 10751517}, {"filename": "/data/shaders/frac_zoom7.glsl", "start": 10751517, "end": 10758410}, {"filename": "/data/shaders/frac_zoom8.glsl", "start": 10758410, "end": 10765371}, {"filename": "/data/shaders/fracal-gen.glsl", "start": 10765371, "end": 10766946}, {"filename": "/data/shaders/fracl_shader04_tri.glsl", "start": 10766946, "end": 10774701}, {"filename": "/data/shaders/fractal _noise.glsl", "start": 10774701, "end": 10774701}, {"filename": "/data/shaders/fractal-code-large-aurora-veil.glsl", "start": 10774701, "end": 10780819}, {"filename": "/data/shaders/fractal-code-large-chromatic-mist.glsl", "start": 10780819, "end": 10786940}, {"filename": "/data/shaders/fractal-code-large-cosmic-embroidery.glsl", "start": 10786940, "end": 10793065}, {"filename": "/data/shaders/fractal-code-large-crystal-drift.glsl", "start": 10793065, "end": 10799185}, {"filename": "/data/shaders/fractal-code-large-deepwater-fold.glsl", "start": 10799185, "end": 10805306}, {"filename": "/data/shaders/fractal-code-large-diamond-breeze.glsl", "start": 10805306, "end": 10811427}, {"filename": "/data/shaders/fractal-code-large-glass-bloom.glsl", "start": 10811427, "end": 10817545}, {"filename": "/data/shaders/fractal-code-large-hologram-cascade.glsl", "start": 10817545, "end": 10823668}, {"filename": "/data/shaders/fractal-code-large-ice-helix.glsl", "start": 10823668, "end": 10829784}, {"filename": "/data/shaders/fractal-code-large-liquid-cathedral.glsl", "start": 10829784, "end": 10835908}, {"filename": "/data/shaders/fractal-code-large-lunar-facets.glsl", "start": 10835908, "end": 10842027}, {"filename": "/data/shaders/fractal-code-large-mercury-fold.glsl", "start": 10842027, "end": 10848146}, {"filename": "/data/shaders/fractal-code-large-nebula-lens.glsl", "start": 10848146, "end": 10854264}, {"filename": "/data/shaders/fractal-code-large-ocean-mandala.glsl", "start": 10854264, "end": 10860385}, {"filename": "/data/shaders/fractal-code-large-opal-current.glsl", "start": 10860385, "end": 10866504}, {"filename": "/data/shaders/fractal-code-large-pearl-tunnel.glsl", "start": 10866504, "end": 10872623}, {"filename": "/data/shaders/fractal-code-large-prism-tide.glsl", "start": 10872623, "end": 10878740}, {"filename": "/data/shaders/fractal-code-large-quartz-orbit.glsl", "start": 10878740, "end": 10884859}, {"filename": "/data/shaders/fractal-code-large-rainbow-glass.glsl", "start": 10884859, "end": 10890979}, {"filename": "/data/shaders/fractal-code-large-silk-vortex.glsl", "start": 10890979, "end": 10897097}, {"filename": "/data/shaders/fractal-code-large-solar-petal.glsl", "start": 10897097, "end": 10903215}, {"filename": "/data/shaders/fractal-code-large-spectral-lattice.glsl", "start": 10903215, "end": 10909338}, {"filename": "/data/shaders/fractal-code-large-starlight-rosette.glsl", "start": 10909338, "end": 10915463}, {"filename": "/data/shaders/fractal-code-large-temporal-gem.glsl", "start": 10915463, "end": 10921583}, {"filename": "/data/shaders/fractal-code-large-velvet-ripple.glsl", "start": 10921583, "end": 10927703}, {"filename": "/data/shaders/fractal-fold-neon.glsl", "start": 10927703, "end": 10931643}, {"filename": "/data/shaders/fractal-fold-sketch.glsl", "start": 10931643, "end": 10935472}, {"filename": "/data/shaders/fractal-foldzx.glsl", "start": 10935472, "end": 10939244}, {"filename": "/data/shaders/fractal-gen.glsl", "start": 10939244, "end": 10943977}, {"filename": "/data/shaders/fractal.glsl", "start": 10943977, "end": 10946191}, {"filename": "/data/shaders/fractal2.glsl", "start": 10946191, "end": 10948470}, {"filename": "/data/shaders/fractal3.glsl", "start": 10948470, "end": 10950736}, {"filename": "/data/shaders/fractal4.glsl", "start": 10950736, "end": 10953408}, {"filename": "/data/shaders/fractal_audio.glsl", "start": 10953408, "end": 10962412}, {"filename": "/data/shaders/fractal_color_wave.glsl", "start": 10962412, "end": 10967255}, {"filename": "/data/shaders/fractal_diamond_rainbow.glsl", "start": 10967255, "end": 10974225}, {"filename": "/data/shaders/fractal_diamond_rainbow_blend.glsl", "start": 10974225, "end": 10980549}, {"filename": "/data/shaders/fractal_diamond_rainbow_no_tex.glsl", "start": 10980549, "end": 10986408}, {"filename": "/data/shaders/fractal_diamond_rainbow_overflow.glsl", "start": 10986408, "end": 10992482}, {"filename": "/data/shaders/fractal_diamond_rainbow_overflow_const.glsl", "start": 10992482, "end": 10998238}, {"filename": "/data/shaders/fractal_liquid_no_zero.glsl", "start": 10998238, "end": 11005760}, {"filename": "/data/shaders/fractal_mirror_aurora_mirror.glsl", "start": 11005760, "end": 11007315}, {"filename": "/data/shaders/fractal_mirror_bass_zoom.glsl", "start": 11007315, "end": 11008929}, {"filename": "/data/shaders/fractal_mirror_bloom_fold.glsl", "start": 11008929, "end": 11011029}, {"filename": "/data/shaders/fractal_mirror_bloom_mirror.glsl", "start": 11011029, "end": 11012833}, {"filename": "/data/shaders/fractal_mirror_cascade_fold.glsl", "start": 11012833, "end": 11014445}, {"filename": "/data/shaders/fractal_mirror_chromatic_fold.glsl", "start": 11014445, "end": 11016635}, {"filename": "/data/shaders/fractal_mirror_chromatic_wave.glsl", "start": 11016635, "end": 11017865}, {"filename": "/data/shaders/fractal_mirror_crystal_fold.glsl", "start": 11017865, "end": 11020017}, {"filename": "/data/shaders/fractal_mirror_depth_wave.glsl", "start": 11020017, "end": 11021665}, {"filename": "/data/shaders/fractal_mirror_diamond_reflect.glsl", "start": 11021665, "end": 11023684}, {"filename": "/data/shaders/fractal_mirror_diamond_wave.glsl", "start": 11023684, "end": 11025486}, {"filename": "/data/shaders/fractal_mirror_double_fold.glsl", "start": 11025486, "end": 11027398}, {"filename": "/data/shaders/fractal_mirror_dual_kaleidoscope.glsl", "start": 11027398, "end": 11029692}, {"filename": "/data/shaders/fractal_mirror_echo_fold.glsl", "start": 11029692, "end": 11031568}, {"filename": "/data/shaders/fractal_mirror_glitch_fractal.glsl", "start": 11031568, "end": 11033328}, {"filename": "/data/shaders/fractal_mirror_grid_fold.glsl", "start": 11033328, "end": 11034805}, {"filename": "/data/shaders/fractal_mirror_helix_mirror.glsl", "start": 11034805, "end": 11036159}, {"filename": "/data/shaders/fractal_mirror_hex_fold.glsl", "start": 11036159, "end": 11037962}, {"filename": "/data/shaders/fractal_mirror_infinite_reflect.glsl", "start": 11037962, "end": 11040031}, {"filename": "/data/shaders/fractal_mirror_julia_warp.glsl", "start": 11040031, "end": 11041494}, {"filename": "/data/shaders/fractal_mirror_kaleidoscope_pulse.glsl", "start": 11041494, "end": 11043817}, {"filename": "/data/shaders/fractal_mirror_logpolar_chromatic.glsl", "start": 11043817, "end": 11046359}, {"filename": "/data/shaders/fractal_mirror_mosaic_fold.glsl", "start": 11046359, "end": 11047721}, {"filename": "/data/shaders/fractal_mirror_nebula_reflect.glsl", "start": 11047721, "end": 11050069}, {"filename": "/data/shaders/fractal_mirror_neon_bulge.glsl", "start": 11050069, "end": 11052195}, {"filename": "/data/shaders/fractal_mirror_neon_ring.glsl", "start": 11052195, "end": 11054376}, {"filename": "/data/shaders/fractal_mirror_neon_stripe.glsl", "start": 11054376, "end": 11056241}, {"filename": "/data/shaders/fractal_mirror_petal_reflect.glsl", "start": 11056241, "end": 11057905}, {"filename": "/data/shaders/fractal_mirror_polar_reflect.glsl", "start": 11057905, "end": 11059181}, {"filename": "/data/shaders/fractal_mirror_prism_split.glsl", "start": 11059181, "end": 11060846}, {"filename": "/data/shaders/fractal_mirror_quad_fold.glsl", "start": 11060846, "end": 11062455}, {"filename": "/data/shaders/fractal_mirror_radial_pulse.glsl", "start": 11062455, "end": 11064110}, {"filename": "/data/shaders/fractal_mirror_ring_expand.glsl", "start": 11064110, "end": 11065920}, {"filename": "/data/shaders/fractal_mirror_ripple_fold.glsl", "start": 11065920, "end": 11067560}, {"filename": "/data/shaders/fractal_mirror_rotating_mirror.glsl", "start": 11067560, "end": 11069260}, {"filename": "/data/shaders/fractal_mirror_shatter_fold.glsl", "start": 11069260, "end": 11071138}, {"filename": "/data/shaders/fractal_mirror_sine_fold_pulse.glsl", "start": 11071138, "end": 11072476}, {"filename": "/data/shaders/fractal_mirror_spiral_flip.glsl", "start": 11072476, "end": 11074390}, {"filename": "/data/shaders/fractal_mirror_spiral_pulse.glsl", "start": 11074390, "end": 11076086}, {"filename": "/data/shaders/fractal_mirror_stripe_kaleidoscope.glsl", "start": 11076086, "end": 11077904}, {"filename": "/data/shaders/fractal_mirror_swirl_mirror.glsl", "start": 11077904, "end": 11079576}, {"filename": "/data/shaders/fractal_mirror_tri_fold.glsl", "start": 11079576, "end": 11081315}, {"filename": "/data/shaders/fractal_mirror_tunnel_reflect.glsl", "start": 11081315, "end": 11082635}, {"filename": "/data/shaders/fractal_mirror_vortex_mirror.glsl", "start": 11082635, "end": 11084160}, {"filename": "/data/shaders/fractal_mirror_warp_glitch.glsl", "start": 11084160, "end": 11085840}, {"filename": "/data/shaders/fractal_mirror_wave_fold.glsl", "start": 11085840, "end": 11086879}, {"filename": "/data/shaders/fractal_mirror_zoom_pulse.glsl", "start": 11086879, "end": 11088680}, {"filename": "/data/shaders/fractal_no_zero.glsl", "start": 11088680, "end": 11095976}, {"filename": "/data/shaders/fractal_noise.glsl", "start": 11095976, "end": 11104036}, {"filename": "/data/shaders/fractal_recursion.glsl", "start": 11104036, "end": 11106715}, {"filename": "/data/shaders/fractal_texture_large-nowrap.glsl", "start": 11106715, "end": 11112656}, {"filename": "/data/shaders/fractal_texture_large.glsl", "start": 11112656, "end": 11118657}, {"filename": "/data/shaders/fractal_texture_large_audio.glsl", "start": 11118657, "end": 11124845}, {"filename": "/data/shaders/fractal_texture_large_spectrum.glsl", "start": 11124845, "end": 11131473}, {"filename": "/data/shaders/fractal_texture_no_wrap.glsl", "start": 11131473, "end": 11137497}, {"filename": "/data/shaders/fractal_vhs.glsl", "start": 11137497, "end": 11140327}, {"filename": "/data/shaders/fractal_zoom_e.glsl", "start": 11140327, "end": 11143133}, {"filename": "/data/shaders/funny_mirror.glsl", "start": 11143133, "end": 11143544}, {"filename": "/data/shaders/funnymirror2.glsl", "start": 11143544, "end": 11143982}, {"filename": "/data/shaders/g5.glsl", "start": 11143982, "end": 11145571}, {"filename": "/data/shaders/g_drum.glsl", "start": 11145571, "end": 11146302}, {"filename": "/data/shaders/g_drum_by_mouse.glsl", "start": 11146302, "end": 11146867}, {"filename": "/data/shaders/g_swirl.glsl", "start": 11146867, "end": 11147846}, {"filename": "/data/shaders/g_ufo.glsl", "start": 11147846, "end": 11148551}, {"filename": "/data/shaders/g_ufo_3d.glsl", "start": 11148551, "end": 11149620}, {"filename": "/data/shaders/g_ufo_3d_v2.glsl", "start": 11149620, "end": 11151091}, {"filename": "/data/shaders/g_ufo_movement.glsl", "start": 11151091, "end": 11153005}, {"filename": "/data/shaders/game_amber_mono.glsl", "start": 11153005, "end": 11153446}, {"filename": "/data/shaders/game_anamorphic.glsl", "start": 11153446, "end": 11154158}, {"filename": "/data/shaders/game_anime_cel.glsl", "start": 11154158, "end": 11154953}, {"filename": "/data/shaders/game_ant_aurora_tunnel.glsl", "start": 11154953, "end": 11155506}, {"filename": "/data/shaders/game_ant_chrome_wave.glsl", "start": 11155506, "end": 11156085}, {"filename": "/data/shaders/game_ant_cosmic_web.glsl", "start": 11156085, "end": 11156754}, {"filename": "/data/shaders/game_ant_crystal_pulse.glsl", "start": 11156754, "end": 11157279}, {"filename": "/data/shaders/game_ant_deep_bloom.glsl", "start": 11157279, "end": 11157879}, {"filename": "/data/shaders/game_ant_diamond_storm.glsl", "start": 11157879, "end": 11158490}, {"filename": "/data/shaders/game_ant_fire_spoke.glsl", "start": 11158490, "end": 11159000}, {"filename": "/data/shaders/game_ant_frac_cosine.glsl", "start": 11159000, "end": 11159448}, {"filename": "/data/shaders/game_ant_frac_neon.glsl", "start": 11159448, "end": 11160244}, {"filename": "/data/shaders/game_ant_frac_smooth.glsl", "start": 11160244, "end": 11160733}, {"filename": "/data/shaders/game_ant_fractal_ocean.glsl", "start": 11160733, "end": 11161233}, {"filename": "/data/shaders/game_ant_gem_bump.glsl", "start": 11161233, "end": 11161974}, {"filename": "/data/shaders/game_ant_gem_glass.glsl", "start": 11161974, "end": 11162556}, {"filename": "/data/shaders/game_ant_gem_kaleido.glsl", "start": 11162556, "end": 11163339}, {"filename": "/data/shaders/game_ant_gem_pencil.glsl", "start": 11163339, "end": 11163945}, {"filename": "/data/shaders/game_ant_gem_polar.glsl", "start": 11163945, "end": 11164538}, {"filename": "/data/shaders/game_ant_gem_rainbow.glsl", "start": 11164538, "end": 11165060}, {"filename": "/data/shaders/game_ant_gem_spectrum.glsl", "start": 11165060, "end": 11165467}, {"filename": "/data/shaders/game_ant_gem_spider.glsl", "start": 11165467, "end": 11166024}, {"filename": "/data/shaders/game_ant_glass_mandala.glsl", "start": 11166024, "end": 11166634}, {"filename": "/data/shaders/game_ant_hypno_lens.glsl", "start": 11166634, "end": 11167192}, {"filename": "/data/shaders/game_ant_ice_ripple.glsl", "start": 11167192, "end": 11167728}, {"filename": "/data/shaders/game_ant_liquid_mirror.glsl", "start": 11167728, "end": 11168276}, {"filename": "/data/shaders/game_ant_mercury_bloom.glsl", "start": 11168276, "end": 11168940}, {"filename": "/data/shaders/game_ant_metal_aurora.glsl", "start": 11168940, "end": 11169470}, {"filename": "/data/shaders/game_ant_metal_cascade.glsl", "start": 11169470, "end": 11169915}, {"filename": "/data/shaders/game_ant_metal_chrome.glsl", "start": 11169915, "end": 11170426}, {"filename": "/data/shaders/game_ant_metal_coil.glsl", "start": 11170426, "end": 11170917}, {"filename": "/data/shaders/game_ant_metal_crystal.glsl", "start": 11170917, "end": 11171554}, {"filename": "/data/shaders/game_ant_metal_ember.glsl", "start": 11171554, "end": 11172141}, {"filename": "/data/shaders/game_ant_metal_flux.glsl", "start": 11172141, "end": 11172561}, {"filename": "/data/shaders/game_ant_metal_forge.glsl", "start": 11172561, "end": 11173090}, {"filename": "/data/shaders/game_ant_metal_fracture.glsl", "start": 11173090, "end": 11173835}, {"filename": "/data/shaders/game_ant_metal_glacier.glsl", "start": 11173835, "end": 11174385}, {"filename": "/data/shaders/game_ant_metal_helix.glsl", "start": 11174385, "end": 11174966}, {"filename": "/data/shaders/game_ant_metal_inferno.glsl", "start": 11174966, "end": 11175407}, {"filename": "/data/shaders/game_ant_metal_lattice.glsl", "start": 11175407, "end": 11175885}, {"filename": "/data/shaders/game_ant_metal_nebula.glsl", "start": 11175885, "end": 11176352}, {"filename": "/data/shaders/game_ant_metal_opal.glsl", "start": 11176352, "end": 11176904}, {"filename": "/data/shaders/game_ant_metal_orbital.glsl", "start": 11176904, "end": 11177472}, {"filename": "/data/shaders/game_ant_metal_prism.glsl", "start": 11177472, "end": 11177977}, {"filename": "/data/shaders/game_ant_metal_pulse.glsl", "start": 11177977, "end": 11178435}, {"filename": "/data/shaders/game_ant_metal_ripple.glsl", "start": 11178435, "end": 11178961}, {"filename": "/data/shaders/game_ant_metal_shard.glsl", "start": 11178961, "end": 11179555}, {"filename": "/data/shaders/game_ant_metal_storm.glsl", "start": 11179555, "end": 11180129}, {"filename": "/data/shaders/game_ant_metal_tessera.glsl", "start": 11180129, "end": 11180535}, {"filename": "/data/shaders/game_ant_metal_vortex.glsl", "start": 11180535, "end": 11181138}, {"filename": "/data/shaders/game_ant_metal_weave.glsl", "start": 11181138, "end": 11181642}, {"filename": "/data/shaders/game_ant_molten_web.glsl", "start": 11181642, "end": 11182579}, {"filename": "/data/shaders/game_ant_nebula_fold.glsl", "start": 11182579, "end": 11183048}, {"filename": "/data/shaders/game_arcade_marquee.glsl", "start": 11183048, "end": 11183761}, {"filename": "/data/shaders/game_arcane.glsl", "start": 11183761, "end": 11184267}, {"filename": "/data/shaders/game_ascii_art.glsl", "start": 11184267, "end": 11185524}, {"filename": "/data/shaders/game_bleach_bypass.glsl", "start": 11185524, "end": 11185951}, {"filename": "/data/shaders/game_boss_warning.glsl", "start": 11185951, "end": 11186547}, {"filename": "/data/shaders/game_bullet_time.glsl", "start": 11186547, "end": 11187237}, {"filename": "/data/shaders/game_cel_outline.glsl", "start": 11187237, "end": 11188321}, {"filename": "/data/shaders/game_cga.glsl", "start": 11188321, "end": 11188907}, {"filename": "/data/shaders/game_chromatic_edges.glsl", "start": 11188907, "end": 11189401}, {"filename": "/data/shaders/game_cinematic_cool.glsl", "start": 11189401, "end": 11189844}, {"filename": "/data/shaders/game_cinematic_warm.glsl", "start": 11189844, "end": 11190366}, {"filename": "/data/shaders/game_codex_arcane_runes.glsl", "start": 11190366, "end": 11191074}, {"filename": "/data/shaders/game_codex_boss_aura.glsl", "start": 11191074, "end": 11191734}, {"filename": "/data/shaders/game_codex_bullet_time_focus.glsl", "start": 11191734, "end": 11192436}, {"filename": "/data/shaders/game_codex_critical_hit.glsl", "start": 11192436, "end": 11193121}, {"filename": "/data/shaders/game_codex_freeze_frame.glsl", "start": 11193121, "end": 11193745}, {"filename": "/data/shaders/game_codex_lava_damage.glsl", "start": 11193745, "end": 11194379}, {"filename": "/data/shaders/game_codex_low_ammo_warning.glsl", "start": 11194379, "end": 11195031}, {"filename": "/data/shaders/game_codex_magic_barrier.glsl", "start": 11195031, "end": 11195752}, {"filename": "/data/shaders/game_codex_pixel_pickup.glsl", "start": 11195752, "end": 11196464}, {"filename": "/data/shaders/game_codex_portal_rift.glsl", "start": 11196464, "end": 11197262}, {"filename": "/data/shaders/game_codex_powerup_glow.glsl", "start": 11197262, "end": 11197995}, {"filename": "/data/shaders/game_codex_radar_ping.glsl", "start": 11197995, "end": 11198697}, {"filename": "/data/shaders/game_codex_rage_meter.glsl", "start": 11198697, "end": 11199370}, {"filename": "/data/shaders/game_codex_shadow_realm.glsl", "start": 11199370, "end": 11200016}, {"filename": "/data/shaders/game_codex_shield_hit.glsl", "start": 11200016, "end": 11200711}, {"filename": "/data/shaders/game_codex_speed_boost.glsl", "start": 11200711, "end": 11201399}, {"filename": "/data/shaders/game_codex_stealth_cloak.glsl", "start": 11201399, "end": 11202081}, {"filename": "/data/shaders/game_codex_target_lock.glsl", "start": 11202081, "end": 11202945}, {"filename": "/data/shaders/game_codex_toxic_cloud.glsl", "start": 11202945, "end": 11203814}, {"filename": "/data/shaders/game_codex_underwater_depth.glsl", "start": 11203814, "end": 11204498}, {"filename": "/data/shaders/game_cross_process.glsl", "start": 11204498, "end": 11204987}, {"filename": "/data/shaders/game_crt_curve.glsl", "start": 11204987, "end": 11205811}, {"filename": "/data/shaders/game_cyberpunk_neon.glsl", "start": 11205811, "end": 11206548}, {"filename": "/data/shaders/game_damage_pulse.glsl", "start": 11206548, "end": 11207040}, {"filename": "/data/shaders/game_desert_heat.glsl", "start": 11207040, "end": 11207506}, {"filename": "/data/shaders/game_disco_floor.glsl", "start": 11207506, "end": 11208216}, {"filename": "/data/shaders/game_dither_bayer.glsl", "start": 11208216, "end": 11208896}, {"filename": "/data/shaders/game_dream_glow.glsl", "start": 11208896, "end": 11209604}, {"filename": "/data/shaders/game_drunk_wobble.glsl", "start": 11209604, "end": 11209989}, {"filename": "/data/shaders/game_dust_motes.glsl", "start": 11209989, "end": 11210749}, {"filename": "/data/shaders/game_emp_blast.glsl", "start": 11210749, "end": 11211352}, {"filename": "/data/shaders/game_fake_ssao.glsl", "start": 11211352, "end": 11212038}, {"filename": "/data/shaders/game_film_grain.glsl", "start": 11212038, "end": 11212554}, {"filename": "/data/shaders/game_fog.glsl", "start": 11212554, "end": 11213035}, {"filename": "/data/shaders/game_freeze_crystal.glsl", "start": 11213035, "end": 11213866}, {"filename": "/data/shaders/game_frostbite.glsl", "start": 11213866, "end": 11214490}, {"filename": "/data/shaders/game_gameboy_dmg.glsl", "start": 11214490, "end": 11215122}, {"filename": "/data/shaders/game_gba_tint.glsl", "start": 11215122, "end": 11215613}, {"filename": "/data/shaders/game_genesis.glsl", "start": 11215613, "end": 11215955}, {"filename": "/data/shaders/game_ghost_trail.glsl", "start": 11215955, "end": 11216621}, {"filename": "/data/shaders/game_green_mono.glsl", "start": 11216621, "end": 11217051}, {"filename": "/data/shaders/game_hacker_terminal.glsl", "start": 11217051, "end": 11217735}, {"filename": "/data/shaders/game_halftone.glsl", "start": 11217735, "end": 11218240}, {"filename": "/data/shaders/game_hdr_punch.glsl", "start": 11218240, "end": 11218737}, {"filename": "/data/shaders/game_heart_beat.glsl", "start": 11218737, "end": 11219423}, {"filename": "/data/shaders/game_hologram.glsl", "start": 11219423, "end": 11220187}, {"filename": "/data/shaders/game_kaleido_combat.glsl", "start": 11220187, "end": 11220665}, {"filename": "/data/shaders/game_lava_world.glsl", "start": 11220665, "end": 11221322}, {"filename": "/data/shaders/game_lcd_grid.glsl", "start": 11221322, "end": 11221872}, {"filename": "/data/shaders/game_letterbox.glsl", "start": 11221872, "end": 11222420}, {"filename": "/data/shaders/game_low_battery.glsl", "start": 11222420, "end": 11223002}, {"filename": "/data/shaders/game_matrix_rain.glsl", "start": 11223002, "end": 11224022}, {"filename": "/data/shaders/game_minimap_glow.glsl", "start": 11224022, "end": 11224706}, {"filename": "/data/shaders/game_motion_blur.glsl", "start": 11224706, "end": 11225222}, {"filename": "/data/shaders/game_mushroom_trip.glsl", "start": 11225222, "end": 11226177}, {"filename": "/data/shaders/game_nes_palette.glsl", "start": 11226177, "end": 11226486}, {"filename": "/data/shaders/game_night_vision.glsl", "start": 11226486, "end": 11227238}, {"filename": "/data/shaders/game_noir.glsl", "start": 11227238, "end": 11227673}, {"filename": "/data/shaders/game_oil_paint.glsl", "start": 11227673, "end": 11228712}, {"filename": "/data/shaders/game_pencil_lines.glsl", "start": 11228712, "end": 11229400}, {"filename": "/data/shaders/game_phosphor.glsl", "start": 11229400, "end": 11230071}, {"filename": "/data/shaders/game_pixelate_4x.glsl", "start": 11230071, "end": 11230410}, {"filename": "/data/shaders/game_pixelsort_glitch.glsl", "start": 11230410, "end": 11231113}, {"filename": "/data/shaders/game_polaroid.glsl", "start": 11231113, "end": 11231590}, {"filename": "/data/shaders/game_psx_dither.glsl", "start": 11231590, "end": 11232162}, {"filename": "/data/shaders/game_radial_focus.glsl", "start": 11232162, "end": 11232844}, {"filename": "/data/shaders/game_radio_static.glsl", "start": 11232844, "end": 11233453}, {"filename": "/data/shaders/game_rage_mode.glsl", "start": 11233453, "end": 11234149}, {"filename": "/data/shaders/game_rain.glsl", "start": 11234149, "end": 11234907}, {"filename": "/data/shaders/game_scanlines_hd.glsl", "start": 11234907, "end": 11235271}, {"filename": "/data/shaders/game_sepia_warm.glsl", "start": 11235271, "end": 11235735}, {"filename": "/data/shaders/game_sharpen.glsl", "start": 11235735, "end": 11236372}, {"filename": "/data/shaders/game_snow.glsl", "start": 11236372, "end": 11237346}, {"filename": "/data/shaders/game_speedlines.glsl", "start": 11237346, "end": 11238016}, {"filename": "/data/shaders/game_subtle_bloom.glsl", "start": 11238016, "end": 11238804}, {"filename": "/data/shaders/game_super_saiyan.glsl", "start": 11238804, "end": 11239594}, {"filename": "/data/shaders/game_technicolor.glsl", "start": 11239594, "end": 11240008}, {"filename": "/data/shaders/game_thermal.glsl", "start": 11240008, "end": 11240781}, {"filename": "/data/shaders/game_toxic.glsl", "start": 11240781, "end": 11241305}, {"filename": "/data/shaders/game_underwater.glsl", "start": 11241305, "end": 11241971}, {"filename": "/data/shaders/game_vignette_breath.glsl", "start": 11241971, "end": 11242424}, {"filename": "/data/shaders/game_void_warp.glsl", "start": 11242424, "end": 11243064}, {"filename": "/data/shaders/gaura.glsl", "start": 11243064, "end": 11244336}, {"filename": "/data/shaders/gblur.glsl", "start": 11244336, "end": 11245957}, {"filename": "/data/shaders/gboil.glsl", "start": 11245957, "end": 11246807}, {"filename": "/data/shaders/gem-af.glsl", "start": 11246807, "end": 11248250}, {"filename": "/data/shaders/gem-aura.glsl", "start": 11248250, "end": 11252619}, {"filename": "/data/shaders/gem-color-spiral.glsl", "start": 11252619, "end": 11255127}, {"filename": "/data/shaders/gem-color-spsiral.glsl", "start": 11255127, "end": 11257635}, {"filename": "/data/shaders/gem-deep.glsl", "start": 11257635, "end": 11259399}, {"filename": "/data/shaders/gem-fish.glsl", "start": 11259399, "end": 11261137}, {"filename": "/data/shaders/gem-hue-frac-spectrum16.glsl", "start": 11261137, "end": 11264616}, {"filename": "/data/shaders/gem-hue-frac.glsl", "start": 11264616, "end": 11266545}, {"filename": "/data/shaders/gem-image.glsl", "start": 11266545, "end": 11268111}, {"filename": "/data/shaders/gem-light-frac.glsl", "start": 11268111, "end": 11269989}, {"filename": "/data/shaders/gem-pong.glsl", "start": 11269989, "end": 11272313}, {"filename": "/data/shaders/gem-rainbow-metal3-slider.glsl", "start": 11272313, "end": 11275526}, {"filename": "/data/shaders/gem-rainbow-metal3-spectrum.glsl", "start": 11275526, "end": 11283121}, {"filename": "/data/shaders/gem-rainbow-metal3.glsl", "start": 11283121, "end": 11285391}, {"filename": "/data/shaders/gem-ripple.glsl", "start": 11285391, "end": 11287156}, {"filename": "/data/shaders/gem-spiral-cont-ts.glsl", "start": 11287156, "end": 11288467}, {"filename": "/data/shaders/gem-spiral-cont.glsl", "start": 11288467, "end": 11289857}, {"filename": "/data/shaders/gem-spiral-cont2.glsl", "start": 11289857, "end": 11291536}, {"filename": "/data/shaders/gem-spiral-frac.glsl", "start": 11291536, "end": 11294064}, {"filename": "/data/shaders/gem-spiral-full-audio.glsl", "start": 11294064, "end": 11297717}, {"filename": "/data/shaders/gem-spiral-full.glsl", "start": 11297717, "end": 11300176}, {"filename": "/data/shaders/gem-spoke.glsl", "start": 11300176, "end": 11301789}, {"filename": "/data/shaders/gem_bump.glsl", "start": 11301789, "end": 11303935}, {"filename": "/data/shaders/gem_frac.glsl", "start": 11303935, "end": 11305236}, {"filename": "/data/shaders/gem_frac_spir.glsl", "start": 11305236, "end": 11308916}, {"filename": "/data/shaders/gem_fractal_bump.glsl", "start": 11308916, "end": 11313600}, {"filename": "/data/shaders/gem_glass.glsl", "start": 11313600, "end": 11315591}, {"filename": "/data/shaders/gem_kale_large.glsl", "start": 11315591, "end": 11319977}, {"filename": "/data/shaders/gem_metal.glsl", "start": 11319977, "end": 11321781}, {"filename": "/data/shaders/gem_p.glsl", "start": 11321781, "end": 11324044}, {"filename": "/data/shaders/gem_pencil_sketch.glsl", "start": 11324044, "end": 11326187}, {"filename": "/data/shaders/gem_pencil_sketch2.glsl", "start": 11326187, "end": 11328190}, {"filename": "/data/shaders/gem_plas.glsl", "start": 11328190, "end": 11329767}, {"filename": "/data/shaders/gem_polar.glsl", "start": 11329767, "end": 11331335}, {"filename": "/data/shaders/gem_rainbow_metal.glsl", "start": 11331335, "end": 11333725}, {"filename": "/data/shaders/gem_rainbow_metal_audio.glsl", "start": 11333725, "end": 11339272}, {"filename": "/data/shaders/gem_rainbow_metal_movement.glsl", "start": 11339272, "end": 11341674}, {"filename": "/data/shaders/gem_rainbow_spectrum.glsl", "start": 11341674, "end": 11343819}, {"filename": "/data/shaders/gem_spectrum_extreme.glsl", "start": 11343819, "end": 11345486}, {"filename": "/data/shaders/gem_spectrum_test.glsl", "start": 11345486, "end": 11347201}, {"filename": "/data/shaders/gem_spiderweb.glsl", "start": 11347201, "end": 11349944}, {"filename": "/data/shaders/gem_spiderweb_tunnel.glsl", "start": 11349944, "end": 11352426}, {"filename": "/data/shaders/gem_txt.glsl", "start": 11352426, "end": 11354576}, {"filename": "/data/shaders/genergy.glsl", "start": 11354576, "end": 11355684}, {"filename": "/data/shaders/geo-pi.glsl", "start": 11355684, "end": 11356790}, {"filename": "/data/shaders/geometric.glsl", "start": 11356790, "end": 11357614}, {"filename": "/data/shaders/geometric2.glsl", "start": 11357614, "end": 11358493}, {"filename": "/data/shaders/geometric3.glsl", "start": 11358493, "end": 11359382}, {"filename": "/data/shaders/geometric4.glsl", "start": 11359382, "end": 11360092}, {"filename": "/data/shaders/geometric5.glsl", "start": 11360092, "end": 11360993}, {"filename": "/data/shaders/gfs.glsl", "start": 11360993, "end": 11362063}, {"filename": "/data/shaders/gghost.glsl", "start": 11362063, "end": 11362731}, {"filename": "/data/shaders/ggrad.glsl", "start": 11362731, "end": 11363300}, {"filename": "/data/shaders/ghost_echo_cache.glsl", "start": 11363300, "end": 11365262}, {"filename": "/data/shaders/ghost_fracture_cache.glsl", "start": 11365262, "end": 11368672}, {"filename": "/data/shaders/gkale.glsl", "start": 11368672, "end": 11369887}, {"filename": "/data/shaders/gkale_echo.glsl", "start": 11369887, "end": 11372131}, {"filename": "/data/shaders/gkale_echo2.glsl", "start": 11372131, "end": 11376242}, {"filename": "/data/shaders/gkalei.glsl", "start": 11376242, "end": 11378045}, {"filename": "/data/shaders/glass_PI.glsl", "start": 11378045, "end": 11378928}, {"filename": "/data/shaders/glass_mouse.glsl", "start": 11378928, "end": 11379583}, {"filename": "/data/shaders/glass_mouse_rad.glsl", "start": 11379583, "end": 11380271}, {"filename": "/data/shaders/glitch-no-noise-mouse.glsl", "start": 11380271, "end": 11383278}, {"filename": "/data/shaders/glitch-no-noise.glsl", "start": 11383278, "end": 11385853}, {"filename": "/data/shaders/glitch-noise.glsl", "start": 11385853, "end": 11387045}, {"filename": "/data/shaders/glitch-react-color.glsl", "start": 11387045, "end": 11387977}, {"filename": "/data/shaders/glitch-react.glsl", "start": 11387977, "end": 11388647}, {"filename": "/data/shaders/glitch-zoom.glsl", "start": 11388647, "end": 11391438}, {"filename": "/data/shaders/glitch1.glsl", "start": 11391438, "end": 11392572}, {"filename": "/data/shaders/glitch_boil.glsl", "start": 11392572, "end": 11393972}, {"filename": "/data/shaders/glitch_boil2.glsl", "start": 11393972, "end": 11394691}, {"filename": "/data/shaders/glitch_effect.glsl", "start": 11394691, "end": 11395733}, {"filename": "/data/shaders/glitch_jump.glsl", "start": 11395733, "end": 11396063}, {"filename": "/data/shaders/glitch_light.glsl", "start": 11396063, "end": 11396918}, {"filename": "/data/shaders/glitch_rainbow.glsl", "start": 11396918, "end": 11398101}, {"filename": "/data/shaders/glitch_wave.glsl", "start": 11398101, "end": 11398718}, {"filename": "/data/shaders/glitchf.glsl", "start": 11398718, "end": 11399764}, {"filename": "/data/shaders/glitchy-rainbow.glsl", "start": 11399764, "end": 11401047}, {"filename": "/data/shaders/glitchy-squish.glsl", "start": 11401047, "end": 11401775}, {"filename": "/data/shaders/glitchy.glsl", "start": 11401775, "end": 11402718}, {"filename": "/data/shaders/glow.glsl", "start": 11402718, "end": 11404451}, {"filename": "/data/shaders/gltichtest.glsl", "start": 11404451, "end": 11406097}, {"filename": "/data/shaders/gmir.glsl", "start": 11406097, "end": 11409732}, {"filename": "/data/shaders/gmir2.glsl", "start": 11409732, "end": 11410585}, {"filename": "/data/shaders/goo.glsl", "start": 11410585, "end": 11411183}, {"filename": "/data/shaders/gpt_echo.glsl", "start": 11411183, "end": 11411607}, {"filename": "/data/shaders/gpt_halluc.glsl", "start": 11411607, "end": 11417746}, {"filename": "/data/shaders/gpt_trip.glsl", "start": 11417746, "end": 11421883}, {"filename": "/data/shaders/gpt_trip2.glsl", "start": 11421883, "end": 11428287}, {"filename": "/data/shaders/gptsmooth.glsl", "start": 11428287, "end": 11430505}, {"filename": "/data/shaders/gptsmooth2.glsl", "start": 11430505, "end": 11432350}, {"filename": "/data/shaders/gptswirl.glsl", "start": 11432350, "end": 11433066}, {"filename": "/data/shaders/gptswirl2.glsl", "start": 11433066, "end": 11434050}, {"filename": "/data/shaders/grad.glsl", "start": 11434050, "end": 11435785}, {"filename": "/data/shaders/grad2.glsl", "start": 11435785, "end": 11437061}, {"filename": "/data/shaders/grad3.glsl", "start": 11437061, "end": 11438311}, {"filename": "/data/shaders/grad_color.glsl", "start": 11438311, "end": 11438873}, {"filename": "/data/shaders/gradient.glsl", "start": 11438873, "end": 11439842}, {"filename": "/data/shaders/gradient_color.glsl", "start": 11439842, "end": 11440671}, {"filename": "/data/shaders/gradient_position.glsl", "start": 11440671, "end": 11441708}, {"filename": "/data/shaders/gradient_shift.glsl", "start": 11441708, "end": 11443099}, {"filename": "/data/shaders/grainbow.glsl", "start": 11443099, "end": 11444153}, {"filename": "/data/shaders/grayscale.glsl", "start": 11444153, "end": 11444437}, {"filename": "/data/shaders/green.glsl", "start": 11444437, "end": 11445445}, {"filename": "/data/shaders/green_aura.glsl", "start": 11445445, "end": 11446591}, {"filename": "/data/shaders/green_echo.glsl", "start": 11446591, "end": 11447292}, {"filename": "/data/shaders/green_gradient.glsl", "start": 11447292, "end": 11448315}, {"filename": "/data/shaders/green_gradient_strobe.glsl", "start": 11448315, "end": 11449494}, {"filename": "/data/shaders/green_move.glsl", "start": 11449494, "end": 11450148}, {"filename": "/data/shaders/green_strobe.glsl", "start": 11450148, "end": 11451195}, {"filename": "/data/shaders/greenblue.glsl", "start": 11451195, "end": 11452192}, {"filename": "/data/shaders/grid-spiral.glsl", "start": 11452192, "end": 11453367}, {"filename": "/data/shaders/grid_by_mouse.glsl", "start": 11453367, "end": 11454325}, {"filename": "/data/shaders/grid_pattern.glsl", "start": 11454325, "end": 11454750}, {"filename": "/data/shaders/grid_rand_strobe.glsl", "start": 11454750, "end": 11456061}, {"filename": "/data/shaders/grid_sin.glsl", "start": 11456061, "end": 11457078}, {"filename": "/data/shaders/grid_strobe.glsl", "start": 11457078, "end": 11458359}, {"filename": "/data/shaders/gridcolor_gradient.glsl", "start": 11458359, "end": 11459494}, {"filename": "/data/shaders/gsupercool.glsl", "start": 11459494, "end": 11461498}, {"filename": "/data/shaders/gsupercool2.glsl", "start": 11461498, "end": 11463313}, {"filename": "/data/shaders/gt_rotate.glsl", "start": 11463313, "end": 11464029}, {"filename": "/data/shaders/gtrail.glsl", "start": 11464029, "end": 11465007}, {"filename": "/data/shaders/gtrail2.glsl", "start": 11465007, "end": 11466169}, {"filename": "/data/shaders/gvortex.glsl", "start": 11466169, "end": 11467178}, {"filename": "/data/shaders/gxor.glsl", "start": 11467178, "end": 11467842}, {"filename": "/data/shaders/gxor2.glsl", "start": 11467842, "end": 11468729}, {"filename": "/data/shaders/hallu_code_abyssal_opaline.glsl", "start": 11468729, "end": 11475144}, {"filename": "/data/shaders/hallu_code_vesper_xenolith.glsl", "start": 11475144, "end": 11481919}, {"filename": "/data/shaders/halluc_acid_fire.glsl", "start": 11481919, "end": 11486015}, {"filename": "/data/shaders/halluc_audio.glsl", "start": 11486015, "end": 11493211}, {"filename": "/data/shaders/halluc_ember_storm.glsl", "start": 11493211, "end": 11497204}, {"filename": "/data/shaders/halluc_fog_tunnel.glsl", "start": 11497204, "end": 11501355}, {"filename": "/data/shaders/halluc_gem.glsl", "start": 11501355, "end": 11506438}, {"filename": "/data/shaders/halluc_gem2.glsl", "start": 11506438, "end": 11511854}, {"filename": "/data/shaders/halluc_ink_bloom.glsl", "start": 11511854, "end": 11515895}, {"filename": "/data/shaders/halluc_lava_crack.glsl", "start": 11515895, "end": 11519979}, {"filename": "/data/shaders/halluc_liquid.glsl", "start": 11519979, "end": 11525539}, {"filename": "/data/shaders/halluc_liquid_fractal.glsl", "start": 11525539, "end": 11529592}, {"filename": "/data/shaders/halluc_mercury_pop.glsl", "start": 11529592, "end": 11533048}, {"filename": "/data/shaders/halluc_oil_slick.glsl", "start": 11533048, "end": 11536888}, {"filename": "/data/shaders/halluc_plasma_flame.glsl", "start": 11536888, "end": 11540786}, {"filename": "/data/shaders/halluc_pop.glsl", "start": 11540786, "end": 11544838}, {"filename": "/data/shaders/halluc_pop2.glsl", "start": 11544838, "end": 11548276}, {"filename": "/data/shaders/halluc_smoke_curl.glsl", "start": 11548276, "end": 11552362}, {"filename": "/data/shaders/heartthrob.glsl", "start": 11552362, "end": 11553588}, {"filename": "/data/shaders/heartthrob2.glsl", "start": 11553588, "end": 11554264}, {"filename": "/data/shaders/heat-wave.glsl", "start": 11554264, "end": 11554863}, {"filename": "/data/shaders/heat.glsl", "start": 11554863, "end": 11555359}, {"filename": "/data/shaders/hue-mouse.glsl", "start": 11555359, "end": 11557610}, {"filename": "/data/shaders/huei_af2.glsl", "start": 11557610, "end": 11561097}, {"filename": "/data/shaders/huri.glsl", "start": 11561097, "end": 11561647}, {"filename": "/data/shaders/huri1.glsl", "start": 11561647, "end": 11562419}, {"filename": "/data/shaders/huri2.glsl", "start": 11562419, "end": 11563260}, {"filename": "/data/shaders/huri3.glsl", "start": 11563260, "end": 11564104}, {"filename": "/data/shaders/huri_af.glsl", "start": 11564104, "end": 11566978}, {"filename": "/data/shaders/huri_create_mouse.glsl", "start": 11566978, "end": 11569250}, {"filename": "/data/shaders/hurixyz.glsl", "start": 11569250, "end": 11570866}, {"filename": "/data/shaders/huriz.glsl", "start": 11570866, "end": 11571660}, {"filename": "/data/shaders/ice.glsl", "start": 11571660, "end": 11572762}, {"filename": "/data/shaders/index.bak", "start": 11572762, "end": 11604329}, {"filename": "/data/shaders/index.old.txt", "start": 11604329, "end": 11635304}, {"filename": "/data/shaders/index.txt", "start": 11635304, "end": 11686444}, {"filename": "/data/shaders/index.txt.bak", "start": 11686444, "end": 11726307}, {"filename": "/data/shaders/index.txt.bak2", "start": 11726307, "end": 11756790}, {"filename": "/data/shaders/index.txt.bak3", "start": 11756790, "end": 11787765}, {"filename": "/data/shaders/index_purple.glsl", "start": 11787765, "end": 11788527}, {"filename": "/data/shaders/infinite_fractal_tunnel.glsl", "start": 11788527, "end": 11792056}, {"filename": "/data/shaders/inflate-ripple.glsl", "start": 11792056, "end": 11792556}, {"filename": "/data/shaders/inflate.glsl", "start": 11792556, "end": 11792859}, {"filename": "/data/shaders/inthesky.glsl", "start": 11792859, "end": 11794791}, {"filename": "/data/shaders/inthesky_cache_astral_tessellation.glsl", "start": 11794791, "end": 11803090}, {"filename": "/data/shaders/inthesky_cache_aurora_rupture.glsl", "start": 11803090, "end": 11810856}, {"filename": "/data/shaders/inthesky_cache_celestial_cathedral.glsl", "start": 11810856, "end": 11818514}, {"filename": "/data/shaders/inthesky_cache_fractal_firmament.glsl", "start": 11818514, "end": 11826276}, {"filename": "/data/shaders/inthesky_cache_horizon_implosion.glsl", "start": 11826276, "end": 11834061}, {"filename": "/data/shaders/inthesky_cache_nebula_mirrors.glsl", "start": 11834061, "end": 11841852}, {"filename": "/data/shaders/inthesky_cache_prismatic_thunderhead.glsl", "start": 11841852, "end": 11849654}, {"filename": "/data/shaders/inthesky_cache_solar_pillar_array.glsl", "start": 11849654, "end": 11857463}, {"filename": "/data/shaders/inthesky_cache_stained_glass_heavens.glsl", "start": 11857463, "end": 11865878}, {"filename": "/data/shaders/inthesky_cache_storm_crown.glsl", "start": 11865878, "end": 11873651}, {"filename": "/data/shaders/inv_cache_code_afterimage_choir.glsl", "start": 11873651, "end": 11880319}, {"filename": "/data/shaders/inv_cache_code_breathing_architecture.glsl", "start": 11880319, "end": 11887125}, {"filename": "/data/shaders/inv_cache_code_cortical_form_constants.glsl", "start": 11887125, "end": 11893927}, {"filename": "/data/shaders/inv_cache_code_dreamlogic_mosaic.glsl", "start": 11893927, "end": 11901208}, {"filename": "/data/shaders/inv_cache_code_entoptic_constellation.glsl", "start": 11901208, "end": 11908016}, {"filename": "/data/shaders/inv_cache_code_lucid_field.glsl", "start": 11908016, "end": 11914657}, {"filename": "/data/shaders/inv_cache_code_peripheral_drift.glsl", "start": 11914657, "end": 11921340}, {"filename": "/data/shaders/inv_cache_code_phosphene_swarm.glsl", "start": 11921340, "end": 11928224}, {"filename": "/data/shaders/inv_cache_code_scintillating_aura.glsl", "start": 11928224, "end": 11934954}, {"filename": "/data/shaders/inv_cache_code_synesthetic_veil.glsl", "start": 11934954, "end": 11941666}, {"filename": "/data/shaders/inversion_code_cache_arc_constellation.glsl", "start": 11941666, "end": 11950743}, {"filename": "/data/shaders/inversion_code_cache_borromean_energy.glsl", "start": 11950743, "end": 11959773}, {"filename": "/data/shaders/inversion_code_cache_cellular_aegis.glsl", "start": 11959773, "end": 11969178}, {"filename": "/data/shaders/inversion_code_cache_charge_topography.glsl", "start": 11969178, "end": 11978028}, {"filename": "/data/shaders/inversion_code_cache_dark_energy_lens.glsl", "start": 11978028, "end": 11986901}, {"filename": "/data/shaders/inversion_code_cache_dipole_choir.glsl", "start": 11986901, "end": 11995809}, {"filename": "/data/shaders/inversion_code_cache_electrostatic_dunes.glsl", "start": 11995809, "end": 12004682}, {"filename": "/data/shaders/inversion_code_cache_flux_lattice.glsl", "start": 12004682, "end": 12013487}, {"filename": "/data/shaders/inversion_code_cache_gravitational_shear.glsl", "start": 12013487, "end": 12022386}, {"filename": "/data/shaders/inversion_code_cache_harmonic_glyphs.glsl", "start": 12022386, "end": 12031287}, {"filename": "/data/shaders/inversion_code_cache_hex_current.glsl", "start": 12031287, "end": 12040344}, {"filename": "/data/shaders/inversion_code_cache_ion_veil.glsl", "start": 12040344, "end": 12049239}, {"filename": "/data/shaders/inversion_code_cache_laser_caustics.glsl", "start": 12049239, "end": 12058139}, {"filename": "/data/shaders/inversion_code_cache_moire_tensor.glsl", "start": 12058139, "end": 12067032}, {"filename": "/data/shaders/inversion_code_cache_phonon_crystal.glsl", "start": 12067032, "end": 12075955}, {"filename": "/data/shaders/inversion_code_cache_photon_mesh.glsl", "start": 12075955, "end": 12084887}, {"filename": "/data/shaders/inversion_code_cache_plasma_filaments.glsl", "start": 12084887, "end": 12093798}, {"filename": "/data/shaders/inversion_code_cache_quantum_foam.glsl", "start": 12093798, "end": 12103006}, {"filename": "/data/shaders/inversion_code_cache_quasicrystal_field.glsl", "start": 12103006, "end": 12111913}, {"filename": "/data/shaders/inversion_code_cache_reaction_front.glsl", "start": 12111913, "end": 12120849}, {"filename": "/data/shaders/inversion_code_cache_resonance_cavity.glsl", "start": 12120849, "end": 12129763}, {"filename": "/data/shaders/inversion_code_cache_spectral_terrain.glsl", "start": 12129763, "end": 12138682}, {"filename": "/data/shaders/inversion_code_cache_synaptic_canopy.glsl", "start": 12138682, "end": 12147886}, {"filename": "/data/shaders/inversion_code_cache_tachyon_ribbons.glsl", "start": 12147886, "end": 12156887}, {"filename": "/data/shaders/inversion_code_cache_vacuum_bloom.glsl", "start": 12156887, "end": 12165816}, {"filename": "/data/shaders/julia.glsl", "start": 12165816, "end": 12174337}, {"filename": "/data/shaders/kale.glsl", "start": 12174337, "end": 12175167}, {"filename": "/data/shaders/kale2.glsl", "start": 12175167, "end": 12176011}, {"filename": "/data/shaders/kale3.glsl", "start": 12176011, "end": 12176860}, {"filename": "/data/shaders/kale4.glsl", "start": 12176860, "end": 12177733}, {"filename": "/data/shaders/kale_code_astral_rose.glsl", "start": 12177733, "end": 12185405}, {"filename": "/data/shaders/kale_code_cathedral_vortex.glsl", "start": 12185405, "end": 12193083}, {"filename": "/data/shaders/kale_code_chromatic_shrine.glsl", "start": 12193083, "end": 12200762}, {"filename": "/data/shaders/kale_code_crystal_maelstrom.glsl", "start": 12200762, "end": 12208440}, {"filename": "/data/shaders/kale_code_diamond_tempest.glsl", "start": 12208440, "end": 12216117}, {"filename": "/data/shaders/kale_code_dream_engine.glsl", "start": 12216117, "end": 12223791}, {"filename": "/data/shaders/kale_code_infinite_filigree.glsl", "start": 12223791, "end": 12231470}, {"filename": "/data/shaders/kale_code_liquid_mosaic.glsl", "start": 12231470, "end": 12239144}, {"filename": "/data/shaders/kale_code_nebula_chrysalis.glsl", "start": 12239144, "end": 12246822}, {"filename": "/data/shaders/kale_code_opal_tesseract.glsl", "start": 12246822, "end": 12254497}, {"filename": "/data/shaders/kale_code_phoenix_prism.glsl", "start": 12254497, "end": 12262171}, {"filename": "/data/shaders/kale_code_quantum_orchid.glsl", "start": 12262171, "end": 12269846}, {"filename": "/data/shaders/kale_code_seraphic_lattice.glsl", "start": 12269846, "end": 12277524}, {"filename": "/data/shaders/kale_code_stained_cosmos.glsl", "start": 12277524, "end": 12285199}, {"filename": "/data/shaders/kale_code_velvet_mandala.glsl", "start": 12285199, "end": 12292874}, {"filename": "/data/shaders/kale_mouse.glsl", "start": 12292874, "end": 12293948}, {"filename": "/data/shaders/kscopic.glsl", "start": 12293948, "end": 12294710}, {"filename": "/data/shaders/kscopic_3d_mouse.glsl", "start": 12294710, "end": 12296126}, {"filename": "/data/shaders/light-rings.glsl", "start": 12296126, "end": 12297769}, {"filename": "/data/shaders/light_pulse.glsl", "start": 12297769, "end": 12298845}, {"filename": "/data/shaders/light_spiral.glsl", "start": 12298845, "end": 12299995}, {"filename": "/data/shaders/lightfade.glsl", "start": 12299995, "end": 12300891}, {"filename": "/data/shaders/lightfade_xor.glsl", "start": 12300891, "end": 12302027}, {"filename": "/data/shaders/lightfadedark.glsl", "start": 12302027, "end": 12303085}, {"filename": "/data/shaders/lineinlineout.glsl", "start": 12303085, "end": 12303497}, {"filename": "/data/shaders/liquid_code_chrome_nautilus.glsl", "start": 12303497, "end": 12310051}, {"filename": "/data/shaders/liquid_code_iridescent_whirlpool.glsl", "start": 12310051, "end": 12316916}, {"filename": "/data/shaders/liquid_code_mercury_vortex.glsl", "start": 12316916, "end": 12322892}, {"filename": "/data/shaders/liquid_code_molten_helix.glsl", "start": 12322892, "end": 12329557}, {"filename": "/data/shaders/liquid_code_silver_riptide.glsl", "start": 12329557, "end": 12336392}, {"filename": "/data/shaders/list_rainbow_enter.glsl", "start": 12336392, "end": 12337205}, {"filename": "/data/shaders/lucy.glsl", "start": 12337205, "end": 12337687}, {"filename": "/data/shaders/magic-2.glsl", "start": 12337687, "end": 12338420}, {"filename": "/data/shaders/magic.glsl", "start": 12338420, "end": 12338934}, {"filename": "/data/shaders/mandella1.glsl", "start": 12338934, "end": 12339179}, {"filename": "/data/shaders/matrix.glsl", "start": 12339179, "end": 12339764}, {"filename": "/data/shaders/matrix_mouse.glsl", "start": 12339764, "end": 12341165}, {"filename": "/data/shaders/matrix_mul.glsl", "start": 12341165, "end": 12342424}, {"filename": "/data/shaders/matrix_mul2.glsl", "start": 12342424, "end": 12343699}, {"filename": "/data/shaders/matrix_mul3.glsl", "start": 12343699, "end": 12344886}, {"filename": "/data/shaders/matrix_mul4.glsl", "start": 12344886, "end": 12345923}, {"filename": "/data/shaders/mb.glsl", "start": 12345923, "end": 12347229}, {"filename": "/data/shaders/metal-code-chrome-tide.glsl", "start": 12347229, "end": 12351043}, {"filename": "/data/shaders/metal-code-iridescent-foundry.glsl", "start": 12351043, "end": 12354483}, {"filename": "/data/shaders/metal-code-liquid-tessera.glsl", "start": 12354483, "end": 12357869}, {"filename": "/data/shaders/metal-code-magnetic-ferrofluid.glsl", "start": 12357869, "end": 12361227}, {"filename": "/data/shaders/metal-code-mercury-cathedral.glsl", "start": 12361227, "end": 12364892}, {"filename": "/data/shaders/metal-code-molten-mirror.glsl", "start": 12364892, "end": 12368373}, {"filename": "/data/shaders/metal-code-nanofluid-forge.glsl", "start": 12368373, "end": 12372043}, {"filename": "/data/shaders/metal-code-opal-steel.glsl", "start": 12372043, "end": 12375078}, {"filename": "/data/shaders/metal-code-plasma-alloy.glsl", "start": 12375078, "end": 12378244}, {"filename": "/data/shaders/metal-code-silver-maelstrom.glsl", "start": 12378244, "end": 12381446}, {"filename": "/data/shaders/minimize.glsl", "start": 12381446, "end": 12382031}, {"filename": "/data/shaders/mirdmd.glsl", "start": 12382031, "end": 12382364}, {"filename": "/data/shaders/mirdmd1.glsl", "start": 12382364, "end": 12383598}, {"filename": "/data/shaders/mirdmdr.glsl", "start": 12383598, "end": 12384332}, {"filename": "/data/shaders/mirdmdrcolor.glsl", "start": 12384332, "end": 12385364}, {"filename": "/data/shaders/mirdmdrmatch.glsl", "start": 12385364, "end": 12386367}, {"filename": "/data/shaders/mirdmdrspeed.glsl", "start": 12386367, "end": 12387565}, {"filename": "/data/shaders/mirdmdt.glsl", "start": 12387565, "end": 12388213}, {"filename": "/data/shaders/mirror-air-bowl.glsl", "start": 12388213, "end": 12392394}, {"filename": "/data/shaders/mirror-airshader1.glsl", "start": 12392394, "end": 12395100}, {"filename": "/data/shaders/mirror-atan.glsl", "start": 12395100, "end": 12396558}, {"filename": "/data/shaders/mirror-bowl-by-time.glsl", "start": 12396558, "end": 12397779}, {"filename": "/data/shaders/mirror-bowl.glsl", "start": 12397779, "end": 12399024}, {"filename": "/data/shaders/mirror-bubble-zoom-mouse-audio.glsl", "start": 12399024, "end": 12403857}, {"filename": "/data/shaders/mirror-bubble-zoom-mouse.glsl", "start": 12403857, "end": 12405348}, {"filename": "/data/shaders/mirror-bubble.glsl", "start": 12405348, "end": 12406110}, {"filename": "/data/shaders/mirror-c.glsl", "start": 12406110, "end": 12406515}, {"filename": "/data/shaders/mirror-center.glsl", "start": 12406515, "end": 12407014}, {"filename": "/data/shaders/mirror-code-x-anti-diagonal.glsl", "start": 12407014, "end": 12407213}, {"filename": "/data/shaders/mirror-code-x-bottom-fold.glsl", "start": 12407213, "end": 12407382}, {"filename": "/data/shaders/mirror-code-x-checker-grid.glsl", "start": 12407382, "end": 12407631}, {"filename": "/data/shaders/mirror-code-x-column-strips.glsl", "start": 12407631, "end": 12407885}, {"filename": "/data/shaders/mirror-code-x-cross-fold.glsl", "start": 12407885, "end": 12408126}, {"filename": "/data/shaders/mirror-code-x-diamond-fold.glsl", "start": 12408126, "end": 12408342}, {"filename": "/data/shaders/mirror-code-x-kaleido-eight.glsl", "start": 12408342, "end": 12408863}, {"filename": "/data/shaders/mirror-code-x-kaleido-six.glsl", "start": 12408863, "end": 12409384}, {"filename": "/data/shaders/mirror-code-x-kaleido-twelve.glsl", "start": 12409384, "end": 12409906}, {"filename": "/data/shaders/mirror-code-x-left-fold.glsl", "start": 12409906, "end": 12410075}, {"filename": "/data/shaders/mirror-code-x-main-diagonal.glsl", "start": 12410075, "end": 12410262}, {"filename": "/data/shaders/mirror-code-x-offset-checker.glsl", "start": 12410262, "end": 12410618}, {"filename": "/data/shaders/mirror-code-x-pinwheel-fold.glsl", "start": 12410618, "end": 12411229}, {"filename": "/data/shaders/mirror-code-x-quad-center.glsl", "start": 12411229, "end": 12411384}, {"filename": "/data/shaders/mirror-code-x-quad-corners.glsl", "start": 12411384, "end": 12411539}, {"filename": "/data/shaders/mirror-code-x-quadrant-offset.glsl", "start": 12411539, "end": 12411938}, {"filename": "/data/shaders/mirror-code-x-right-fold.glsl", "start": 12411938, "end": 12412107}, {"filename": "/data/shaders/mirror-code-x-ripple-rings.glsl", "start": 12412107, "end": 12412650}, {"filename": "/data/shaders/mirror-code-x-rotating-axis.glsl", "start": 12412650, "end": 12413061}, {"filename": "/data/shaders/mirror-code-x-row-strips.glsl", "start": 12413061, "end": 12413315}, {"filename": "/data/shaders/mirror-code-x-sliding-axis.glsl", "start": 12413315, "end": 12413696}, {"filename": "/data/shaders/mirror-code-x-spiral-sectors.glsl", "start": 12413696, "end": 12414268}, {"filename": "/data/shaders/mirror-code-x-tile-diagonals.glsl", "start": 12414268, "end": 12414667}, {"filename": "/data/shaders/mirror-code-x-top-fold.glsl", "start": 12414667, "end": 12414836}, {"filename": "/data/shaders/mirror-code-x-wave-seam.glsl", "start": 12414836, "end": 12415211}, {"filename": "/data/shaders/mirror-color-mouse.glsl", "start": 12415211, "end": 12417317}, {"filename": "/data/shaders/mirror-color-o1.glsl", "start": 12417317, "end": 12418440}, {"filename": "/data/shaders/mirror-color-swirl.glsl", "start": 12418440, "end": 12419610}, {"filename": "/data/shaders/mirror-comb-mouse.glsl", "start": 12419610, "end": 12421691}, {"filename": "/data/shaders/mirror-fish-mouse.glsl", "start": 12421691, "end": 12422858}, {"filename": "/data/shaders/mirror-frac-mouse-z2.glsl", "start": 12422858, "end": 12426703}, {"filename": "/data/shaders/mirror-geometric.glsl", "start": 12426703, "end": 12427927}, {"filename": "/data/shaders/mirror-goofy.glsl", "start": 12427927, "end": 12429269}, {"filename": "/data/shaders/mirror-gpt.glsl", "start": 12429269, "end": 12431297}, {"filename": "/data/shaders/mirror-grad.glsl", "start": 12431297, "end": 12432600}, {"filename": "/data/shaders/mirror-in.glsl", "start": 12432600, "end": 12432973}, {"filename": "/data/shaders/mirror-l.glsl", "start": 12432973, "end": 12433277}, {"filename": "/data/shaders/mirror-mandella.glsl", "start": 12433277, "end": 12435604}, {"filename": "/data/shaders/mirror-pebble.glsl", "start": 12435604, "end": 12436429}, {"filename": "/data/shaders/mirror-pong.glsl", "start": 12436429, "end": 12437563}, {"filename": "/data/shaders/mirror-pong2.glsl", "start": 12437563, "end": 12438815}, {"filename": "/data/shaders/mirror-prism.glsl", "start": 12438815, "end": 12440363}, {"filename": "/data/shaders/mirror-psyce-wave-all.glsl", "start": 12440363, "end": 12440942}, {"filename": "/data/shaders/mirror-putty.glsl", "start": 12440942, "end": 12441781}, {"filename": "/data/shaders/mirror-self.glsl", "start": 12441781, "end": 12441951}, {"filename": "/data/shaders/mirror-sin-osc.glsl", "start": 12441951, "end": 12443098}, {"filename": "/data/shaders/mirror-spiral-aura.glsl", "start": 12443098, "end": 12443916}, {"filename": "/data/shaders/mirror-spiral-f.glsl", "start": 12443916, "end": 12444756}, {"filename": "/data/shaders/mirror-spiral.glsl", "start": 12444756, "end": 12445382}, {"filename": "/data/shaders/mirror-swirly.glsl", "start": 12445382, "end": 12446011}, {"filename": "/data/shaders/mirror-twist-gpt.glsl", "start": 12446011, "end": 12447576}, {"filename": "/data/shaders/mirror-twist-sin.glsl", "start": 12447576, "end": 12448298}, {"filename": "/data/shaders/mirror-twist.glsl", "start": 12448298, "end": 12448812}, {"filename": "/data/shaders/mirror-twisted.glsl", "start": 12448812, "end": 12449614}, {"filename": "/data/shaders/mirror-ufo-wrap.glsl", "start": 12449614, "end": 12450796}, {"filename": "/data/shaders/mirror-wrap-dmd6i.glsl", "start": 12450796, "end": 12457177}, {"filename": "/data/shaders/mirror-wrap-edge.glsl", "start": 12457177, "end": 12457691}, {"filename": "/data/shaders/mirror-wrap-new.glsl", "start": 12457691, "end": 12460319}, {"filename": "/data/shaders/mirror-wrap-rotate-snap.glsl", "start": 12460319, "end": 12460774}, {"filename": "/data/shaders/mirror-wrap-rotate.glsl", "start": 12460774, "end": 12461194}, {"filename": "/data/shaders/mirror-wrap-rotate90.glsl", "start": 12461194, "end": 12461573}, {"filename": "/data/shaders/mirror-wrap-scale.glsl", "start": 12461573, "end": 12463836}, {"filename": "/data/shaders/mirror-wrap-sphere.glsl", "start": 12463836, "end": 12464278}, {"filename": "/data/shaders/mirror-wrap-sphere2.glsl", "start": 12464278, "end": 12464589}, {"filename": "/data/shaders/mirror-wrap-spin.glsl", "start": 12464589, "end": 12465198}, {"filename": "/data/shaders/mirror-wrap.glsl", "start": 12465198, "end": 12465399}, {"filename": "/data/shaders/mirror-wrap2.glsl", "start": 12465399, "end": 12465841}, {"filename": "/data/shaders/mirror-zoom.glsl", "start": 12465841, "end": 12467564}, {"filename": "/data/shaders/mirror1.glsl", "start": 12467564, "end": 12468261}, {"filename": "/data/shaders/mirror1_op.glsl", "start": 12468261, "end": 12469219}, {"filename": "/data/shaders/mirror1_s.glsl", "start": 12469219, "end": 12469916}, {"filename": "/data/shaders/mirror2.glsl", "start": 12469916, "end": 12470650}, {"filename": "/data/shaders/mirror2_op.glsl", "start": 12470650, "end": 12471650}, {"filename": "/data/shaders/mirror2_s.glsl", "start": 12471650, "end": 12472384}, {"filename": "/data/shaders/mirror3.glsl", "start": 12472384, "end": 12473118}, {"filename": "/data/shaders/mirror3_op.glsl", "start": 12473118, "end": 12474113}, {"filename": "/data/shaders/mirror3_s.glsl", "start": 12474113, "end": 12474847}, {"filename": "/data/shaders/mirror_8.glsl", "start": 12474847, "end": 12475694}, {"filename": "/data/shaders/mirror_audio.glsl", "start": 12475694, "end": 12479213}, {"filename": "/data/shaders/mirror_b.glsl", "start": 12479213, "end": 12479431}, {"filename": "/data/shaders/mirror_blur1.glsl", "start": 12479431, "end": 12480246}, {"filename": "/data/shaders/mirror_blur2.glsl", "start": 12480246, "end": 12481098}, {"filename": "/data/shaders/mirror_blur3.glsl", "start": 12481098, "end": 12481950}, {"filename": "/data/shaders/mirror_c.glsl", "start": 12481950, "end": 12482689}, {"filename": "/data/shaders/mirror_dir.glsl", "start": 12482689, "end": 12483180}, {"filename": "/data/shaders/mirror_half_dir.glsl", "start": 12483180, "end": 12483651}, {"filename": "/data/shaders/mirror_offset.glsl", "start": 12483651, "end": 12484368}, {"filename": "/data/shaders/mirror_offset_rev.glsl", "start": 12484368, "end": 12485084}, {"filename": "/data/shaders/mirror_p.glsl", "start": 12485084, "end": 12485558}, {"filename": "/data/shaders/mirror_r4.glsl", "start": 12485558, "end": 12486404}, {"filename": "/data/shaders/mirror_r8.glsl", "start": 12486404, "end": 12487250}, {"filename": "/data/shaders/mirror_rad.glsl", "start": 12487250, "end": 12488221}, {"filename": "/data/shaders/mirror_rev.glsl", "start": 12488221, "end": 12489281}, {"filename": "/data/shaders/mirror_rev2.glsl", "start": 12489281, "end": 12490341}, {"filename": "/data/shaders/mirror_rev2y.glsl", "start": 12490341, "end": 12491401}, {"filename": "/data/shaders/mirror_revy.glsl", "start": 12491401, "end": 12492461}, {"filename": "/data/shaders/mirror_x_audio1.glsl", "start": 12492461, "end": 12493245}, {"filename": "/data/shaders/mirror_x_audio10.glsl", "start": 12493245, "end": 12494267}, {"filename": "/data/shaders/mirror_x_audio11.glsl", "start": 12494267, "end": 12495119}, {"filename": "/data/shaders/mirror_x_audio12.glsl", "start": 12495119, "end": 12496097}, {"filename": "/data/shaders/mirror_x_audio13.glsl", "start": 12496097, "end": 12497204}, {"filename": "/data/shaders/mirror_x_audio14.glsl", "start": 12497204, "end": 12497989}, {"filename": "/data/shaders/mirror_x_audio15.glsl", "start": 12497989, "end": 12498899}, {"filename": "/data/shaders/mirror_x_audio16.glsl", "start": 12498899, "end": 12500079}, {"filename": "/data/shaders/mirror_x_audio17.glsl", "start": 12500079, "end": 12501019}, {"filename": "/data/shaders/mirror_x_audio18.glsl", "start": 12501019, "end": 12501951}, {"filename": "/data/shaders/mirror_x_audio19.glsl", "start": 12501951, "end": 12503178}, {"filename": "/data/shaders/mirror_x_audio2.glsl", "start": 12503178, "end": 12504137}, {"filename": "/data/shaders/mirror_x_audio20.glsl", "start": 12504137, "end": 12505327}, {"filename": "/data/shaders/mirror_x_audio21.glsl", "start": 12505327, "end": 12506172}, {"filename": "/data/shaders/mirror_x_audio22.glsl", "start": 12506172, "end": 12507027}, {"filename": "/data/shaders/mirror_x_audio23.glsl", "start": 12507027, "end": 12508066}, {"filename": "/data/shaders/mirror_x_audio24.glsl", "start": 12508066, "end": 12509166}, {"filename": "/data/shaders/mirror_x_audio25.glsl", "start": 12509166, "end": 12510104}, {"filename": "/data/shaders/mirror_x_audio26.glsl", "start": 12510104, "end": 12511035}, {"filename": "/data/shaders/mirror_x_audio27.glsl", "start": 12511035, "end": 12512213}, {"filename": "/data/shaders/mirror_x_audio28.glsl", "start": 12512213, "end": 12513187}, {"filename": "/data/shaders/mirror_x_audio29.glsl", "start": 12513187, "end": 12514438}, {"filename": "/data/shaders/mirror_x_audio3.glsl", "start": 12514438, "end": 12515202}, {"filename": "/data/shaders/mirror_x_audio30.glsl", "start": 12515202, "end": 12516115}, {"filename": "/data/shaders/mirror_x_audio31.glsl", "start": 12516115, "end": 12517364}, {"filename": "/data/shaders/mirror_x_audio32.glsl", "start": 12517364, "end": 12518474}, {"filename": "/data/shaders/mirror_x_audio33.glsl", "start": 12518474, "end": 12519616}, {"filename": "/data/shaders/mirror_x_audio34.glsl", "start": 12519616, "end": 12520846}, {"filename": "/data/shaders/mirror_x_audio35.glsl", "start": 12520846, "end": 12521873}, {"filename": "/data/shaders/mirror_x_audio36.glsl", "start": 12521873, "end": 12523072}, {"filename": "/data/shaders/mirror_x_audio37.glsl", "start": 12523072, "end": 12524181}, {"filename": "/data/shaders/mirror_x_audio38.glsl", "start": 12524181, "end": 12525449}, {"filename": "/data/shaders/mirror_x_audio39.glsl", "start": 12525449, "end": 12526650}, {"filename": "/data/shaders/mirror_x_audio4.glsl", "start": 12526650, "end": 12527492}, {"filename": "/data/shaders/mirror_x_audio40.glsl", "start": 12527492, "end": 12529284}, {"filename": "/data/shaders/mirror_x_audio5.glsl", "start": 12529284, "end": 12530435}, {"filename": "/data/shaders/mirror_x_audio6.glsl", "start": 12530435, "end": 12531302}, {"filename": "/data/shaders/mirror_x_audio7.glsl", "start": 12531302, "end": 12532173}, {"filename": "/data/shaders/mirror_x_audio8.glsl", "start": 12532173, "end": 12533087}, {"filename": "/data/shaders/mirror_x_audio9.glsl", "start": 12533087, "end": 12534150}, {"filename": "/data/shaders/mirrorg.glsl", "start": 12534150, "end": 12534913}, {"filename": "/data/shaders/mix_diag.glsl", "start": 12534913, "end": 12535187}, {"filename": "/data/shaders/move.glsl", "start": 12535187, "end": 12535550}, {"filename": "/data/shaders/moveGL.glsl", "start": 12535550, "end": 12535935}, {"filename": "/data/shaders/moving_gradient.glsl", "start": 12535935, "end": 12536966}, {"filename": "/data/shaders/negative.glsl", "start": 12536966, "end": 12537195}, {"filename": "/data/shaders/negative_xor_strobe.glsl", "start": 12537195, "end": 12538536}, {"filename": "/data/shaders/neon.glsl", "start": 12538536, "end": 12540063}, {"filename": "/data/shaders/neon_mouse.glsl", "start": 12540063, "end": 12541200}, {"filename": "/data/shaders/new_fractal.glsl", "start": 12541200, "end": 12544705}, {"filename": "/data/shaders/new_fractal2.glsl", "start": 12544705, "end": 12548228}, {"filename": "/data/shaders/new_glitch.glsl", "start": 12548228, "end": 12549020}, {"filename": "/data/shaders/new_xor_scale_strobe.glsl", "start": 12549020, "end": 12549735}, {"filename": "/data/shaders/nl.glsl", "start": 12549735, "end": 12551479}, {"filename": "/data/shaders/nofilter.glsl", "start": 12551479, "end": 12552177}, {"filename": "/data/shaders/nothing.glsl", "start": 12552177, "end": 12552309}, {"filename": "/data/shaders/o1-hue.glsl", "start": 12552309, "end": 12554714}, {"filename": "/data/shaders/o3-01.glsl", "start": 12554714, "end": 12556720}, {"filename": "/data/shaders/obj_stretch.glsl", "start": 12556720, "end": 12557586}, {"filename": "/data/shaders/obj_stretch2.glsl", "start": 12557586, "end": 12558441}, {"filename": "/data/shaders/ocean.glsl", "start": 12558441, "end": 12558943}, {"filename": "/data/shaders/ocean_fast.glsl", "start": 12558943, "end": 12559446}, {"filename": "/data/shaders/ol.glsl", "start": 12559446, "end": 12560845}, {"filename": "/data/shaders/old-film-water.glsl", "start": 12560845, "end": 12561828}, {"filename": "/data/shaders/old-film.glsl", "start": 12561828, "end": 12562345}, {"filename": "/data/shaders/old.txt", "start": 12562345, "end": 12569270}, {"filename": "/data/shaders/onoffsep.glsl", "start": 12569270, "end": 12570118}, {"filename": "/data/shaders/open_shadow.glsl", "start": 12570118, "end": 12570702}, {"filename": "/data/shaders/open_shadow_jump.glsl", "start": 12570702, "end": 12571582}, {"filename": "/data/shaders/open_shadow_multi.glsl", "start": 12571582, "end": 12572448}, {"filename": "/data/shaders/open_shadow_multi_x2.glsl", "start": 12572448, "end": 12573315}, {"filename": "/data/shaders/open_up.glsl", "start": 12573315, "end": 12574112}, {"filename": "/data/shaders/optxtime.glsl", "start": 12574112, "end": 12575111}, {"filename": "/data/shaders/optxtime_cos.glsl", "start": 12575111, "end": 12576068}, {"filename": "/data/shaders/optxtime_tex.glsl", "start": 12576068, "end": 12577113}, {"filename": "/data/shaders/output.txt", "start": 12577113, "end": 12584039}, {"filename": "/data/shaders/page_turn.glsl", "start": 12584039, "end": 12584427}, {"filename": "/data/shaders/page_turn2.glsl", "start": 12584427, "end": 12585010}, {"filename": "/data/shaders/page_turn3.glsl", "start": 12585010, "end": 12585607}, {"filename": "/data/shaders/particle_1.glsl", "start": 12585607, "end": 12587050}, {"filename": "/data/shaders/pastel.glsl", "start": 12587050, "end": 12588435}, {"filename": "/data/shaders/pebble.glsl", "start": 12588435, "end": 12588998}, {"filename": "/data/shaders/pebble_fast.glsl", "start": 12588998, "end": 12589562}, {"filename": "/data/shaders/pencil_draw.glsl", "start": 12589562, "end": 12592430}, {"filename": "/data/shaders/pencil_draw_lines.glsl", "start": 12592430, "end": 12595576}, {"filename": "/data/shaders/phantom_drift_cache.glsl", "start": 12595576, "end": 12598076}, {"filename": "/data/shaders/picolor.glsl", "start": 12598076, "end": 12598899}, {"filename": "/data/shaders/pigrid.glsl", "start": 12598899, "end": 12599875}, {"filename": "/data/shaders/pilot_effect_ant_bloom.glsl", "start": 12599875, "end": 12614524}, {"filename": "/data/shaders/pilot_effect_ant_flare.glsl", "start": 12614524, "end": 12629175}, {"filename": "/data/shaders/pilot_effect_ant_lattice.glsl", "start": 12629175, "end": 12643843}, {"filename": "/data/shaders/pilot_effect_ant_pulse.glsl", "start": 12643843, "end": 12658492}, {"filename": "/data/shaders/pilot_effect_ant_ripple.glsl", "start": 12658492, "end": 12673133}, {"filename": "/data/shaders/pilot_effect_ant_storm.glsl", "start": 12673133, "end": 12687778}, {"filename": "/data/shaders/pilot_effect_ant_tunnel.glsl", "start": 12687778, "end": 12702443}, {"filename": "/data/shaders/pilot_effect_ant_weave.glsl", "start": 12702443, "end": 12717100}, {"filename": "/data/shaders/pilot_effect_chrome_bloom.glsl", "start": 12717100, "end": 12731765}, {"filename": "/data/shaders/pilot_effect_chrome_flare.glsl", "start": 12731765, "end": 12746433}, {"filename": "/data/shaders/pilot_effect_chrome_lattice.glsl", "start": 12746433, "end": 12761100}, {"filename": "/data/shaders/pilot_effect_chrome_pulse.glsl", "start": 12761100, "end": 12775761}, {"filename": "/data/shaders/pilot_effect_chrome_ripple.glsl", "start": 12775761, "end": 12790417}, {"filename": "/data/shaders/pilot_effect_chrome_storm.glsl", "start": 12790417, "end": 12805090}, {"filename": "/data/shaders/pilot_effect_chrome_tunnel.glsl", "start": 12805090, "end": 12819745}, {"filename": "/data/shaders/pilot_effect_chrome_weave.glsl", "start": 12819745, "end": 12834409}, {"filename": "/data/shaders/pilot_effect_crystal_bloom.glsl", "start": 12834409, "end": 12849057}, {"filename": "/data/shaders/pilot_effect_crystal_flare.glsl", "start": 12849057, "end": 12863716}, {"filename": "/data/shaders/pilot_effect_crystal_lattice.glsl", "start": 12863716, "end": 12878368}, {"filename": "/data/shaders/pilot_effect_crystal_pulse.glsl", "start": 12878368, "end": 12893031}, {"filename": "/data/shaders/pilot_effect_crystal_ripple.glsl", "start": 12893031, "end": 12907688}, {"filename": "/data/shaders/pilot_effect_crystal_storm.glsl", "start": 12907688, "end": 12922362}, {"filename": "/data/shaders/pilot_effect_crystal_tunnel.glsl", "start": 12922362, "end": 12937022}, {"filename": "/data/shaders/pilot_effect_crystal_weave.glsl", "start": 12937022, "end": 12951695}, {"filename": "/data/shaders/pilot_effect_fractal_bloom.glsl", "start": 12951695, "end": 12966343}, {"filename": "/data/shaders/pilot_effect_fractal_flare.glsl", "start": 12966343, "end": 12981013}, {"filename": "/data/shaders/pilot_effect_fractal_lattice.glsl", "start": 12981013, "end": 12995675}, {"filename": "/data/shaders/pilot_effect_fractal_pulse.glsl", "start": 12995675, "end": 13010340}, {"filename": "/data/shaders/pilot_effect_fractal_ripple.glsl", "start": 13010340, "end": 13025008}, {"filename": "/data/shaders/pilot_effect_fractal_storm.glsl", "start": 13025008, "end": 13039674}, {"filename": "/data/shaders/pilot_effect_fractal_tunnel.glsl", "start": 13039674, "end": 13054324}, {"filename": "/data/shaders/pilot_effect_fractal_weave.glsl", "start": 13054324, "end": 13068990}, {"filename": "/data/shaders/pilot_effect_gem_bloom.glsl", "start": 13068990, "end": 13083644}, {"filename": "/data/shaders/pilot_effect_gem_flare.glsl", "start": 13083644, "end": 13098289}, {"filename": "/data/shaders/pilot_effect_gem_lattice.glsl", "start": 13098289, "end": 13112931}, {"filename": "/data/shaders/pilot_effect_gem_pulse.glsl", "start": 13112931, "end": 13127586}, {"filename": "/data/shaders/pilot_effect_gem_ripple.glsl", "start": 13127586, "end": 13142248}, {"filename": "/data/shaders/pilot_effect_gem_storm.glsl", "start": 13142248, "end": 13156901}, {"filename": "/data/shaders/pilot_effect_gem_tunnel.glsl", "start": 13156901, "end": 13171570}, {"filename": "/data/shaders/pilot_effect_gem_weave.glsl", "start": 13171570, "end": 13186234}, {"filename": "/data/shaders/pilot_effect_kaleido_bloom.glsl", "start": 13186234, "end": 13200883}, {"filename": "/data/shaders/pilot_effect_kaleido_flare.glsl", "start": 13200883, "end": 13215540}, {"filename": "/data/shaders/pilot_effect_kaleido_lattice.glsl", "start": 13215540, "end": 13230190}, {"filename": "/data/shaders/pilot_effect_kaleido_pulse.glsl", "start": 13230190, "end": 13244842}, {"filename": "/data/shaders/pilot_effect_kaleido_ripple.glsl", "start": 13244842, "end": 13259499}, {"filename": "/data/shaders/pilot_effect_kaleido_storm.glsl", "start": 13259499, "end": 13274154}, {"filename": "/data/shaders/pilot_effect_kaleido_tunnel.glsl", "start": 13274154, "end": 13288806}, {"filename": "/data/shaders/pilot_effect_kaleido_weave.glsl", "start": 13288806, "end": 13303467}, {"filename": "/data/shaders/pilot_effect_mirror_bloom.glsl", "start": 13303467, "end": 13318118}, {"filename": "/data/shaders/pilot_effect_mirror_flare.glsl", "start": 13318118, "end": 13332771}, {"filename": "/data/shaders/pilot_effect_mirror_lattice.glsl", "start": 13332771, "end": 13347431}, {"filename": "/data/shaders/pilot_effect_mirror_pulse.glsl", "start": 13347431, "end": 13362088}, {"filename": "/data/shaders/pilot_effect_mirror_ripple.glsl", "start": 13362088, "end": 13376751}, {"filename": "/data/shaders/pilot_effect_mirror_storm.glsl", "start": 13376751, "end": 13391414}, {"filename": "/data/shaders/pilot_effect_mirror_tunnel.glsl", "start": 13391414, "end": 13406089}, {"filename": "/data/shaders/pilot_effect_mirror_weave.glsl", "start": 13406089, "end": 13420758}, {"filename": "/data/shaders/pilot_effect_neon_bloom.glsl", "start": 13420758, "end": 13435411}, {"filename": "/data/shaders/pilot_effect_neon_flare.glsl", "start": 13435411, "end": 13450077}, {"filename": "/data/shaders/pilot_effect_neon_lattice.glsl", "start": 13450077, "end": 13464741}, {"filename": "/data/shaders/pilot_effect_neon_pulse.glsl", "start": 13464741, "end": 13479408}, {"filename": "/data/shaders/pilot_effect_neon_ripple.glsl", "start": 13479408, "end": 13494073}, {"filename": "/data/shaders/pilot_effect_neon_storm.glsl", "start": 13494073, "end": 13508739}, {"filename": "/data/shaders/pilot_effect_neon_tunnel.glsl", "start": 13508739, "end": 13523397}, {"filename": "/data/shaders/pilot_effect_neon_weave.glsl", "start": 13523397, "end": 13538053}, {"filename": "/data/shaders/pilot_effect_prism_bloom.glsl", "start": 13538053, "end": 13552721}, {"filename": "/data/shaders/pilot_effect_prism_flare.glsl", "start": 13552721, "end": 13567383}, {"filename": "/data/shaders/pilot_effect_prism_lattice.glsl", "start": 13567383, "end": 13582044}, {"filename": "/data/shaders/pilot_effect_prism_pulse.glsl", "start": 13582044, "end": 13596700}, {"filename": "/data/shaders/pilot_effect_prism_ripple.glsl", "start": 13596700, "end": 13611356}, {"filename": "/data/shaders/pilot_effect_prism_storm.glsl", "start": 13611356, "end": 13626019}, {"filename": "/data/shaders/pilot_effect_prism_tunnel.glsl", "start": 13626019, "end": 13640687}, {"filename": "/data/shaders/pilot_effect_prism_weave.glsl", "start": 13640687, "end": 13655356}, {"filename": "/data/shaders/pilot_effect_vortex_bloom.glsl", "start": 13655356, "end": 13670021}, {"filename": "/data/shaders/pilot_effect_vortex_flare.glsl", "start": 13670021, "end": 13684691}, {"filename": "/data/shaders/pilot_effect_vortex_lattice.glsl", "start": 13684691, "end": 13699363}, {"filename": "/data/shaders/pilot_effect_vortex_pulse.glsl", "start": 13699363, "end": 13714025}, {"filename": "/data/shaders/pilot_effect_vortex_ripple.glsl", "start": 13714025, "end": 13728707}, {"filename": "/data/shaders/pilot_effect_vortex_storm.glsl", "start": 13728707, "end": 13743373}, {"filename": "/data/shaders/pilot_effect_vortex_tunnel.glsl", "start": 13743373, "end": 13758043}, {"filename": "/data/shaders/pilot_effect_vortex_weave.glsl", "start": 13758043, "end": 13772718}, {"filename": "/data/shaders/pilot_inspiration_map.txt", "start": 13772718, "end": 13780475}, {"filename": "/data/shaders/pilot_random_donor_map.txt", "start": 13780475, "end": 13791192}, {"filename": "/data/shaders/pinch.glsl", "start": 13791192, "end": 13791831}, {"filename": "/data/shaders/pixels.glsl", "start": 13791831, "end": 13792103}, {"filename": "/data/shaders/plasma.glsl", "start": 13792103, "end": 13793405}, {"filename": "/data/shaders/plasma2.glsl", "start": 13793405, "end": 13795231}, {"filename": "/data/shaders/plasma3.glsl", "start": 13795231, "end": 13797257}, {"filename": "/data/shaders/plasma_prism.glsl", "start": 13797257, "end": 13798231}, {"filename": "/data/shaders/plasma_rainbow.glsl", "start": 13798231, "end": 13800581}, {"filename": "/data/shaders/plasma_xor.glsl", "start": 13800581, "end": 13801703}, {"filename": "/data/shaders/pond.glsl", "start": 13801703, "end": 13805882}, {"filename": "/data/shaders/pong-ataan-ex.glsl", "start": 13805882, "end": 13807067}, {"filename": "/data/shaders/pong-atan.glsl", "start": 13807067, "end": 13807696}, {"filename": "/data/shaders/pong-atan2.glsl", "start": 13807696, "end": 13808712}, {"filename": "/data/shaders/pong-atan3.glsl", "start": 13808712, "end": 13809745}, {"filename": "/data/shaders/pong_pi.mp4.glsl", "start": 13809745, "end": 13810704}, {"filename": "/data/shaders/pong_tex.glsl", "start": 13810704, "end": 13811247}, {"filename": "/data/shaders/pool.glsl", "start": 13811247, "end": 13811593}, {"filename": "/data/shaders/pos_light.glsl", "start": 13811593, "end": 13812531}, {"filename": "/data/shaders/prisim_glass_triangle.glsl", "start": 13812531, "end": 13814673}, {"filename": "/data/shaders/prism.glsl", "start": 13814673, "end": 13816073}, {"filename": "/data/shaders/prism_glass.glsl", "start": 13816073, "end": 13818533}, {"filename": "/data/shaders/prism_glass_size.glsl", "start": 13818533, "end": 13820960}, {"filename": "/data/shaders/prism_glass_wall.glsl", "start": 13820960, "end": 13823255}, {"filename": "/data/shaders/prism_glass_wall_noedge.glsl", "start": 13823255, "end": 13825325}, {"filename": "/data/shaders/prism_quad.glsl", "start": 13825325, "end": 13827547}, {"filename": "/data/shaders/prism_quad_by_time.glsl", "start": 13827547, "end": 13829779}, {"filename": "/data/shaders/prism_quad_by_time_fast.glsl", "start": 13829779, "end": 13832013}, {"filename": "/data/shaders/prism_quad_strong.glsl", "start": 13832013, "end": 13834434}, {"filename": "/data/shaders/psych.glsl", "start": 13834434, "end": 13835283}, {"filename": "/data/shaders/psych_pattern.glsl", "start": 13835283, "end": 13836432}, {"filename": "/data/shaders/psych_ripple.glsl", "start": 13836432, "end": 13839333}, {"filename": "/data/shaders/psych_wave_rainbow.glsl", "start": 13839333, "end": 13842608}, {"filename": "/data/shaders/psych_xor.glsl", "start": 13842608, "end": 13843836}, {"filename": "/data/shaders/psyche_ripple.glsl", "start": 13843836, "end": 13844554}, {"filename": "/data/shaders/psyche_wave.glsl", "start": 13844554, "end": 13845051}, {"filename": "/data/shaders/pull.glsl", "start": 13845051, "end": 13845474}, {"filename": "/data/shaders/pull_out.glsl", "start": 13845474, "end": 13845902}, {"filename": "/data/shaders/pulse1.glsl", "start": 13845902, "end": 13847188}, {"filename": "/data/shaders/purple_fade.glsl", "start": 13847188, "end": 13847757}, {"filename": "/data/shaders/purple_haze.glsl", "start": 13847757, "end": 13849870}, {"filename": "/data/shaders/purple_material.glsl", "start": 13849870, "end": 13850504}, {"filename": "/data/shaders/purple_strobe.glsl", "start": 13850504, "end": 13851520}, {"filename": "/data/shaders/putty.glsl", "start": 13851520, "end": 13852255}, {"filename": "/data/shaders/radial.glsl", "start": 13852255, "end": 13852707}, {"filename": "/data/shaders/radial_distortion.glsl", "start": 13852707, "end": 13853889}, {"filename": "/data/shaders/radial_distortion2.glsl", "start": 13853889, "end": 13854610}, {"filename": "/data/shaders/radwarp.glsl", "start": 13854610, "end": 13855093}, {"filename": "/data/shaders/rainbow-code-aurora-silk.glsl", "start": 13855093, "end": 13857950}, {"filename": "/data/shaders/rainbow-code-chromatic-ribbons.glsl", "start": 13857950, "end": 13860196}, {"filename": "/data/shaders/rainbow-code-diamond-wave.glsl", "start": 13860196, "end": 13862701}, {"filename": "/data/shaders/rainbow-code-holographic-facets.glsl", "start": 13862701, "end": 13865307}, {"filename": "/data/shaders/rainbow-code-liquid-pearl.glsl", "start": 13865307, "end": 13867978}, {"filename": "/data/shaders/rainbow-code-neon-topography.glsl", "start": 13867978, "end": 13870404}, {"filename": "/data/shaders/rainbow-code-opal-vortex.glsl", "start": 13870404, "end": 13873006}, {"filename": "/data/shaders/rainbow-code-prismatic-caustics.glsl", "start": 13873006, "end": 13875380}, {"filename": "/data/shaders/rainbow-code-spectral-bloom.glsl", "start": 13875380, "end": 13877494}, {"filename": "/data/shaders/rainbow-code-stained-kaleidoscope.glsl", "start": 13877494, "end": 13879954}, {"filename": "/data/shaders/rainbow-touch.glsl", "start": 13879954, "end": 13880748}, {"filename": "/data/shaders/rainbow.banding.glsl", "start": 13880748, "end": 13881558}, {"filename": "/data/shaders/rainbow_blur.glsl", "start": 13881558, "end": 13884777}, {"filename": "/data/shaders/rainbow_bright.glsl", "start": 13884777, "end": 13886884}, {"filename": "/data/shaders/rainbow_cd.glsl", "start": 13886884, "end": 13889008}, {"filename": "/data/shaders/rainbow_cd_curtain.glsl", "start": 13889008, "end": 13891298}, {"filename": "/data/shaders/rainbow_cd_spin.glsl", "start": 13891298, "end": 13893510}, {"filename": "/data/shaders/rainbow_cd_strobe.glsl", "start": 13893510, "end": 13895696}, {"filename": "/data/shaders/rainbow_color1.glsl", "start": 13895696, "end": 13896407}, {"filename": "/data/shaders/rainbow_color2.glsl", "start": 13896407, "end": 13897138}, {"filename": "/data/shaders/rainbow_color3.glsl", "start": 13897138, "end": 13897858}, {"filename": "/data/shaders/rainbow_color_swirl.glsl", "start": 13897858, "end": 13899230}, {"filename": "/data/shaders/rainbow_expand.glsl", "start": 13899230, "end": 13900178}, {"filename": "/data/shaders/rainbow_expand_noise.glsl", "start": 13900178, "end": 13901859}, {"filename": "/data/shaders/rainbow_expand_noise_xor.glsl", "start": 13901859, "end": 13904118}, {"filename": "/data/shaders/rainbow_expand_noise_xor_light.glsl", "start": 13904118, "end": 13906353}, {"filename": "/data/shaders/rainbow_fractal.glsl", "start": 13906353, "end": 13909812}, {"filename": "/data/shaders/rainbow_fractal_random.glsl", "start": 13909812, "end": 13915865}, {"filename": "/data/shaders/rainbow_ink.glsl", "start": 13915865, "end": 13917008}, {"filename": "/data/shaders/rainbow_ink_t.glsl", "start": 13917008, "end": 13918106}, {"filename": "/data/shaders/rainbow_left.glsl", "start": 13918106, "end": 13918768}, {"filename": "/data/shaders/rainbow_light_swirl.glsl", "start": 13918768, "end": 13920066}, {"filename": "/data/shaders/rainbow_metal_code_aurora_forge.glsl", "start": 13920066, "end": 13929424}, {"filename": "/data/shaders/rainbow_metal_code_chromatic_cascade.glsl", "start": 13929424, "end": 13938780}, {"filename": "/data/shaders/rainbow_metal_code_chrome_bloom.glsl", "start": 13938780, "end": 13948137}, {"filename": "/data/shaders/rainbow_metal_code_cosmic_alloy.glsl", "start": 13948137, "end": 13957492}, {"filename": "/data/shaders/rainbow_metal_code_diamond_liquid.glsl", "start": 13957492, "end": 13966852}, {"filename": "/data/shaders/rainbow_metal_code_electric_iridescence.glsl", "start": 13966852, "end": 13976214}, {"filename": "/data/shaders/rainbow_metal_code_fractal_rainbow.glsl", "start": 13976214, "end": 13985567}, {"filename": "/data/shaders/rainbow_metal_code_holographic_metal.glsl", "start": 13985567, "end": 13994923}, {"filename": "/data/shaders/rainbow_metal_code_hypercolor_alloy.glsl", "start": 13994923, "end": 14004285}, {"filename": "/data/shaders/rainbow_metal_code_infinity_mercury.glsl", "start": 14004285, "end": 14013639}, {"filename": "/data/shaders/rainbow_metal_code_iridescent_ripple.glsl", "start": 14013639, "end": 14023002}, {"filename": "/data/shaders/rainbow_metal_code_kaleido_chrome.glsl", "start": 14023002, "end": 14032361}, {"filename": "/data/shaders/rainbow_metal_code_liquid_mandala.glsl", "start": 14032361, "end": 14041720}, {"filename": "/data/shaders/rainbow_metal_code_magnetic_opal.glsl", "start": 14041720, "end": 14051071}, {"filename": "/data/shaders/rainbow_metal_code_nebula_spectrum.glsl", "start": 14051071, "end": 14060425}, {"filename": "/data/shaders/rainbow_metal_code_neon_mercury.glsl", "start": 14060425, "end": 14069778}, {"filename": "/data/shaders/rainbow_metal_code_opal_vortex.glsl", "start": 14069778, "end": 14079128}, {"filename": "/data/shaders/rainbow_metal_code_plasma_chalice.glsl", "start": 14079128, "end": 14088479}, {"filename": "/data/shaders/rainbow_metal_code_prismatic_tide.glsl", "start": 14088479, "end": 14097838}, {"filename": "/data/shaders/rainbow_metal_code_psychedelic_foundry.glsl", "start": 14097838, "end": 14107196}, {"filename": "/data/shaders/rainbow_metal_code_quantum_forge.glsl", "start": 14107196, "end": 14116551}, {"filename": "/data/shaders/rainbow_metal_code_radiant_helix.glsl", "start": 14116551, "end": 14125909}, {"filename": "/data/shaders/rainbow_metal_code_solar_mercury.glsl", "start": 14125909, "end": 14135265}, {"filename": "/data/shaders/rainbow_metal_code_spectral_coil.glsl", "start": 14135265, "end": 14144624}, {"filename": "/data/shaders/rainbow_metal_code_supernova_metal.glsl", "start": 14144624, "end": 14153980}, {"filename": "/data/shaders/rainbow_metal_fractal.glsl", "start": 14153980, "end": 14156067}, {"filename": "/data/shaders/rainbow_mouse_x1.glsl", "start": 14156067, "end": 14156904}, {"filename": "/data/shaders/rainbow_pi01.glsl", "start": 14156904, "end": 14157998}, {"filename": "/data/shaders/rainbow_pi_neoin.glsl", "start": 14157998, "end": 14159122}, {"filename": "/data/shaders/rainbow_prisim.glsl", "start": 14159122, "end": 14160791}, {"filename": "/data/shaders/rainbow_rip_prism.glsl", "start": 14160791, "end": 14162301}, {"filename": "/data/shaders/rainbow_spiral.glsl", "start": 14162301, "end": 14165421}, {"filename": "/data/shaders/rainbow_spiral_loose.glsl", "start": 14165421, "end": 14168541}, {"filename": "/data/shaders/rainbow_spiral_rev.glsl", "start": 14168541, "end": 14171977}, {"filename": "/data/shaders/rainbow_spiral_rev2.glsl", "start": 14171977, "end": 14175272}, {"filename": "/data/shaders/rainbow_swirl_limit.glsl", "start": 14175272, "end": 14176347}, {"filename": "/data/shaders/rainbow_swirl_limit_discard.glsl", "start": 14176347, "end": 14177498}, {"filename": "/data/shaders/rainbow_top.glsl", "start": 14177498, "end": 14178169}, {"filename": "/data/shaders/rainbow_x_chromatic_storm.glsl", "start": 14178169, "end": 14180958}, {"filename": "/data/shaders/rainbow_x_fractal_bass.glsl", "start": 14180958, "end": 14183165}, {"filename": "/data/shaders/rainbow_x_ink_wave.glsl", "start": 14183165, "end": 14184959}, {"filename": "/data/shaders/rainbow_x_light_bass.glsl", "start": 14184959, "end": 14186999}, {"filename": "/data/shaders/rainbow_x_metal_react.glsl", "start": 14186999, "end": 14189278}, {"filename": "/data/shaders/rainbow_x_prism_pulse.glsl", "start": 14189278, "end": 14191386}, {"filename": "/data/shaders/rainbow_x_pulse_blur.glsl", "start": 14191386, "end": 14193629}, {"filename": "/data/shaders/rainbow_x_spiral_beat.glsl", "start": 14193629, "end": 14195348}, {"filename": "/data/shaders/rainbow_x_swirl_freq.glsl", "start": 14195348, "end": 14197175}, {"filename": "/data/shaders/rainbow_x_xor_beat.glsl", "start": 14197175, "end": 14199972}, {"filename": "/data/shaders/rand_blocks.glsl", "start": 14199972, "end": 14200868}, {"filename": "/data/shaders/random_colors.glsl", "start": 14200868, "end": 14201811}, {"filename": "/data/shaders/random_pixels_static.glsl", "start": 14201811, "end": 14202963}, {"filename": "/data/shaders/random_pos_fish.glsl", "start": 14202963, "end": 14203873}, {"filename": "/data/shaders/random_resize.glsl", "start": 14203873, "end": 14204965}, {"filename": "/data/shaders/random_rgb.glsl", "start": 14204965, "end": 14206156}, {"filename": "/data/shaders/random_rgb_strobe.glsl", "start": 14206156, "end": 14206888}, {"filename": "/data/shaders/random_soul.glsl", "start": 14206888, "end": 14208322}, {"filename": "/data/shaders/random_soul_by_mouse.glsl", "start": 14208322, "end": 14209313}, {"filename": "/data/shaders/random_spectrum_deep_melt.glsl", "start": 14209313, "end": 14212166}, {"filename": "/data/shaders/random_spectrum_diamond_rings.glsl", "start": 14212166, "end": 14216283}, {"filename": "/data/shaders/random_spectrum_fractal_kaleidoscope.glsl", "start": 14216283, "end": 14219257}, {"filename": "/data/shaders/random_spectrum_glass_vortex.glsl", "start": 14219257, "end": 14221325}, {"filename": "/data/shaders/random_spectrum_mercury_vortex.glsl", "start": 14221325, "end": 14224746}, {"filename": "/data/shaders/random_spectrum_neon_organism.glsl", "start": 14224746, "end": 14228136}, {"filename": "/data/shaders/random_spectrum_plasma_neon.glsl", "start": 14228136, "end": 14231527}, {"filename": "/data/shaders/random_spectrum_tunnel_rings.glsl", "start": 14231527, "end": 14234281}, {"filename": "/data/shaders/random_spectrum_wormhole.glsl", "start": 14234281, "end": 14237987}, {"filename": "/data/shaders/random_square.glsl", "start": 14237987, "end": 14239083}, {"filename": "/data/shaders/random_strobe.glsl", "start": 14239083, "end": 14239652}, {"filename": "/data/shaders/random_strobe_time.glsl", "start": 14239652, "end": 14240668}, {"filename": "/data/shaders/random_uneven_square.glsl", "start": 14240668, "end": 14241915}, {"filename": "/data/shaders/random_x_audio_aurora.glsl", "start": 14241915, "end": 14244491}, {"filename": "/data/shaders/random_x_audio_bass_tunnel.glsl", "start": 14244491, "end": 14245750}, {"filename": "/data/shaders/random_x_audio_blackhole_pull.glsl", "start": 14245750, "end": 14247533}, {"filename": "/data/shaders/random_x_audio_chromatic_pulse.glsl", "start": 14247533, "end": 14248888}, {"filename": "/data/shaders/random_x_audio_color_orbit.glsl", "start": 14248888, "end": 14250631}, {"filename": "/data/shaders/random_x_audio_crystal_breathe.glsl", "start": 14250631, "end": 14252199}, {"filename": "/data/shaders/random_x_audio_cyclone_twist.glsl", "start": 14252199, "end": 14253515}, {"filename": "/data/shaders/random_x_audio_diamond_pulse.glsl", "start": 14253515, "end": 14255072}, {"filename": "/data/shaders/random_x_audio_echo_cascade.glsl", "start": 14255072, "end": 14256509}, {"filename": "/data/shaders/random_x_audio_fisheye_throb.glsl", "start": 14256509, "end": 14257969}, {"filename": "/data/shaders/random_x_audio_fractal_bloom.glsl", "start": 14257969, "end": 14259984}, {"filename": "/data/shaders/random_x_audio_freq_kaleidoscope.glsl", "start": 14259984, "end": 14261298}, {"filename": "/data/shaders/random_x_audio_freq_rings.glsl", "start": 14261298, "end": 14263092}, {"filename": "/data/shaders/random_x_audio_glass_crack.glsl", "start": 14263092, "end": 14265344}, {"filename": "/data/shaders/random_x_audio_glitch_pulse.glsl", "start": 14265344, "end": 14266881}, {"filename": "/data/shaders/random_x_audio_glitch_tear.glsl", "start": 14266881, "end": 14268695}, {"filename": "/data/shaders/random_x_audio_heat_shimmer.glsl", "start": 14268695, "end": 14270111}, {"filename": "/data/shaders/random_x_audio_hexgrid.glsl", "start": 14270111, "end": 14271913}, {"filename": "/data/shaders/random_x_audio_mirror_fold.glsl", "start": 14271913, "end": 14273362}, {"filename": "/data/shaders/random_x_audio_mosaic_beat.glsl", "start": 14273362, "end": 14274880}, {"filename": "/data/shaders/random_x_audio_nebula_flow.glsl", "start": 14274880, "end": 14277065}, {"filename": "/data/shaders/random_x_audio_neon_edge.glsl", "start": 14277065, "end": 14278906}, {"filename": "/data/shaders/random_x_audio_pixel_scatter.glsl", "start": 14278906, "end": 14280420}, {"filename": "/data/shaders/random_x_audio_pixelsort.glsl", "start": 14280420, "end": 14281858}, {"filename": "/data/shaders/random_x_audio_plasma_storm.glsl", "start": 14281858, "end": 14283362}, {"filename": "/data/shaders/random_x_audio_polar_warp.glsl", "start": 14283362, "end": 14284843}, {"filename": "/data/shaders/random_x_audio_prism_shatter.glsl", "start": 14284843, "end": 14286528}, {"filename": "/data/shaders/random_x_audio_radial_blur.glsl", "start": 14286528, "end": 14288118}, {"filename": "/data/shaders/random_x_audio_rgb_split.glsl", "start": 14288118, "end": 14289645}, {"filename": "/data/shaders/random_x_audio_ripple_rings.glsl", "start": 14289645, "end": 14291068}, {"filename": "/data/shaders/random_x_audio_shockwave.glsl", "start": 14291068, "end": 14292712}, {"filename": "/data/shaders/random_x_audio_sine_fold.glsl", "start": 14292712, "end": 14294040}, {"filename": "/data/shaders/random_x_audio_spiral_zoom.glsl", "start": 14294040, "end": 14295440}, {"filename": "/data/shaders/random_x_audio_strobe_color.glsl", "start": 14295440, "end": 14296826}, {"filename": "/data/shaders/random_x_audio_tile_explode.glsl", "start": 14296826, "end": 14298528}, {"filename": "/data/shaders/random_x_audio_twist_mirror.glsl", "start": 14298528, "end": 14300087}, {"filename": "/data/shaders/random_x_audio_vortex_spin.glsl", "start": 14300087, "end": 14301468}, {"filename": "/data/shaders/random_x_audio_warp_tunnel.glsl", "start": 14301468, "end": 14302907}, {"filename": "/data/shaders/random_x_audio_wave_grid.glsl", "start": 14302907, "end": 14304408}, {"filename": "/data/shaders/random_x_audio_zoom_strobe.glsl", "start": 14304408, "end": 14305966}, {"filename": "/data/shaders/react.glsl", "start": 14305966, "end": 14306292}, {"filename": "/data/shaders/react10.glsl", "start": 14306292, "end": 14306698}, {"filename": "/data/shaders/react11.glsl", "start": 14306698, "end": 14307564}, {"filename": "/data/shaders/react12.glsl", "start": 14307564, "end": 14307898}, {"filename": "/data/shaders/react13.glsl", "start": 14307898, "end": 14308329}, {"filename": "/data/shaders/react14.glsl", "start": 14308329, "end": 14309818}, {"filename": "/data/shaders/react15.glsl", "start": 14309818, "end": 14310342}, {"filename": "/data/shaders/react16.glsl", "start": 14310342, "end": 14311222}, {"filename": "/data/shaders/react17.glsl", "start": 14311222, "end": 14311872}, {"filename": "/data/shaders/react18.glsl", "start": 14311872, "end": 14312436}, {"filename": "/data/shaders/react19.glsl", "start": 14312436, "end": 14315096}, {"filename": "/data/shaders/react2.glsl", "start": 14315096, "end": 14315476}, {"filename": "/data/shaders/react3.glsl", "start": 14315476, "end": 14315831}, {"filename": "/data/shaders/react4.glsl", "start": 14315831, "end": 14316097}, {"filename": "/data/shaders/react5.glsl", "start": 14316097, "end": 14316604}, {"filename": "/data/shaders/react6.glsl", "start": 14316604, "end": 14317193}, {"filename": "/data/shaders/react7.glsl", "start": 14317193, "end": 14317756}, {"filename": "/data/shaders/react8.glsl", "start": 14317756, "end": 14318471}, {"filename": "/data/shaders/react9.glsl", "start": 14318471, "end": 14319497}, {"filename": "/data/shaders/react_x_chromatic_split.glsl", "start": 14319497, "end": 14320592}, {"filename": "/data/shaders/react_x_double_spin.glsl", "start": 14320592, "end": 14321706}, {"filename": "/data/shaders/react_x_drum_zoom.glsl", "start": 14321706, "end": 14322325}, {"filename": "/data/shaders/react_x_glitch_peak.glsl", "start": 14322325, "end": 14323312}, {"filename": "/data/shaders/react_x_hash_glitch.glsl", "start": 14323312, "end": 14324361}, {"filename": "/data/shaders/react_x_moving_phase.glsl", "start": 14324361, "end": 14325592}, {"filename": "/data/shaders/react_x_multi_rainbow.glsl", "start": 14325592, "end": 14327557}, {"filename": "/data/shaders/react_x_noise_glitch.glsl", "start": 14327557, "end": 14328842}, {"filename": "/data/shaders/react_x_phase_stretch.glsl", "start": 14328842, "end": 14330456}, {"filename": "/data/shaders/react_x_pingpong_glitch.glsl", "start": 14330456, "end": 14331801}, {"filename": "/data/shaders/react_x_rainbow_beat.glsl", "start": 14331801, "end": 14333251}, {"filename": "/data/shaders/react_x_rainbow_flow.glsl", "start": 14333251, "end": 14334271}, {"filename": "/data/shaders/react_x_ring_spiral.glsl", "start": 14334271, "end": 14336156}, {"filename": "/data/shaders/react_x_ripple_bass.glsl", "start": 14336156, "end": 14336939}, {"filename": "/data/shaders/react_x_spin_amp.glsl", "start": 14336939, "end": 14337655}, {"filename": "/data/shaders/react_x_spiral_twist.glsl", "start": 14337655, "end": 14338423}, {"filename": "/data/shaders/react_x_swirl_deep.glsl", "start": 14338423, "end": 14339197}, {"filename": "/data/shaders/react_x_vert_warp.glsl", "start": 14339197, "end": 14340072}, {"filename": "/data/shaders/react_x_wave_distort.glsl", "start": 14340072, "end": 14340802}, {"filename": "/data/shaders/react_x_xor_gradient.glsl", "start": 14340802, "end": 14343253}, {"filename": "/data/shaders/recep-2.glsl", "start": 14343253, "end": 14343842}, {"filename": "/data/shaders/recep.glsl", "start": 14343842, "end": 14344384}, {"filename": "/data/shaders/rect1.glsl", "start": 14344384, "end": 14344746}, {"filename": "/data/shaders/rect_spiral.glsl", "start": 14344746, "end": 14345398}, {"filename": "/data/shaders/red.glsl", "start": 14345398, "end": 14346409}, {"filename": "/data/shaders/red_strobe.glsl", "start": 14346409, "end": 14347456}, {"filename": "/data/shaders/remove_flicker.glsl", "start": 14347456, "end": 14350119}, {"filename": "/data/shaders/reverse.glsl", "start": 14350119, "end": 14350647}, {"filename": "/data/shaders/reverse_alpha.glsl", "start": 14350647, "end": 14351283}, {"filename": "/data/shaders/reverse_vert.glsl", "start": 14351283, "end": 14351810}, {"filename": "/data/shaders/reverse_xor.glsl", "start": 14351810, "end": 14352845}, {"filename": "/data/shaders/reverse_xy.glsl", "start": 14352845, "end": 14353352}, {"filename": "/data/shaders/rgb.glsl", "start": 14353352, "end": 14354222}, {"filename": "/data/shaders/rgb_blur.glsl", "start": 14354222, "end": 14354970}, {"filename": "/data/shaders/rgb_blur_time.glsl", "start": 14354970, "end": 14355780}, {"filename": "/data/shaders/rgb_control.glsl", "start": 14355780, "end": 14356822}, {"filename": "/data/shaders/rgb_fade.glsl", "start": 14356822, "end": 14357514}, {"filename": "/data/shaders/rgb_fade_xor.glsl", "start": 14357514, "end": 14358582}, {"filename": "/data/shaders/rgb_time.glsl", "start": 14358582, "end": 14359500}, {"filename": "/data/shaders/rgb_time_xor.glsl", "start": 14359500, "end": 14360535}, {"filename": "/data/shaders/rgbchecker.glsl", "start": 14360535, "end": 14361517}, {"filename": "/data/shaders/rgbt.glsl", "start": 14361517, "end": 14362287}, {"filename": "/data/shaders/rgbvar_inc.glsl", "start": 14362287, "end": 14363343}, {"filename": "/data/shaders/rhue.glsl", "start": 14363343, "end": 14364359}, {"filename": "/data/shaders/rightintwo.glsl", "start": 14364359, "end": 14365479}, {"filename": "/data/shaders/ripple-amp.glsl", "start": 14365479, "end": 14366388}, {"filename": "/data/shaders/ripple.glsl", "start": 14366388, "end": 14367530}, {"filename": "/data/shaders/ripple_a.glsl", "start": 14367530, "end": 14368866}, {"filename": "/data/shaders/ripple_cache.glsl", "start": 14368866, "end": 14371186}, {"filename": "/data/shaders/ripple_cycle.glsl", "start": 14371186, "end": 14371603}, {"filename": "/data/shaders/ripple_cycle2.glsl", "start": 14371603, "end": 14372392}, {"filename": "/data/shaders/ripple_cycle_circle.glsl", "start": 14372392, "end": 14373550}, {"filename": "/data/shaders/ripple_cycle_prism.glsl", "start": 14373550, "end": 14374213}, {"filename": "/data/shaders/ripple_left.glsl", "start": 14374213, "end": 14374638}, {"filename": "/data/shaders/ripple_mouse2.glsl", "start": 14374638, "end": 14375686}, {"filename": "/data/shaders/ripple_out.glsl", "start": 14375686, "end": 14376065}, {"filename": "/data/shaders/ripple_outMouse.glsl", "start": 14376065, "end": 14377434}, {"filename": "/data/shaders/ripple_ppong.glsl", "start": 14377434, "end": 14378298}, {"filename": "/data/shaders/ripple_prisim.glsl", "start": 14378298, "end": 14379466}, {"filename": "/data/shaders/ripple_prism.glsl", "start": 14379466, "end": 14380225}, {"filename": "/data/shaders/ripple_rainbow.glsl", "start": 14380225, "end": 14382023}, {"filename": "/data/shaders/rotate_xyz.glsl", "start": 14382023, "end": 14383238}, {"filename": "/data/shaders/rotate_xyz_zoom.glsl", "start": 14383238, "end": 14384746}, {"filename": "/data/shaders/sac-geo-xor.glsl", "start": 14384746, "end": 14386370}, {"filename": "/data/shaders/sac-geo.glsl", "start": 14386370, "end": 14387550}, {"filename": "/data/shaders/sbrv.glsl", "start": 14387550, "end": 14387994}, {"filename": "/data/shaders/scale.glsl", "start": 14387994, "end": 14388926}, {"filename": "/data/shaders/scramble-2.glsl", "start": 14388926, "end": 14389584}, {"filename": "/data/shaders/scramble-3.glsl", "start": 14389584, "end": 14390170}, {"filename": "/data/shaders/scramble.glsl", "start": 14390170, "end": 14390560}, {"filename": "/data/shaders/seek.glsl", "start": 14390560, "end": 14392550}, {"filename": "/data/shaders/seek2.glsl", "start": 14392550, "end": 14394580}, {"filename": "/data/shaders/self_alphablend_scale.glsl", "start": 14394580, "end": 14395257}, {"filename": "/data/shaders/self_alphablend_scale_xor.glsl", "start": 14395257, "end": 14396248}, {"filename": "/data/shaders/sepia.glsl", "start": 14396248, "end": 14396714}, {"filename": "/data/shaders/set_swirl_pos_mosue.glsl", "start": 14396714, "end": 14397956}, {"filename": "/data/shaders/shader1.glsl", "start": 14397956, "end": 14398775}, {"filename": "/data/shaders/shaders.txt", "start": 14398775, "end": 14402753}, {"filename": "/data/shaders/shake.glsl", "start": 14402753, "end": 14403034}, {"filename": "/data/shaders/sheet.glsl", "start": 14403034, "end": 14403655}, {"filename": "/data/shaders/shift.glsl", "start": 14403655, "end": 14405697}, {"filename": "/data/shaders/shift_dark.glsl", "start": 14405697, "end": 14407886}, {"filename": "/data/shaders/shift_grad.glsl", "start": 14407886, "end": 14409468}, {"filename": "/data/shaders/shrink_texture.glsl", "start": 14409468, "end": 14409823}, {"filename": "/data/shaders/shrink_texture_4.glsl", "start": 14409823, "end": 14410159}, {"filename": "/data/shaders/sin.glsl", "start": 14410159, "end": 14410953}, {"filename": "/data/shaders/sin_delic.glsl", "start": 14410953, "end": 14411805}, {"filename": "/data/shaders/sine1.glsl", "start": 14411805, "end": 14412752}, {"filename": "/data/shaders/sine2.glsl", "start": 14412752, "end": 14413789}, {"filename": "/data/shaders/sine_grad.glsl", "start": 14413789, "end": 14414635}, {"filename": "/data/shaders/sine_grad2.glsl", "start": 14414635, "end": 14415733}, {"filename": "/data/shaders/sine_grad3.glsl", "start": 14415733, "end": 14416749}, {"filename": "/data/shaders/sine_grad4.glsl", "start": 14416749, "end": 14417783}, {"filename": "/data/shaders/sine_grad5.glsl", "start": 14417783, "end": 14418938}, {"filename": "/data/shaders/sine_grad6.glsl", "start": 14418938, "end": 14420260}, {"filename": "/data/shaders/sine_grad7.glsl", "start": 14420260, "end": 14421643}, {"filename": "/data/shaders/sine_ripple.glsl", "start": 14421643, "end": 14423086}, {"filename": "/data/shaders/sine_swirl.glsl", "start": 14423086, "end": 14424260}, {"filename": "/data/shaders/sine_time.glsl", "start": 14424260, "end": 14424769}, {"filename": "/data/shaders/sing.glsl", "start": 14424769, "end": 14425221}, {"filename": "/data/shaders/single-glittch.glsl", "start": 14425221, "end": 14426515}, {"filename": "/data/shaders/singleton-noise.glsl", "start": 14426515, "end": 14427783}, {"filename": "/data/shaders/skinny.glsl", "start": 14427783, "end": 14428097}, {"filename": "/data/shaders/slither.glsl", "start": 14428097, "end": 14429111}, {"filename": "/data/shaders/slither_vert.glsl", "start": 14429111, "end": 14430139}, {"filename": "/data/shaders/smoke.glsl", "start": 14430139, "end": 14431295}, {"filename": "/data/shaders/smooth_gl.glsl", "start": 14431295, "end": 14433965}, {"filename": "/data/shaders/smooth_pixel.glsl", "start": 14433965, "end": 14435113}, {"filename": "/data/shaders/smooth_var.glsl", "start": 14435113, "end": 14436707}, {"filename": "/data/shaders/snake.glsl", "start": 14436707, "end": 14437075}, {"filename": "/data/shaders/snake_dir.glsl", "start": 14437075, "end": 14437891}, {"filename": "/data/shaders/snake_updown.glsl", "start": 14437891, "end": 14438264}, {"filename": "/data/shaders/snes.glsl", "start": 14438264, "end": 14438636}, {"filename": "/data/shaders/sorted_i.txt", "start": 14438636, "end": 14454447}, {"filename": "/data/shaders/source_strobe.glsl", "start": 14454447, "end": 14455542}, {"filename": "/data/shaders/spaz.glsl", "start": 14455542, "end": 14456169}, {"filename": "/data/shaders/spectral_smear_cache.glsl", "start": 14456169, "end": 14458902}, {"filename": "/data/shaders/spectrum_cache1.glsl", "start": 14458902, "end": 14461643}, {"filename": "/data/shaders/spectrum_cache2.glsl", "start": 14461643, "end": 14464484}, {"filename": "/data/shaders/spectrum_cache3.glsl", "start": 14464484, "end": 14467791}, {"filename": "/data/shaders/spectrum_cache4.glsl", "start": 14467791, "end": 14470982}, {"filename": "/data/shaders/spiral-aura.glsl", "start": 14470982, "end": 14471759}, {"filename": "/data/shaders/spiral-center.glsl", "start": 14471759, "end": 14473447}, {"filename": "/data/shaders/spiral-code-chromatic-tunnel.glsl", "start": 14473447, "end": 14474797}, {"filename": "/data/shaders/spiral-code-cosmic-drain.glsl", "start": 14474797, "end": 14476254}, {"filename": "/data/shaders/spiral-code-crystal-maelstrom.glsl", "start": 14476254, "end": 14478022}, {"filename": "/data/shaders/spiral-code-double-helix.glsl", "start": 14478022, "end": 14479407}, {"filename": "/data/shaders/spiral-code-electric-spiral.glsl", "start": 14479407, "end": 14480870}, {"filename": "/data/shaders/spiral-code-flame-whorl.glsl", "start": 14480870, "end": 14482709}, {"filename": "/data/shaders/spiral-code-fractal-coil.glsl", "start": 14482709, "end": 14484001}, {"filename": "/data/shaders/spiral-code-galaxy-vortex.glsl", "start": 14484001, "end": 14485286}, {"filename": "/data/shaders/spiral-code-hypnotic-spiral.glsl", "start": 14485286, "end": 14486608}, {"filename": "/data/shaders/spiral-code-ice-spiral.glsl", "start": 14486608, "end": 14487991}, {"filename": "/data/shaders/spiral-code-kaleido-bloom.glsl", "start": 14487991, "end": 14489111}, {"filename": "/data/shaders/spiral-code-liquid-pinwheel.glsl", "start": 14489111, "end": 14490149}, {"filename": "/data/shaders/spiral-code-log-polar-echo.glsl", "start": 14490149, "end": 14491785}, {"filename": "/data/shaders/spiral-code-mirror-spiral.glsl", "start": 14491785, "end": 14492890}, {"filename": "/data/shaders/spiral-code-nautilus-glass.glsl", "start": 14492890, "end": 14494197}, {"filename": "/data/shaders/spiral-code-nebula-arms.glsl", "start": 14494197, "end": 14495955}, {"filename": "/data/shaders/spiral-code-orbital-spiral.glsl", "start": 14495955, "end": 14497613}, {"filename": "/data/shaders/spiral-code-pearl-swirl.glsl", "start": 14497613, "end": 14498932}, {"filename": "/data/shaders/spiral-code-prism-cyclone.glsl", "start": 14498932, "end": 14500143}, {"filename": "/data/shaders/spiral-code-quantum-spiral.glsl", "start": 14500143, "end": 14501667}, {"filename": "/data/shaders/spiral-code-ripple-coil.glsl", "start": 14501667, "end": 14502747}, {"filename": "/data/shaders/spiral-code-rose-vortex.glsl", "start": 14502747, "end": 14504056}, {"filename": "/data/shaders/spiral-code-silk-spiral.glsl", "start": 14504056, "end": 14505384}, {"filename": "/data/shaders/spiral-code-stardust-lens.glsl", "start": 14505384, "end": 14507042}, {"filename": "/data/shaders/spiral-code-tidal-spiral.glsl", "start": 14507042, "end": 14508222}, {"filename": "/data/shaders/spiral-mouse.glsl", "start": 14508222, "end": 14510099}, {"filename": "/data/shaders/spiral_by_time.glsl", "start": 14510099, "end": 14511546}, {"filename": "/data/shaders/spiral_edge.glsl", "start": 14511546, "end": 14512283}, {"filename": "/data/shaders/spiral_mirror.glsl", "start": 14512283, "end": 14513015}, {"filename": "/data/shaders/spiral_mirror_2.glsl", "start": 14513015, "end": 14514049}, {"filename": "/data/shaders/spiral_mirror_rev.glsl", "start": 14514049, "end": 14514785}, {"filename": "/data/shaders/spiral_mirror_top.glsl", "start": 14514785, "end": 14515554}, {"filename": "/data/shaders/spiral_mirror_wave.glsl", "start": 14515554, "end": 14516517}, {"filename": "/data/shaders/spiral_square.glsl", "start": 14516517, "end": 14516967}, {"filename": "/data/shaders/spiral_wave.glsl", "start": 14516967, "end": 14518687}, {"filename": "/data/shaders/spiral_wave_1.glsl", "start": 14518687, "end": 14519437}, {"filename": "/data/shaders/splash-x.glsl", "start": 14519437, "end": 14519889}, {"filename": "/data/shaders/splash-y.glsl", "start": 14519889, "end": 14520341}, {"filename": "/data/shaders/splash.glsl", "start": 14520341, "end": 14520786}, {"filename": "/data/shaders/square_grid.glsl", "start": 14520786, "end": 14522169}, {"filename": "/data/shaders/square_grid2.glsl", "start": 14522169, "end": 14523566}, {"filename": "/data/shaders/squares.glsl", "start": 14523566, "end": 14524626}, {"filename": "/data/shaders/srainbow.glsl", "start": 14524626, "end": 14525898}, {"filename": "/data/shaders/star.glsl", "start": 14525898, "end": 14526789}, {"filename": "/data/shaders/star5.glsl", "start": 14526789, "end": 14527838}, {"filename": "/data/shaders/starX.glsl", "start": 14527838, "end": 14528604}, {"filename": "/data/shaders/starX2.glsl", "start": 14528604, "end": 14529370}, {"filename": "/data/shaders/starX3.glsl", "start": 14529370, "end": 14530179}, {"filename": "/data/shaders/starX4.glsl", "start": 14530179, "end": 14530919}, {"filename": "/data/shaders/stardust.glsl", "start": 14530919, "end": 14532358}, {"filename": "/data/shaders/strobe.glsl", "start": 14532358, "end": 14532963}, {"filename": "/data/shaders/strobe_colors.glsl", "start": 14532963, "end": 14533966}, {"filename": "/data/shaders/strobe_in_out.glsl", "start": 14533966, "end": 14535054}, {"filename": "/data/shaders/strobe_light.glsl", "start": 14535054, "end": 14535628}, {"filename": "/data/shaders/strobe_x1.glsl", "start": 14535628, "end": 14536401}, {"filename": "/data/shaders/subtle_xor.glsl", "start": 14536401, "end": 14537317}, {"filename": "/data/shaders/sum.glsl", "start": 14537317, "end": 14540105}, {"filename": "/data/shaders/surround.glsl", "start": 14540105, "end": 14542117}, {"filename": "/data/shaders/sweb.glsl", "start": 14542117, "end": 14543387}, {"filename": "/data/shaders/swirlMouse.glsl", "start": 14543387, "end": 14545326}, {"filename": "/data/shaders/swirl_by_mouse.glsl", "start": 14545326, "end": 14547638}, {"filename": "/data/shaders/swirl_by_mouse2.glsl", "start": 14547638, "end": 14550088}, {"filename": "/data/shaders/swirl_by_mouse_full.glsl", "start": 14550088, "end": 14551805}, {"filename": "/data/shaders/swirl_by_mouse_pos.glsl", "start": 14551805, "end": 14553083}, {"filename": "/data/shaders/swirl_cache.glsl", "start": 14553083, "end": 14557258}, {"filename": "/data/shaders/system.log.txt", "start": 14557258, "end": 14573714}, {"filename": "/data/shaders/tan.glsl", "start": 14573714, "end": 14574508}, {"filename": "/data/shaders/tdye.glsl", "start": 14574508, "end": 14576174}, {"filename": "/data/shaders/tdye2.glsl", "start": 14576174, "end": 14579356}, {"filename": "/data/shaders/tear.glsl", "start": 14579356, "end": 14579771}, {"filename": "/data/shaders/tearing.glsl", "start": 14579771, "end": 14580211}, {"filename": "/data/shaders/tearing_max.glsl", "start": 14580211, "end": 14581069}, {"filename": "/data/shaders/tearing_single.glsl", "start": 14581069, "end": 14581321}, {"filename": "/data/shaders/tearing_spiral.glsl", "start": 14581321, "end": 14582405}, {"filename": "/data/shaders/temporal_prism_cache.glsl", "start": 14582405, "end": 14584786}, {"filename": "/data/shaders/test.glsl", "start": 14584786, "end": 14588143}, {"filename": "/data/shaders/test_cache.glsl", "start": 14588143, "end": 14590215}, {"filename": "/data/shaders/tex_fold.glsl", "start": 14590215, "end": 14590740}, {"filename": "/data/shaders/texture_scale.glsl", "start": 14590740, "end": 14591528}, {"filename": "/data/shaders/thick_glass.glsl", "start": 14591528, "end": 14592033}, {"filename": "/data/shaders/time_frac.glsl", "start": 14592033, "end": 14592591}, {"filename": "/data/shaders/time_frac_xor.glsl", "start": 14592591, "end": 14593584}, {"filename": "/data/shaders/time_xor.glsl", "start": 14593584, "end": 14594584}, {"filename": "/data/shaders/timeflash.glsl", "start": 14594584, "end": 14595571}, {"filename": "/data/shaders/timeval_alpha.glsl", "start": 14595571, "end": 14596560}, {"filename": "/data/shaders/today.glsl", "start": 14596560, "end": 14597237}, {"filename": "/data/shaders/tornado.glsl", "start": 14597237, "end": 14598427}, {"filename": "/data/shaders/tremor1.glsl", "start": 14598427, "end": 14599124}, {"filename": "/data/shaders/tremor2.glsl", "start": 14599124, "end": 14600058}, {"filename": "/data/shaders/tremor3.glsl", "start": 14600058, "end": 14600809}, {"filename": "/data/shaders/tremor4.glsl", "start": 14600809, "end": 14601759}, {"filename": "/data/shaders/tridim.glsl", "start": 14601759, "end": 14602473}, {"filename": "/data/shaders/tripple.glsl", "start": 14602473, "end": 14602853}, {"filename": "/data/shaders/tripple2.glsl", "start": 14602853, "end": 14603471}, {"filename": "/data/shaders/triwavedistort.glsl", "start": 14603471, "end": 14603993}, {"filename": "/data/shaders/tunnel_x.glsl", "start": 14603993, "end": 14605973}, {"filename": "/data/shaders/tv_show.glsl", "start": 14605973, "end": 14606671}, {"filename": "/data/shaders/twarp.glsl", "start": 14606671, "end": 14607001}, {"filename": "/data/shaders/twarp2.glsl", "start": 14607001, "end": 14607345}, {"filename": "/data/shaders/twirl_tex.glsl", "start": 14607345, "end": 14607935}, {"filename": "/data/shaders/twist-code-chromatic-vortex.glsl", "start": 14607935, "end": 14608906}, {"filename": "/data/shaders/twist-code-cosmic-auger.glsl", "start": 14608906, "end": 14610303}, {"filename": "/data/shaders/twist-code-deep-space-drill.glsl", "start": 14610303, "end": 14611411}, {"filename": "/data/shaders/twist-code-double-helix.glsl", "start": 14611411, "end": 14612447}, {"filename": "/data/shaders/twist-code-event-horizon.glsl", "start": 14612447, "end": 14613599}, {"filename": "/data/shaders/twist-code-feedback-spiral.glsl", "start": 14613599, "end": 14614642}, {"filename": "/data/shaders/twist-code-fractal-conduit.glsl", "start": 14614642, "end": 14615762}, {"filename": "/data/shaders/twist-code-gravity-well.glsl", "start": 14615762, "end": 14616858}, {"filename": "/data/shaders/twist-code-hypercube-vortex.glsl", "start": 14616858, "end": 14618238}, {"filename": "/data/shaders/twist-code-infinite-bore.glsl", "start": 14618238, "end": 14619441}, {"filename": "/data/shaders/twist-code-kaleido-throat.glsl", "start": 14619441, "end": 14620413}, {"filename": "/data/shaders/twist-code-mirror-abyss.glsl", "start": 14620413, "end": 14621363}, {"filename": "/data/shaders/twist-code-neon-collapse.glsl", "start": 14621363, "end": 14622362}, {"filename": "/data/shaders/twist-code-octave-wormhole.glsl", "start": 14622362, "end": 14623405}, {"filename": "/data/shaders/twist-code-orbital-rip.glsl", "start": 14623405, "end": 14624441}, {"filename": "/data/shaders/twist-code-parallel-helix.glsl", "start": 14624441, "end": 14625532}, {"filename": "/data/shaders/twist-code-phase-cyclone.glsl", "start": 14625532, "end": 14626507}, {"filename": "/data/shaders/twist-code-prism-singularity.glsl", "start": 14626507, "end": 14627591}, {"filename": "/data/shaders/twist-code-quantum-tunnel.glsl", "start": 14627591, "end": 14628598}, {"filename": "/data/shaders/twist-code-radial-overdrive.glsl", "start": 14628598, "end": 14629533}, {"filename": "/data/shaders/twist-code-serpent-tunnel.glsl", "start": 14629533, "end": 14630542}, {"filename": "/data/shaders/twist-code-singularity-maelstrom.glsl", "start": 14630542, "end": 14631776}, {"filename": "/data/shaders/twist-code-spiral-shockwave.glsl", "start": 14631776, "end": 14632740}, {"filename": "/data/shaders/twist-code-tidal-twister.glsl", "start": 14632740, "end": 14633724}, {"filename": "/data/shaders/twist-code-turbine-rift.glsl", "start": 14633724, "end": 14634718}, {"filename": "/data/shaders/twist.glsl", "start": 14634718, "end": 14635668}, {"filename": "/data/shaders/twist_audio.glsl", "start": 14635668, "end": 14636680}, {"filename": "/data/shaders/twist_audio_ex_bass_pulse.glsl", "start": 14636680, "end": 14637593}, {"filename": "/data/shaders/twist_audio_ex_bass_quake.glsl", "start": 14637593, "end": 14638573}, {"filename": "/data/shaders/twist_audio_ex_chromatic.glsl", "start": 14638573, "end": 14639624}, {"filename": "/data/shaders/twist_audio_ex_color_split.glsl", "start": 14639624, "end": 14640528}, {"filename": "/data/shaders/twist_audio_ex_concentric_rings.glsl", "start": 14640528, "end": 14641469}, {"filename": "/data/shaders/twist_audio_ex_dual_axis.glsl", "start": 14641469, "end": 14642381}, {"filename": "/data/shaders/twist_audio_ex_fbm_audio.glsl", "start": 14642381, "end": 14643548}, {"filename": "/data/shaders/twist_audio_ex_freq_layers.glsl", "start": 14643548, "end": 14644520}, {"filename": "/data/shaders/twist_audio_ex_inverse_radius.glsl", "start": 14644520, "end": 14645283}, {"filename": "/data/shaders/twist_audio_ex_kaleido_freq.glsl", "start": 14645283, "end": 14646281}, {"filename": "/data/shaders/twist_audio_ex_mid_swirl.glsl", "start": 14646281, "end": 14647290}, {"filename": "/data/shaders/twist_audio_ex_mirror_audio.glsl", "start": 14647290, "end": 14648258}, {"filename": "/data/shaders/twist_audio_ex_octave_stack.glsl", "start": 14648258, "end": 14649252}, {"filename": "/data/shaders/twist_audio_ex_polar_freq.glsl", "start": 14649252, "end": 14650140}, {"filename": "/data/shaders/twist_audio_ex_quad_freq.glsl", "start": 14650140, "end": 14651194}, {"filename": "/data/shaders/twist_audio_ex_radial_bands.glsl", "start": 14651194, "end": 14652011}, {"filename": "/data/shaders/twist_audio_ex_ripple_freq.glsl", "start": 14652011, "end": 14653021}, {"filename": "/data/shaders/twist_audio_ex_sine_audio.glsl", "start": 14653021, "end": 14654001}, {"filename": "/data/shaders/twist_audio_ex_spectrum_spiral.glsl", "start": 14654001, "end": 14654864}, {"filename": "/data/shaders/twist_audio_ex_spiral_zoom.glsl", "start": 14654864, "end": 14655715}, {"filename": "/data/shaders/twist_audio_ex_treble_jitter.glsl", "start": 14655715, "end": 14656646}, {"filename": "/data/shaders/twist_audio_ex_treble_static.glsl", "start": 14656646, "end": 14657604}, {"filename": "/data/shaders/twist_audio_ex_vortex_pump.glsl", "start": 14657604, "end": 14658450}, {"filename": "/data/shaders/twist_audio_ex_wave_collapse.glsl", "start": 14658450, "end": 14659395}, {"filename": "/data/shaders/twist_audio_ex_zoom_beat.glsl", "start": 14659395, "end": 14660202}, {"filename": "/data/shaders/twist_directions.glsl", "start": 14660202, "end": 14661295}, {"filename": "/data/shaders/twist_full.glsl", "start": 14661295, "end": 14662307}, {"filename": "/data/shaders/twist_gpt.glsl", "start": 14662307, "end": 14663798}, {"filename": "/data/shaders/twist_gpt_full.glsl", "start": 14663798, "end": 14665310}, {"filename": "/data/shaders/twistex.glsl", "start": 14665310, "end": 14666484}, {"filename": "/data/shaders/underwater.glsl", "start": 14666484, "end": 14667713}, {"filename": "/data/shaders/underwaterenchanced.glsl", "start": 14667713, "end": 14668788}, {"filename": "/data/shaders/vertex.glsl", "start": 14668788, "end": 14669037}, {"filename": "/data/shaders/vhs-code-chroma-bleed.glsl", "start": 14669037, "end": 14670468}, {"filename": "/data/shaders/vhs-code-dropout.glsl", "start": 14670468, "end": 14671814}, {"filename": "/data/shaders/vhs-code-ghosting.glsl", "start": 14671814, "end": 14673144}, {"filename": "/data/shaders/vhs-code-head-switch.glsl", "start": 14673144, "end": 14674429}, {"filename": "/data/shaders/vhs-code-home-movie.glsl", "start": 14674429, "end": 14675800}, {"filename": "/data/shaders/vhs-code-kung-fury.glsl", "start": 14675800, "end": 14679150}, {"filename": "/data/shaders/vhs-code-pause-jitter.glsl", "start": 14679150, "end": 14680509}, {"filename": "/data/shaders/vhs-code-rf-noise.glsl", "start": 14680509, "end": 14681840}, {"filename": "/data/shaders/vhs-code-tape-warp.glsl", "start": 14681840, "end": 14683126}, {"filename": "/data/shaders/vhs-code-tracking-roll.glsl", "start": 14683126, "end": 14684374}, {"filename": "/data/shaders/vhs-code-worn-tape.glsl", "start": 14684374, "end": 14685708}, {"filename": "/data/shaders/vhs-color-mode.glsl", "start": 14685708, "end": 14688863}, {"filename": "/data/shaders/vhs-palette.glsl", "start": 14688863, "end": 14689177}, {"filename": "/data/shaders/vhs.glsl", "start": 14689177, "end": 14690059}, {"filename": "/data/shaders/vhs2.glsl", "start": 14690059, "end": 14693549}, {"filename": "/data/shaders/vhs_analog.glsl", "start": 14693549, "end": 14696316}, {"filename": "/data/shaders/vhs_color.glsl", "start": 14696316, "end": 14696874}, {"filename": "/data/shaders/vhs_damage.glsl", "start": 14696874, "end": 14699160}, {"filename": "/data/shaders/vhs_lofi.glsl", "start": 14699160, "end": 14702294}, {"filename": "/data/shaders/warp-atan.glsl", "start": 14702294, "end": 14703127}, {"filename": "/data/shaders/warp-ppong-time.glsl", "start": 14703127, "end": 14703996}, {"filename": "/data/shaders/warp_ppong.glsl", "start": 14703996, "end": 14704508}, {"filename": "/data/shaders/warp_ppong_by_mouse.glsl", "start": 14704508, "end": 14705344}, {"filename": "/data/shaders/warp_ppong_mouse.glsl", "start": 14705344, "end": 14707873}, {"filename": "/data/shaders/warp_tunnel.glsl", "start": 14707873, "end": 14708761}, {"filename": "/data/shaders/warpcursor.glsl", "start": 14708761, "end": 14709855}, {"filename": "/data/shaders/water-cursor.glsl", "start": 14709855, "end": 14711157}, {"filename": "/data/shaders/water-fold-ex..glsl", "start": 14711157, "end": 14712597}, {"filename": "/data/shaders/water.glsl", "start": 14712597, "end": 14713114}, {"filename": "/data/shaders/water_full.glsl", "start": 14713114, "end": 14713659}, {"filename": "/data/shaders/water_hq_01_caustic_shallows.glsl", "start": 14713659, "end": 14714501}, {"filename": "/data/shaders/water_hq_02_ocean_swell.glsl", "start": 14714501, "end": 14715241}, {"filename": "/data/shaders/water_hq_03_rain_ripples.glsl", "start": 14715241, "end": 14716269}, {"filename": "/data/shaders/water_hq_04_glass_refraction.glsl", "start": 14716269, "end": 14717172}, {"filename": "/data/shaders/water_hq_05_deep_current.glsl", "start": 14717172, "end": 14718294}, {"filename": "/data/shaders/water_hq_06_pool_caustics.glsl", "start": 14718294, "end": 14719095}, {"filename": "/data/shaders/water_hq_07_river_flow.glsl", "start": 14719095, "end": 14720099}, {"filename": "/data/shaders/water_hq_08_whirlpool.glsl", "start": 14720099, "end": 14720956}, {"filename": "/data/shaders/water_hq_09_choppy_surface.glsl", "start": 14720956, "end": 14721811}, {"filename": "/data/shaders/water_hq_10_calm_lake.glsl", "start": 14721811, "end": 14722709}, {"filename": "/data/shaders/water_hq_11_tidal_lens.glsl", "start": 14722709, "end": 14723448}, {"filename": "/data/shaders/water_hq_12_cross_sea.glsl", "start": 14723448, "end": 14724223}, {"filename": "/data/shaders/water_hq_13_underwater_drift.glsl", "start": 14724223, "end": 14725303}, {"filename": "/data/shaders/water_hq_14_droplet_lenses.glsl", "start": 14725303, "end": 14726373}, {"filename": "/data/shaders/water_hq_15_bubble_stream.glsl", "start": 14726373, "end": 14727471}, {"filename": "/data/shaders/water_hq_16_shoreline.glsl", "start": 14727471, "end": 14728362}, {"filename": "/data/shaders/water_hq_17_liquid_mirror.glsl", "start": 14728362, "end": 14729236}, {"filename": "/data/shaders/water_hq_18_storm_surface.glsl", "start": 14729236, "end": 14730157}, {"filename": "/data/shaders/water_hq_19_crystal_water.glsl", "start": 14730157, "end": 14731388}, {"filename": "/data/shaders/water_hq_20_moonlit_water.glsl", "start": 14731388, "end": 14732310}, {"filename": "/data/shaders/water_hq_21_aqua_prism.glsl", "start": 14732310, "end": 14733159}, {"filename": "/data/shaders/water_hq_22_thermal_spring.glsl", "start": 14733159, "end": 14734185}, {"filename": "/data/shaders/water_hq_23_silk_current.glsl", "start": 14734185, "end": 14735036}, {"filename": "/data/shaders/water_hq_24_waterfall.glsl", "start": 14735036, "end": 14736209}, {"filename": "/data/shaders/water_hq_25_coral_lagoon.glsl", "start": 14736209, "end": 14737162}, {"filename": "/data/shaders/water_prism.glsl", "start": 14737162, "end": 14738505}, {"filename": "/data/shaders/water_r.glsl", "start": 14738505, "end": 14740028}, {"filename": "/data/shaders/water_rgb.glsl", "start": 14740028, "end": 14742197}, {"filename": "/data/shaders/waterbend.glsl", "start": 14742197, "end": 14743991}, {"filename": "/data/shaders/wave-l.glsl", "start": 14743991, "end": 14744340}, {"filename": "/data/shaders/wave_diag.glsl", "start": 14744340, "end": 14745042}, {"filename": "/data/shaders/wave_spiral.glsl", "start": 14745042, "end": 14745860}, {"filename": "/data/shaders/webgl_cache/4ac_rand.glsl", "start": 14745860, "end": 14749460}, {"filename": "/data/shaders/webgl_cache/8bit-norm.glsl", "start": 14749460, "end": 14752712}, {"filename": "/data/shaders/webgl_cache/8bit.glsl", "start": 14752712, "end": 14756089}, {"filename": "/data/shaders/webgl_cache/8huri.glsl", "start": 14756089, "end": 14759890}, {"filename": "/data/shaders/webgl_cache/BubbleMouse.glsl", "start": 14759890, "end": 14763953}, {"filename": "/data/shaders/webgl_cache/Dispersion.glsl", "start": 14763953, "end": 14770650}, {"filename": "/data/shaders/webgl_cache/DispersionRotate.glsl", "start": 14770650, "end": 14777538}, {"filename": "/data/shaders/webgl_cache/DispersionX.glsl", "start": 14777538, "end": 14782720}, {"filename": "/data/shaders/webgl_cache/DispersionY.glsl", "start": 14782720, "end": 14787902}, {"filename": "/data/shaders/webgl_cache/DispersionZ.glsl", "start": 14787902, "end": 14793084}, {"filename": "/data/shaders/webgl_cache/Electric_Fold.glsl", "start": 14793084, "end": 14801584}, {"filename": "/data/shaders/webgl_cache/Electric_Fold_Texture.glsl", "start": 14801584, "end": 14809403}, {"filename": "/data/shaders/webgl_cache/Fractal1.glsl", "start": 14809403, "end": 14812955}, {"filename": "/data/shaders/webgl_cache/HyperFocusTrails.glsl", "start": 14812955, "end": 14819174}, {"filename": "/data/shaders/webgl_cache/HyperVortex.glsl", "start": 14819174, "end": 14825621}, {"filename": "/data/shaders/webgl_cache/Light_Rainbow_Swirl.glsl", "start": 14825621, "end": 14832780}, {"filename": "/data/shaders/webgl_cache/Liquid_Censorship.glsl", "start": 14832780, "end": 14836720}, {"filename": "/data/shaders/webgl_cache/Liquid_Crystal.glsl", "start": 14836720, "end": 14844775}, {"filename": "/data/shaders/webgl_cache/Liquid_Crystal_2.glsl", "start": 14844775, "end": 14852855}, {"filename": "/data/shaders/webgl_cache/Liquid_Heat.glsl", "start": 14852855, "end": 14861738}, {"filename": "/data/shaders/webgl_cache/Liquid_Heat_blend.glsl", "start": 14861738, "end": 14870644}, {"filename": "/data/shaders/webgl_cache/Liquid_Light_Rainbow_Blend.glsl", "start": 14870644, "end": 14878376}, {"filename": "/data/shaders/webgl_cache/Liquid_Light_Time.glsl", "start": 14878376, "end": 14886922}, {"filename": "/data/shaders/webgl_cache/Liquid_Rainbow_Wave.glsl", "start": 14886922, "end": 14894979}, {"filename": "/data/shaders/webgl_cache/PI_x1.mp4.glsl", "start": 14894979, "end": 14900901}, {"filename": "/data/shaders/webgl_cache/Temporal.glsl", "start": 14900901, "end": 14905415}, {"filename": "/data/shaders/webgl_cache/XorKern.glsl", "start": 14905415, "end": 14911332}, {"filename": "/data/shaders/webgl_cache/XorKernBrighter.glsl", "start": 14911332, "end": 14917350}, {"filename": "/data/shaders/webgl_cache/acid_color2.glsl", "start": 14917350, "end": 14920867}, {"filename": "/data/shaders/webgl_cache/acidcam.glsl", "start": 14920867, "end": 14923387}, {"filename": "/data/shaders/webgl_cache/acidcolor.glsl", "start": 14923387, "end": 14926603}, {"filename": "/data/shaders/webgl_cache/addup_blend.glsl", "start": 14926603, "end": 14929894}, {"filename": "/data/shaders/webgl_cache/addup_cos.glsl", "start": 14929894, "end": 14933185}, {"filename": "/data/shaders/webgl_cache/af_scale2.glsl", "start": 14933185, "end": 14937769}, {"filename": "/data/shaders/webgl_cache/af_scale3.glsl", "start": 14937769, "end": 14942371}, {"filename": "/data/shaders/webgl_cache/af_scale_pulse.glsl", "start": 14942371, "end": 14946976}, {"filename": "/data/shaders/webgl_cache/af_scale_puple.glsl", "start": 14946976, "end": 14952263}, {"filename": "/data/shaders/webgl_cache/af_scale_spectrum.glsl", "start": 14952263, "end": 14957032}, {"filename": "/data/shaders/webgl_cache/air.glsl", "start": 14957032, "end": 14960069}, {"filename": "/data/shaders/webgl_cache/air_full.glsl", "start": 14960069, "end": 14963087}, {"filename": "/data/shaders/webgl_cache/air_full_mouse.glsl", "start": 14963087, "end": 14966450}, {"filename": "/data/shaders/webgl_cache/all_colors.glsl", "start": 14966450, "end": 14971515}, {"filename": "/data/shaders/webgl_cache/alpha_diamond.glsl", "start": 14971515, "end": 14975122}, {"filename": "/data/shaders/webgl_cache/alpha_sin_os.glsl", "start": 14975122, "end": 14978009}, {"filename": "/data/shaders/webgl_cache/analog.glsl", "start": 14978009, "end": 14981151}, {"filename": "/data/shaders/webgl_cache/and_smooth.glsl", "start": 14981151, "end": 14985668}, {"filename": "/data/shaders/webgl_cache/apart.glsl", "start": 14985668, "end": 14988885}, {"filename": "/data/shaders/webgl_cache/apart_mouse.glsl", "start": 14988885, "end": 14992693}, {"filename": "/data/shaders/webgl_cache/atan-bowl.glsl", "start": 14992693, "end": 14996436}, {"filename": "/data/shaders/webgl_cache/atan-glitch.glsl", "start": 14996436, "end": 15001033}, {"filename": "/data/shaders/webgl_cache/aura2.glsl", "start": 15001033, "end": 15009391}, {"filename": "/data/shaders/webgl_cache/aura3.glsl", "start": 15009391, "end": 15012604}, {"filename": "/data/shaders/webgl_cache/aura4.glsl", "start": 15012604, "end": 15016800}, {"filename": "/data/shaders/webgl_cache/balloon.glsl", "start": 15016800, "end": 15020024}, {"filename": "/data/shaders/webgl_cache/bend2.glsl", "start": 15020024, "end": 15023171}, {"filename": "/data/shaders/webgl_cache/bend2mouse.glsl", "start": 15023171, "end": 15027242}, {"filename": "/data/shaders/webgl_cache/bend_dir.glsl", "start": 15027242, "end": 15030873}, {"filename": "/data/shaders/webgl_cache/bend_rev.glsl", "start": 15030873, "end": 15033999}, {"filename": "/data/shaders/webgl_cache/bi_cycle.glsl", "start": 15033999, "end": 15037122}, {"filename": "/data/shaders/webgl_cache/bit_rainbow.glsl", "start": 15037122, "end": 15040613}, {"filename": "/data/shaders/webgl_cache/blackhole.glsl", "start": 15040613, "end": 15043570}, {"filename": "/data/shaders/webgl_cache/blank.glsl", "start": 15043570, "end": 15048304}, {"filename": "/data/shaders/webgl_cache/block_pixels.glsl", "start": 15048304, "end": 15051057}, {"filename": "/data/shaders/webgl_cache/blurry.glsl", "start": 15051057, "end": 15054159}, {"filename": "/data/shaders/webgl_cache/bowl-by_time.glsl", "start": 15054159, "end": 15057680}, {"filename": "/data/shaders/webgl_cache/bowl.glsl", "start": 15057680, "end": 15061229}, {"filename": "/data/shaders/webgl_cache/bright.glsl", "start": 15061229, "end": 15064493}, {"filename": "/data/shaders/webgl_cache/bright_rainbow.glsl", "start": 15064493, "end": 15067590}, {"filename": "/data/shaders/webgl_cache/brighten.glsl", "start": 15067590, "end": 15070566}, {"filename": "/data/shaders/webgl_cache/brighten_rev.glsl", "start": 15070566, "end": 15073560}, {"filename": "/data/shaders/webgl_cache/brokenglass.glsl", "start": 15073560, "end": 15077275}, {"filename": "/data/shaders/webgl_cache/brot-zoom-mouse.glsl", "start": 15077275, "end": 15081036}, {"filename": "/data/shaders/webgl_cache/bubble-2.glsl", "start": 15081036, "end": 15084127}, {"filename": "/data/shaders/webgl_cache/bubble-move.glsl", "start": 15084127, "end": 15087526}, {"filename": "/data/shaders/webgl_cache/bubble-zoom-mouse.glsl", "start": 15087526, "end": 15091077}, {"filename": "/data/shaders/webgl_cache/bubble.glsl", "start": 15091077, "end": 15093904}, {"filename": "/data/shaders/webgl_cache/bubbleGL.glsl", "start": 15093904, "end": 15097317}, {"filename": "/data/shaders/webgl_cache/bubble_amp.glsl", "start": 15097317, "end": 15100550}, {"filename": "/data/shaders/webgl_cache/c_ripple.glsl", "start": 15100550, "end": 15103486}, {"filename": "/data/shaders/webgl_cache/cane.glsl", "start": 15103486, "end": 15106723}, {"filename": "/data/shaders/webgl_cache/cd.glsl", "start": 15106723, "end": 15109573}, {"filename": "/data/shaders/webgl_cache/cd_zoom.glsl", "start": 15109573, "end": 15112525}, {"filename": "/data/shaders/webgl_cache/cd_zoom_out.glsl", "start": 15112525, "end": 15115452}, {"filename": "/data/shaders/webgl_cache/chue.glsl", "start": 15115452, "end": 15118690}, {"filename": "/data/shaders/webgl_cache/cmod.glsl", "start": 15118690, "end": 15123159}, {"filename": "/data/shaders/webgl_cache/code_flux_mouse.glsl", "start": 15123159, "end": 15126997}, {"filename": "/data/shaders/webgl_cache/code_rev3.glsl", "start": 15126997, "end": 15129992}, {"filename": "/data/shaders/webgl_cache/code_wave.glsl", "start": 15129992, "end": 15132725}, {"filename": "/data/shaders/webgl_cache/code_wave_rev.glsl", "start": 15132725, "end": 15135466}, {"filename": "/data/shaders/webgl_cache/code_wave_side.glsl", "start": 15135466, "end": 15138205}, {"filename": "/data/shaders/webgl_cache/codex_aurora_vortex.glsl", "start": 15138205, "end": 15141910}, {"filename": "/data/shaders/webgl_cache/codex_cartoon.glsl", "start": 15141910, "end": 15147236}, {"filename": "/data/shaders/webgl_cache/codex_cartoon_color.glsl", "start": 15147236, "end": 15153052}, {"filename": "/data/shaders/webgl_cache/codex_cartoon_saturday.glsl", "start": 15153052, "end": 15159636}, {"filename": "/data/shaders/webgl_cache/codex_cartoon_vhs.glsl", "start": 15159636, "end": 15167759}, {"filename": "/data/shaders/webgl_cache/codex_chrome_spiderweb.glsl", "start": 15167759, "end": 15171356}, {"filename": "/data/shaders/webgl_cache/codex_complex_aura.glsl", "start": 15171356, "end": 15177659}, {"filename": "/data/shaders/webgl_cache/codex_fractal_orchid.glsl", "start": 15177659, "end": 15181305}, {"filename": "/data/shaders/webgl_cache/codex_glass_mosaic_flow.glsl", "start": 15181305, "end": 15184952}, {"filename": "/data/shaders/webgl_cache/codex_glitch_bloom_wormhole.glsl", "start": 15184952, "end": 15188858}, {"filename": "/data/shaders/webgl_cache/codex_glitch_folded_scanburn.glsl", "start": 15188858, "end": 15192571}, {"filename": "/data/shaders/webgl_cache/codex_glitch_fractal_datamosh.glsl", "start": 15192571, "end": 15196406}, {"filename": "/data/shaders/webgl_cache/codex_glitch_kaleido_tear.glsl", "start": 15196406, "end": 15200253}, {"filename": "/data/shaders/webgl_cache/codex_glitch_logpolar_rift.glsl", "start": 15200253, "end": 15203855}, {"filename": "/data/shaders/webgl_cache/codex_glitch_mandel_wound.glsl", "start": 15203855, "end": 15207628}, {"filename": "/data/shaders/webgl_cache/codex_glitch_neon_escape.glsl", "start": 15207628, "end": 15211287}, {"filename": "/data/shaders/webgl_cache/codex_glitch_psychic_tiles.glsl", "start": 15211287, "end": 15214983}, {"filename": "/data/shaders/webgl_cache/codex_glitch_recursive_xor.glsl", "start": 15214983, "end": 15218680}, {"filename": "/data/shaders/webgl_cache/codex_glitch_shard_stutter.glsl", "start": 15218680, "end": 15222434}, {"filename": "/data/shaders/webgl_cache/codex_grad_fractal_aurora_mandala.glsl", "start": 15222434, "end": 15226076}, {"filename": "/data/shaders/webgl_cache/codex_grad_fractal_crystal_bloom.glsl", "start": 15226076, "end": 15229666}, {"filename": "/data/shaders/webgl_cache/codex_grad_fractal_electric_garden.glsl", "start": 15229666, "end": 15233519}, {"filename": "/data/shaders/webgl_cache/codex_grad_fractal_hypernova_eye.glsl", "start": 15233519, "end": 15237274}, {"filename": "/data/shaders/webgl_cache/codex_grad_fractal_inferno_lattice.glsl", "start": 15237274, "end": 15240984}, {"filename": "/data/shaders/webgl_cache/codex_grad_fractal_nebula_gate.glsl", "start": 15240984, "end": 15244661}, {"filename": "/data/shaders/webgl_cache/codex_grad_fractal_oceanic_void.glsl", "start": 15244661, "end": 15248243}, {"filename": "/data/shaders/webgl_cache/codex_grad_fractal_prism_storm.glsl", "start": 15248243, "end": 15252030}, {"filename": "/data/shaders/webgl_cache/codex_grad_fractal_royal_shatter.glsl", "start": 15252030, "end": 15255876}, {"filename": "/data/shaders/webgl_cache/codex_grad_fractal_solar_crown.glsl", "start": 15255876, "end": 15259553}, {"filename": "/data/shaders/webgl_cache/codex_liquid_tessellation.glsl", "start": 15259553, "end": 15263599}, {"filename": "/data/shaders/webgl_cache/codex_neon_topography.glsl", "start": 15263599, "end": 15267134}, {"filename": "/data/shaders/webgl_cache/codex_polar_echo_lens.glsl", "start": 15267134, "end": 15270840}, {"filename": "/data/shaders/webgl_cache/codex_prism_feedback.glsl", "start": 15270840, "end": 15274671}, {"filename": "/data/shaders/webgl_cache/codex_quantum_bloom.glsl", "start": 15274671, "end": 15278319}, {"filename": "/data/shaders/webgl_cache/codex_signal_decay.glsl", "start": 15278319, "end": 15281945}, {"filename": "/data/shaders/webgl_cache/codex_vhs_chroma_bleed.glsl", "start": 15281945, "end": 15285318}, {"filename": "/data/shaders/webgl_cache/codex_vhs_crt_combo.glsl", "start": 15285318, "end": 15288968}, {"filename": "/data/shaders/webgl_cache/codex_vhs_ghost_luma.glsl", "start": 15288968, "end": 15292373}, {"filename": "/data/shaders/webgl_cache/codex_vhs_head_switch.glsl", "start": 15292373, "end": 15295822}, {"filename": "/data/shaders/webgl_cache/codex_vhs_magnetic_dropouts.glsl", "start": 15295822, "end": 15299261}, {"filename": "/data/shaders/webgl_cache/codex_vhs_midnight_dub.glsl", "start": 15299261, "end": 15302915}, {"filename": "/data/shaders/webgl_cache/codex_vhs_pause_jitter.glsl", "start": 15302915, "end": 15306296}, {"filename": "/data/shaders/webgl_cache/codex_vhs_rainbow_noise.glsl", "start": 15306296, "end": 15309611}, {"filename": "/data/shaders/webgl_cache/codex_vhs_tape_warp.glsl", "start": 15309611, "end": 15313014}, {"filename": "/data/shaders/webgl_cache/codex_vhs_tracking_roll.glsl", "start": 15313014, "end": 15316649}, {"filename": "/data/shaders/webgl_cache/color-bloom.glsl", "start": 15316649, "end": 15320189}, {"filename": "/data/shaders/webgl_cache/color-f-diag.glsl", "start": 15320189, "end": 15323206}, {"filename": "/data/shaders/webgl_cache/color-f.glsl", "start": 15323206, "end": 15326203}, {"filename": "/data/shaders/webgl_cache/color-f2.glsl", "start": 15326203, "end": 15329350}, {"filename": "/data/shaders/webgl_cache/color-mod-dark.glsl", "start": 15329350, "end": 15332599}, {"filename": "/data/shaders/webgl_cache/color-mod.glsl", "start": 15332599, "end": 15335764}, {"filename": "/data/shaders/webgl_cache/color-o.glsl", "start": 15335764, "end": 15338750}, {"filename": "/data/shaders/webgl_cache/color_effect.glsl", "start": 15338750, "end": 15342383}, {"filename": "/data/shaders/webgl_cache/color_effect_xor.glsl", "start": 15342383, "end": 15346048}, {"filename": "/data/shaders/webgl_cache/color_g.glsl", "start": 15346048, "end": 15349939}, {"filename": "/data/shaders/webgl_cache/color_grad1.glsl", "start": 15349939, "end": 15352972}, {"filename": "/data/shaders/webgl_cache/color_grad_rainbow.glsl", "start": 15352972, "end": 15357318}, {"filename": "/data/shaders/webgl_cache/color_increase.glsl", "start": 15357318, "end": 15360244}, {"filename": "/data/shaders/webgl_cache/color_l.glsl", "start": 15360244, "end": 15364239}, {"filename": "/data/shaders/webgl_cache/color_pool.glsl", "start": 15364239, "end": 15367500}, {"filename": "/data/shaders/webgl_cache/color_swirl-o1.glsl", "start": 15367500, "end": 15370847}, {"filename": "/data/shaders/webgl_cache/color_swirl_beautiful.glsl", "start": 15370847, "end": 15374574}, {"filename": "/data/shaders/webgl_cache/color_swirl_c.glsl", "start": 15374574, "end": 15378336}, {"filename": "/data/shaders/webgl_cache/color_swirl_g.glsl", "start": 15378336, "end": 15382484}, {"filename": "/data/shaders/webgl_cache/color_swirl_o1_mouse.glsl", "start": 15382484, "end": 15385905}, {"filename": "/data/shaders/webgl_cache/color_swirl_sin.glsl", "start": 15385905, "end": 15389284}, {"filename": "/data/shaders/webgl_cache/color_swirl_sin_mouse.glsl", "start": 15389284, "end": 15392752}, {"filename": "/data/shaders/webgl_cache/color_w.glsl", "start": 15392752, "end": 15397046}, {"filename": "/data/shaders/webgl_cache/color_w2.glsl", "start": 15397046, "end": 15400570}, {"filename": "/data/shaders/webgl_cache/color_w_mouse.glsl", "start": 15400570, "end": 15404936}, {"filename": "/data/shaders/webgl_cache/colordivflash.glsl", "start": 15404936, "end": 15408182}, {"filename": "/data/shaders/webgl_cache/comb.glsl", "start": 15408182, "end": 15411692}, {"filename": "/data/shaders/webgl_cache/comb3-frac-mouse.glsl", "start": 15411692, "end": 15416046}, {"filename": "/data/shaders/webgl_cache/comb3-frac-mouse2.glsl", "start": 15416046, "end": 15420400}, {"filename": "/data/shaders/webgl_cache/comb3.glsl", "start": 15420400, "end": 15424463}, {"filename": "/data/shaders/webgl_cache/comb3_geo_mouse.glsl", "start": 15424463, "end": 15429986}, {"filename": "/data/shaders/webgl_cache/comb3_mouse.glsl", "start": 15429986, "end": 15434340}, {"filename": "/data/shaders/webgl_cache/comp_inc.glsl", "start": 15434340, "end": 15439081}, {"filename": "/data/shaders/webgl_cache/comp_mouse.glsl", "start": 15439081, "end": 15443009}, {"filename": "/data/shaders/webgl_cache/comp_quality.glsl", "start": 15443009, "end": 15446845}, {"filename": "/data/shaders/webgl_cache/comp_zoom.glsl", "start": 15446845, "end": 15450261}, {"filename": "/data/shaders/webgl_cache/compo.glsl", "start": 15450261, "end": 15454253}, {"filename": "/data/shaders/webgl_cache/composite-static.glsl", "start": 15454253, "end": 15460597}, {"filename": "/data/shaders/webgl_cache/composite.glsl", "start": 15460597, "end": 15463761}, {"filename": "/data/shaders/webgl_cache/composite2.glsl", "start": 15463761, "end": 15467494}, {"filename": "/data/shaders/webgl_cache/composite3.glsl", "start": 15467494, "end": 15474549}, {"filename": "/data/shaders/webgl_cache/composite_crt.glsl", "start": 15474549, "end": 15478694}, {"filename": "/data/shaders/webgl_cache/composite_vhs.glsl", "start": 15478694, "end": 15482770}, {"filename": "/data/shaders/webgl_cache/composite_vhs_flat.glsl", "start": 15482770, "end": 15486610}, {"filename": "/data/shaders/webgl_cache/cosine_swirl.glsl", "start": 15486610, "end": 15490142}, {"filename": "/data/shaders/webgl_cache/cosine_swirl_two.glsl", "start": 15490142, "end": 15494164}, {"filename": "/data/shaders/webgl_cache/cripple.glsl", "start": 15494164, "end": 15496923}, {"filename": "/data/shaders/webgl_cache/cripple2.glsl", "start": 15496923, "end": 15499721}, {"filename": "/data/shaders/webgl_cache/crt.glsl", "start": 15499721, "end": 15502732}, {"filename": "/data/shaders/webgl_cache/crystal-2.glsl", "start": 15502732, "end": 15506052}, {"filename": "/data/shaders/webgl_cache/crystal-3.glsl", "start": 15506052, "end": 15509651}, {"filename": "/data/shaders/webgl_cache/crystal-4.glsl", "start": 15509651, "end": 15513236}, {"filename": "/data/shaders/webgl_cache/crystal.glsl", "start": 15513236, "end": 15516402}, {"filename": "/data/shaders/webgl_cache/crystalball.glsl", "start": 15516402, "end": 15519662}, {"filename": "/data/shaders/webgl_cache/cwave.glsl", "start": 15519662, "end": 15522470}, {"filename": "/data/shaders/webgl_cache/cwave2.glsl", "start": 15522470, "end": 15525501}, {"filename": "/data/shaders/webgl_cache/cxripple.glsl", "start": 15525501, "end": 15528620}, {"filename": "/data/shaders/webgl_cache/cyclone_zoom.glsl", "start": 15528620, "end": 15531773}, {"filename": "/data/shaders/webgl_cache/cyclone_zoom_mouse.glsl", "start": 15531773, "end": 15535018}, {"filename": "/data/shaders/webgl_cache/d1.glsl", "start": 15535018, "end": 15538360}, {"filename": "/data/shaders/webgl_cache/damaged_vcr.glsl", "start": 15538360, "end": 15543893}, {"filename": "/data/shaders/webgl_cache/dark_psych.glsl", "start": 15543893, "end": 15547726}, {"filename": "/data/shaders/webgl_cache/dark_psych_wave.glsl", "start": 15547726, "end": 15551933}, {"filename": "/data/shaders/webgl_cache/dark_rainbow.glsl", "start": 15551933, "end": 15555044}, {"filename": "/data/shaders/webgl_cache/dark_rainbow2.glsl", "start": 15555044, "end": 15558356}, {"filename": "/data/shaders/webgl_cache/dark_rainbow_limit.glsl", "start": 15558356, "end": 15561649}, {"filename": "/data/shaders/webgl_cache/dark_rainbow_rubber_soul.glsl", "start": 15561649, "end": 15564953}, {"filename": "/data/shaders/webgl_cache/darkb.glsl", "start": 15564953, "end": 15567841}, {"filename": "/data/shaders/webgl_cache/darkg.glsl", "start": 15567841, "end": 15570729}, {"filename": "/data/shaders/webgl_cache/darkr.glsl", "start": 15570729, "end": 15573617}, {"filename": "/data/shaders/webgl_cache/data.glsl", "start": 15573617, "end": 15578283}, {"filename": "/data/shaders/webgl_cache/deepcolor.glsl", "start": 15578283, "end": 15582901}, {"filename": "/data/shaders/webgl_cache/deeps1.glsl", "start": 15582901, "end": 15587778}, {"filename": "/data/shaders/webgl_cache/deeps3.glsl", "start": 15587778, "end": 15591435}, {"filename": "/data/shaders/webgl_cache/deepseek3.glsl", "start": 15591435, "end": 15596266}, {"filename": "/data/shaders/webgl_cache/deepseek4.glsl", "start": 15596266, "end": 15601006}, {"filename": "/data/shaders/webgl_cache/deform.glsl", "start": 15601006, "end": 15603820}, {"filename": "/data/shaders/webgl_cache/deform_cos.glsl", "start": 15603820, "end": 15606634}, {"filename": "/data/shaders/webgl_cache/deform_tan.glsl", "start": 15606634, "end": 15609448}, {"filename": "/data/shaders/webgl_cache/delic.glsl", "start": 15609448, "end": 15612632}, {"filename": "/data/shaders/webgl_cache/dhue.glsl", "start": 15612632, "end": 15616166}, {"filename": "/data/shaders/webgl_cache/diagsquare.glsl", "start": 15616166, "end": 15619270}, {"filename": "/data/shaders/webgl_cache/diagsquarecircle.glsl", "start": 15619270, "end": 15622534}, {"filename": "/data/shaders/webgl_cache/diamond_pattern_1.glsl", "start": 15622534, "end": 15626608}, {"filename": "/data/shaders/webgl_cache/disk.glsl", "start": 15626608, "end": 15629695}, {"filename": "/data/shaders/webgl_cache/distort-disp.glsl", "start": 15629695, "end": 15632691}, {"filename": "/data/shaders/webgl_cache/distortf.glsl", "start": 15632691, "end": 15636135}, {"filename": "/data/shaders/webgl_cache/distortf2.glsl", "start": 15636135, "end": 15639770}, {"filename": "/data/shaders/webgl_cache/distortf3.glsl", "start": 15639770, "end": 15642999}, {"filename": "/data/shaders/webgl_cache/distortf4.glsl", "start": 15642999, "end": 15646259}, {"filename": "/data/shaders/webgl_cache/dmd_gpt.glsl", "start": 15646259, "end": 15649485}, {"filename": "/data/shaders/webgl_cache/dmdc.glsl", "start": 15649485, "end": 15652779}, {"filename": "/data/shaders/webgl_cache/drain_bend.glsl", "start": 15652779, "end": 15656321}, {"filename": "/data/shaders/webgl_cache/drain_mandella.glsl", "start": 15656321, "end": 15660888}, {"filename": "/data/shaders/webgl_cache/drain_mirror.glsl", "start": 15660888, "end": 15664091}, {"filename": "/data/shaders/webgl_cache/drain_mirror_top.glsl", "start": 15664091, "end": 15667295}, {"filename": "/data/shaders/webgl_cache/drain_mouse.glsl", "start": 15667295, "end": 15670604}, {"filename": "/data/shaders/webgl_cache/drain_rainbow.glsl", "start": 15670604, "end": 15674879}, {"filename": "/data/shaders/webgl_cache/drain_reset.glsl", "start": 15674879, "end": 15678046}, {"filename": "/data/shaders/webgl_cache/ds1.glsl", "start": 15678046, "end": 15681821}, {"filename": "/data/shaders/webgl_cache/dseek.glsl", "start": 15681821, "end": 15685680}, {"filename": "/data/shaders/webgl_cache/echo-pingpong.glsl", "start": 15685680, "end": 15689021}, {"filename": "/data/shaders/webgl_cache/echo-pingpong_c.glsl", "start": 15689021, "end": 15692451}, {"filename": "/data/shaders/webgl_cache/echo.glsl", "start": 15692451, "end": 15695526}, {"filename": "/data/shaders/webgl_cache/echo1.glsl", "start": 15695526, "end": 15698249}, {"filename": "/data/shaders/webgl_cache/echo2.glsl", "start": 15698249, "end": 15700974}, {"filename": "/data/shaders/webgl_cache/echo3.glsl", "start": 15700974, "end": 15703752}, {"filename": "/data/shaders/webgl_cache/echo_2.glsl", "start": 15703752, "end": 15706828}, {"filename": "/data/shaders/webgl_cache/echo_3.glsl", "start": 15706828, "end": 15709957}, {"filename": "/data/shaders/webgl_cache/echo_4.glsl", "start": 15709957, "end": 15713087}, {"filename": "/data/shaders/webgl_cache/echo_4_mix.glsl", "start": 15713087, "end": 15716014}, {"filename": "/data/shaders/webgl_cache/echo_4_rev.glsl", "start": 15716014, "end": 15719306}, {"filename": "/data/shaders/webgl_cache/echo_5_mix_rev.glsl", "start": 15719306, "end": 15722231}, {"filename": "/data/shaders/webgl_cache/echo_alpha.glsl", "start": 15722231, "end": 15725329}, {"filename": "/data/shaders/webgl_cache/echo_alpha_three.glsl", "start": 15725329, "end": 15728432}, {"filename": "/data/shaders/webgl_cache/echo_alpha_three_rgb.glsl", "start": 15728432, "end": 15731616}, {"filename": "/data/shaders/webgl_cache/echo_alpha_two.glsl", "start": 15731616, "end": 15734720}, {"filename": "/data/shaders/webgl_cache/echo_bgr.glsl", "start": 15734720, "end": 15737957}, {"filename": "/data/shaders/webgl_cache/echo_cf.glsl", "start": 15737957, "end": 15741386}, {"filename": "/data/shaders/webgl_cache/echo_color.glsl", "start": 15741386, "end": 15744545}, {"filename": "/data/shaders/webgl_cache/echo_div2.glsl", "start": 15744545, "end": 15747706}, {"filename": "/data/shaders/webgl_cache/echo_div4.glsl", "start": 15747706, "end": 15750867}, {"filename": "/data/shaders/webgl_cache/echo_div4_rainbow.glsl", "start": 15750867, "end": 15754717}, {"filename": "/data/shaders/webgl_cache/echo_g.glsl", "start": 15754717, "end": 15758000}, {"filename": "/data/shaders/webgl_cache/echo_mirror_1.glsl", "start": 15758000, "end": 15761153}, {"filename": "/data/shaders/webgl_cache/echo_mirror_2.glsl", "start": 15761153, "end": 15764331}, {"filename": "/data/shaders/webgl_cache/echo_mirror_3.glsl", "start": 15764331, "end": 15767508}, {"filename": "/data/shaders/webgl_cache/echo_mix.glsl", "start": 15767508, "end": 15770848}, {"filename": "/data/shaders/webgl_cache/echo_mix_colors.glsl", "start": 15770848, "end": 15774126}, {"filename": "/data/shaders/webgl_cache/echo_mix_colors_rev.glsl", "start": 15774126, "end": 15777404}, {"filename": "/data/shaders/webgl_cache/echo_rainbow_spin.glsl", "start": 15777404, "end": 15780791}, {"filename": "/data/shaders/webgl_cache/echo_rainbow_swirl.glsl", "start": 15780791, "end": 15784396}, {"filename": "/data/shaders/webgl_cache/echo_rand.glsl", "start": 15784396, "end": 15787759}, {"filename": "/data/shaders/webgl_cache/echo_rand_2.glsl", "start": 15787759, "end": 15791078}, {"filename": "/data/shaders/webgl_cache/echo_rev_x2.glsl", "start": 15791078, "end": 15794222}, {"filename": "/data/shaders/webgl_cache/echo_rgb.glsl", "start": 15794222, "end": 15797459}, {"filename": "/data/shaders/webgl_cache/echo_rgb1.glsl", "start": 15797459, "end": 15800578}, {"filename": "/data/shaders/webgl_cache/echo_rgb_mouse.glsl", "start": 15800578, "end": 15804051}, {"filename": "/data/shaders/webgl_cache/echo_s.glsl", "start": 15804051, "end": 15807536}, {"filename": "/data/shaders/webgl_cache/echo_s_rgb.glsl", "start": 15807536, "end": 15811160}, {"filename": "/data/shaders/webgl_cache/echo_shift.glsl", "start": 15811160, "end": 15814596}, {"filename": "/data/shaders/webgl_cache/echo_sin.glsl", "start": 15814596, "end": 15817591}, {"filename": "/data/shaders/webgl_cache/echo_sin_particle.glsl", "start": 15817591, "end": 15821619}, {"filename": "/data/shaders/webgl_cache/echo_two.glsl", "start": 15821619, "end": 15824693}, {"filename": "/data/shaders/webgl_cache/echo_x2.glsl", "start": 15824693, "end": 15828423}, {"filename": "/data/shaders/webgl_cache/echo_x3.glsl", "start": 15828423, "end": 15831724}, {"filename": "/data/shaders/webgl_cache/echo_x4.glsl", "start": 15831724, "end": 15835089}, {"filename": "/data/shaders/webgl_cache/echo_x5.glsl", "start": 15835089, "end": 15838256}, {"filename": "/data/shaders/webgl_cache/echo_x6.glsl", "start": 15838256, "end": 15841425}, {"filename": "/data/shaders/webgl_cache/echo_xor_color_blend.glsl", "start": 15841425, "end": 15845349}, {"filename": "/data/shaders/webgl_cache/edge_black.glsl", "start": 15845349, "end": 15848993}, {"filename": "/data/shaders/webgl_cache/edge_face.glsl", "start": 15848993, "end": 15852938}, {"filename": "/data/shaders/webgl_cache/edge_fill.glsl", "start": 15852938, "end": 15856995}, {"filename": "/data/shaders/webgl_cache/edge_pencil.glsl", "start": 15856995, "end": 15860854}, {"filename": "/data/shaders/webgl_cache/edge_rainbow.glsl", "start": 15860854, "end": 15864765}, {"filename": "/data/shaders/webgl_cache/effect.glsl", "start": 15864765, "end": 15867959}, {"filename": "/data/shaders/webgl_cache/elastic.glsl", "start": 15867959, "end": 15871288}, {"filename": "/data/shaders/webgl_cache/elastic2.glsl", "start": 15871288, "end": 15876926}, {"filename": "/data/shaders/webgl_cache/elastic_mouse.glsl", "start": 15876926, "end": 15881548}, {"filename": "/data/shaders/webgl_cache/empty1.glsl", "start": 15881548, "end": 15884114}, {"filename": "/data/shaders/webgl_cache/es1.glsl", "start": 15884114, "end": 15888403}, {"filename": "/data/shaders/webgl_cache/expand_mul.glsl", "start": 15888403, "end": 15891211}, {"filename": "/data/shaders/webgl_cache/expand_mul_fast.glsl", "start": 15891211, "end": 15894028}, {"filename": "/data/shaders/webgl_cache/expand_spot.glsl", "start": 15894028, "end": 15897039}, {"filename": "/data/shaders/webgl_cache/eye2.glsl", "start": 15897039, "end": 15903340}, {"filename": "/data/shaders/webgl_cache/eyes.glsl", "start": 15903340, "end": 15907816}, {"filename": "/data/shaders/webgl_cache/faster.glsl", "start": 15907816, "end": 15912967}, {"filename": "/data/shaders/webgl_cache/fat-blue.glsl", "start": 15912967, "end": 15915887}, {"filename": "/data/shaders/webgl_cache/fat-green.glsl", "start": 15915887, "end": 15918807}, {"filename": "/data/shaders/webgl_cache/fat-red.glsl", "start": 15918807, "end": 15921727}, {"filename": "/data/shaders/webgl_cache/fat-rgb.glsl", "start": 15921727, "end": 15924956}, {"filename": "/data/shaders/webgl_cache/fat-slow.glsl", "start": 15924956, "end": 15927714}, {"filename": "/data/shaders/webgl_cache/fat.glsl", "start": 15927714, "end": 15930425}, {"filename": "/data/shaders/webgl_cache/fill_black.glsl", "start": 15930425, "end": 15933086}, {"filename": "/data/shaders/webgl_cache/fill_black_fold.glsl", "start": 15933086, "end": 15937484}, {"filename": "/data/shaders/webgl_cache/fish_pos_mouse.glsl", "start": 15937484, "end": 15940941}, {"filename": "/data/shaders/webgl_cache/fisheye.glsl", "start": 15940941, "end": 15943903}, {"filename": "/data/shaders/webgl_cache/fisheye_mouse.glsl", "start": 15943903, "end": 15947524}, {"filename": "/data/shaders/webgl_cache/fisheye_pi.mp4.glsl", "start": 15947524, "end": 15951342}, {"filename": "/data/shaders/webgl_cache/fisheye_warp.glsl", "start": 15951342, "end": 15954215}, {"filename": "/data/shaders/webgl_cache/focus.glsl", "start": 15954215, "end": 15957162}, {"filename": "/data/shaders/webgl_cache/fold-fov.glsl", "start": 15957162, "end": 15960223}, {"filename": "/data/shaders/webgl_cache/fold-mirror.glsl", "start": 15960223, "end": 15962941}, {"filename": "/data/shaders/webgl_cache/fold-spin.glsl", "start": 15962941, "end": 15965965}, {"filename": "/data/shaders/webgl_cache/fold-water.glsl", "start": 15965965, "end": 15969445}, {"filename": "/data/shaders/webgl_cache/fold.glsl", "start": 15969445, "end": 15972153}, {"filename": "/data/shaders/webgl_cache/g5.glsl", "start": 15972153, "end": 15976065}, {"filename": "/data/shaders/webgl_cache/g_drum.glsl", "start": 15976065, "end": 15979192}, {"filename": "/data/shaders/webgl_cache/g_drum_by_mouse.glsl", "start": 15979192, "end": 15982129}, {"filename": "/data/shaders/webgl_cache/g_swirl.glsl", "start": 15982129, "end": 15985504}, {"filename": "/data/shaders/webgl_cache/g_ufo.glsl", "start": 15985504, "end": 15988605}, {"filename": "/data/shaders/webgl_cache/g_ufo_3d.glsl", "start": 15988605, "end": 15992070}, {"filename": "/data/shaders/webgl_cache/g_ufo_3d_v2.glsl", "start": 15992070, "end": 15995937}, {"filename": "/data/shaders/webgl_cache/game_amber_mono.glsl", "start": 15995937, "end": 15998774}, {"filename": "/data/shaders/webgl_cache/game_anamorphic.glsl", "start": 15998774, "end": 16001882}, {"filename": "/data/shaders/webgl_cache/game_anime_cel.glsl", "start": 16001882, "end": 16005073}, {"filename": "/data/shaders/webgl_cache/game_ant_aurora_tunnel.glsl", "start": 16005073, "end": 16008022}, {"filename": "/data/shaders/webgl_cache/game_ant_chrome_wave.glsl", "start": 16008022, "end": 16010997}, {"filename": "/data/shaders/webgl_cache/game_ant_cosmic_web.glsl", "start": 16010997, "end": 16014062}, {"filename": "/data/shaders/webgl_cache/game_ant_crystal_pulse.glsl", "start": 16014062, "end": 16016983}, {"filename": "/data/shaders/webgl_cache/game_ant_deep_bloom.glsl", "start": 16016983, "end": 16019979}, {"filename": "/data/shaders/webgl_cache/game_ant_diamond_storm.glsl", "start": 16019979, "end": 16022986}, {"filename": "/data/shaders/webgl_cache/game_ant_fire_spoke.glsl", "start": 16022986, "end": 16025892}, {"filename": "/data/shaders/webgl_cache/game_ant_frac_cosine.glsl", "start": 16025892, "end": 16028736}, {"filename": "/data/shaders/webgl_cache/game_ant_frac_neon.glsl", "start": 16028736, "end": 16031928}, {"filename": "/data/shaders/webgl_cache/game_ant_frac_smooth.glsl", "start": 16031928, "end": 16034813}, {"filename": "/data/shaders/webgl_cache/game_ant_fractal_ocean.glsl", "start": 16034813, "end": 16037709}, {"filename": "/data/shaders/webgl_cache/game_ant_gem_bump.glsl", "start": 16037709, "end": 16040846}, {"filename": "/data/shaders/webgl_cache/game_ant_gem_glass.glsl", "start": 16040846, "end": 16043824}, {"filename": "/data/shaders/webgl_cache/game_ant_gem_kaleido.glsl", "start": 16043824, "end": 16047003}, {"filename": "/data/shaders/webgl_cache/game_ant_gem_pencil.glsl", "start": 16047003, "end": 16050005}, {"filename": "/data/shaders/webgl_cache/game_ant_gem_polar.glsl", "start": 16050005, "end": 16052994}, {"filename": "/data/shaders/webgl_cache/game_ant_gem_rainbow.glsl", "start": 16052994, "end": 16055912}, {"filename": "/data/shaders/webgl_cache/game_ant_gem_spider.glsl", "start": 16055912, "end": 16058865}, {"filename": "/data/shaders/webgl_cache/game_ant_glass_mandala.glsl", "start": 16058865, "end": 16061871}, {"filename": "/data/shaders/webgl_cache/game_ant_hypno_lens.glsl", "start": 16061871, "end": 16064825}, {"filename": "/data/shaders/webgl_cache/game_ant_ice_ripple.glsl", "start": 16064825, "end": 16067757}, {"filename": "/data/shaders/webgl_cache/game_ant_liquid_mirror.glsl", "start": 16067757, "end": 16070701}, {"filename": "/data/shaders/webgl_cache/game_ant_mercury_bloom.glsl", "start": 16070701, "end": 16073761}, {"filename": "/data/shaders/webgl_cache/game_ant_metal_aurora.glsl", "start": 16073761, "end": 16076687}, {"filename": "/data/shaders/webgl_cache/game_ant_metal_cascade.glsl", "start": 16076687, "end": 16079528}, {"filename": "/data/shaders/webgl_cache/game_ant_metal_chrome.glsl", "start": 16079528, "end": 16082435}, {"filename": "/data/shaders/webgl_cache/game_ant_metal_coil.glsl", "start": 16082435, "end": 16085322}, {"filename": "/data/shaders/webgl_cache/game_ant_metal_crystal.glsl", "start": 16085322, "end": 16088355}, {"filename": "/data/shaders/webgl_cache/game_ant_metal_ember.glsl", "start": 16088355, "end": 16091338}, {"filename": "/data/shaders/webgl_cache/game_ant_metal_flux.glsl", "start": 16091338, "end": 16094154}, {"filename": "/data/shaders/webgl_cache/game_ant_metal_forge.glsl", "start": 16094154, "end": 16097079}, {"filename": "/data/shaders/webgl_cache/game_ant_metal_fracture.glsl", "start": 16097079, "end": 16100220}, {"filename": "/data/shaders/webgl_cache/game_ant_metal_glacier.glsl", "start": 16100220, "end": 16103166}, {"filename": "/data/shaders/webgl_cache/game_ant_metal_helix.glsl", "start": 16103166, "end": 16106143}, {"filename": "/data/shaders/webgl_cache/game_ant_metal_inferno.glsl", "start": 16106143, "end": 16108980}, {"filename": "/data/shaders/webgl_cache/game_ant_metal_lattice.glsl", "start": 16108980, "end": 16111854}, {"filename": "/data/shaders/webgl_cache/game_ant_metal_nebula.glsl", "start": 16111854, "end": 16114717}, {"filename": "/data/shaders/webgl_cache/game_ant_metal_opal.glsl", "start": 16114717, "end": 16117665}, {"filename": "/data/shaders/webgl_cache/game_ant_metal_orbital.glsl", "start": 16117665, "end": 16120629}, {"filename": "/data/shaders/webgl_cache/game_ant_metal_prism.glsl", "start": 16120629, "end": 16123530}, {"filename": "/data/shaders/webgl_cache/game_ant_metal_pulse.glsl", "start": 16123530, "end": 16126384}, {"filename": "/data/shaders/webgl_cache/game_ant_metal_ripple.glsl", "start": 16126384, "end": 16129306}, {"filename": "/data/shaders/webgl_cache/game_ant_metal_shard.glsl", "start": 16129306, "end": 16132296}, {"filename": "/data/shaders/webgl_cache/game_ant_metal_storm.glsl", "start": 16132296, "end": 16135266}, {"filename": "/data/shaders/webgl_cache/game_ant_metal_tessera.glsl", "start": 16135266, "end": 16138068}, {"filename": "/data/shaders/webgl_cache/game_ant_metal_vortex.glsl", "start": 16138068, "end": 16141067}, {"filename": "/data/shaders/webgl_cache/game_ant_metal_weave.glsl", "start": 16141067, "end": 16143967}, {"filename": "/data/shaders/webgl_cache/game_ant_molten_web.glsl", "start": 16143967, "end": 16147300}, {"filename": "/data/shaders/webgl_cache/game_ant_nebula_fold.glsl", "start": 16147300, "end": 16150165}, {"filename": "/data/shaders/webgl_cache/game_arcade_marquee.glsl", "start": 16150165, "end": 16153274}, {"filename": "/data/shaders/webgl_cache/game_arcane.glsl", "start": 16153274, "end": 16156176}, {"filename": "/data/shaders/webgl_cache/game_ascii_art.glsl", "start": 16156176, "end": 16159829}, {"filename": "/data/shaders/webgl_cache/game_bleach_bypass.glsl", "start": 16159829, "end": 16162652}, {"filename": "/data/shaders/webgl_cache/game_boss_warning.glsl", "start": 16162652, "end": 16165644}, {"filename": "/data/shaders/webgl_cache/game_bullet_time.glsl", "start": 16165644, "end": 16168730}, {"filename": "/data/shaders/webgl_cache/game_cel_outline.glsl", "start": 16168730, "end": 16172210}, {"filename": "/data/shaders/webgl_cache/game_cga.glsl", "start": 16172210, "end": 16175192}, {"filename": "/data/shaders/webgl_cache/game_chromatic_edges.glsl", "start": 16175192, "end": 16178082}, {"filename": "/data/shaders/webgl_cache/game_cinematic_cool.glsl", "start": 16178082, "end": 16180921}, {"filename": "/data/shaders/webgl_cache/game_cinematic_warm.glsl", "start": 16180921, "end": 16183839}, {"filename": "/data/shaders/webgl_cache/game_codex_arcane_runes.glsl", "start": 16183839, "end": 16186943}, {"filename": "/data/shaders/webgl_cache/game_codex_boss_aura.glsl", "start": 16186943, "end": 16189999}, {"filename": "/data/shaders/webgl_cache/game_codex_bullet_time_focus.glsl", "start": 16189999, "end": 16193097}, {"filename": "/data/shaders/webgl_cache/game_codex_critical_hit.glsl", "start": 16193097, "end": 16196178}, {"filename": "/data/shaders/webgl_cache/game_codex_freeze_frame.glsl", "start": 16196178, "end": 16199198}, {"filename": "/data/shaders/webgl_cache/game_codex_lava_damage.glsl", "start": 16199198, "end": 16202228}, {"filename": "/data/shaders/webgl_cache/game_codex_low_ammo_warning.glsl", "start": 16202228, "end": 16205276}, {"filename": "/data/shaders/webgl_cache/game_codex_magic_barrier.glsl", "start": 16205276, "end": 16208393}, {"filename": "/data/shaders/webgl_cache/game_codex_pixel_pickup.glsl", "start": 16208393, "end": 16211501}, {"filename": "/data/shaders/webgl_cache/game_codex_portal_rift.glsl", "start": 16211501, "end": 16214695}, {"filename": "/data/shaders/webgl_cache/game_codex_powerup_glow.glsl", "start": 16214695, "end": 16217824}, {"filename": "/data/shaders/webgl_cache/game_codex_radar_ping.glsl", "start": 16217824, "end": 16220922}, {"filename": "/data/shaders/webgl_cache/game_codex_rage_meter.glsl", "start": 16220922, "end": 16223991}, {"filename": "/data/shaders/webgl_cache/game_codex_shadow_realm.glsl", "start": 16223991, "end": 16227033}, {"filename": "/data/shaders/webgl_cache/game_codex_shield_hit.glsl", "start": 16227033, "end": 16230124}, {"filename": "/data/shaders/webgl_cache/game_codex_speed_boost.glsl", "start": 16230124, "end": 16233208}, {"filename": "/data/shaders/webgl_cache/game_codex_stealth_cloak.glsl", "start": 16233208, "end": 16236286}, {"filename": "/data/shaders/webgl_cache/game_codex_target_lock.glsl", "start": 16236286, "end": 16239546}, {"filename": "/data/shaders/webgl_cache/game_codex_toxic_cloud.glsl", "start": 16239546, "end": 16242811}, {"filename": "/data/shaders/webgl_cache/game_codex_underwater_depth.glsl", "start": 16242811, "end": 16245891}, {"filename": "/data/shaders/webgl_cache/game_cross_process.glsl", "start": 16245891, "end": 16248776}, {"filename": "/data/shaders/webgl_cache/game_crt_curve.glsl", "start": 16248776, "end": 16251996}, {"filename": "/data/shaders/webgl_cache/game_cyberpunk_neon.glsl", "start": 16251996, "end": 16255129}, {"filename": "/data/shaders/webgl_cache/game_damage_pulse.glsl", "start": 16255129, "end": 16258017}, {"filename": "/data/shaders/webgl_cache/game_desert_heat.glsl", "start": 16258017, "end": 16260879}, {"filename": "/data/shaders/webgl_cache/game_disco_floor.glsl", "start": 16260879, "end": 16263985}, {"filename": "/data/shaders/webgl_cache/game_dither_bayer.glsl", "start": 16263985, "end": 16267067}, {"filename": "/data/shaders/webgl_cache/game_dream_glow.glsl", "start": 16267067, "end": 16270171}, {"filename": "/data/shaders/webgl_cache/game_drunk_wobble.glsl", "start": 16270171, "end": 16272952}, {"filename": "/data/shaders/webgl_cache/game_dust_motes.glsl", "start": 16272952, "end": 16276108}, {"filename": "/data/shaders/webgl_cache/game_emp_blast.glsl", "start": 16276108, "end": 16279107}, {"filename": "/data/shaders/webgl_cache/game_fake_ssao.glsl", "start": 16279107, "end": 16282189}, {"filename": "/data/shaders/webgl_cache/game_film_grain.glsl", "start": 16282189, "end": 16285101}, {"filename": "/data/shaders/webgl_cache/game_fog.glsl", "start": 16285101, "end": 16287978}, {"filename": "/data/shaders/webgl_cache/game_freeze_crystal.glsl", "start": 16287978, "end": 16291205}, {"filename": "/data/shaders/webgl_cache/game_frostbite.glsl", "start": 16291205, "end": 16294225}, {"filename": "/data/shaders/webgl_cache/game_gameboy_dmg.glsl", "start": 16294225, "end": 16297253}, {"filename": "/data/shaders/webgl_cache/game_gba_tint.glsl", "start": 16297253, "end": 16300140}, {"filename": "/data/shaders/webgl_cache/game_genesis.glsl", "start": 16300140, "end": 16302878}, {"filename": "/data/shaders/webgl_cache/game_ghost_trail.glsl", "start": 16302878, "end": 16305940}, {"filename": "/data/shaders/webgl_cache/game_green_mono.glsl", "start": 16305940, "end": 16308766}, {"filename": "/data/shaders/webgl_cache/game_hacker_terminal.glsl", "start": 16308766, "end": 16311846}, {"filename": "/data/shaders/webgl_cache/game_halftone.glsl", "start": 16311846, "end": 16314747}, {"filename": "/data/shaders/webgl_cache/game_hdr_punch.glsl", "start": 16314747, "end": 16317640}, {"filename": "/data/shaders/webgl_cache/game_heart_beat.glsl", "start": 16317640, "end": 16320722}, {"filename": "/data/shaders/webgl_cache/game_hologram.glsl", "start": 16320722, "end": 16323882}, {"filename": "/data/shaders/webgl_cache/game_kaleido_combat.glsl", "start": 16323882, "end": 16326756}, {"filename": "/data/shaders/webgl_cache/game_lava_world.glsl", "start": 16326756, "end": 16329809}, {"filename": "/data/shaders/webgl_cache/game_lcd_grid.glsl", "start": 16329809, "end": 16332755}, {"filename": "/data/shaders/webgl_cache/game_letterbox.glsl", "start": 16332755, "end": 16335699}, {"filename": "/data/shaders/webgl_cache/game_low_battery.glsl", "start": 16335699, "end": 16338677}, {"filename": "/data/shaders/webgl_cache/game_matrix_rain.glsl", "start": 16338677, "end": 16342093}, {"filename": "/data/shaders/webgl_cache/game_minimap_glow.glsl", "start": 16342093, "end": 16345173}, {"filename": "/data/shaders/webgl_cache/game_motion_blur.glsl", "start": 16345173, "end": 16348085}, {"filename": "/data/shaders/webgl_cache/game_mushroom_trip.glsl", "start": 16348085, "end": 16351436}, {"filename": "/data/shaders/webgl_cache/game_nes_palette.glsl", "start": 16351436, "end": 16354143}, {"filename": "/data/shaders/webgl_cache/game_night_vision.glsl", "start": 16354143, "end": 16357291}, {"filename": "/data/shaders/webgl_cache/game_noir.glsl", "start": 16357291, "end": 16360122}, {"filename": "/data/shaders/webgl_cache/game_oil_paint.glsl", "start": 16360122, "end": 16363557}, {"filename": "/data/shaders/webgl_cache/game_pencil_lines.glsl", "start": 16363557, "end": 16366641}, {"filename": "/data/shaders/webgl_cache/game_phosphor.glsl", "start": 16366641, "end": 16369708}, {"filename": "/data/shaders/webgl_cache/game_pixelate_4x.glsl", "start": 16369708, "end": 16372443}, {"filename": "/data/shaders/webgl_cache/game_pixelsort_glitch.glsl", "start": 16372443, "end": 16375542}, {"filename": "/data/shaders/webgl_cache/game_polaroid.glsl", "start": 16375542, "end": 16378415}, {"filename": "/data/shaders/webgl_cache/game_psx_dither.glsl", "start": 16378415, "end": 16381389}, {"filename": "/data/shaders/webgl_cache/game_radial_focus.glsl", "start": 16381389, "end": 16384467}, {"filename": "/data/shaders/webgl_cache/game_radio_static.glsl", "start": 16384467, "end": 16387472}, {"filename": "/data/shaders/webgl_cache/game_rage_mode.glsl", "start": 16387472, "end": 16390564}, {"filename": "/data/shaders/webgl_cache/game_rain.glsl", "start": 16390564, "end": 16393718}, {"filename": "/data/shaders/webgl_cache/game_scanlines_hd.glsl", "start": 16393718, "end": 16396478}, {"filename": "/data/shaders/webgl_cache/game_sepia_warm.glsl", "start": 16396478, "end": 16399338}, {"filename": "/data/shaders/webgl_cache/game_sharpen.glsl", "start": 16399338, "end": 16402371}, {"filename": "/data/shaders/webgl_cache/game_snow.glsl", "start": 16402371, "end": 16405741}, {"filename": "/data/shaders/webgl_cache/game_speedlines.glsl", "start": 16405741, "end": 16408807}, {"filename": "/data/shaders/webgl_cache/game_subtle_bloom.glsl", "start": 16408807, "end": 16411991}, {"filename": "/data/shaders/webgl_cache/game_super_saiyan.glsl", "start": 16411991, "end": 16415177}, {"filename": "/data/shaders/webgl_cache/game_technicolor.glsl", "start": 16415177, "end": 16417987}, {"filename": "/data/shaders/webgl_cache/game_thermal.glsl", "start": 16417987, "end": 16421156}, {"filename": "/data/shaders/webgl_cache/game_toxic.glsl", "start": 16421156, "end": 16424076}, {"filename": "/data/shaders/webgl_cache/game_underwater.glsl", "start": 16424076, "end": 16427138}, {"filename": "/data/shaders/webgl_cache/game_vignette_breath.glsl", "start": 16427138, "end": 16429987}, {"filename": "/data/shaders/webgl_cache/game_void_warp.glsl", "start": 16429987, "end": 16433023}, {"filename": "/data/shaders/webgl_cache/gaura.glsl", "start": 16433023, "end": 16436680}, {"filename": "/data/shaders/webgl_cache/gblur.glsl", "start": 16436680, "end": 16440710}, {"filename": "/data/shaders/webgl_cache/gboil.glsl", "start": 16440710, "end": 16443956}, {"filename": "/data/shaders/webgl_cache/gem-af.glsl", "start": 16443956, "end": 16447771}, {"filename": "/data/shaders/webgl_cache/gem-aura.glsl", "start": 16447771, "end": 16454497}, {"filename": "/data/shaders/webgl_cache/gem-color-spiral.glsl", "start": 16454497, "end": 16459344}, {"filename": "/data/shaders/webgl_cache/gem-color-spsiral.glsl", "start": 16459344, "end": 16464191}, {"filename": "/data/shaders/webgl_cache/gem-deep.glsl", "start": 16464191, "end": 16468327}, {"filename": "/data/shaders/webgl_cache/gem-hue-frac.glsl", "start": 16468327, "end": 16472653}, {"filename": "/data/shaders/webgl_cache/gem-light-frac.glsl", "start": 16472653, "end": 16476914}, {"filename": "/data/shaders/webgl_cache/gem-pong.glsl", "start": 16476914, "end": 16481606}, {"filename": "/data/shaders/webgl_cache/gem-ripple.glsl", "start": 16481606, "end": 16485754}, {"filename": "/data/shaders/webgl_cache/gem-spiral-cont.glsl", "start": 16485754, "end": 16489494}, {"filename": "/data/shaders/webgl_cache/gem-spiral-cont2.glsl", "start": 16489494, "end": 16493527}, {"filename": "/data/shaders/webgl_cache/gem-spiral-full.glsl", "start": 16493527, "end": 16498323}, {"filename": "/data/shaders/webgl_cache/gem-spoke.glsl", "start": 16498323, "end": 16502300}, {"filename": "/data/shaders/webgl_cache/gem_frac.glsl", "start": 16502300, "end": 16505973}, {"filename": "/data/shaders/webgl_cache/gem_polar.glsl", "start": 16505973, "end": 16509924}, {"filename": "/data/shaders/webgl_cache/genergy.glsl", "start": 16509924, "end": 16513413}, {"filename": "/data/shaders/webgl_cache/geo-pi.glsl", "start": 16513413, "end": 16516884}, {"filename": "/data/shaders/webgl_cache/geometric.glsl", "start": 16516884, "end": 16520103}, {"filename": "/data/shaders/webgl_cache/geometric2.glsl", "start": 16520103, "end": 16523353}, {"filename": "/data/shaders/webgl_cache/geometric3.glsl", "start": 16523353, "end": 16526613}, {"filename": "/data/shaders/webgl_cache/geometric4.glsl", "start": 16526613, "end": 16529719}, {"filename": "/data/shaders/webgl_cache/geometric5.glsl", "start": 16529719, "end": 16533016}, {"filename": "/data/shaders/webgl_cache/gfs.glsl", "start": 16533016, "end": 16536471}, {"filename": "/data/shaders/webgl_cache/gghost.glsl", "start": 16536471, "end": 16539524}, {"filename": "/data/shaders/webgl_cache/ggrad.glsl", "start": 16539524, "end": 16542489}, {"filename": "/data/shaders/webgl_cache/gkale.glsl", "start": 16542489, "end": 16546062}, {"filename": "/data/shaders/webgl_cache/gkale_echo.glsl", "start": 16546062, "end": 16550632}, {"filename": "/data/shaders/webgl_cache/gkale_echo2.glsl", "start": 16550632, "end": 16557020}, {"filename": "/data/shaders/webgl_cache/gkalei.glsl", "start": 16557020, "end": 16561154}, {"filename": "/data/shaders/webgl_cache/glass_PI.glsl", "start": 16561154, "end": 16564414}, {"filename": "/data/shaders/webgl_cache/glass_mouse.glsl", "start": 16564414, "end": 16567447}, {"filename": "/data/shaders/webgl_cache/glass_mouse_rad.glsl", "start": 16567447, "end": 16570512}, {"filename": "/data/shaders/webgl_cache/glitch-no-noise-mouse.glsl", "start": 16570512, "end": 16575845}, {"filename": "/data/shaders/webgl_cache/glitch-no-noise.glsl", "start": 16575845, "end": 16580748}, {"filename": "/data/shaders/webgl_cache/glitch-noise.glsl", "start": 16580748, "end": 16584302}, {"filename": "/data/shaders/webgl_cache/glitch-react-color.glsl", "start": 16584302, "end": 16587596}, {"filename": "/data/shaders/webgl_cache/glitch-react.glsl", "start": 16587596, "end": 16590662}, {"filename": "/data/shaders/webgl_cache/glitch-zoom.glsl", "start": 16590662, "end": 16595768}, {"filename": "/data/shaders/webgl_cache/glitch1.glsl", "start": 16595768, "end": 16599254}, {"filename": "/data/shaders/webgl_cache/glitch_boil.glsl", "start": 16599254, "end": 16603050}, {"filename": "/data/shaders/webgl_cache/glitch_boil2.glsl", "start": 16603050, "end": 16606165}, {"filename": "/data/shaders/webgl_cache/glitch_jump.glsl", "start": 16606165, "end": 16608891}, {"filename": "/data/shaders/webgl_cache/glitch_light.glsl", "start": 16608891, "end": 16612111}, {"filename": "/data/shaders/webgl_cache/glitch_wave.glsl", "start": 16612111, "end": 16615128}, {"filename": "/data/shaders/webgl_cache/glitchf.glsl", "start": 16615128, "end": 16618574}, {"filename": "/data/shaders/webgl_cache/glitchy-rainbow.glsl", "start": 16618574, "end": 16622249}, {"filename": "/data/shaders/webgl_cache/glitchy-squish.glsl", "start": 16622249, "end": 16625369}, {"filename": "/data/shaders/webgl_cache/glitchy.glsl", "start": 16625369, "end": 16628708}, {"filename": "/data/shaders/webgl_cache/gltichtest.glsl", "start": 16628708, "end": 16632664}, {"filename": "/data/shaders/webgl_cache/gmir.glsl", "start": 16632664, "end": 16638683}, {"filename": "/data/shaders/webgl_cache/gmir2.glsl", "start": 16638683, "end": 16641921}, {"filename": "/data/shaders/webgl_cache/goo.glsl", "start": 16641921, "end": 16644915}, {"filename": "/data/shaders/webgl_cache/gpt_echo.glsl", "start": 16644915, "end": 16647740}, {"filename": "/data/shaders/webgl_cache/gpt_halluc.glsl", "start": 16647740, "end": 16655884}, {"filename": "/data/shaders/webgl_cache/gpt_trip.glsl", "start": 16655884, "end": 16662421}, {"filename": "/data/shaders/webgl_cache/gpt_trip2.glsl", "start": 16662421, "end": 16671225}, {"filename": "/data/shaders/webgl_cache/gptsmooth2.glsl", "start": 16671225, "end": 16675466}, {"filename": "/data/shaders/webgl_cache/gptswirl.glsl", "start": 16675466, "end": 16678578}, {"filename": "/data/shaders/webgl_cache/gptswirl2.glsl", "start": 16678578, "end": 16681958}, {"filename": "/data/shaders/webgl_cache/grad.glsl", "start": 16681958, "end": 16686034}, {"filename": "/data/shaders/webgl_cache/grad2.glsl", "start": 16686034, "end": 16689672}, {"filename": "/data/shaders/webgl_cache/grad3.glsl", "start": 16689672, "end": 16693285}, {"filename": "/data/shaders/webgl_cache/grad_color.glsl", "start": 16693285, "end": 16696243}, {"filename": "/data/shaders/webgl_cache/grainbow.glsl", "start": 16696243, "end": 16699698}, {"filename": "/data/shaders/webgl_cache/grayscale.glsl", "start": 16699698, "end": 16702378}, {"filename": "/data/shaders/webgl_cache/green_echo.glsl", "start": 16702378, "end": 16705479}, {"filename": "/data/shaders/webgl_cache/grid-spiral.glsl", "start": 16705479, "end": 16709020}, {"filename": "/data/shaders/webgl_cache/grid_pattern.glsl", "start": 16709020, "end": 16711846}, {"filename": "/data/shaders/webgl_cache/grid_strobe.glsl", "start": 16711846, "end": 16715766}, {"filename": "/data/shaders/webgl_cache/gsupercool.glsl", "start": 16715766, "end": 16720110}, {"filename": "/data/shaders/webgl_cache/gsupercool2.glsl", "start": 16720110, "end": 16724265}, {"filename": "/data/shaders/webgl_cache/gt_rotate.glsl", "start": 16724265, "end": 16727377}, {"filename": "/data/shaders/webgl_cache/gtrail.glsl", "start": 16727377, "end": 16730732}, {"filename": "/data/shaders/webgl_cache/gtrail2.glsl", "start": 16730732, "end": 16734271}, {"filename": "/data/shaders/webgl_cache/gvortex.glsl", "start": 16734271, "end": 16737665}, {"filename": "/data/shaders/webgl_cache/gxor.glsl", "start": 16737665, "end": 16740714}, {"filename": "/data/shaders/webgl_cache/gxor2.glsl", "start": 16740714, "end": 16743986}, {"filename": "/data/shaders/webgl_cache/halluc_gem.glsl", "start": 16743986, "end": 16751032}, {"filename": "/data/shaders/webgl_cache/halluc_gem2.glsl", "start": 16751032, "end": 16758424}, {"filename": "/data/shaders/webgl_cache/halluc_liquid.glsl", "start": 16758424, "end": 16765958}, {"filename": "/data/shaders/webgl_cache/halluc_pop.glsl", "start": 16765958, "end": 16771929}, {"filename": "/data/shaders/webgl_cache/halluc_pop2.glsl", "start": 16771929, "end": 16777334}, {"filename": "/data/shaders/webgl_cache/heartthrob2.glsl", "start": 16777334, "end": 16780406}, {"filename": "/data/shaders/webgl_cache/heat-wave.glsl", "start": 16780406, "end": 16783401}, {"filename": "/data/shaders/webgl_cache/heat.glsl", "start": 16783401, "end": 16786293}, {"filename": "/data/shaders/webgl_cache/hue-mouse.glsl", "start": 16786293, "end": 16790869}, {"filename": "/data/shaders/webgl_cache/huei_af2.glsl", "start": 16790869, "end": 16796655}, {"filename": "/data/shaders/webgl_cache/huri.glsl", "start": 16796655, "end": 16799601}, {"filename": "/data/shaders/webgl_cache/huri1.glsl", "start": 16799601, "end": 16802768}, {"filename": "/data/shaders/webgl_cache/huri2.glsl", "start": 16802768, "end": 16806004}, {"filename": "/data/shaders/webgl_cache/huri3.glsl", "start": 16806004, "end": 16809244}, {"filename": "/data/shaders/webgl_cache/huri_af.glsl", "start": 16809244, "end": 16814432}, {"filename": "/data/shaders/webgl_cache/huri_create_mouse.glsl", "start": 16814432, "end": 16819036}, {"filename": "/data/shaders/webgl_cache/hurixyz.glsl", "start": 16819036, "end": 16823048}, {"filename": "/data/shaders/webgl_cache/huriz.glsl", "start": 16823048, "end": 16826238}, {"filename": "/data/shaders/webgl_cache/ice.glsl", "start": 16826238, "end": 16829736}, {"filename": "/data/shaders/webgl_cache/index.txt", "start": 16829736, "end": 16850014}, {"filename": "/data/shaders/webgl_cache/inflate-ripple.glsl", "start": 16850014, "end": 16852910}, {"filename": "/data/shaders/webgl_cache/inflate.glsl", "start": 16852910, "end": 16855609}, {"filename": "/data/shaders/webgl_cache/kale.glsl", "start": 16855609, "end": 16858835}, {"filename": "/data/shaders/webgl_cache/kale2.glsl", "start": 16858835, "end": 16862075}, {"filename": "/data/shaders/webgl_cache/kale3.glsl", "start": 16862075, "end": 16865320}, {"filename": "/data/shaders/webgl_cache/kale4.glsl", "start": 16865320, "end": 16868589}, {"filename": "/data/shaders/webgl_cache/kale_mouse.glsl", "start": 16868589, "end": 16872019}, {"filename": "/data/shaders/webgl_cache/kscopic.glsl", "start": 16872019, "end": 16875177}, {"filename": "/data/shaders/webgl_cache/kscopic_3d_mouse.glsl", "start": 16875177, "end": 16878937}, {"filename": "/data/shaders/webgl_cache/light-rings.glsl", "start": 16878937, "end": 16882911}, {"filename": "/data/shaders/webgl_cache/lineinlineout.glsl", "start": 16882911, "end": 16885723}, {"filename": "/data/shaders/webgl_cache/list_rainbow_enter.glsl", "start": 16885723, "end": 16888911}, {"filename": "/data/shaders/webgl_cache/lucy.glsl", "start": 16888911, "end": 16891789}, {"filename": "/data/shaders/webgl_cache/magic-2.glsl", "start": 16891789, "end": 16894910}, {"filename": "/data/shaders/webgl_cache/magic.glsl", "start": 16894910, "end": 16897820}, {"filename": "/data/shaders/webgl_cache/matrix.glsl", "start": 16897820, "end": 16900791}, {"filename": "/data/shaders/webgl_cache/matrix_mouse.glsl", "start": 16900791, "end": 16904558}, {"filename": "/data/shaders/webgl_cache/mb.glsl", "start": 16904558, "end": 16908264}, {"filename": "/data/shaders/webgl_cache/minimize.glsl", "start": 16908264, "end": 16911245}, {"filename": "/data/shaders/webgl_cache/mirdmd.glsl", "start": 16911245, "end": 16913963}, {"filename": "/data/shaders/webgl_cache/mirdmd1.glsl", "start": 16913963, "end": 16917582}, {"filename": "/data/shaders/webgl_cache/mirdmdr.glsl", "start": 16917582, "end": 16920701}, {"filename": "/data/shaders/webgl_cache/mirdmdrcolor.glsl", "start": 16920701, "end": 16924118}, {"filename": "/data/shaders/webgl_cache/mirdmdrmatch.glsl", "start": 16924118, "end": 16927506}, {"filename": "/data/shaders/webgl_cache/mirdmdrspeed.glsl", "start": 16927506, "end": 16931085}, {"filename": "/data/shaders/webgl_cache/mirdmdt.glsl", "start": 16931085, "end": 16934118}, {"filename": "/data/shaders/webgl_cache/mirror-atan.glsl", "start": 16934118, "end": 16937933}, {"filename": "/data/shaders/webgl_cache/mirror-bowl-by-time.glsl", "start": 16937933, "end": 16941508}, {"filename": "/data/shaders/webgl_cache/mirror-bowl.glsl", "start": 16941508, "end": 16945103}, {"filename": "/data/shaders/webgl_cache/mirror-bubble-zoom-mouse.glsl", "start": 16945103, "end": 16948930}, {"filename": "/data/shaders/webgl_cache/mirror-bubble.glsl", "start": 16948930, "end": 16952054}, {"filename": "/data/shaders/webgl_cache/mirror-c.glsl", "start": 16952054, "end": 16954842}, {"filename": "/data/shaders/webgl_cache/mirror-center.glsl", "start": 16954842, "end": 16957709}, {"filename": "/data/shaders/webgl_cache/mirror-code-x-anti-diagonal.glsl", "start": 16957709, "end": 16960308}, {"filename": "/data/shaders/webgl_cache/mirror-code-x-bottom-fold.glsl", "start": 16960308, "end": 16962877}, {"filename": "/data/shaders/webgl_cache/mirror-code-x-checker-grid.glsl", "start": 16962877, "end": 16965526}, {"filename": "/data/shaders/webgl_cache/mirror-code-x-column-strips.glsl", "start": 16965526, "end": 16968180}, {"filename": "/data/shaders/webgl_cache/mirror-code-x-cross-fold.glsl", "start": 16968180, "end": 16970821}, {"filename": "/data/shaders/webgl_cache/mirror-code-x-diamond-fold.glsl", "start": 16970821, "end": 16973437}, {"filename": "/data/shaders/webgl_cache/mirror-code-x-kaleido-eight.glsl", "start": 16973437, "end": 16976358}, {"filename": "/data/shaders/webgl_cache/mirror-code-x-kaleido-six.glsl", "start": 16976358, "end": 16979279}, {"filename": "/data/shaders/webgl_cache/mirror-code-x-kaleido-twelve.glsl", "start": 16979279, "end": 16982201}, {"filename": "/data/shaders/webgl_cache/mirror-code-x-left-fold.glsl", "start": 16982201, "end": 16984770}, {"filename": "/data/shaders/webgl_cache/mirror-code-x-main-diagonal.glsl", "start": 16984770, "end": 16987357}, {"filename": "/data/shaders/webgl_cache/mirror-code-x-offset-checker.glsl", "start": 16987357, "end": 16990113}, {"filename": "/data/shaders/webgl_cache/mirror-code-x-pinwheel-fold.glsl", "start": 16990113, "end": 16993124}, {"filename": "/data/shaders/webgl_cache/mirror-code-x-quad-center.glsl", "start": 16993124, "end": 16995679}, {"filename": "/data/shaders/webgl_cache/mirror-code-x-quad-corners.glsl", "start": 16995679, "end": 16998234}, {"filename": "/data/shaders/webgl_cache/mirror-code-x-quadrant-offset.glsl", "start": 16998234, "end": 17001033}, {"filename": "/data/shaders/webgl_cache/mirror-code-x-right-fold.glsl", "start": 17001033, "end": 17003602}, {"filename": "/data/shaders/webgl_cache/mirror-code-x-ripple-rings.glsl", "start": 17003602, "end": 17006545}, {"filename": "/data/shaders/webgl_cache/mirror-code-x-rotating-axis.glsl", "start": 17006545, "end": 17009356}, {"filename": "/data/shaders/webgl_cache/mirror-code-x-row-strips.glsl", "start": 17009356, "end": 17012010}, {"filename": "/data/shaders/webgl_cache/mirror-code-x-sliding-axis.glsl", "start": 17012010, "end": 17014791}, {"filename": "/data/shaders/webgl_cache/mirror-code-x-spiral-sectors.glsl", "start": 17014791, "end": 17017763}, {"filename": "/data/shaders/webgl_cache/mirror-code-x-tile-diagonals.glsl", "start": 17017763, "end": 17020562}, {"filename": "/data/shaders/webgl_cache/mirror-code-x-top-fold.glsl", "start": 17020562, "end": 17023131}, {"filename": "/data/shaders/webgl_cache/mirror-code-x-wave-seam.glsl", "start": 17023131, "end": 17025906}, {"filename": "/data/shaders/webgl_cache/mirror-color-mouse.glsl", "start": 17025906, "end": 17030343}, {"filename": "/data/shaders/webgl_cache/mirror-color-o1.glsl", "start": 17030343, "end": 17033829}, {"filename": "/data/shaders/webgl_cache/mirror-color-swirl.glsl", "start": 17033829, "end": 17037358}, {"filename": "/data/shaders/webgl_cache/mirror-comb-mouse.glsl", "start": 17037358, "end": 17041778}, {"filename": "/data/shaders/webgl_cache/mirror-fish-mouse.glsl", "start": 17041778, "end": 17045304}, {"filename": "/data/shaders/webgl_cache/mirror-frac-mouse-z2.glsl", "start": 17045304, "end": 17051434}, {"filename": "/data/shaders/webgl_cache/mirror-goofy.glsl", "start": 17051434, "end": 17055126}, {"filename": "/data/shaders/webgl_cache/mirror-grad.glsl", "start": 17055126, "end": 17058792}, {"filename": "/data/shaders/webgl_cache/mirror-in.glsl", "start": 17058792, "end": 17061561}, {"filename": "/data/shaders/webgl_cache/mirror-l.glsl", "start": 17061561, "end": 17064265}, {"filename": "/data/shaders/webgl_cache/mirror-mandella.glsl", "start": 17064265, "end": 17068922}, {"filename": "/data/shaders/webgl_cache/mirror-pebble.glsl", "start": 17068922, "end": 17072120}, {"filename": "/data/shaders/webgl_cache/mirror-pong.glsl", "start": 17072120, "end": 17075619}, {"filename": "/data/shaders/webgl_cache/mirror-pong2.glsl", "start": 17075619, "end": 17079230}, {"filename": "/data/shaders/webgl_cache/mirror-prism.glsl", "start": 17079230, "end": 17083123}, {"filename": "/data/shaders/webgl_cache/mirror-psyce-wave-all.glsl", "start": 17083123, "end": 17086082}, {"filename": "/data/shaders/webgl_cache/mirror-putty.glsl", "start": 17086082, "end": 17089277}, {"filename": "/data/shaders/webgl_cache/mirror-self.glsl", "start": 17089277, "end": 17091837}, {"filename": "/data/shaders/webgl_cache/mirror-sin-osc.glsl", "start": 17091837, "end": 17095348}, {"filename": "/data/shaders/webgl_cache/mirror-spiral-aura.glsl", "start": 17095348, "end": 17098530}, {"filename": "/data/shaders/webgl_cache/mirror-spiral-f.glsl", "start": 17098530, "end": 17101766}, {"filename": "/data/shaders/webgl_cache/mirror-spiral.glsl", "start": 17101766, "end": 17104788}, {"filename": "/data/shaders/webgl_cache/mirror-swirly.glsl", "start": 17104788, "end": 17107795}, {"filename": "/data/shaders/webgl_cache/mirror-twist-sin.glsl", "start": 17107795, "end": 17110913}, {"filename": "/data/shaders/webgl_cache/mirror-twist.glsl", "start": 17110913, "end": 17113823}, {"filename": "/data/shaders/webgl_cache/mirror-twisted.glsl", "start": 17113823, "end": 17116991}, {"filename": "/data/shaders/webgl_cache/mirror-ufo-wrap.glsl", "start": 17116991, "end": 17120524}, {"filename": "/data/shaders/webgl_cache/mirror-wrap-dmd6i.glsl", "start": 17120524, "end": 17129128}, {"filename": "/data/shaders/webgl_cache/mirror-wrap-edge.glsl", "start": 17129128, "end": 17132016}, {"filename": "/data/shaders/webgl_cache/mirror-wrap-rotate-snap.glsl", "start": 17132016, "end": 17134849}, {"filename": "/data/shaders/webgl_cache/mirror-wrap-rotate.glsl", "start": 17134849, "end": 17137647}, {"filename": "/data/shaders/webgl_cache/mirror-wrap-rotate90.glsl", "start": 17137647, "end": 17140406}, {"filename": "/data/shaders/webgl_cache/mirror-wrap-scale.glsl", "start": 17140406, "end": 17145068}, {"filename": "/data/shaders/webgl_cache/mirror-wrap-spin.glsl", "start": 17145068, "end": 17148052}, {"filename": "/data/shaders/webgl_cache/mirror-wrap.glsl", "start": 17148052, "end": 17150637}, {"filename": "/data/shaders/webgl_cache/mirror-zoom.glsl", "start": 17150637, "end": 17154711}, {"filename": "/data/shaders/webgl_cache/mirror_8.glsl", "start": 17154711, "end": 17157958}, {"filename": "/data/shaders/webgl_cache/mirror_b.glsl", "start": 17157958, "end": 17160576}, {"filename": "/data/shaders/webgl_cache/mirror_c.glsl", "start": 17160576, "end": 17163715}, {"filename": "/data/shaders/webgl_cache/mirror_dir.glsl", "start": 17163715, "end": 17166599}, {"filename": "/data/shaders/webgl_cache/mirror_half_dir.glsl", "start": 17166599, "end": 17169467}, {"filename": "/data/shaders/webgl_cache/mirror_p.glsl", "start": 17169467, "end": 17172341}, {"filename": "/data/shaders/webgl_cache/mirror_r4.glsl", "start": 17172341, "end": 17175587}, {"filename": "/data/shaders/webgl_cache/mirror_r8.glsl", "start": 17175587, "end": 17178833}, {"filename": "/data/shaders/webgl_cache/mirror_rad.glsl", "start": 17178833, "end": 17182204}, {"filename": "/data/shaders/webgl_cache/mirrorg.glsl", "start": 17182204, "end": 17185352}, {"filename": "/data/shaders/webgl_cache/mix_diag.glsl", "start": 17185352, "end": 17188022}, {"filename": "/data/shaders/webgl_cache/move.glsl", "start": 17188022, "end": 17190770}, {"filename": "/data/shaders/webgl_cache/moveGL.glsl", "start": 17190770, "end": 17193540}, {"filename": "/data/shaders/webgl_cache/negative.glsl", "start": 17193540, "end": 17196165}, {"filename": "/data/shaders/webgl_cache/neon_mouse.glsl", "start": 17196165, "end": 17199661}, {"filename": "/data/shaders/webgl_cache/new_fractal.glsl", "start": 17199661, "end": 17205476}, {"filename": "/data/shaders/webgl_cache/new_fractal2.glsl", "start": 17205476, "end": 17211309}, {"filename": "/data/shaders/webgl_cache/new_glitch.glsl", "start": 17211309, "end": 17214481}, {"filename": "/data/shaders/webgl_cache/new_xor_scale_strobe.glsl", "start": 17214481, "end": 17217590}, {"filename": "/data/shaders/webgl_cache/nofilter.glsl", "start": 17217590, "end": 17220690}, {"filename": "/data/shaders/webgl_cache/nothing.glsl", "start": 17220690, "end": 17223209}, {"filename": "/data/shaders/webgl_cache/o1-hue.glsl", "start": 17223209, "end": 17227928}, {"filename": "/data/shaders/webgl_cache/o3-01.glsl", "start": 17227928, "end": 17232226}, {"filename": "/data/shaders/webgl_cache/obj_stretch2.glsl", "start": 17232226, "end": 17235429}, {"filename": "/data/shaders/webgl_cache/ocean.glsl", "start": 17235429, "end": 17238326}, {"filename": "/data/shaders/webgl_cache/ocean_fast.glsl", "start": 17238326, "end": 17241224}, {"filename": "/data/shaders/webgl_cache/ol.glsl", "start": 17241224, "end": 17244979}, {"filename": "/data/shaders/webgl_cache/old-film-water.glsl", "start": 17244979, "end": 17248358}, {"filename": "/data/shaders/webgl_cache/old-film.glsl", "start": 17248358, "end": 17251271}, {"filename": "/data/shaders/webgl_cache/onoffsep.glsl", "start": 17251271, "end": 17254519}, {"filename": "/data/shaders/webgl_cache/open_shadow.glsl", "start": 17254519, "end": 17257499}, {"filename": "/data/shaders/webgl_cache/open_shadow_jump.glsl", "start": 17257499, "end": 17260775}, {"filename": "/data/shaders/webgl_cache/open_shadow_multi.glsl", "start": 17260775, "end": 17264037}, {"filename": "/data/shaders/webgl_cache/open_shadow_multi_x2.glsl", "start": 17264037, "end": 17267300}, {"filename": "/data/shaders/webgl_cache/open_up.glsl", "start": 17267300, "end": 17270493}, {"filename": "/data/shaders/webgl_cache/page_turn.glsl", "start": 17270493, "end": 17273277}, {"filename": "/data/shaders/webgl_cache/page_turn2.glsl", "start": 17273277, "end": 17276256}, {"filename": "/data/shaders/webgl_cache/page_turn3.glsl", "start": 17276256, "end": 17279249}, {"filename": "/data/shaders/webgl_cache/particle_1.glsl", "start": 17279249, "end": 17283038}, {"filename": "/data/shaders/webgl_cache/pastel.glsl", "start": 17283038, "end": 17286834}, {"filename": "/data/shaders/webgl_cache/pebble.glsl", "start": 17286834, "end": 17289789}, {"filename": "/data/shaders/webgl_cache/pebble_fast.glsl", "start": 17289789, "end": 17292745}, {"filename": "/data/shaders/webgl_cache/pinch.glsl", "start": 17292745, "end": 17295776}, {"filename": "/data/shaders/webgl_cache/pixels.glsl", "start": 17295776, "end": 17298444}, {"filename": "/data/shaders/webgl_cache/plasma2.glsl", "start": 17298444, "end": 17302604}, {"filename": "/data/shaders/webgl_cache/plasma3.glsl", "start": 17302604, "end": 17306958}, {"filename": "/data/shaders/webgl_cache/plasma_prism.glsl", "start": 17306958, "end": 17310328}, {"filename": "/data/shaders/webgl_cache/pong-ataan-ex.glsl", "start": 17310328, "end": 17313865}, {"filename": "/data/shaders/webgl_cache/pong-atan.glsl", "start": 17313865, "end": 17316890}, {"filename": "/data/shaders/webgl_cache/pong-atan2.glsl", "start": 17316890, "end": 17320298}, {"filename": "/data/shaders/webgl_cache/pong-atan3.glsl", "start": 17320298, "end": 17323723}, {"filename": "/data/shaders/webgl_cache/pong_pi.mp4.glsl", "start": 17323723, "end": 17327034}, {"filename": "/data/shaders/webgl_cache/pong_tex.glsl", "start": 17327034, "end": 17329973}, {"filename": "/data/shaders/webgl_cache/pool.glsl", "start": 17329973, "end": 17332717}, {"filename": "/data/shaders/webgl_cache/prism.glsl", "start": 17332717, "end": 17336513}, {"filename": "/data/shaders/webgl_cache/psych.glsl", "start": 17336513, "end": 17339762}, {"filename": "/data/shaders/webgl_cache/psyche_ripple.glsl", "start": 17339762, "end": 17342876}, {"filename": "/data/shaders/webgl_cache/psyche_wave.glsl", "start": 17342876, "end": 17345773}, {"filename": "/data/shaders/webgl_cache/pull.glsl", "start": 17345773, "end": 17348592}, {"filename": "/data/shaders/webgl_cache/pull_out.glsl", "start": 17348592, "end": 17351416}, {"filename": "/data/shaders/webgl_cache/purple_fade.glsl", "start": 17351416, "end": 17354382}, {"filename": "/data/shaders/webgl_cache/purple_haze.glsl", "start": 17354382, "end": 17358820}, {"filename": "/data/shaders/webgl_cache/putty.glsl", "start": 17358820, "end": 17361947}, {"filename": "/data/shaders/webgl_cache/radial.glsl", "start": 17361947, "end": 17364795}, {"filename": "/data/shaders/webgl_cache/radial_distortion.glsl", "start": 17364795, "end": 17368365}, {"filename": "/data/shaders/webgl_cache/radial_distortion2.glsl", "start": 17368365, "end": 17371482}, {"filename": "/data/shaders/webgl_cache/radwarp.glsl", "start": 17371482, "end": 17374362}, {"filename": "/data/shaders/webgl_cache/rainbow-touch.glsl", "start": 17374362, "end": 17377536}, {"filename": "/data/shaders/webgl_cache/rainbow.banding.glsl", "start": 17377536, "end": 17380742}, {"filename": "/data/shaders/webgl_cache/rainbow_cd.glsl", "start": 17380742, "end": 17385267}, {"filename": "/data/shaders/webgl_cache/rainbow_cd_curtain.glsl", "start": 17385267, "end": 17389958}, {"filename": "/data/shaders/webgl_cache/rainbow_cd_spin.glsl", "start": 17389958, "end": 17394566}, {"filename": "/data/shaders/webgl_cache/rainbow_cd_strobe.glsl", "start": 17394566, "end": 17399153}, {"filename": "/data/shaders/webgl_cache/rainbow_color1.glsl", "start": 17399153, "end": 17402265}, {"filename": "/data/shaders/webgl_cache/rainbow_color2.glsl", "start": 17402265, "end": 17405397}, {"filename": "/data/shaders/webgl_cache/rainbow_color3.glsl", "start": 17405397, "end": 17408518}, {"filename": "/data/shaders/webgl_cache/rainbow_expand.glsl", "start": 17408518, "end": 17411867}, {"filename": "/data/shaders/webgl_cache/rainbow_expand_noise.glsl", "start": 17411867, "end": 17415895}, {"filename": "/data/shaders/webgl_cache/rainbow_fractal.glsl", "start": 17415895, "end": 17421664}, {"filename": "/data/shaders/webgl_cache/rainbow_fractal_random.glsl", "start": 17421664, "end": 17429930}, {"filename": "/data/shaders/webgl_cache/rainbow_ink.glsl", "start": 17429930, "end": 17433470}, {"filename": "/data/shaders/webgl_cache/rainbow_ink_t.glsl", "start": 17433470, "end": 17436969}, {"filename": "/data/shaders/webgl_cache/rainbow_left.glsl", "start": 17436969, "end": 17440031}, {"filename": "/data/shaders/webgl_cache/rainbow_light_swirl.glsl", "start": 17440031, "end": 17443725}, {"filename": "/data/shaders/webgl_cache/rainbow_mouse_x1.glsl", "start": 17443725, "end": 17446939}, {"filename": "/data/shaders/webgl_cache/rainbow_pi01.glsl", "start": 17446939, "end": 17450375}, {"filename": "/data/shaders/webgl_cache/rainbow_pi_neoin.glsl", "start": 17450375, "end": 17453840}, {"filename": "/data/shaders/webgl_cache/rainbow_rip_prism.glsl", "start": 17453840, "end": 17457751}, {"filename": "/data/shaders/webgl_cache/rainbow_spiral_rev2.glsl", "start": 17457751, "end": 17463459}, {"filename": "/data/shaders/webgl_cache/rainbow_swirl_limit.glsl", "start": 17463459, "end": 17466933}, {"filename": "/data/shaders/webgl_cache/rainbow_swirl_limit_discard.glsl", "start": 17466933, "end": 17470483}, {"filename": "/data/shaders/webgl_cache/rainbow_top.glsl", "start": 17470483, "end": 17473554}, {"filename": "/data/shaders/webgl_cache/rand_blocks.glsl", "start": 17473554, "end": 17476850}, {"filename": "/data/shaders/webgl_cache/random_pixels_static.glsl", "start": 17476850, "end": 17480405}, {"filename": "/data/shaders/webgl_cache/random_pos_fish.glsl", "start": 17480405, "end": 17483676}, {"filename": "/data/shaders/webgl_cache/random_resize.glsl", "start": 17483676, "end": 17487168}, {"filename": "/data/shaders/webgl_cache/random_soul.glsl", "start": 17487168, "end": 17490947}, {"filename": "/data/shaders/webgl_cache/random_soul_by_mouse.glsl", "start": 17490947, "end": 17494295}, {"filename": "/data/shaders/webgl_cache/react.glsl", "start": 17494295, "end": 17497022}, {"filename": "/data/shaders/webgl_cache/react10.glsl", "start": 17497022, "end": 17499829}, {"filename": "/data/shaders/webgl_cache/react11.glsl", "start": 17499829, "end": 17503088}, {"filename": "/data/shaders/webgl_cache/react12.glsl", "start": 17503088, "end": 17505823}, {"filename": "/data/shaders/webgl_cache/react13.glsl", "start": 17505823, "end": 17508651}, {"filename": "/data/shaders/webgl_cache/react14.glsl", "start": 17508651, "end": 17512547}, {"filename": "/data/shaders/webgl_cache/react15.glsl", "start": 17512547, "end": 17515476}, {"filename": "/data/shaders/webgl_cache/react16.glsl", "start": 17515476, "end": 17518748}, {"filename": "/data/shaders/webgl_cache/react17.glsl", "start": 17518748, "end": 17521794}, {"filename": "/data/shaders/webgl_cache/react18.glsl", "start": 17521794, "end": 17524754}, {"filename": "/data/shaders/webgl_cache/react2.glsl", "start": 17524754, "end": 17527535}, {"filename": "/data/shaders/webgl_cache/react3.glsl", "start": 17527535, "end": 17530291}, {"filename": "/data/shaders/webgl_cache/react4.glsl", "start": 17530291, "end": 17532958}, {"filename": "/data/shaders/webgl_cache/react5.glsl", "start": 17532958, "end": 17535866}, {"filename": "/data/shaders/webgl_cache/react6.glsl", "start": 17535866, "end": 17538856}, {"filename": "/data/shaders/webgl_cache/react7.glsl", "start": 17538856, "end": 17541820}, {"filename": "/data/shaders/webgl_cache/react8.glsl", "start": 17541820, "end": 17544931}, {"filename": "/data/shaders/webgl_cache/react9.glsl", "start": 17544931, "end": 17548358}, {"filename": "/data/shaders/webgl_cache/recep-2.glsl", "start": 17548358, "end": 17551319}, {"filename": "/data/shaders/webgl_cache/recep.glsl", "start": 17551319, "end": 17554257}, {"filename": "/data/shaders/webgl_cache/rect1.glsl", "start": 17554257, "end": 17557004}, {"filename": "/data/shaders/webgl_cache/reverse.glsl", "start": 17557004, "end": 17559933}, {"filename": "/data/shaders/webgl_cache/reverse_alpha.glsl", "start": 17559933, "end": 17562968}, {"filename": "/data/shaders/webgl_cache/reverse_vert.glsl", "start": 17562968, "end": 17565896}, {"filename": "/data/shaders/webgl_cache/reverse_xy.glsl", "start": 17565896, "end": 17568804}, {"filename": "/data/shaders/webgl_cache/rgb_blur.glsl", "start": 17568804, "end": 17571953}, {"filename": "/data/shaders/webgl_cache/rgbt.glsl", "start": 17571953, "end": 17575123}, {"filename": "/data/shaders/webgl_cache/rhue.glsl", "start": 17575123, "end": 17578544}, {"filename": "/data/shaders/webgl_cache/ripple-amp.glsl", "start": 17578544, "end": 17581818}, {"filename": "/data/shaders/webgl_cache/ripple_a.glsl", "start": 17581818, "end": 17585549}, {"filename": "/data/shaders/webgl_cache/ripple_cycle.glsl", "start": 17585549, "end": 17588367}, {"filename": "/data/shaders/webgl_cache/ripple_cycle2.glsl", "start": 17588367, "end": 17591545}, {"filename": "/data/shaders/webgl_cache/ripple_cycle_circle.glsl", "start": 17591545, "end": 17595104}, {"filename": "/data/shaders/webgl_cache/ripple_cycle_prism.glsl", "start": 17595104, "end": 17598150}, {"filename": "/data/shaders/webgl_cache/ripple_left.glsl", "start": 17598150, "end": 17600976}, {"filename": "/data/shaders/webgl_cache/ripple_mouse2.glsl", "start": 17600976, "end": 17604386}, {"filename": "/data/shaders/webgl_cache/ripple_out.glsl", "start": 17604386, "end": 17607166}, {"filename": "/data/shaders/webgl_cache/ripple_outMouse.glsl", "start": 17607166, "end": 17610879}, {"filename": "/data/shaders/webgl_cache/ripple_ppong.glsl", "start": 17610879, "end": 17614139}, {"filename": "/data/shaders/webgl_cache/ripple_prism.glsl", "start": 17614139, "end": 17617299}, {"filename": "/data/shaders/webgl_cache/ripple_rainbow.glsl", "start": 17617299, "end": 17621498}, {"filename": "/data/shaders/webgl_cache/rotate_xyz.glsl", "start": 17621498, "end": 17625109}, {"filename": "/data/shaders/webgl_cache/rotate_xyz_zoom.glsl", "start": 17625109, "end": 17629013}, {"filename": "/data/shaders/webgl_cache/sac-geo-xor.glsl", "start": 17629013, "end": 17633021}, {"filename": "/data/shaders/webgl_cache/sac-geo.glsl", "start": 17633021, "end": 17636601}, {"filename": "/data/shaders/webgl_cache/sbrv.glsl", "start": 17636601, "end": 17639445}, {"filename": "/data/shaders/webgl_cache/scramble-2.glsl", "start": 17639445, "end": 17642489}, {"filename": "/data/shaders/webgl_cache/scramble-3.glsl", "start": 17642489, "end": 17645471}, {"filename": "/data/shaders/webgl_cache/scramble.glsl", "start": 17645471, "end": 17648257}, {"filename": "/data/shaders/webgl_cache/seek.glsl", "start": 17648257, "end": 17652550}, {"filename": "/data/shaders/webgl_cache/seek2.glsl", "start": 17652550, "end": 17656886}, {"filename": "/data/shaders/webgl_cache/sepia.glsl", "start": 17656886, "end": 17659748}, {"filename": "/data/shaders/webgl_cache/set_swirl_pos_mosue.glsl", "start": 17659748, "end": 17663349}, {"filename": "/data/shaders/webgl_cache/shake.glsl", "start": 17663349, "end": 17666026}, {"filename": "/data/shaders/webgl_cache/shrink_texture.glsl", "start": 17666026, "end": 17668774}, {"filename": "/data/shaders/webgl_cache/shrink_texture_4.glsl", "start": 17668774, "end": 17671507}, {"filename": "/data/shaders/webgl_cache/sin_delic.glsl", "start": 17671507, "end": 17674755}, {"filename": "/data/shaders/webgl_cache/sine1.glsl", "start": 17674755, "end": 17678056}, {"filename": "/data/shaders/webgl_cache/sine2.glsl", "start": 17678056, "end": 17681441}, {"filename": "/data/shaders/webgl_cache/sine_grad.glsl", "start": 17681441, "end": 17684683}, {"filename": "/data/shaders/webgl_cache/sine_grad2.glsl", "start": 17684683, "end": 17688177}, {"filename": "/data/shaders/webgl_cache/sine_grad3.glsl", "start": 17688177, "end": 17691589}, {"filename": "/data/shaders/webgl_cache/sine_grad4.glsl", "start": 17691589, "end": 17695019}, {"filename": "/data/shaders/webgl_cache/sine_grad5.glsl", "start": 17695019, "end": 17698532}, {"filename": "/data/shaders/webgl_cache/sine_grad6.glsl", "start": 17698532, "end": 17702250}, {"filename": "/data/shaders/webgl_cache/sine_grad7.glsl", "start": 17702250, "end": 17705983}, {"filename": "/data/shaders/webgl_cache/sine_ripple.glsl", "start": 17705983, "end": 17709778}, {"filename": "/data/shaders/webgl_cache/sine_swirl.glsl", "start": 17709778, "end": 17713310}, {"filename": "/data/shaders/webgl_cache/sine_time.glsl", "start": 17713310, "end": 17716215}, {"filename": "/data/shaders/webgl_cache/sing.glsl", "start": 17716215, "end": 17719063}, {"filename": "/data/shaders/webgl_cache/single-glittch.glsl", "start": 17719063, "end": 17722757}, {"filename": "/data/shaders/webgl_cache/singleton-noise.glsl", "start": 17722757, "end": 17726425}, {"filename": "/data/shaders/webgl_cache/skinny.glsl", "start": 17726425, "end": 17729135}, {"filename": "/data/shaders/webgl_cache/slither.glsl", "start": 17729135, "end": 17732546}, {"filename": "/data/shaders/webgl_cache/slither_vert.glsl", "start": 17732546, "end": 17735971}, {"filename": "/data/shaders/webgl_cache/snake.glsl", "start": 17735971, "end": 17738739}, {"filename": "/data/shaders/webgl_cache/snake_dir.glsl", "start": 17738739, "end": 17741921}, {"filename": "/data/shaders/webgl_cache/snake_updown.glsl", "start": 17741921, "end": 17744690}, {"filename": "/data/shaders/webgl_cache/snes.glsl", "start": 17744690, "end": 17747458}, {"filename": "/data/shaders/webgl_cache/spaz.glsl", "start": 17747458, "end": 17750481}, {"filename": "/data/shaders/webgl_cache/spiral-aura.glsl", "start": 17750481, "end": 17753622}, {"filename": "/data/shaders/webgl_cache/spiral-center.glsl", "start": 17753622, "end": 17757706}, {"filename": "/data/shaders/webgl_cache/spiral-code-chromatic-tunnel.glsl", "start": 17757706, "end": 17761452}, {"filename": "/data/shaders/webgl_cache/spiral-code-cosmic-drain.glsl", "start": 17761452, "end": 17765305}, {"filename": "/data/shaders/webgl_cache/spiral-code-crystal-maelstrom.glsl", "start": 17765305, "end": 17769469}, {"filename": "/data/shaders/webgl_cache/spiral-code-double-helix.glsl", "start": 17769469, "end": 17773250}, {"filename": "/data/shaders/webgl_cache/spiral-code-electric-spiral.glsl", "start": 17773250, "end": 17777109}, {"filename": "/data/shaders/webgl_cache/spiral-code-flame-whorl.glsl", "start": 17777109, "end": 17781344}, {"filename": "/data/shaders/webgl_cache/spiral-code-fractal-coil.glsl", "start": 17781344, "end": 17785032}, {"filename": "/data/shaders/webgl_cache/spiral-code-galaxy-vortex.glsl", "start": 17785032, "end": 17788713}, {"filename": "/data/shaders/webgl_cache/spiral-code-hypnotic-spiral.glsl", "start": 17788713, "end": 17792431}, {"filename": "/data/shaders/webgl_cache/spiral-code-ice-spiral.glsl", "start": 17792431, "end": 17796210}, {"filename": "/data/shaders/webgl_cache/spiral-code-kaleido-bloom.glsl", "start": 17796210, "end": 17799726}, {"filename": "/data/shaders/webgl_cache/spiral-code-liquid-pinwheel.glsl", "start": 17799726, "end": 17803160}, {"filename": "/data/shaders/webgl_cache/spiral-code-log-polar-echo.glsl", "start": 17803160, "end": 17807192}, {"filename": "/data/shaders/webgl_cache/spiral-code-mirror-spiral.glsl", "start": 17807192, "end": 17810693}, {"filename": "/data/shaders/webgl_cache/spiral-code-nautilus-glass.glsl", "start": 17810693, "end": 17814396}, {"filename": "/data/shaders/webgl_cache/spiral-code-nebula-arms.glsl", "start": 17814396, "end": 17818550}, {"filename": "/data/shaders/webgl_cache/spiral-code-orbital-spiral.glsl", "start": 17818550, "end": 17822604}, {"filename": "/data/shaders/webgl_cache/spiral-code-pearl-swirl.glsl", "start": 17822604, "end": 17826319}, {"filename": "/data/shaders/webgl_cache/spiral-code-prism-cyclone.glsl", "start": 17826319, "end": 17829926}, {"filename": "/data/shaders/webgl_cache/spiral-code-quantum-spiral.glsl", "start": 17829926, "end": 17833846}, {"filename": "/data/shaders/webgl_cache/spiral-code-ripple-coil.glsl", "start": 17833846, "end": 17837322}, {"filename": "/data/shaders/webgl_cache/spiral-code-rose-vortex.glsl", "start": 17837322, "end": 17841027}, {"filename": "/data/shaders/webgl_cache/spiral-code-silk-spiral.glsl", "start": 17841027, "end": 17844751}, {"filename": "/data/shaders/webgl_cache/spiral-code-stardust-lens.glsl", "start": 17844751, "end": 17848805}, {"filename": "/data/shaders/webgl_cache/spiral-code-tidal-spiral.glsl", "start": 17848805, "end": 17852381}, {"filename": "/data/shaders/webgl_cache/spiral-mouse.glsl", "start": 17852381, "end": 17856654}, {"filename": "/data/shaders/webgl_cache/spiral_by_time.glsl", "start": 17856654, "end": 17860444}, {"filename": "/data/shaders/webgl_cache/spiral_edge.glsl", "start": 17860444, "end": 17863550}, {"filename": "/data/shaders/webgl_cache/spiral_mirror.glsl", "start": 17863550, "end": 17866678}, {"filename": "/data/shaders/webgl_cache/spiral_mirror_2.glsl", "start": 17866678, "end": 17870061}, {"filename": "/data/shaders/webgl_cache/spiral_mirror_rev.glsl", "start": 17870061, "end": 17873193}, {"filename": "/data/shaders/webgl_cache/spiral_mirror_top.glsl", "start": 17873193, "end": 17876358}, {"filename": "/data/shaders/webgl_cache/spiral_mirror_wave.glsl", "start": 17876358, "end": 17879719}, {"filename": "/data/shaders/webgl_cache/spiral_square.glsl", "start": 17879719, "end": 17882569}, {"filename": "/data/shaders/webgl_cache/spiral_wave_1.glsl", "start": 17882569, "end": 17885700}, {"filename": "/data/shaders/webgl_cache/splash-x.glsl", "start": 17885700, "end": 17888548}, {"filename": "/data/shaders/webgl_cache/splash-y.glsl", "start": 17888548, "end": 17891396}, {"filename": "/data/shaders/webgl_cache/splash.glsl", "start": 17891396, "end": 17894237}, {"filename": "/data/shaders/webgl_cache/srainbow.glsl", "start": 17894237, "end": 17897902}, {"filename": "/data/shaders/webgl_cache/star5.glsl", "start": 17897902, "end": 17901345}, {"filename": "/data/shaders/webgl_cache/starX.glsl", "start": 17901345, "end": 17904509}, {"filename": "/data/shaders/webgl_cache/starX2.glsl", "start": 17904509, "end": 17907673}, {"filename": "/data/shaders/webgl_cache/starX3.glsl", "start": 17907673, "end": 17910880}, {"filename": "/data/shaders/webgl_cache/starX4.glsl", "start": 17910880, "end": 17914016}, {"filename": "/data/shaders/webgl_cache/strobe.glsl", "start": 17914016, "end": 17917017}, {"filename": "/data/shaders/webgl_cache/strobe_light.glsl", "start": 17917017, "end": 17919992}, {"filename": "/data/shaders/webgl_cache/surround.glsl", "start": 17919992, "end": 17924400}, {"filename": "/data/shaders/webgl_cache/sweb.glsl", "start": 17924400, "end": 17928066}, {"filename": "/data/shaders/webgl_cache/swirlMouse.glsl", "start": 17928066, "end": 17932316}, {"filename": "/data/shaders/webgl_cache/swirl_by_mouse.glsl", "start": 17932316, "end": 17936953}, {"filename": "/data/shaders/webgl_cache/swirl_by_mouse2.glsl", "start": 17936953, "end": 17941725}, {"filename": "/data/shaders/webgl_cache/swirl_by_mouse_full.glsl", "start": 17941725, "end": 17945799}, {"filename": "/data/shaders/webgl_cache/swirl_by_mouse_pos.glsl", "start": 17945799, "end": 17949436}, {"filename": "/data/shaders/webgl_cache/tear.glsl", "start": 17949436, "end": 17952247}, {"filename": "/data/shaders/webgl_cache/tearing.glsl", "start": 17952247, "end": 17955084}, {"filename": "/data/shaders/webgl_cache/tearing_max.glsl", "start": 17955084, "end": 17958330}, {"filename": "/data/shaders/webgl_cache/tearing_single.glsl", "start": 17958330, "end": 17960983}, {"filename": "/data/shaders/webgl_cache/tearing_spiral.glsl", "start": 17960983, "end": 17964456}, {"filename": "/data/shaders/webgl_cache/tex_fold.glsl", "start": 17964456, "end": 17967377}, {"filename": "/data/shaders/webgl_cache/thick_glass.glsl", "start": 17967377, "end": 17970260}, {"filename": "/data/shaders/webgl_cache/time_frac.glsl", "start": 17970260, "end": 17973221}, {"filename": "/data/shaders/webgl_cache/time_frac_xor.glsl", "start": 17973221, "end": 17976621}, {"filename": "/data/shaders/webgl_cache/tremor1.glsl", "start": 17976621, "end": 17979718}, {"filename": "/data/shaders/webgl_cache/tremor2.glsl", "start": 17979718, "end": 17983052}, {"filename": "/data/shaders/webgl_cache/tremor3.glsl", "start": 17983052, "end": 17986203}, {"filename": "/data/shaders/webgl_cache/tremor4.glsl", "start": 17986203, "end": 17989553}, {"filename": "/data/shaders/webgl_cache/tridim.glsl", "start": 17989553, "end": 17992659}, {"filename": "/data/shaders/webgl_cache/tripple.glsl", "start": 17992659, "end": 17995434}, {"filename": "/data/shaders/webgl_cache/tripple2.glsl", "start": 17995434, "end": 17998443}, {"filename": "/data/shaders/webgl_cache/triwavedistort.glsl", "start": 17998443, "end": 18001361}, {"filename": "/data/shaders/webgl_cache/tv_show.glsl", "start": 18001361, "end": 18004455}, {"filename": "/data/shaders/webgl_cache/twarp.glsl", "start": 18004455, "end": 18007181}, {"filename": "/data/shaders/webgl_cache/twarp2.glsl", "start": 18007181, "end": 18009921}, {"filename": "/data/shaders/webgl_cache/twirl_tex.glsl", "start": 18009921, "end": 18012912}, {"filename": "/data/shaders/webgl_cache/twist-code-chromatic-vortex.glsl", "start": 18012912, "end": 18016279}, {"filename": "/data/shaders/webgl_cache/twist-code-cosmic-auger.glsl", "start": 18016279, "end": 18020072}, {"filename": "/data/shaders/webgl_cache/twist-code-deep-space-drill.glsl", "start": 18020072, "end": 18023576}, {"filename": "/data/shaders/webgl_cache/twist-code-double-helix.glsl", "start": 18023576, "end": 18027008}, {"filename": "/data/shaders/webgl_cache/twist-code-event-horizon.glsl", "start": 18027008, "end": 18030556}, {"filename": "/data/shaders/webgl_cache/twist-code-feedback-spiral.glsl", "start": 18030556, "end": 18033995}, {"filename": "/data/shaders/webgl_cache/twist-code-fractal-conduit.glsl", "start": 18033995, "end": 18037511}, {"filename": "/data/shaders/webgl_cache/twist-code-gravity-well.glsl", "start": 18037511, "end": 18041003}, {"filename": "/data/shaders/webgl_cache/twist-code-hypercube-vortex.glsl", "start": 18041003, "end": 18044779}, {"filename": "/data/shaders/webgl_cache/twist-code-infinite-bore.glsl", "start": 18044779, "end": 18048378}, {"filename": "/data/shaders/webgl_cache/twist-code-kaleido-throat.glsl", "start": 18048378, "end": 18051746}, {"filename": "/data/shaders/webgl_cache/twist-code-mirror-abyss.glsl", "start": 18051746, "end": 18055092}, {"filename": "/data/shaders/webgl_cache/twist-code-neon-collapse.glsl", "start": 18055092, "end": 18058487}, {"filename": "/data/shaders/webgl_cache/twist-code-octave-wormhole.glsl", "start": 18058487, "end": 18061926}, {"filename": "/data/shaders/webgl_cache/twist-code-orbital-rip.glsl", "start": 18061926, "end": 18065358}, {"filename": "/data/shaders/webgl_cache/twist-code-parallel-helix.glsl", "start": 18065358, "end": 18068845}, {"filename": "/data/shaders/webgl_cache/twist-code-phase-cyclone.glsl", "start": 18068845, "end": 18072216}, {"filename": "/data/shaders/webgl_cache/twist-code-prism-singularity.glsl", "start": 18072216, "end": 18075696}, {"filename": "/data/shaders/webgl_cache/twist-code-quantum-tunnel.glsl", "start": 18075696, "end": 18079099}, {"filename": "/data/shaders/webgl_cache/twist-code-radial-overdrive.glsl", "start": 18079099, "end": 18082430}, {"filename": "/data/shaders/webgl_cache/twist-code-serpent-tunnel.glsl", "start": 18082430, "end": 18085835}, {"filename": "/data/shaders/webgl_cache/twist-code-singularity-maelstrom.glsl", "start": 18085835, "end": 18089465}, {"filename": "/data/shaders/webgl_cache/twist-code-spiral-shockwave.glsl", "start": 18089465, "end": 18092825}, {"filename": "/data/shaders/webgl_cache/twist-code-tidal-twister.glsl", "start": 18092825, "end": 18096205}, {"filename": "/data/shaders/webgl_cache/twist-code-turbine-rift.glsl", "start": 18096205, "end": 18099595}, {"filename": "/data/shaders/webgl_cache/twist.glsl", "start": 18099595, "end": 18102938}, {"filename": "/data/shaders/webgl_cache/twist_directions.glsl", "start": 18102938, "end": 18106409}, {"filename": "/data/shaders/webgl_cache/twistex.glsl", "start": 18106409, "end": 18109942}, {"filename": "/data/shaders/webgl_cache/underwaterenchanced.glsl", "start": 18109942, "end": 18113380}, {"filename": "/data/shaders/webgl_cache/vhs-code-chroma-bleed.glsl", "start": 18113380, "end": 18117207}, {"filename": "/data/shaders/webgl_cache/vhs-code-dropout.glsl", "start": 18117207, "end": 18120949}, {"filename": "/data/shaders/webgl_cache/vhs-code-ghosting.glsl", "start": 18120949, "end": 18124675}, {"filename": "/data/shaders/webgl_cache/vhs-code-head-switch.glsl", "start": 18124675, "end": 18128356}, {"filename": "/data/shaders/webgl_cache/vhs-code-home-movie.glsl", "start": 18128356, "end": 18132123}, {"filename": "/data/shaders/webgl_cache/vhs-code-kung-fury.glsl", "start": 18132123, "end": 18137869}, {"filename": "/data/shaders/webgl_cache/vhs-code-pause-jitter.glsl", "start": 18137869, "end": 18141624}, {"filename": "/data/shaders/webgl_cache/vhs-code-rf-noise.glsl", "start": 18141624, "end": 18145351}, {"filename": "/data/shaders/webgl_cache/vhs-code-tape-warp.glsl", "start": 18145351, "end": 18149033}, {"filename": "/data/shaders/webgl_cache/vhs-code-tracking-roll.glsl", "start": 18149033, "end": 18152677}, {"filename": "/data/shaders/webgl_cache/vhs-code-worn-tape.glsl", "start": 18152677, "end": 18156407}, {"filename": "/data/shaders/webgl_cache/vhs-color-mode.glsl", "start": 18156407, "end": 18161958}, {"filename": "/data/shaders/webgl_cache/vhs-palette.glsl", "start": 18161958, "end": 18164668}, {"filename": "/data/shaders/webgl_cache/vhs.glsl", "start": 18164668, "end": 18167946}, {"filename": "/data/shaders/webgl_cache/vhs2.glsl", "start": 18167946, "end": 18173732}, {"filename": "/data/shaders/webgl_cache/vhs_color.glsl", "start": 18173732, "end": 18176686}, {"filename": "/data/shaders/webgl_cache/vhs_damage.glsl", "start": 18176686, "end": 18181235}, {"filename": "/data/shaders/webgl_cache/warp-atan.glsl", "start": 18181235, "end": 18184464}, {"filename": "/data/shaders/webgl_cache/warp-ppong-time.glsl", "start": 18184464, "end": 18187689}, {"filename": "/data/shaders/webgl_cache/warp_ppong.glsl", "start": 18187689, "end": 18190597}, {"filename": "/data/shaders/webgl_cache/warp_ppong_by_mouse.glsl", "start": 18190597, "end": 18193800}, {"filename": "/data/shaders/webgl_cache/warp_ppong_mouse.glsl", "start": 18193800, "end": 18198645}, {"filename": "/data/shaders/webgl_cache/water-cursor.glsl", "start": 18198645, "end": 18202282}, {"filename": "/data/shaders/webgl_cache/water.glsl", "start": 18202282, "end": 18205200}, {"filename": "/data/shaders/webgl_cache/water_full.glsl", "start": 18205200, "end": 18208146}, {"filename": "/data/shaders/webgl_cache/water_hq_01_caustic_shallows.glsl", "start": 18208146, "end": 18211384}, {"filename": "/data/shaders/webgl_cache/water_hq_02_ocean_swell.glsl", "start": 18211384, "end": 18214520}, {"filename": "/data/shaders/webgl_cache/water_hq_03_rain_ripples.glsl", "start": 18214520, "end": 18217944}, {"filename": "/data/shaders/webgl_cache/water_hq_04_glass_refraction.glsl", "start": 18217944, "end": 18221243}, {"filename": "/data/shaders/webgl_cache/water_hq_05_deep_current.glsl", "start": 18221243, "end": 18224761}, {"filename": "/data/shaders/webgl_cache/water_hq_06_pool_caustics.glsl", "start": 18224761, "end": 18227958}, {"filename": "/data/shaders/webgl_cache/water_hq_07_river_flow.glsl", "start": 18227958, "end": 18231358}, {"filename": "/data/shaders/webgl_cache/water_hq_08_whirlpool.glsl", "start": 18231358, "end": 18234611}, {"filename": "/data/shaders/webgl_cache/water_hq_09_choppy_surface.glsl", "start": 18234611, "end": 18237862}, {"filename": "/data/shaders/webgl_cache/water_hq_10_calm_lake.glsl", "start": 18237862, "end": 18241156}, {"filename": "/data/shaders/webgl_cache/water_hq_11_tidal_lens.glsl", "start": 18241156, "end": 18244291}, {"filename": "/data/shaders/webgl_cache/water_hq_12_cross_sea.glsl", "start": 18244291, "end": 18247462}, {"filename": "/data/shaders/webgl_cache/water_hq_13_underwater_drift.glsl", "start": 18247462, "end": 18250938}, {"filename": "/data/shaders/webgl_cache/water_hq_14_droplet_lenses.glsl", "start": 18250938, "end": 18254404}, {"filename": "/data/shaders/webgl_cache/water_hq_15_bubble_stream.glsl", "start": 18254404, "end": 18257898}, {"filename": "/data/shaders/webgl_cache/water_hq_16_shoreline.glsl", "start": 18257898, "end": 18261185}, {"filename": "/data/shaders/webgl_cache/water_hq_17_liquid_mirror.glsl", "start": 18261185, "end": 18264455}, {"filename": "/data/shaders/webgl_cache/water_hq_18_storm_surface.glsl", "start": 18264455, "end": 18267772}, {"filename": "/data/shaders/webgl_cache/water_hq_19_crystal_water.glsl", "start": 18267772, "end": 18271399}, {"filename": "/data/shaders/webgl_cache/water_hq_20_moonlit_water.glsl", "start": 18271399, "end": 18274717}, {"filename": "/data/shaders/webgl_cache/water_hq_21_aqua_prism.glsl", "start": 18274717, "end": 18277962}, {"filename": "/data/shaders/webgl_cache/water_hq_22_thermal_spring.glsl", "start": 18277962, "end": 18281384}, {"filename": "/data/shaders/webgl_cache/water_hq_23_silk_current.glsl", "start": 18281384, "end": 18284631}, {"filename": "/data/shaders/webgl_cache/water_hq_24_waterfall.glsl", "start": 18284631, "end": 18288200}, {"filename": "/data/shaders/webgl_cache/water_hq_25_coral_lagoon.glsl", "start": 18288200, "end": 18291549}, {"filename": "/data/shaders/webgl_cache/water_prism.glsl", "start": 18291549, "end": 18295288}, {"filename": "/data/shaders/webgl_cache/water_r.glsl", "start": 18295288, "end": 18299164}, {"filename": "/data/shaders/webgl_cache/water_rgb.glsl", "start": 18299164, "end": 18303672}, {"filename": "/data/shaders/webgl_cache/wave-l.glsl", "start": 18303672, "end": 18306413}, {"filename": "/data/shaders/webgl_cache/wave_diag.glsl", "start": 18306413, "end": 18309512}, {"filename": "/data/shaders/webgl_cache/wave_spiral.glsl", "start": 18309512, "end": 18312726}, {"filename": "/data/shaders/webgl_cache/whirlx.glsl", "start": 18312726, "end": 18315815}, {"filename": "/data/shaders/webgl_cache/whitelight.glsl", "start": 18315815, "end": 18318737}, {"filename": "/data/shaders/webgl_cache/wlight.glsl", "start": 18318737, "end": 18322863}, {"filename": "/data/shaders/webgl_cache/wspiral.glsl", "start": 18322863, "end": 18326866}, {"filename": "/data/shaders/webgl_cache/xorMouse.glsl", "start": 18326866, "end": 18330868}, {"filename": "/data/shaders/webgl_cache/xor_code_bitplane_aurora.glsl", "start": 18330868, "end": 18334844}, {"filename": "/data/shaders/webgl_cache/xor_code_cosmic_halftone.glsl", "start": 18334844, "end": 18338754}, {"filename": "/data/shaders/webgl_cache/xor_code_glass_circuit.glsl", "start": 18338754, "end": 18342792}, {"filename": "/data/shaders/webgl_cache/xor_code_neon_topology.glsl", "start": 18342792, "end": 18346580}, {"filename": "/data/shaders/webgl_cache/xor_code_opal_feedback.glsl", "start": 18346580, "end": 18350503}, {"filename": "/data/shaders/webgl_cache/xor_code_prismatic_bloom.glsl", "start": 18350503, "end": 18354628}, {"filename": "/data/shaders/webgl_cache/xor_code_quantum_palette.glsl", "start": 18354628, "end": 18358445}, {"filename": "/data/shaders/webgl_cache/xor_code_rgb_smooth_aurora_blend.glsl", "start": 18358445, "end": 18362570}, {"filename": "/data/shaders/webgl_cache/xor_code_rgb_smooth_liquid_pastel.glsl", "start": 18362570, "end": 18366732}, {"filename": "/data/shaders/webgl_cache/xor_code_rgb_smooth_silk_gradient.glsl", "start": 18366732, "end": 18370891}, {"filename": "/data/shaders/webgl_cache/xor_code_rgb_smooth_spectral_mist.glsl", "start": 18370891, "end": 18374875}, {"filename": "/data/shaders/webgl_cache/xor_code_rgb_smooth_velvet_prism.glsl", "start": 18374875, "end": 18379031}, {"filename": "/data/shaders/webgl_cache/xor_code_solarized_ribbons.glsl", "start": 18379031, "end": 18382884}, {"filename": "/data/shaders/webgl_cache/xor_rgb_rainbow_mouse.glsl", "start": 18382884, "end": 18387583}, {"filename": "/data/shaders/webgl_cache/yin-yang.glsl", "start": 18387583, "end": 18391012}, {"filename": "/data/shaders/webgl_cache/zigzag.glsl", "start": 18391012, "end": 18394036}, {"filename": "/data/shaders/webgl_cache/zoom_fish.glsl", "start": 18394036, "end": 18397683}, {"filename": "/data/shaders/webgl_cache/zoom_in_out_mouse.glsl", "start": 18397683, "end": 18400872}, {"filename": "/data/shaders/webgl_cache/zoom_pong.glsl", "start": 18400872, "end": 18403992}, {"filename": "/data/shaders/webgl_cache/zoom_pulse3.glsl", "start": 18403992, "end": 18406917}, {"filename": "/data/shaders/weirdlines.glsl", "start": 18406917, "end": 18408037}, {"filename": "/data/shaders/whirlx.glsl", "start": 18408037, "end": 18408726}, {"filename": "/data/shaders/whitelight.glsl", "start": 18408726, "end": 18409247}, {"filename": "/data/shaders/wlight.glsl", "start": 18409247, "end": 18410972}, {"filename": "/data/shaders/wormhole_audio.glsl", "start": 18410972, "end": 18419730}, {"filename": "/data/shaders/wrap_around.glsl", "start": 18419730, "end": 18420510}, {"filename": "/data/shaders/wspiral.glsl", "start": 18420510, "end": 18422112}, {"filename": "/data/shaders/xcordstrobe.glsl", "start": 18422112, "end": 18423144}, {"filename": "/data/shaders/xorMouse.glsl", "start": 18423144, "end": 18424823}, {"filename": "/data/shaders/xor_code_bitplane_aurora.glsl", "start": 18424823, "end": 18426403}, {"filename": "/data/shaders/xor_code_chromatic_weave.glsl", "start": 18426403, "end": 18427745}, {"filename": "/data/shaders/xor_code_cosmic_halftone.glsl", "start": 18427745, "end": 18429259}, {"filename": "/data/shaders/xor_code_glass_circuit.glsl", "start": 18429259, "end": 18430901}, {"filename": "/data/shaders/xor_code_kaleido_spectrum.glsl", "start": 18430901, "end": 18432348}, {"filename": "/data/shaders/xor_code_neon_topology.glsl", "start": 18432348, "end": 18433740}, {"filename": "/data/shaders/xor_code_opal_feedback.glsl", "start": 18433740, "end": 18435267}, {"filename": "/data/shaders/xor_code_prismatic_bloom.glsl", "start": 18435267, "end": 18436996}, {"filename": "/data/shaders/xor_code_quantum_palette.glsl", "start": 18436996, "end": 18438417}, {"filename": "/data/shaders/xor_code_rgb_smooth_aurora_blend.glsl", "start": 18438417, "end": 18440146}, {"filename": "/data/shaders/xor_code_rgb_smooth_liquid_pastel.glsl", "start": 18440146, "end": 18441912}, {"filename": "/data/shaders/xor_code_rgb_smooth_silk_gradient.glsl", "start": 18441912, "end": 18443675}, {"filename": "/data/shaders/xor_code_rgb_smooth_spectral_mist.glsl", "start": 18443675, "end": 18445263}, {"filename": "/data/shaders/xor_code_rgb_smooth_velvet_prism.glsl", "start": 18445263, "end": 18447023}, {"filename": "/data/shaders/xor_code_solarized_ribbons.glsl", "start": 18447023, "end": 18448480}, {"filename": "/data/shaders/xor_increase.glsl", "start": 18448480, "end": 18449755}, {"filename": "/data/shaders/xor_positional_offset.glsl", "start": 18449755, "end": 18451003}, {"filename": "/data/shaders/xor_rgb.glsl", "start": 18451003, "end": 18451809}, {"filename": "/data/shaders/xor_rgb_dark.glsl", "start": 18451809, "end": 18454039}, {"filename": "/data/shaders/xor_rgb_dark_2.glsl", "start": 18454039, "end": 18456275}, {"filename": "/data/shaders/xor_rgb_dark_3.glsl", "start": 18456275, "end": 18458509}, {"filename": "/data/shaders/xor_rgb_dark_4.glsl", "start": 18458509, "end": 18460788}, {"filename": "/data/shaders/xor_rgb_dark_rainbow.glsl", "start": 18460788, "end": 18463608}, {"filename": "/data/shaders/xor_rgb_new.glsl", "start": 18463608, "end": 18465036}, {"filename": "/data/shaders/xor_rgb_rainbow_1.glsl", "start": 18465036, "end": 18468633}, {"filename": "/data/shaders/xor_rgb_rainbow_mouse.glsl", "start": 18468633, "end": 18471014}, {"filename": "/data/shaders/xor_rgb_rainbow_swirl_limit.glsl", "start": 18471014, "end": 18473943}, {"filename": "/data/shaders/xor_rgb_smooth.glsl", "start": 18473943, "end": 18476110}, {"filename": "/data/shaders/xor_rgb_smoother.glsl", "start": 18476110, "end": 18478326}, {"filename": "/data/shaders/xor_rgb_smoother_mouse.glsl", "start": 18478326, "end": 18480805}, {"filename": "/data/shaders/xor_sine_swirl.glsl", "start": 18480805, "end": 18482302}, {"filename": "/data/shaders/xorsheet.glsl", "start": 18482302, "end": 18483729}, {"filename": "/data/shaders/xorsheet_2.glsl", "start": 18483729, "end": 18484779}, {"filename": "/data/shaders/xorstrobe.glsl", "start": 18484779, "end": 18485895}, {"filename": "/data/shaders/yin-yang.glsl", "start": 18485895, "end": 18486923}, {"filename": "/data/shaders/zigzag.glsl", "start": 18486923, "end": 18487555}, {"filename": "/data/shaders/zoom_fish.glsl", "start": 18487555, "end": 18488855}, {"filename": "/data/shaders/zoom_in_out_mouse.glsl", "start": 18488855, "end": 18489677}, {"filename": "/data/shaders/zoom_pong.glsl", "start": 18489677, "end": 18490433}, {"filename": "/data/shaders/zoom_pulse.glsl", "start": 18490433, "end": 18491328}, {"filename": "/data/shaders/zoom_pulse2.glsl", "start": 18491328, "end": 18492412}, {"filename": "/data/shaders/zoom_pulse3.glsl", "start": 18492412, "end": 18492941}], "remote_package_size": 18492941});

  })();

// end include: /tmp/tmp63tejlnq.js
// include: /tmp/tmpsd66wfkx.js

    // All the pre-js content up to here must remain later on, we need to run
    // it.
    if ((typeof ENVIRONMENT_IS_WASM_WORKER != 'undefined' && ENVIRONMENT_IS_WASM_WORKER) || (typeof ENVIRONMENT_IS_PTHREAD != 'undefined' && ENVIRONMENT_IS_PTHREAD) || (typeof ENVIRONMENT_IS_AUDIO_WORKLET != 'undefined' && ENVIRONMENT_IS_AUDIO_WORKLET)) Module['preRun'] = [];
    var necessaryPreJSTasks = Module['preRun'].slice();
  // end include: /tmp/tmpsd66wfkx.js
// include: /tmp/tmpgeq_llna.js

    if (!Module['preRun']) throw 'Module.preRun should exist because file support used it; did a pre-js delete it?';
    necessaryPreJSTasks.forEach((task) => {
      if (Module['preRun'].indexOf(task) < 0) throw 'All preRun tasks that exist before user pre-js code should remain after; did you replace Module or modify Module.preRun?';
    });
  // end include: /tmp/tmpgeq_llna.js


var programArgs = [];
var thisProgram = './this.program';
var quit_ = (status, toThrow) => {
  throw toThrow;
};

// In MODULARIZE mode _scriptName needs to be captured already at the very top of the page immediately when the page is parsed, so it is generated there
// before the page load. In non-MODULARIZE modes generate it here.
var _scriptName = globalThis.document?.currentScript?.src;

// `/` should be present at the end if `scriptDirectory` is not empty
var scriptDirectory = '';
function locateFile(path) {
  if (Module['locateFile']) {
    return Module['locateFile'](path, scriptDirectory);
  }
  return scriptDirectory + path;
}

// Hooks that are implemented differently in different runtime environments.
var readAsync, readBinary;

if (ENVIRONMENT_IS_SHELL) {

} else

// Note that this includes Node.js workers when relevant (pthreads is enabled).
// Node.js workers are detected as a combination of ENVIRONMENT_IS_WORKER and
// ENVIRONMENT_IS_NODE.
if (ENVIRONMENT_IS_WEB || ENVIRONMENT_IS_WORKER) {
  try {
    scriptDirectory = new URL('.', _scriptName).href; // includes trailing slash
  } catch {
    // Must be a `blob:` or `data:` URL (e.g. `blob:http://site.com/etc/etc`), we cannot
    // infer anything from them.
  }

  if (!(globalThis.window || globalThis.WorkerGlobalScope)) throw new Error('not compiled for this environment (did you build to HTML and try to run it not on the web, or set ENVIRONMENT to something - like node - and run it someplace else - like on the web?)');

  {
// include: web_or_worker_shell_read.js
readAsync = async (url) => {
    assert(!isFileURI(url), "readAsync does not work with file:// URLs");
    var response = await fetch(url, { credentials: 'same-origin' });
    if (response.ok) {
      return response.arrayBuffer();
    }
    throw new Error(response.status + ' : ' + response.url);
  };
// end include: web_or_worker_shell_read.js
  }
} else
{
  throw new Error('environment detection error');
}

var out = console.log.bind(console);
var err = console.error.bind(console);

var IDBFS = 'IDBFS is no longer included by default; build with -lidbfs.js';
var PROXYFS = 'PROXYFS is no longer included by default; build with -lproxyfs.js';
var WORKERFS = 'WORKERFS is no longer included by default; build with -lworkerfs.js';
var FETCHFS = 'FETCHFS is no longer included by default; build with -lfetchfs.js';
var ICASEFS = 'ICASEFS is no longer included by default; build with -licasefs.js';
var JSFILEFS = 'JSFILEFS is no longer included by default; build with -ljsfilefs.js';
var OPFS = 'OPFS is no longer included by default; build with -lopfs.js';

var NODEFS = 'NODEFS is no longer included by default; build with -lnodefs.js';

// perform assertions in shell.js after we set up out() and err(), as otherwise
// if an assertion fails it cannot print the message

assert(!ENVIRONMENT_IS_WORKER, 'worker environment detected but not enabled at build time (add `worker` to `-sENVIRONMENT` to enable)');

assert(!ENVIRONMENT_IS_NODE, 'node environment detected but not enabled at build time (add `node` to `-sENVIRONMENT` to enable)');

assert(!ENVIRONMENT_IS_SHELL, 'shell environment detected but not enabled at build time (add `shell` to `-sENVIRONMENT` to enable)');

// end include: shell.js

// include: preamble.js
// === Preamble library stuff ===

// Documentation for the public APIs defined in this file must be updated in:
//    site/source/docs/api_reference/preamble.js.rst
// A prebuilt local version of the documentation is available at:
//    site/build/text/docs/api_reference/preamble.js.txt
// You can also build docs locally as HTML or other formats in site/
// An online HTML version (which may be of a different version of Emscripten)
//    is up at http://kripken.github.io/emscripten-site/docs/api_reference/preamble.js.html

var wasmBinary;

if (!globalThis.WebAssembly) {
  err('no native wasm support detected');
}

// Wasm globals

//========================================
// Runtime essentials
//========================================

// whether we are quitting the application. no code should run after this.
// set in exit() and abort()
var ABORT = false;

// set by exit() and abort().  Passed to 'onExit' handler.
// NOTE: This is also used as the process return code in shell environments
// but only when noExitRuntime is false.
var EXITSTATUS;

// In STRICT mode, we only define assert() when ASSERTIONS is set.  i.e. we
// don't define it at all in release modes.  This matches the behaviour of
// MINIMAL_RUNTIME.
// TODO(sbc): Make this the default even without STRICT enabled.
/** @type {function(*, string=)} */
function assert(condition, text) {
  if (!condition) {
    abort('Assertion failed' + (text ? ': ' + text : ''));
  }
}

// We used to include malloc/free by default in the past. Show a helpful error in
// builds with assertions.

/**
 * Indicates whether filename is delivered via file protocol (as opposed to http/https)
 * @noinline
 */
var isFileURI = (filename) => filename.startsWith('file://');

// include: runtime_common.js
// include: runtime_exceptions.js
// Base Emscripten EH error class
class EmscriptenEH extends Error {}

class EmscriptenSjLj extends EmscriptenEH {}

class CppException extends EmscriptenEH {
  constructor(excPtr) {
    super(excPtr);
    this.excPtr = excPtr;
    const excInfo = getExceptionMessage(this);
    this.name = excInfo[0];
    this.message = excInfo[1];
  }
}

// end include: runtime_exceptions.js
// include: runtime_debug.js
var runtimeDebug = true; // Switch to false at runtime to disable logging at the right times

// Used by XXXXX_DEBUG settings to output debug messages.
function dbg(...args) {
  if (!runtimeDebug && typeof runtimeDebug != 'undefined') return;
  // TODO(sbc): Make this configurable somehow.  Its not always convenient for
  // logging to show up as warnings.
  console.warn(...args);
}

// Endianness check
(() => {
  var h16 = new Int16Array(1);
  var h8 = new Int8Array(h16.buffer);
  h16[0] = 0x6373;
  if (h8[0] !== 0x73 || h8[1] !== 0x63) abort('Runtime error: expected the system to be little-endian! (Run with -sSUPPORT_BIG_ENDIAN to bypass)');
})();

function consumedModuleProp(prop) {
  var value = Module[prop];
  var msg = `Attempt to modify \`Module.${prop}\` after it has already been processed.  This can happen, for example, when code is injected via '--post-js' rather than '--pre-js'`;
  if (Array.isArray(value)) {
    value = new Proxy(value, {
      set(target, key, val) {
        abort(msg);
        return false;
      },
      defineProperty(target, key, descriptor) {
        abort(msg);
        return false;
      },
      deleteProperty(target, key) {
        abort(msg);
        return false;
      }
    });
  }
  Object.defineProperty(Module, prop, {
    configurable: true,
    get() { return value; },
    set() {
      abort(msg);
    }
  });
}

function makeInvalidEarlyAccess(name) {
  return () => assert(false, `call to '${name}' via reference taken before Wasm module initialization`);

}

function ignoredModuleProp(prop) {
  if (Object.getOwnPropertyDescriptor(Module, prop)) {
    abort(`\`Module.${prop}\` was supplied but \`${prop}\` not included in INCOMING_MODULE_JS_API`);
  }
}

// forcing the filesystem exports a few things by default
function isExportedByForceFilesystem(name) {
  return name === 'FS_createPath' ||
         name === 'FS_createDataFile' ||
         name === 'FS_createPreloadedFile' ||
         name === 'FS_preloadFile' ||
         name === 'FS_unlink' ||
         name === 'addRunDependency' ||
         // The old FS has some functionality that WasmFS lacks.
         name === 'FS_createLazyFile' ||
         name === 'FS_createDevice' ||
         name === 'removeRunDependency';
}

/**
 * Intercept access to a symbols in the global symbol.  This enables us to give
 * informative warnings/errors when folks attempt to use symbols they did not
 * include in their build, or no symbols that no longer exist.
 *
 * We don't define this in MODULARIZE mode since in that mode emscripten symbols
 * are never placed in the global scope.
 */
function hookGlobalSymbolAccess(sym, func) {
  if (!Object.getOwnPropertyDescriptor(globalThis, sym)) {
    Object.defineProperty(globalThis, sym, {
      configurable: true,
      get() {
        func();
        return undefined;
      }
    });
  }
}

function missingGlobal(sym, msg) {
  hookGlobalSymbolAccess(sym, () => {
    warnOnce(`\`${sym}\` is no longer defined by emscripten. ${msg}`);
  });
}

missingGlobal('buffer', 'Please use HEAP8.buffer or wasmMemory.buffer');
missingGlobal('asm', 'Please use wasmExports instead');

function missingLibrarySymbol(sym) {
  hookGlobalSymbolAccess(sym, () => {
    // Can't `abort()` here because it would break code that does runtime
    // checks.  e.g. `if (typeof SDL === 'undefined')`.
    var msg = `\`${sym}\` is a library symbol and not included by default; add it to your library.js __deps or to DEFAULT_LIBRARY_FUNCS_TO_INCLUDE on the command line`;
    // DEFAULT_LIBRARY_FUNCS_TO_INCLUDE requires the name as it appears in
    // library.js, which means $name for a JS name with no prefix, or name
    // for a JS name like _name.
    var librarySymbol = sym;
    if (!librarySymbol.startsWith('_')) {
      librarySymbol = '$' + sym;
    }
    msg += ` (e.g. -sDEFAULT_LIBRARY_FUNCS_TO_INCLUDE='${librarySymbol}')`;
    if (isExportedByForceFilesystem(sym)) {
      msg += '. Alternatively, forcing filesystem support (-sFORCE_FILESYSTEM) can export this for you';
    }
    warnOnce(msg);
  });

  // Any symbol that is not included from the JS library is also (by definition)
  // not exported on the Module object.
  unexportedRuntimeSymbol(sym);
}

function unexportedRuntimeSymbol(sym) {
  if (!Object.getOwnPropertyDescriptor(Module, sym)) {
    Object.defineProperty(Module, sym, {
      configurable: true,
      get() {
        var msg = `'${sym}' was not exported. add it to EXPORTED_RUNTIME_METHODS (see the Emscripten FAQ)`;
        if (isExportedByForceFilesystem(sym)) {
          msg += '. Alternatively, forcing filesystem support (-sFORCE_FILESYSTEM) can export this for you';
        }
        abort(msg);
      },
    });
  }
}

// end include: runtime_debug.js
// include: runtime_stack_check.js
const stackCookie1 = 0x02135467;
const stackCookie2 = 0x89BACDFE;

// Initializes the stack cookie. Called at the startup of main and at the startup of each thread in pthreads mode.
function writeStackCookie() {
  var max = _emscripten_stack_get_end();
  assert((max & 3) == 0);
  // If the stack ends at address zero we write our cookies 4 bytes into the
  // stack.  This prevents interference with SAFE_HEAP and ASAN which also
  // monitor writes to address zero.
  if (max == 0) {
    max += 4;
  }
  // The stack grow downwards towards _emscripten_stack_get_end.
  // We write cookies to the final two words in the stack and detect if they are
  // ever overwritten.
  HEAPU32[((max)>>2)] = stackCookie1;
  HEAPU32[(((max)+(4))>>2)] = stackCookie2;
  // Also test the global address 0 for integrity.
  HEAPU32[((0)>>2)] = 1668509029;
}

function u32ToHexString(num) {
  return '0x' + (num >>> 0).toString(16).padStart(8, '0');
}

function checkStackCookie() {
  if (ABORT) return;
  var max = _emscripten_stack_get_end();
  // See writeStackCookie().
  if (max == 0) {
    max += 4;
  }
  var val1 = HEAPU32[((max)>>2)];
  var val2 = HEAPU32[(((max)+(4))>>2)];
  if (val1 != stackCookie1 || val2 != stackCookie2) {
    abort(`Stack overflow! Stack cookie has been overwritten at ${ptrToString(max)}, expected hex dwords ${u32ToHexString(stackCookie2)} and ${u32ToHexString(stackCookie1)}, but received ${u32ToHexString(val2)} ${u32ToHexString(val1)}`);
  }
  // Also test the global address 0 for integrity.
  if (HEAPU32[((0)>>2)] != 0x63736d65 /* 'emsc' */) {
    abort('Runtime error: The application has corrupted its heap memory area (address zero)!');
  }
}
// end include: runtime_stack_check.js
// Memory management

var runtimeInitialized = false;



// When ALLOW_MEMORY_GROWTH is enabled, the conversion from Wasm
// memory to ArrayBuffer requires some additional logic.
function getMemoryBuffer() {
  return wasmMemory.buffer;
}

function updateMemoryViews() {
  // If we already have a heap that is resizeable/growable buffer we don't
  // need to do anything in updateMemoryViews.
  if (HEAP8?.buffer?.resizable) return;
  var b = getMemoryBuffer();
  HEAP8 = new Int8Array(b);
  HEAP16 = new Int16Array(b);
  Module['HEAPU8'] = HEAPU8 = new Uint8Array(b);
  HEAPU16 = new Uint16Array(b);
  HEAP32 = new Int32Array(b);
  HEAPU32 = new Uint32Array(b);
  HEAPF32 = new Float32Array(b);
  HEAPF64 = new Float64Array(b);
  HEAP64 = new BigInt64Array(b);
  HEAPU64 = new BigUint64Array(b);
}

// include: memoryprofiler.js
// end include: memoryprofiler.js
// end include: runtime_common.js
assert(globalThis.Int32Array && globalThis.Float64Array && Int32Array.prototype.subarray && Int32Array.prototype.set,
       'JS engine does not provide full typed array support');

function preRun() {
  var preRun = Module['preRun'];
  if (preRun) {
    if (typeof preRun == 'function') preRun = [preRun];
    onPreRuns.push(...preRun);
  }
  consumedModuleProp('preRun');
  // Begin ATPRERUNS hooks
  callRuntimeCallbacks(onPreRuns);
  // End ATPRERUNS hooks
}

function initRuntime() {
  assert(!runtimeInitialized);
  runtimeInitialized = true;

  checkStackCookie();

  // Begin ATINITS hooks
  if (!Module['noFSInit'] && !FS.initialized) FS.init();
TTY.init();
  // End ATINITS hooks

  wasmExports['__wasm_call_ctors']();

  // Begin ATPOSTCTORS hooks
  FS.ignorePermissions = false;
  // End ATPOSTCTORS hooks

  checkStackCookie();
}

function postRun() {
  checkStackCookie();

  var postRun = Module['postRun'];
  if (postRun) {
    if (typeof postRun == 'function') postRun = [postRun];
    onPostRuns.push(...postRun);
  }
  consumedModuleProp('postRun');

  // Begin ATPOSTRUNS hooks
  callRuntimeCallbacks(onPostRuns);
  // End ATPOSTRUNS hooks
}

/**
 * @param {string|number=} what
 */
function abort(what) {
  Module['onAbort']?.(what);

  what = `Aborted(${what})`;
  // TODO(sbc): Should we remove printing and leave it up to whoever
  // catches the exception?
  err(what);

  ABORT = true;

  if (what.search(/RuntimeError: [Uu]nreachable/) >= 0) {
    what += '. "unreachable" may be due to ASYNCIFY_STACK_SIZE not being large enough (try increasing it)';
  }

  // Use a wasm runtime error, because a JS error might be seen as a foreign
  // exception, which means we'd run destructors on it. We need the error to
  // simply make the program stop.
  // FIXME This approach does not work in Wasm EH because it currently does not assume
  // all RuntimeErrors are from traps; it decides whether a RuntimeError is from
  // a trap or not based on a hidden field within the object. So at the moment
  // we don't have a way of throwing a wasm trap from JS. TODO Make a JS API that
  // allows this in the wasm spec.

  // Suppress closure compiler warning here. Closure compiler's builtin extern
  // definition for WebAssembly.RuntimeError claims it takes no arguments even
  // though it can.
  // TODO(https://github.com/google/closure-compiler/pull/3913): Remove if/when upstream closure gets fixed.
  /** @suppress {checkTypes} */
  var e = new WebAssembly.RuntimeError(what);

  // Throw the error whether or not MODULARIZE is set because abort is used
  // in code paths apart from instantiation where an exception is expected
  // to be thrown when abort is called.
  throw e;
}

function createExportWrapper(name, func, nargs) {
  assert(func);
  return (...args) => {
    assert(runtimeInitialized, `native function \`${name}\` called before runtime initialization`);
    // Only assert for too many arguments. Too few can be valid since the missing arguments will be zero filled.
    assert(args.length <= nargs, `native function \`${name}\` called with ${args.length} args but expects ${nargs}`);
    return func(...args);
  };
}

var wasmBinaryFile;

function findWasmBinary() {
  return locateFile('MX_app.wasm');
}

function getBinarySync(file) {
  if (file == wasmBinaryFile && wasmBinary) {
    return new Uint8Array(wasmBinary);
  }
  if (readBinary) {
    return readBinary(file);
  }
  // Throwing a plain string here, even though it not normally advisable since
  // this gets turning into an `abort` in instantiateArrayBuffer.
  throw 'both async and sync fetching of the wasm failed';
}

async function getWasmBinary(binaryFile) {
  // If we don't have the binary yet, load it asynchronously using readAsync.
  if (!wasmBinary) {
    // Fetch the binary using readAsync
    try {
      var response = await readAsync(binaryFile);
      return new Uint8Array(response);
    } catch {
      // Fall back to getBinarySync below;
    }
  }

  // Otherwise, getBinarySync should be able to get it synchronously
  return getBinarySync(binaryFile);
}

async function instantiateArrayBuffer(binaryFile, imports) {
  try {
    var binary = await getWasmBinary(binaryFile);
    var instance = await WebAssembly.instantiate(binary, imports);
    return instance;
  } catch (reason) {
    err(`failed to asynchronously prepare wasm: ${reason}`);

    // Warn on some common problems.
    if (isFileURI(binaryFile)) {
      err(`warning: Loading from a file URI (${binaryFile}) is not supported in most browsers. See https://emscripten.org/docs/getting_started/FAQ.html#how-do-i-run-a-local-webserver-for-testing-why-does-my-program-stall-in-downloading-or-preparing`);
    }
    abort(reason);
  }
}

async function instantiateAsync(binary, binaryFile, imports) {
  if (!binary
     ) {
    try {
      var response = fetch(binaryFile, { credentials: 'same-origin' });
      var instantiationResult = await WebAssembly.instantiateStreaming(response, imports);
      return instantiationResult;
    } catch (reason) {
      // We expect the most common failure cause to be a bad MIME type for the binary,
      // in which case falling back to ArrayBuffer instantiation should work.
      err(`wasm streaming compile failed: ${reason}`);
      err('falling back to ArrayBuffer instantiation');
      // fall back of instantiateArrayBuffer below
    };
  }
  return instantiateArrayBuffer(binaryFile, imports);
}

function getWasmImports() {
  // instrumenting imports is used in asyncify in two ways: to add assertions
  // that check for proper import use, and for JSPI we use them to set up
  // the Promise API on the import side.
  Asyncify.instrumentWasmImports(wasmImports);
  // prepare imports
  var imports = {
    'env': wasmImports,
    'wasi_snapshot_preview1': wasmImports,
  };
  return imports;
}

// Create the wasm instance.
// Receives the wasm imports, returns the exports.
async function createWasm() {
  // Load the wasm module and create an instance of using native support in the JS engine.
  // handle a generated wasm instance, receiving its exports and
  // performing other necessary setup
  function receiveInstance(instance) {
    wasmExports = instance.exports;

    wasmExports = Asyncify.instrumentWasmExports(wasmExports);

    assignWasmExports(wasmExports);

    updateMemoryViews();

    return wasmExports;
  }

  // Prefer streaming instantiation if available.
  // Async compilation can be confusing when an error on the page overwrites Module
  // (for example, if the order of elements is wrong, and the one defining Module is
  // later), so we save Module and check it later.
  var trueModule = Module;
  function receiveInstantiationResult(result) {
    // 'result' is a ResultObject object which has both the module and instance.
    // receiveInstance() will swap in the exports (to Module.asm) so they can be called
    assert(Module === trueModule, 'the Module object should not be replaced during async compilation - perhaps the order of HTML elements is wrong?');
    trueModule = null;
    // TODO: Due to Closure regression https://github.com/google/closure-compiler/issues/3193, the above line no longer optimizes out down to the following line.
    // When the regression is fixed, can restore the above PTHREADS-enabled path.
    return receiveInstance(result['instance']);
  }

  var info = getWasmImports();

  // User shell pages can write their own Module.instantiateWasm = function(imports, successCallback) callback
  // to manually instantiate the Wasm module themselves. This allows pages to
  // run the instantiation parallel to any other async startup actions they are
  // performing.
  // Also pthreads and wasm workers initialize the wasm instance through this
  // path.
  var instantiateWasm = Module['instantiateWasm'];
  if (instantiateWasm) {
    return new Promise((resolve) => {
      try {
        instantiateWasm(info, (inst) => resolve(receiveInstance(inst)));
      } catch(e) {
        err(`Module.instantiateWasm callback failed with error: ${e}`);
        throw e;
      }
    });
  }

  wasmBinaryFile ??= findWasmBinary();
  var result = await instantiateAsync(wasmBinary, wasmBinaryFile, info);
  var exports = receiveInstantiationResult(result);
  return exports;
}

// end include: preamble.js

// Begin JS library code


  class ExitStatus {
      name = 'ExitStatus';
      constructor(status) {
        this.message = `Program terminated with exit(${status})`;
        this.status = status;
      }
    }

  /** @type {!Int32Array} */
  var HEAP32;

  /** @type {!Int8Array} */
  var HEAP8;

  /** @type {!Uint32Array} */
  var HEAPU32;

  var callRuntimeCallbacks = (callbacks) => {
      while (callbacks.length > 0) {
        // Pass the module as the first argument.
        callbacks.shift()(Module);
      }
    };
  var onPostRuns = [];
  var addOnPostRun = (cb) => onPostRuns.push(cb);

  var onPreRuns = [];
  var addOnPreRun = (cb) => onPreRuns.push(cb);


  var dynCalls = {
  };
  var dynCallLegacy = (sig, ptr, args) => {
      sig = sig.replace(/p/g, 'i')
      assert(sig in dynCalls, `bad function pointer type - sig is not in dynCalls: '${sig}'`);
      if (args?.length) {
        // j (64-bit integer) is fine, and is implemented as a BigInt. Without
        // legalization, the number of parameters should match (j is not expanded
        // into two i's).
        assert(args.length === sig.length - 1);
      } else {
        assert(sig.length == 1);
      }
      var f = dynCalls[sig];
      return f(ptr, ...args);
    };
  var dynCall = (sig, ptr, args = [], promising = false) => {
      assert(ptr, `null function pointer in dynCall`);
      assert(!promising, 'async dynCall is not supported in this mode')
      var rtn = dynCallLegacy(sig, ptr, args);
  
      function convert(rtn) {
        return rtn;
      }
  
      return convert(rtn);
    };

  var noExitRuntime = true;

  function ptrToString(ptr) {
      assert(typeof ptr === 'number', `ptrToString expects a number, got ${typeof ptr}`);
      // Convert to 32-bit unsigned value
      ptr >>>= 0;
      return '0x' + ptr.toString(16).padStart(8, '0');
    }

  var stackRestore = (val) => __emscripten_stack_restore(val);

  var stackSave = () => _emscripten_stack_get_current();

  var warnOnce = (text) => {
      warnOnce.shown ||= {};
      if (!warnOnce.shown[text]) {
        warnOnce.shown[text] = 1;
        err(text);
      }
    };

  

  var UTF8Decoder = globalThis.TextDecoder && new TextDecoder();
  
  
    /**
   * heapOrArray is either a regular array, or a JavaScript typed array view.
   * @param {number} idx
   * @param {number=} maxBytesToRead
   * @param {boolean=} ignoreNul
   * @return {number}
   */
  var findStringEnd = (heapOrArray, idx, maxBytesToRead, ignoreNul) => {
      var maxIdx = idx + maxBytesToRead;
      if (ignoreNul) return maxIdx;
      // TextDecoder needs to know the byte length in advance, it doesn't stop on
      // null terminator by itself.
      // As a tiny code save trick, compare idx against maxIdx using a negation,
      // so that maxBytesToRead=undefined/NaN means Infinity.
      while (heapOrArray[idx] && !(idx >= maxIdx)) ++idx;
      return idx;
    };
  
  
    /**
   * Given a pointer 'idx' to a null-terminated UTF8-encoded string in the given
   * array that contains uint8 values, returns a copy of that string as a
   * Javascript String object.
   * heapOrArray is either a regular array, or a JavaScript typed array view.
   * @param {number=} idx
   * @param {number=} maxBytesToRead
   * @param {boolean=} ignoreNul - If true, the function will not stop on a NUL character.
   * @return {string}
   */
  var UTF8ArrayToString = (heapOrArray, idx = 0, maxBytesToRead, ignoreNul) => {
  
      var endPtr = findStringEnd(heapOrArray, idx, maxBytesToRead, ignoreNul);
  
      // When using conditional TextDecoder, skip it for short strings as the overhead of the native call is not worth it.
      if (endPtr - idx > 16 && heapOrArray.buffer && UTF8Decoder) {
        return UTF8Decoder.decode(heapOrArray.subarray(idx, endPtr));
      }
      var str = '';
      while (idx < endPtr) {
        // For UTF8 byte structure, see:
        // http://en.wikipedia.org/wiki/UTF-8#Description
        // https://www.ietf.org/rfc/rfc2279.txt
        // https://tools.ietf.org/html/rfc3629
        var u0 = heapOrArray[idx++];
        if (!(u0 & 0x80)) { str += String.fromCharCode(u0); continue; }
        var u1 = heapOrArray[idx++] & 63;
        if ((u0 & 0xE0) == 0xC0) { str += String.fromCharCode(((u0 & 31) << 6) | u1); continue; }
        var u2 = heapOrArray[idx++] & 63;
        if ((u0 & 0xF0) == 0xE0) {
          u0 = ((u0 & 15) << 12) | (u1 << 6) | u2;
        } else {
          if ((u0 & 0xF8) != 0xF0) warnOnce(`Invalid UTF-8 leading byte ${ptrToString(u0)} encountered when deserializing a UTF-8 string in wasm memory to a JS string!`);
          u0 = ((u0 & 7) << 18) | (u1 << 12) | (u2 << 6) | (heapOrArray[idx++] & 63);
        }
  
        if (u0 < 0x10000) {
          str += String.fromCharCode(u0);
        } else {
          var ch = u0 - 0x10000;
          str += String.fromCharCode(0xD800 | (ch >> 10), 0xDC00 | (ch & 0x3FF));
        }
      }
      return str;
    };
  
  /** @type {!Uint8Array} */
  var HEAPU8;
  
    /**
   * Given a pointer 'ptr' to a null-terminated UTF8-encoded string in the
   * emscripten HEAP, returns a copy of that string as a Javascript String object.
   *
   * @param {number} ptr
   * @param {number=} maxBytesToRead - An optional length that specifies the
   *   maximum number of bytes to read. You can omit this parameter to scan the
   *   string until the first 0 byte. If maxBytesToRead is passed, and the string
   *   at [ptr, ptr+maxBytesToReadr[ contains a null byte in the middle, then the
   *   string will cut short at that byte index.
   * @param {boolean=} ignoreNul - If true, the function will not stop on a NUL character.
   * @return {string}
   */
  var UTF8ToString = (ptr, maxBytesToRead, ignoreNul) => {
      assert(typeof ptr == 'number', `UTF8ToString expects a number (got ${typeof ptr})`);
      return ptr ? UTF8ArrayToString(HEAPU8, ptr, maxBytesToRead, ignoreNul) : '';
    };
  var ___assert_fail = (condition, filename, line, func) =>
      abort(`Assertion failed: ${UTF8ToString(condition)}, at: ` + [filename ? UTF8ToString(filename) : 'unknown filename', line, func ? UTF8ToString(func) : 'unknown function']);

  var exceptionCaught =  [];
  
  
  var uncaughtExceptionCount = 0;
  
  
  class ExceptionInfo {
      // excPtr - Thrown object pointer to wrap. Metadata pointer is calculated from it.
      constructor(excPtr) {
        this.excPtr = excPtr;
        this.ptr = excPtr - 24;
      }
  
      set_type(type) {
        HEAPU32[(((this.ptr)+(4))>>2)] = type;
      }
  
      get_type() {
        return HEAPU32[(((this.ptr)+(4))>>2)];
      }
  
      set_destructor(destructor) {
        HEAPU32[(((this.ptr)+(8))>>2)] = destructor;
      }
  
      get_destructor() {
        return HEAPU32[(((this.ptr)+(8))>>2)];
      }
  
      set_caught(caught) {
        caught = caught ? 1 : 0;
        HEAP8[(this.ptr)+(12)] = caught;
      }
  
      get_caught() {
        return HEAP8[(this.ptr)+(12)] != 0;
      }
  
      set_rethrown(rethrown) {
        rethrown = rethrown ? 1 : 0;
        HEAP8[(this.ptr)+(13)] = rethrown;
      }
  
      get_rethrown() {
        return HEAP8[(this.ptr)+(13)] != 0;
      }
  
      // Initialize native structure fields. Should be called once after allocated.
      init(type, destructor) {
        this.set_adjusted_ptr(0);
        this.set_type(type);
        this.set_destructor(destructor);
      }
  
      set_adjusted_ptr(adjustedPtr) {
        HEAPU32[(((this.ptr)+(16))>>2)] = adjustedPtr;
      }
  
      get_adjusted_ptr() {
        return HEAPU32[(((this.ptr)+(16))>>2)];
      }
    }
  var ___cxa_begin_catch = (ptr) => {
      var info = new ExceptionInfo(ptr);
      if (!info.get_caught()) {
        info.set_caught(true);
        uncaughtExceptionCount--;
      }
      info.set_rethrown(false);
      exceptionCaught.push(info);
      return ___cxa_get_exception_ptr(ptr);
    };

  
  
  
  var exceptionLast = null;
  var ___cxa_end_catch = () => {
      // Clear state flag.
      _setThrew(0, 0);
      assert(exceptionCaught.length > 0);
      // Call destructor if one is registered then clear it.
      var info = exceptionCaught.pop();
  
      ___cxa_decrement_exception_refcount(info.excPtr);
      exceptionLast = null; // XXX in decRef?
    };

  var setTempRet0 = (val) => __emscripten_tempret_set(val);
  
  
  
  var findMatchingCatch = (args) => {
      var thrown = exceptionLast?.excPtr;
      if (!thrown) {
        // just pass through the null ptr
        setTempRet0(0);
        return 0;
      }
      var info = new ExceptionInfo(thrown);
      info.set_adjusted_ptr(thrown);
      var thrownType = info.get_type();
      if (!thrownType) {
        // just pass through the thrown ptr
        setTempRet0(0);
        return thrown;
      }
  
      // can_catch receives a **, add indirection
      // The different catch blocks are denoted by different types.
      // Due to inheritance, those types may not precisely match the
      // type of the thrown object. Find one which matches, and
      // return the type of the catch block which should be called.
      for (var caughtType of args) {
        if (!caughtType || caughtType === thrownType) {
          // Catch all clause matched or exactly the same type is caught
          break;
        }
        var adjusted_ptr_addr = info.ptr + 16;
        if (___cxa_can_catch(caughtType, thrownType, adjusted_ptr_addr)) {
          setTempRet0(caughtType);
          return thrown;
        }
      }
      setTempRet0(thrownType);
      return thrown;
    };
  var ___cxa_find_matching_catch_2 = () => findMatchingCatch([]);

  var ___cxa_find_matching_catch_3 = (arg0) => findMatchingCatch([arg0]);

  
  
  
  
  var __Unwind_RaiseException = (ex) => {
      throw ex;
    };
  var ___cxa_rethrow = () => {
      if (!exceptionCaught.length) {
        abort('no exception to throw');
      }
      var info = exceptionCaught.at(-1);
      var ptr = info.excPtr;
      info.set_rethrown(true);
      info.set_caught(false);
      uncaughtExceptionCount++;
      ___cxa_increment_exception_refcount(ptr);
      ptr = exceptionLast = new CppException(ptr);
      __Unwind_RaiseException(ptr);
    };

  
  
  
  
  
  
  
  
  var stackAlloc = (sz) => __emscripten_stack_alloc(sz);
  
  
  var getExceptionMessageCommon = (ptr) => {
      var sp = stackSave();
      var type_addr_addr = stackAlloc(4);
      var message_addr_addr = stackAlloc(4);
      ___get_exception_message(ptr, type_addr_addr, message_addr_addr);
      var type_addr = HEAPU32[((type_addr_addr)>>2)];
      var message_addr = HEAPU32[((message_addr_addr)>>2)];
      var type = UTF8ToString(type_addr);
      _free(type_addr);
      var message;
      if (message_addr) {
        message = UTF8ToString(message_addr);
        _free(message_addr);
      }
      stackRestore(sp);
      return [type, message];
    };
  var getExceptionMessage = (exn) => getExceptionMessageCommon(exn.excPtr);
  
  var decrementExceptionRefcount = (exn) => ___cxa_decrement_exception_refcount(exn.excPtr);
  
  var incrementExceptionRefcount = (exn) => ___cxa_increment_exception_refcount(exn.excPtr);
  
  var ___cxa_throw = (ptr, type, destructor) => {
      var info = new ExceptionInfo(ptr);
      // Initialize ExceptionInfo content after it was allocated in __cxa_allocate_exception.
      info.init(type, destructor);
      ___cxa_increment_exception_refcount(ptr);
      ptr = exceptionLast = new CppException(ptr);
      uncaughtExceptionCount++;
      __Unwind_RaiseException(ptr);
    };

  var ___cxa_uncaught_exceptions = () => uncaughtExceptionCount;

  
  var __Unwind_Resume = (ex) => {
      throw ex;
    };
  var ___resumeException = (ptr) => {
      ptr = exceptionLast ??= new CppException(ptr);
      __Unwind_Resume(ptr);
    };

  var syscallGetVarargI = () => {
      assert(SYSCALLS.varargs != undefined);
      // the `+` prepended here is necessary to convince the JSCompiler that varargs is indeed a number.
      var ret = HEAP32[((+SYSCALLS.varargs)>>2)];
      SYSCALLS.varargs += 4;
      return ret;
    };
  var syscallGetVarargP = syscallGetVarargI;
  
  
  var PATH = {
  isAbs:(path) => path.charAt(0) === '/',
  splitPath:(filename) => {
        var splitPathRe = /^(\/?|)([\s\S]*?)((?:\.{1,2}|[^\/]+?|)(\.[^.\/]*|))(?:[\/]*)$/;
        return splitPathRe.exec(filename).slice(1);
      },
  normalizeArray:(parts, allowAboveRoot) => {
        // if the path tries to go above the root, `up` ends up > 0
        var up = 0;
        for (var i = parts.length - 1; i >= 0; i--) {
          var last = parts[i];
          if (last === '.') {
            parts.splice(i, 1);
          } else if (last === '..') {
            parts.splice(i, 1);
            up++;
          } else if (up) {
            parts.splice(i, 1);
            up--;
          }
        }
        // if the path is allowed to go above the root, restore leading ..s
        if (allowAboveRoot) {
          for (; up; up--) {
            parts.unshift('..');
          }
        }
        return parts;
      },
  normalize:(path) => {
        var isAbsolute = PATH.isAbs(path),
            trailingSlash = path.slice(-1) === '/';
        // Normalize the path
        path = PATH.normalizeArray(path.split('/').filter((p) => !!p), !isAbsolute).join('/');
        if (!path && !isAbsolute) {
          path = '.';
        }
        if (path && trailingSlash) {
          path += '/';
        }
        return (isAbsolute ? '/' : '') + path;
      },
  dirname:(path) => {
        var result = PATH.splitPath(path),
            root = result[0],
            dir = result[1];
        if (!root && !dir) {
          // No dirname whatsoever
          return '.';
        }
        if (dir) {
          // It has a dirname, strip trailing slash
          dir = dir.slice(0, -1);
        }
        return root + dir;
      },
  basename:(path) => path && path.match(/([^\/]+|\/)\/*$/)[1],
join:(...paths) => PATH.normalize(paths.join('/')),
join2:(l, r) => PATH.normalize(l + '/' + r),
};

var initRandomFill = () => {

    return (view) => (crypto.getRandomValues(view), 0);
  };
var randomFill = (view) => (randomFill = initRandomFill())(view);



var PATH_FS = {
resolve:(...args) => {
      var resolvedPath = '',
        resolvedAbsolute = false;
      for (var i = args.length - 1; i >= -1 && !resolvedAbsolute; i--) {
        var path = (i >= 0) ? args[i] : FS.cwd();
        // Skip empty and invalid entries
        if (typeof path != 'string') {
          throw new TypeError('Arguments to path.resolve must be strings');
        } else if (!path) {
          return ''; // an invalid portion invalidates the whole thing
        }
        resolvedPath = path + '/' + resolvedPath;
        resolvedAbsolute = PATH.isAbs(path);
      }
      // At this point the path should be resolved to a full absolute path, but
      // handle relative paths to be safe (might happen when process.cwd() fails)
      resolvedPath = PATH.normalizeArray(resolvedPath.split('/').filter((p) => !!p), !resolvedAbsolute).join('/');
      return ((resolvedAbsolute ? '/' : '') + resolvedPath) || '.';
    },
relative:(from, to) => {
      from = PATH_FS.resolve(from).slice(1);
      to = PATH_FS.resolve(to).slice(1);
      function trim(arr) {
        var start = 0;
        for (; start < arr.length; start++) {
          if (arr[start] !== '') break;
        }
        var end = arr.length - 1;
        for (; end >= 0; end--) {
          if (arr[end] !== '') break;
        }
        if (start > end) return [];
        return arr.slice(start, end - start + 1);
      }
      var fromParts = trim(from.split('/'));
      var toParts = trim(to.split('/'));
      var length = Math.min(fromParts.length, toParts.length);
      var samePartsLength = length;
      for (var i = 0; i < length; i++) {
        if (fromParts[i] !== toParts[i]) {
          samePartsLength = i;
          break;
        }
      }
      var outputParts = [];
      for (var i = samePartsLength; i < fromParts.length; i++) {
        outputParts.push('..');
      }
      outputParts = outputParts.concat(toParts.slice(samePartsLength));
      return outputParts.join('/');
    },
};



var FS_stdin_getChar_buffer = [];

var lengthBytesUTF8 = (str) => {
    var len = 0;
    for (var i = 0; i < str.length; ++i) {
      // Gotcha: charCodeAt returns a 16-bit word that is a UTF-16 encoded code
      // unit, not a Unicode code point of the character! So decode
      // UTF16->UTF32->UTF8.
      // See http://unicode.org/faq/utf_bom.html#utf16-3
      var c = str.charCodeAt(i); // possibly a lead surrogate
      if (c <= 0x7F) {
        len++;
      } else if (c <= 0x7FF) {
        len += 2;
      } else if (c >= 0xD800 && c <= 0xDFFF) {
        len += 4; ++i;
      } else {
        len += 3;
      }
    }
    return len;
  };

var stringToUTF8Array = (str, heap, outIdx, maxBytesToWrite) => {
    assert(typeof str === 'string', `stringToUTF8Array expects a string (got ${typeof str})`);
    // Parameter maxBytesToWrite is not optional. Negative values, 0, null,
    // undefined and false each don't write out any bytes.
    if (!(maxBytesToWrite > 0))
      return 0;

    var startIdx = outIdx;
    var endIdx = outIdx + maxBytesToWrite - 1; // -1 for string null terminator.
    for (var i = 0; i < str.length; ++i) {
      // For UTF8 byte structure, see http://en.wikipedia.org/wiki/UTF-8#Description
      // and https://www.ietf.org/rfc/rfc2279.txt
      // and https://tools.ietf.org/html/rfc3629
      var u = str.codePointAt(i);
      if (u <= 0x7F) {
        if (outIdx >= endIdx) break;
        heap[outIdx++] = u;
      } else if (u <= 0x7FF) {
        if (outIdx + 1 >= endIdx) break;
        heap[outIdx++] = 0xC0 | (u >> 6);
        heap[outIdx++] = 0x80 | (u & 63);
      } else if (u <= 0xFFFF) {
        if (outIdx + 2 >= endIdx) break;
        heap[outIdx++] = 0xE0 | (u >> 12);
        heap[outIdx++] = 0x80 | ((u >> 6) & 63);
        heap[outIdx++] = 0x80 | (u & 63);
      } else {
        if (outIdx + 3 >= endIdx) break;
        if (u > 0x10FFFF) warnOnce(`Invalid Unicode code point ${ptrToString(u)} encountered when serializing a JS string to a UTF-8 string in wasm memory! (Valid unicode code points should be in range 0-0x10FFFF).`);
        heap[outIdx++] = 0xF0 | (u >> 18);
        heap[outIdx++] = 0x80 | ((u >> 12) & 63);
        heap[outIdx++] = 0x80 | ((u >> 6) & 63);
        heap[outIdx++] = 0x80 | (u & 63);
        // Gotcha: if codePoint is over 0xFFFF, it is represented as a surrogate pair in UTF-16.
        // We need to manually skip over the second code unit for correct iteration.
        i++;
      }
    }
    // Null-terminate the pointer to the buffer.
    heap[outIdx] = 0;
    return outIdx - startIdx;
  };
/** @type {function(string, boolean=, number=)} */
  var intArrayFromString = (stringy, dontAddNull, length) => {
      var len = length > 0 ? length : lengthBytesUTF8(stringy)+1;
      var u8array = new Array(len);
      var numBytesWritten = stringToUTF8Array(stringy, u8array, 0, u8array.length);
      if (dontAddNull) u8array.length = numBytesWritten;
      return u8array;
    };
  var FS_stdin_getChar = () => {
      if (!FS_stdin_getChar_buffer.length) {
        var result = null;
        if (globalThis.window?.prompt) {
          // Browser.
          result = window.prompt('Input: ');  // returns null on cancel
          if (result !== null) {
            result += '\n';
          }
        } else
        {}
        if (!result) {
          return null;
        }
        FS_stdin_getChar_buffer = intArrayFromString(result, true);
      }
      return FS_stdin_getChar_buffer.shift();
    };
  var TTY = {
  ttys:[],
  init() {
        // https://github.com/emscripten-core/emscripten/pull/1555
        // if (ENVIRONMENT_IS_NODE) {
        //   // currently, FS.init does not distinguish if process.stdin is a file or TTY
        //   // device, it always assumes it's a TTY device. because of this, we're forcing
        //   // process.stdin to UTF8 encoding to at least make stdin reading compatible
        //   // with text files until FS.init can be refactored.
        //   process.stdin.setEncoding('utf8');
        // }
      },
  shutdown() {
        // https://github.com/emscripten-core/emscripten/pull/1555
        // if (ENVIRONMENT_IS_NODE) {
        //   // inolen: any idea as to why node -e 'process.stdin.read()' wouldn't exit immediately (with process.stdin being a tty)?
        //   // isaacs: because now it's reading from the stream, you've expressed interest in it, so that read() kicks off a _read() which creates a ReadReq operation
        //   // inolen: I thought read() in that case was a synchronous operation that just grabbed some amount of buffered data if it exists?
        //   // isaacs: it is. but it also triggers a _read() call, which calls readStart() on the handle
        //   // isaacs: do process.stdin.pause() and i'd think it'd probably close the pending call
        //   process.stdin.pause();
        // }
      },
  register(dev, ops) {
        TTY.ttys[dev] = { input: [], output: [], ops: ops };
        FS.registerDevice(dev, TTY.stream_ops);
      },
  stream_ops:{
  open(stream) {
          var tty = TTY.ttys[stream.node.rdev];
          if (!tty) {
            throw new FS.ErrnoError(43);
          }
          stream.tty = tty;
          stream.seekable = false;
        },
  close(stream) {
          // flush any pending line data
          stream.tty.ops.fsync(stream.tty);
        },
  fsync(stream) {
          stream.tty.ops.fsync(stream.tty);
        },
  read(stream, buffer, offset, length, pos /* ignored */) {
          if (!stream.tty || !stream.tty.ops.get_char) {
            throw new FS.ErrnoError(60);
          }
          var bytesRead = 0;
          for (var i = 0; i < length; i++) {
            var result;
            try {
              result = stream.tty.ops.get_char(stream.tty);
            } catch (e) {
              throw new FS.ErrnoError(29);
            }
            if (result === undefined && !bytesRead) {
              throw new FS.ErrnoError(6);
            }
            if (result === null || result === undefined) break;
            bytesRead++;
            buffer[offset+i] = result;
            // We currently only support canonical mode (ICANON), where
            // read(2) returns as soon as a line delimiter is read.
            if (result === 10) break;
          }
          if (bytesRead) {
            stream.node.atime = Date.now();
          }
          return bytesRead;
        },
  write(stream, buffer, offset, length, pos) {
          if (!stream.tty || !stream.tty.ops.put_char) {
            throw new FS.ErrnoError(60);
          }
          try {
            for (var i = 0; i < length; i++) {
              stream.tty.ops.put_char(stream.tty, buffer[offset+i]);
            }
          } catch (e) {
            throw new FS.ErrnoError(29);
          }
          if (length) {
            stream.node.mtime = stream.node.ctime = Date.now();
          }
          return i;
        },
  },
  default_tty_ops:{
  get_char(tty) {
          return FS_stdin_getChar();
        },
  put_char(tty, val) {
          if (val === null || val === 10) {
            out(UTF8ArrayToString(tty.output));
            tty.output = [];
          } else {
            if (val != 0) tty.output.push(val); // val == 0 would cut text output off in the middle.
          }
        },
  fsync(tty) {
          if (tty.output?.length > 0) {
            out(UTF8ArrayToString(tty.output));
            tty.output = [];
          }
        },
  ioctl_tcgets(tty) {
          // typical setting
          return {
            c_iflag: 25856,
            c_oflag: 5,
            c_cflag: 191,
            c_lflag: 35387,
            c_cc: [
              0x03, 0x1c, 0x7f, 0x15, 0x04, 0x00, 0x01, 0x00, 0x11, 0x13, 0x1a, 0x00,
              0x12, 0x0f, 0x17, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            ]
          };
        },
  ioctl_tcsets(tty, optional_actions, data) {
          // currently just ignore
          return 0;
        },
  ioctl_tiocgwinsz(tty) {
          return [24, 80];
        },
  },
  default_tty1_ops:{
  put_char(tty, val) {
          if (val === null || val === 10) {
            err(UTF8ArrayToString(tty.output));
            tty.output = [];
          } else {
            if (val != 0) tty.output.push(val);
          }
        },
  fsync(tty) {
          if (tty.output?.length > 0) {
            err(UTF8ArrayToString(tty.output));
            tty.output = [];
          }
        },
  },
  };
  
  
  var zeroMemory = (ptr, size) => HEAPU8.fill(0, ptr, ptr + size);
  
  var alignMemory = (size, alignment) => {
      assert(alignment, 'alignment argument is required');
      return Math.ceil(size / alignment) * alignment;
    };
  var mmapAlloc = (size) => {
      size = alignMemory(size, 65536);
      var ptr = _emscripten_builtin_memalign(65536, size);
      if (ptr) zeroMemory(ptr, size);
      return ptr;
    };
  
  var MEMFS = {
  ops_table:null,
  mount(mount) {
        return MEMFS.createNode(null, '/', 16895, 0);
      },
  createNode(parent, name, mode, dev) {
        if (FS.isBlkdev(mode) || FS.isFIFO(mode)) {
          // not supported
          throw new FS.ErrnoError(63);
        }
        MEMFS.ops_table ||= {
          dir: {
            node: {
              getattr: MEMFS.node_ops.getattr,
              setattr: MEMFS.node_ops.setattr,
              lookup: MEMFS.node_ops.lookup,
              mknod: MEMFS.node_ops.mknod,
              rename: MEMFS.node_ops.rename,
              unlink: MEMFS.node_ops.unlink,
              rmdir: MEMFS.node_ops.rmdir,
              readdir: MEMFS.node_ops.readdir,
              symlink: MEMFS.node_ops.symlink
            },
            stream: {
              llseek: MEMFS.stream_ops.llseek
            }
          },
          file: {
            node: {
              getattr: MEMFS.node_ops.getattr,
              setattr: MEMFS.node_ops.setattr
            },
            stream: {
              llseek: MEMFS.stream_ops.llseek,
              read: MEMFS.stream_ops.read,
              write: MEMFS.stream_ops.write,
              mmap: MEMFS.stream_ops.mmap,
              msync: MEMFS.stream_ops.msync
            }
          },
          link: {
            node: {
              getattr: MEMFS.node_ops.getattr,
              setattr: MEMFS.node_ops.setattr,
              readlink: MEMFS.node_ops.readlink
            },
            stream: {}
          },
          chrdev: {
            node: {
              getattr: MEMFS.node_ops.getattr,
              setattr: MEMFS.node_ops.setattr
            },
            stream: FS.chrdev_stream_ops
          }
        };
        var node = FS.createNode(parent, name, mode, dev);
        if (FS.isDir(node.mode)) {
          node.node_ops = MEMFS.ops_table.dir.node;
          node.stream_ops = MEMFS.ops_table.dir.stream;
          node.contents = {};
        } else if (FS.isFile(node.mode)) {
          node.node_ops = MEMFS.ops_table.file.node;
          node.stream_ops = MEMFS.ops_table.file.stream;
          // The actual number of bytes used in the typed array, as opposed to
          // contents.length which gives the whole capacity.
          node.usedBytes = 0;
          // The byte data of the file is stored in a typed array.
          // Note: typed arrays are not resizable like normal JS arrays are, so
          // there is a small penalty involved for appending file writes that
          // continuously grow a file similar to std::vector capacity vs used.
          node.contents = MEMFS.emptyFileContents ??= new Uint8Array(0);
        } else if (FS.isLink(node.mode)) {
          node.node_ops = MEMFS.ops_table.link.node;
          node.stream_ops = MEMFS.ops_table.link.stream;
        } else if (FS.isChrdev(node.mode)) {
          node.node_ops = MEMFS.ops_table.chrdev.node;
          node.stream_ops = MEMFS.ops_table.chrdev.stream;
        }
        node.atime = node.mtime = node.ctime = Date.now();
        // add the new node to the parent
        if (parent) {
          parent.contents[name] = node;
          parent.atime = parent.mtime = parent.ctime = node.atime;
        }
        return node;
      },
  getFileDataAsTypedArray(node) {
        assert(FS.isFile(node.mode), 'getFileDataAsTypedArray called on non-file');
        return node.contents.subarray(0, node.usedBytes); // Make sure to not return excess unused bytes.
      },
  expandFileStorage(node, newCapacity) {
        var prevCapacity = node.contents.length;
        if (prevCapacity >= newCapacity) return; // No need to expand, the storage was already large enough.
        // Don't expand strictly to the given requested limit if it's only a very
        // small increase, but instead geometrically grow capacity.
        // For small filesizes (<1MB), perform size*2 geometric increase, but for
        // large sizes, do a much more conservative size*1.125 increase to avoid
        // overshooting the allocation cap by a very large margin.
        var CAPACITY_DOUBLING_MAX = 1024 * 1024;
        newCapacity = Math.max(newCapacity, (prevCapacity * (prevCapacity < CAPACITY_DOUBLING_MAX ? 2.0 : 1.125)) >>> 0);
        if (prevCapacity) newCapacity = Math.max(newCapacity, 256); // At minimum allocate 256b for each file when expanding.
        var oldContents = MEMFS.getFileDataAsTypedArray(node);
        node.contents = new Uint8Array(newCapacity); // Allocate new storage.
        node.contents.set(oldContents);
      },
  resizeFileStorage(node, newSize) {
        if (node.usedBytes == newSize) return;
        var oldContents = node.contents;
        node.contents = new Uint8Array(newSize); // Allocate new storage.
        node.contents.set(oldContents.subarray(0, Math.min(newSize, node.usedBytes))); // Copy old data over to the new storage.
        node.usedBytes = newSize;
      },
  node_ops:{
  getattr(node) {
          var attr = {};
          // device numbers reuse inode numbers.
          attr.dev = FS.isChrdev(node.mode) ? node.id : 1;
          attr.ino = node.id;
          attr.mode = node.mode;
          attr.nlink = 1;
          attr.uid = 0;
          attr.gid = 0;
          attr.rdev = node.rdev;
          if (FS.isDir(node.mode)) {
            attr.size = 4096;
          } else if (FS.isFile(node.mode)) {
            attr.size = node.usedBytes;
          } else if (FS.isLink(node.mode)) {
            attr.size = node.link.length;
          } else {
            attr.size = 0;
          }
          attr.atime = new Date(node.atime);
          attr.mtime = new Date(node.mtime);
          attr.ctime = new Date(node.ctime);
          // NOTE: In our implementation, st_blocks = Math.ceil(st_size/st_blksize),
          //       but this is not required by the standard.
          attr.blksize = 4096;
          attr.blocks = Math.ceil(attr.size / attr.blksize);
          return attr;
        },
  setattr(node, attr) {
          for (const key of ['mode', 'atime', 'mtime', 'ctime']) {
            if (attr[key] != null) {
              node[key] = attr[key];
            }
          }
          if (attr.size !== undefined) {
            MEMFS.resizeFileStorage(node, attr.size);
          }
        },
  lookup(parent, name) {
          throw new FS.ErrnoError(44);
        },
  mknod(parent, name, mode, dev) {
          return MEMFS.createNode(parent, name, mode, dev);
        },
  rename(old_node, new_dir, new_name) {
          var new_node;
          try {
            new_node = FS.lookupNode(new_dir, new_name);
          } catch (e) {}
          if (new_node) {
            if (FS.isDir(old_node.mode)) {
              // if we're overwriting a directory at new_name, make sure it's empty.
              for (var i in new_node.contents) {
                throw new FS.ErrnoError(55);
              }
            }
            FS.hashRemoveNode(new_node);
          }
          // do the internal rewiring
          delete old_node.parent.contents[old_node.name];
          new_dir.contents[new_name] = old_node;
          old_node.name = new_name;
          new_dir.ctime = new_dir.mtime = old_node.parent.ctime = old_node.parent.mtime = Date.now();
        },
  unlink(parent, name) {
          delete parent.contents[name];
          parent.ctime = parent.mtime = Date.now();
        },
  rmdir(parent, name) {
          var node = FS.lookupNode(parent, name);
          for (var i in node.contents) {
            throw new FS.ErrnoError(55);
          }
          delete parent.contents[name];
          parent.ctime = parent.mtime = Date.now();
        },
  readdir(node) {
          return ['.', '..', ...Object.keys(node.contents)];
        },
  symlink(parent, newname, oldpath) {
          var node = MEMFS.createNode(parent, newname, 0o777 | 40960, 0);
          node.link = oldpath;
          return node;
        },
  readlink(node) {
          if (!FS.isLink(node.mode)) {
            throw new FS.ErrnoError(28);
          }
          return node.link;
        },
  },
  stream_ops:{
  read(stream, buffer, offset, length, position) {
          var contents = stream.node.contents;
          if (position >= stream.node.usedBytes) return 0;
          var size = Math.min(stream.node.usedBytes - position, length);
          assert(size >= 0);
          buffer.set(contents.subarray(position, position + size), offset);
          return size;
        },
  write(stream, buffer, offset, length, position, canOwn) {
          assert(buffer.subarray, 'FS.write expects a TypedArray');
          // If the buffer is located in main memory (HEAP), and if
          // memory can grow, we can't hold on to references of the
          // memory buffer, as they may get invalidated. That means we
          // need to copy its contents.
          if (buffer.buffer === HEAP8.buffer) {
            canOwn = false;
          }
  
          if (!length) return 0;
          var node = stream.node;
          node.mtime = node.ctime = Date.now();
  
          if (canOwn) {
            assert(!position, 'canOwn must imply no weird position inside the file');
            node.contents = buffer.subarray(offset, offset + length);
            node.usedBytes = length;
          } else if (!node.usedBytes && !position) { // If this is a simple first write to an empty file, do a fast set since we don't need to care about old data.
            node.contents = buffer.slice(offset, offset + length);
            node.usedBytes = length;
          } else {
            MEMFS.expandFileStorage(node, position+length);
            // Use typed array write which is available.
            node.contents.set(buffer.subarray(offset, offset + length), position);
            node.usedBytes = Math.max(node.usedBytes, position + length);
          }
          return length;
        },
  llseek(stream, offset, whence) {
          var position = offset;
          if (whence === 1) {
            position += stream.position;
          } else if (whence === 2) {
            if (FS.isFile(stream.node.mode)) {
              position += stream.node.usedBytes;
            }
          }
          if (position < 0) {
            throw new FS.ErrnoError(28);
          }
          return position;
        },
  mmap(stream, length, position, prot, flags) {
          if (!FS.isFile(stream.node.mode)) {
            throw new FS.ErrnoError(43);
          }
          var ptr;
          var allocated;
          var contents = stream.node.contents;
          // Only make a new copy when MAP_PRIVATE is specified.
          if (!(flags & 2) && contents.buffer === HEAP8.buffer) {
            // We can't emulate MAP_SHARED when the file is not backed by the
            // buffer we're mapping to (e.g. the HEAP buffer).
            allocated = false;
            ptr = contents.byteOffset;
          } else {
            allocated = true;
            ptr = mmapAlloc(length);
            if (!ptr) {
              throw new FS.ErrnoError(48);
            }
            if (contents) {
              // Try to avoid unnecessary slices.
              if (position > 0 || position + length < contents.length) {
                if (contents.subarray) {
                  contents = contents.subarray(position, position + length);
                } else {
                  contents = Array.prototype.slice.call(contents, position, position + length);
                }
              }
              HEAP8.set(contents, ptr);
            }
          }
          return { ptr, allocated };
        },
  msync(stream, buffer, offset, length, mmapFlags) {
          MEMFS.stream_ops.write(stream, buffer, 0, length, offset, false);
          // should we check if bytesWritten and length are the same?
          return 0;
        },
  },
  };
  
  var FS_modeStringToFlags = (str) => {
      if (typeof str != 'string') return str;
      var flagModes = {
        'r': 0,
        'r+': 2,
        'w': 512 | 64 | 1,
        'w+': 512 | 64 | 2,
        'a': 1024 | 64 | 1,
        'a+': 1024 | 64 | 2,
      };
      var flags = flagModes[str];
      if (typeof flags == 'undefined') {
        throw new Error(`Unknown file open mode: ${str}`);
      }
      return flags;
    };
  
  var FS_fileDataToTypedArray = (data) => {
      if (typeof data == 'string') {
        data = intArrayFromString(data, true);
      }
      if (!data.subarray) {
        data = new Uint8Array(data);
      }
      return data;
    };
  
  var FS_getMode = (canRead, canWrite) => {
      var mode = 0;
      if (canRead) mode |= 292 | 73;
      if (canWrite) mode |= 146;
      return mode;
    };
  
  
  
  
  var strError = (errno) => UTF8ToString(_strerror(errno));
  
  var ERRNO_CODES = {
      'EPERM': 63,
      'ENOENT': 44,
      'ESRCH': 71,
      'EINTR': 27,
      'EIO': 29,
      'ENXIO': 60,
      'E2BIG': 1,
      'ENOEXEC': 45,
      'EBADF': 8,
      'ECHILD': 12,
      'EAGAIN': 6,
      'EWOULDBLOCK': 6,
      'ENOMEM': 48,
      'EACCES': 2,
      'EFAULT': 21,
      'ENOTBLK': 105,
      'EBUSY': 10,
      'EEXIST': 20,
      'EXDEV': 75,
      'ENODEV': 43,
      'ENOTDIR': 54,
      'EISDIR': 31,
      'EINVAL': 28,
      'ENFILE': 41,
      'EMFILE': 33,
      'ENOTTY': 59,
      'ETXTBSY': 74,
      'EFBIG': 22,
      'ENOSPC': 51,
      'ESPIPE': 70,
      'EROFS': 69,
      'EMLINK': 34,
      'EPIPE': 64,
      'EDOM': 18,
      'ERANGE': 68,
      'ENOMSG': 49,
      'EIDRM': 24,
      'ECHRNG': 106,
      'EL2NSYNC': 156,
      'EL3HLT': 107,
      'EL3RST': 108,
      'ELNRNG': 109,
      'EUNATCH': 110,
      'ENOCSI': 111,
      'EL2HLT': 112,
      'EDEADLK': 16,
      'ENOLCK': 46,
      'EBADE': 113,
      'EBADR': 114,
      'EXFULL': 115,
      'ENOANO': 104,
      'EBADRQC': 103,
      'EBADSLT': 102,
      'EDEADLOCK': 16,
      'EBFONT': 101,
      'ENOSTR': 100,
      'ENODATA': 116,
      'ETIME': 117,
      'ENOSR': 118,
      'ENONET': 119,
      'ENOPKG': 120,
      'EREMOTE': 121,
      'ENOLINK': 47,
      'EADV': 122,
      'ESRMNT': 123,
      'ECOMM': 124,
      'EPROTO': 65,
      'EMULTIHOP': 36,
      'EDOTDOT': 125,
      'EBADMSG': 9,
      'ENOTUNIQ': 126,
      'EBADFD': 127,
      'EREMCHG': 128,
      'ELIBACC': 129,
      'ELIBBAD': 130,
      'ELIBSCN': 131,
      'ELIBMAX': 132,
      'ELIBEXEC': 133,
      'ENOSYS': 52,
      'ENOTEMPTY': 55,
      'ENAMETOOLONG': 37,
      'ELOOP': 32,
      'EOPNOTSUPP': 138,
      'EPFNOSUPPORT': 139,
      'ECONNRESET': 15,
      'ENOBUFS': 42,
      'EAFNOSUPPORT': 5,
      'EPROTOTYPE': 67,
      'ENOTSOCK': 57,
      'ENOPROTOOPT': 50,
      'ESHUTDOWN': 140,
      'ECONNREFUSED': 14,
      'EADDRINUSE': 3,
      'ECONNABORTED': 13,
      'ENETUNREACH': 40,
      'ENETDOWN': 38,
      'ETIMEDOUT': 73,
      'EHOSTDOWN': 142,
      'EHOSTUNREACH': 23,
      'EINPROGRESS': 26,
      'EALREADY': 7,
      'EDESTADDRREQ': 17,
      'EMSGSIZE': 35,
      'EPROTONOSUPPORT': 66,
      'ESOCKTNOSUPPORT': 137,
      'EADDRNOTAVAIL': 4,
      'ENETRESET': 39,
      'EISCONN': 30,
      'ENOTCONN': 53,
      'ETOOMANYREFS': 141,
      'EUSERS': 136,
      'EDQUOT': 19,
      'ESTALE': 72,
      'ENOTSUP': 138,
      'ENOMEDIUM': 148,
      'EILSEQ': 25,
      'EOVERFLOW': 61,
      'ECANCELED': 11,
      'ENOTRECOVERABLE': 56,
      'EOWNERDEAD': 62,
      'ESTRPIPE': 135,
    };
  
  var asyncLoad = async (url) => {
      var arrayBuffer = await readAsync(url);
      assert(arrayBuffer, `Loading data file "${url}" failed (no arrayBuffer).`);
      return new Uint8Array(arrayBuffer);
    };
  
  
  var FS_createDataFile = (...args) => FS.createDataFile(...args);
  
  var getUniqueRunDependency = (id) => {
      var orig = id;
      while (1) {
        if (!runDependencyTracking[id]) return id;
        id = orig + Math.random();
      }
    };
  
  var dependenciesPromise = null;
  var resolveRunDependencies = async () => dependenciesPromise;
  var runDependencies = 0;
  
  
  var dependenciesPromiseResolve = null;
  
  var runDependencyTracking = {
  };
  
  var runDependencyWatcher = null;
  var removeRunDependency = (id) => {
      runDependencies--;
  
      Module['monitorRunDependencies']?.(runDependencies);
  
      assert(id, 'removeRunDependency requires an ID');
      assert(runDependencyTracking[id]);
      delete runDependencyTracking[id];
      if (!runDependencies) {
        if (runDependencyWatcher !== null) {
          clearInterval(runDependencyWatcher);
          runDependencyWatcher = null;
        }
        dependenciesPromiseResolve();
      }
    };
  
  
  
  
  var addRunDependency = (id) => {
      if (!runDependencies) {
        dependenciesPromise = new Promise((resolve) => dependenciesPromiseResolve = resolve);
      }
      runDependencies++;
  
      Module['monitorRunDependencies']?.(runDependencies);
  
      assert(id, 'addRunDependency requires an ID')
      assert(!runDependencyTracking[id]);
      runDependencyTracking[id] = 1;
      if (!runDependencyWatcher && globalThis.setInterval) {
        // Check for missing dependencies every few seconds
        runDependencyWatcher = setInterval(() => {
          if (ABORT) {
            clearInterval(runDependencyWatcher);
            runDependencyWatcher = null;
            return;
          }
          var shown = false;
          for (var dep in runDependencyTracking) {
            if (!shown) {
              shown = true;
              err('still waiting on run dependencies:');
            }
            err(`dependency: ${dep}`);
          }
          if (shown) {
            err('(end of list)');
          }
        }, 10000);
      }
    };
  
  
  var preloadPlugins = [];
  var FS_handledByPreloadPlugin = async (byteArray, fullname) => {
      // Ensure plugins are ready.
      if (typeof Browser != 'undefined') Browser.init();
  
      for (var plugin of preloadPlugins) {
        if (plugin['canHandle'](fullname)) {
          assert(plugin['handle'].constructor.name === 'AsyncFunction', 'Filesystem plugin handlers must be async functions (See #24914)')
          return plugin['handle'](byteArray, fullname);
        }
      }
      // If no plugin handled this file then return the original/unmodified
      // byteArray.
      return byteArray;
    };
  var FS_preloadFile = async (parent, name, url, canRead, canWrite, dontCreateFile, canOwn, preFinish) => {
      // TODO we should allow people to just pass in a complete filename instead
      // of parent and name being that we just join them anyways
      var fullname = name ? PATH_FS.resolve(PATH.join2(parent, name)) : parent;
      var dep = getUniqueRunDependency(`cp ${fullname}`); // might have several active requests for the same fullname
      addRunDependency(dep);
  
      try {
        var byteArray = url;
        if (typeof url == 'string') {
          byteArray = await asyncLoad(url);
        }
  
        byteArray = await FS_handledByPreloadPlugin(byteArray, fullname);
        preFinish?.();
        if (!dontCreateFile) {
          FS_createDataFile(parent, name, byteArray, canRead, canWrite, canOwn);
        }
      } finally {
        removeRunDependency(dep);
      }
    };
  var FS_createPreloadedFile = (parent, name, url, canRead, canWrite, onload, onerror, dontCreateFile, canOwn, preFinish) => {
      FS_preloadFile(parent, name, url, canRead, canWrite, dontCreateFile, canOwn, preFinish).then(onload).catch(onerror);
    };
  
  var FS = {
  root:null,
  mounts:[],
  devices:{
  },
  streams:[],
  nextInode:1,
  nameTable:null,
  currentPath:"/",
  initialized:false,
  ignorePermissions:true,
  filesystems:null,
  syncFSRequests:0,
  ErrnoError:class extends Error {
        name = 'ErrnoError';
        // We set the `name` property to be able to identify `FS.ErrnoError`
        // - the `name` is a standard ECMA-262 property of error objects. Kind of good to have it anyway.
        // - when using PROXYFS, an error can come from an underlying FS
        // as different FS objects have their own FS.ErrnoError each,
        // the test `err instanceof FS.ErrnoError` won't detect an error coming from another filesystem, causing bugs.
        // we'll use the reliable test `err.name == "ErrnoError"` instead
        constructor(errno) {
          super(runtimeInitialized ? strError(errno) : '');
          this.errno = errno;
          for (var key in ERRNO_CODES) {
            if (ERRNO_CODES[key] === errno) {
              this.code = key;
              break;
            }
          }
        }
      },
  FSStream:class {
        shared = {};
        get object() {
          return this.node;
        }
        set object(val) {
          this.node = val;
        }
        get isRead() {
          return (this.flags & 2097155) !== 1;
        }
        get isWrite() {
          return (this.flags & 2097155) !== 0;
        }
        get isAppend() {
          return (this.flags & 1024);
        }
        get flags() {
          return this.shared.flags;
        }
        set flags(val) {
          this.shared.flags = val;
        }
        get position() {
          return this.shared.position;
        }
        set position(val) {
          this.shared.position = val;
        }
      },
  FSNode:class {
        node_ops = {};
        stream_ops = {};
        readMode = 292 | 73;
        writeMode = 146;
        mounted = null;
        constructor(parent, name, mode, rdev) {
          if (!parent) {
            parent = this;  // root node sets parent to itself
          }
          this.parent = parent;
          this.mount = parent.mount;
          this.id = FS.nextInode++;
          this.name = name;
          this.mode = mode;
          this.rdev = rdev;
          this.atime = this.mtime = this.ctime = Date.now();
        }
        get read() {
          return (this.mode & this.readMode) === this.readMode;
        }
        set read(val) {
          val ? this.mode |= this.readMode : this.mode &= ~this.readMode;
        }
        get write() {
          return (this.mode & this.writeMode) === this.writeMode;
        }
        set write(val) {
          val ? this.mode |= this.writeMode : this.mode &= ~this.writeMode;
        }
        get isFolder() {
          return FS.isDir(this.mode);
        }
        get isDevice() {
          return FS.isChrdev(this.mode);
        }
        // The per-inode readiness wait-queue. The node carries a Set of listener
        // entries {cb}; producers (SOCKFS, PIPEFS) call notifyListeners on a
        // readiness transition, and poll()/epoll consume it. It lives on the node
        // (not the fd) so dup'd fds share one queue. Only nodes that derive real
        // readiness (sockets, pipes, and an epoll's own node) ever use this -
        // always-ready types (regular files, ttys) never register or notify.
        addListener(cb, exclusive = false) {
          var entry = {cb, exclusive};
          var listeners = (this.listeners ??= new Set());
          listeners.add(entry);
          return {listeners, entry};
        }
        notifyListeners(flags) {
          // Iterates the set without copying, which is safe ONLY under a
          // load-bearing contract that every internal listener must honour:
          //   1. A listener must not run user code synchronously (a poll waiter only
          //      resolves a Promise; an epoll registration only re-lists +
          //      re-notifies; the epoll callback only schedules a tick). User code
          //      runs on a later tick, never inside this loop.
          //   2. A listener may delete entries only from ITS OWN waiter, never from
          //      a sibling node's set that may be mid-iteration. (Deleting an entry
          //      of the set being iterated here is fine - a Set tolerates removal of
          //      a not-yet-visited entry mid-iteration; mutating a *different* node's
          //      set is fine because that set is not being iterated.)
          // Violating either gives silently skipped wakeups that are near-impossible
          // to reproduce. Any new producer/listener must preserve it.
          if (!this.listeners) return;
          // Fire every non-exclusive listener. Among EPOLLEXCLUSIVE registrations
          // (one fd watched by several epolls) wake only one, rotating round-robin
          // per node, to avoid a thundering herd. (Only epoll registrations are ever
          // exclusive; poll waiters and a node's own consumers are not.)
          var excl;
          for (var entry of this.listeners) {
            if (entry.exclusive) (excl ||= []).push(entry);
            else entry.cb(flags);
          }
          if (excl) {
            var i = (this.exclTurn || 0) % excl.length;
            this.exclTurn = i + 1;
            excl[i].cb(flags);
          }
        }
      },
  lookupPath(path, opts = {}) {
        if (!path) {
          throw new FS.ErrnoError(44);
        }
        opts.follow_mount ??= true
  
        if (!PATH.isAbs(path)) {
          path = FS.cwd() + '/' + path;
        }
  
        // limit max consecutive symlinks to SYMLOOP_MAX.
        linkloop: for (var nlinks = 0; nlinks < 40; nlinks++) {
          // split the absolute path
          var parts = path.split('/').filter((p) => !!p);
  
          // start at the root
          var current = FS.root;
          var current_path = '/';
  
          for (var i = 0; i < parts.length; i++) {
            var islast = (i === parts.length-1);
            if (islast && opts.parent) {
              // stop resolving
              break;
            }
  
            if (parts[i] === '.') {
              continue;
            }
  
            if (parts[i] === '..') {
              current_path = PATH.dirname(current_path);
              if (FS.isRoot(current)) {
                path = current_path + '/' + parts.slice(i + 1).join('/');
                // We're making progress here, don't let many consecutive ..'s
                // lead to ELOOP
                nlinks--;
                continue linkloop;
              } else {
                current = current.parent;
              }
              continue;
            }
  
            current_path = PATH.join2(current_path, parts[i]);
            try {
              current = FS.lookupNode(current, parts[i]);
            } catch (e) {
              // if noent_okay is true, suppress a ENOENT in the last component
              // and return an object with an undefined node. This is needed for
              // resolving symlinks in the path when creating a file.
              if ((e?.errno === 44) && islast && opts.noent_okay) {
                return { path: current_path };
              }
              throw e;
            }
  
            // jump to the mount's root node if this is a mountpoint
            if (FS.isMountpoint(current) && (!islast || opts.follow_mount)) {
              current = current.mounted.root;
            }
  
            // by default, lookupPath will not follow a symlink if it is the final path component.
            // setting opts.follow = true will override this behavior.
            if (FS.isLink(current.mode) && (!islast || opts.follow)) {
              if (!current.node_ops.readlink) {
                throw new FS.ErrnoError(52);
              }
              var link = current.node_ops.readlink(current);
              if (!PATH.isAbs(link)) {
                link = PATH.dirname(current_path) + '/' + link;
              }
              path = link + '/' + parts.slice(i + 1).join('/');
              continue linkloop;
            }
          }
          return { path: current_path, node: current };
        }
        throw new FS.ErrnoError(32);
      },
  getPath(node) {
        var path;
        while (true) {
          if (FS.isRoot(node)) {
            var mount = node.mount.mountpoint;
            if (!path) return mount;
            return mount[mount.length-1] !== '/' ? `${mount}/${path}` : mount + path;
          }
          path = path ? `${node.name}/${path}` : node.name;
          node = node.parent;
        }
      },
  hashName(parentid, name) {
        var hash = 0;
  
        for (var i = 0; i < name.length; i++) {
          hash = ((hash << 5) - hash + name.charCodeAt(i)) | 0;
        }
        return ((parentid + hash) >>> 0) % FS.nameTable.length;
      },
  hashAddNode(node) {
        var hash = FS.hashName(node.parent.id, node.name);
        node.name_next = FS.nameTable[hash];
        FS.nameTable[hash] = node;
      },
  hashRemoveNode(node) {
        var hash = FS.hashName(node.parent.id, node.name);
        if (FS.nameTable[hash] === node) {
          FS.nameTable[hash] = node.name_next;
        } else {
          var current = FS.nameTable[hash];
          while (current) {
            if (current.name_next === node) {
              current.name_next = node.name_next;
              break;
            }
            current = current.name_next;
          }
        }
      },
  lookupNode(parent, name) {
        var errCode = FS.mayLookup(parent);
        if (errCode) {
          throw new FS.ErrnoError(errCode);
        }
        var hash = FS.hashName(parent.id, name);
        for (var node = FS.nameTable[hash]; node; node = node.name_next) {
          var nodeName = node.name;
          if (node.parent.id === parent.id && nodeName === name) {
            return node;
          }
        }
        // if we failed to find it in the cache, call into the VFS
        return FS.lookup(parent, name);
      },
  createNode(parent, name, mode, rdev) {
        assert(typeof parent == 'object')
        var node = new FS.FSNode(parent, name, mode, rdev);
  
        FS.hashAddNode(node);
  
        return node;
      },
  destroyNode(node) {
        FS.hashRemoveNode(node);
      },
  isRoot(node) {
        return node === node.parent;
      },
  isMountpoint(node) {
        return !!node.mounted;
      },
  isFile(mode) {
        return (mode & 61440) === 32768;
      },
  isDir(mode) {
        return (mode & 61440) === 16384;
      },
  isLink(mode) {
        return (mode & 61440) === 40960;
      },
  isChrdev(mode) {
        return (mode & 61440) === 8192;
      },
  isBlkdev(mode) {
        return (mode & 61440) === 24576;
      },
  isFIFO(mode) {
        return (mode & 61440) === 4096;
      },
  isSocket(mode) {
        return (mode & 49152) === 49152;
      },
  flagsToPermissionString(flag) {
        var perms = ['r', 'w', 'rw'][flag & 3];
        if ((flag & 512)) {
          perms += 'w';
        }
        return perms;
      },
  nodePermissions(node, perms) {
        if (FS.ignorePermissions) {
          return 0;
        }
        // return 0 if any user, group or owner bits are set.
        if (perms.includes('r') && !(node.mode & 292)) {
          return 2;
        }
        if (perms.includes('w') && !(node.mode & 146)) {
          return 2;
        }
        if (perms.includes('x') && !(node.mode & 73)) {
          return 2;
        }
        return 0;
      },
  mayLookup(dir) {
        if (!FS.isDir(dir.mode)) return 54;
        var errCode = FS.nodePermissions(dir, 'x');
        if (errCode) return errCode;
        if (!dir.node_ops.lookup) return 2;
        return 0;
      },
  mayCreate(dir, name) {
        if (!FS.isDir(dir.mode)) {
          return 54;
        }
        try {
          var node = FS.lookupNode(dir, name);
          return 20;
        } catch (e) {
        }
        return FS.nodePermissions(dir, 'wx');
      },
  mayDelete(dir, name, isdir) {
        var node;
        try {
          node = FS.lookupNode(dir, name);
        } catch (e) {
          return e.errno;
        }
        var errCode = FS.nodePermissions(dir, 'wx');
        if (errCode) {
          return errCode;
        }
        if (isdir) {
          if (!FS.isDir(node.mode)) {
            return 54;
          }
          if (FS.isRoot(node) || FS.getPath(node) === FS.cwd()) {
            return 10;
          }
        } else if (FS.isDir(node.mode)) {
          return 31;
        }
        return 0;
      },
  mayOpen(node, flags) {
        if (!node) {
          return 44;
        }
        if (FS.isLink(node.mode)) {
          return 32;
        }
        var mode = FS.flagsToPermissionString(flags);
        if (FS.isDir(node.mode)) {
          // opening for write
          // TODO: check for O_SEARCH? (== search for dir only)
          if (mode !== 'r' || (flags & (512 | 64))) {
            return 31;
          }
        }
        return FS.nodePermissions(node, mode);
      },
  checkOpExists(op, err) {
        if (!op) {
          throw new FS.ErrnoError(err);
        }
        return op;
      },
  MAX_OPEN_FDS:4096,
  nextfd() {
        for (var fd = 0; fd <= FS.MAX_OPEN_FDS; fd++) {
          if (!FS.streams[fd]) {
            return fd;
          }
        }
        throw new FS.ErrnoError(33);
      },
  getStreamChecked(fd) {
        var stream = FS.getStream(fd);
        if (!stream) {
          throw new FS.ErrnoError(8);
        }
        return stream;
      },
  getStream:(fd) => FS.streams[fd],
  createStream(stream, fd = -1) {
        assert(fd >= -1);
  
        // clone it, so we can return an instance of FSStream
        stream = Object.assign(new FS.FSStream(), stream);
        if (fd == -1) {
          fd = FS.nextfd();
        }
        stream.fd = fd;
        FS.streams[fd] = stream;
        return stream;
      },
  closeStream(fd) {
        FS.streams[fd] = null;
      },
  dupStream(origStream, fd = -1) {
        var stream = FS.createStream(origStream, fd);
        stream.stream_ops?.dup?.(stream);
        return stream;
      },
  doSetAttr(stream, node, attr) {
        var setattr = stream?.stream_ops.setattr;
        var arg = setattr ? stream : node;
        setattr ??= node.node_ops.setattr;
        FS.checkOpExists(setattr, 63)
        try {
          setattr(arg, attr);
        } catch (e) {
          if (e instanceof RangeError) {
            throw new FS.ErrnoError(22);
          }
          throw e;
        }
      },
  chrdev_stream_ops:{
  open(stream) {
          var device = FS.getDevice(stream.node.rdev);
          // override node's stream ops with the device's
          stream.stream_ops = device.stream_ops;
          // forward the open call
          stream.stream_ops.open?.(stream);
        },
  llseek() {
          throw new FS.ErrnoError(70);
        },
  },
  major:(dev) => ((dev) >> 8),
  minor:(dev) => ((dev) & 0xff),
  makedev:(ma, mi) => ((ma) << 8 | (mi)),
  registerDevice(dev, ops) {
        FS.devices[dev] = { stream_ops: ops };
      },
  getDevice:(dev) => FS.devices[dev],
  getMounts(mount) {
        var mounts = [];
        var check = [mount];
  
        while (check.length) {
          var m = check.pop();
  
          mounts.push(m);
  
          check.push(...m.mounts);
        }
  
        return mounts;
      },
  syncfs(populate, callback) {
        if (typeof populate == 'function') {
          callback = populate;
          populate = false;
        }
  
        FS.syncFSRequests++;
  
        if (FS.syncFSRequests > 1) {
          err(`warning: ${FS.syncFSRequests} FS.syncfs operations in flight at once, probably just doing extra work`);
        }
  
        var mounts = FS.getMounts(FS.root.mount);
        var completed = 0;
  
        function doCallback(errCode) {
          assert(FS.syncFSRequests > 0);
          FS.syncFSRequests--;
          return callback(errCode);
        }
  
        function done(errCode) {
          if (errCode) {
            if (!done.errored) {
              done.errored = true;
              return doCallback(errCode);
            }
            return;
          }
          if (++completed >= mounts.length) {
            doCallback(null);
          }
        };
  
        // sync all mounts
        for (var mount of mounts) {
          if (mount.type.syncfs) {
            mount.type.syncfs(mount, populate, done);
          } else {
            done(null);
          }
        }
      },
  mount(type, opts, mountpoint) {
        if (typeof type == 'string') {
          // The filesystem was not included, and instead we have an error
          // message stored in the variable.
          throw type;
        }
        var root = mountpoint === '/';
        var pseudo = !mountpoint;
        var node;
  
        if (root && FS.root) {
          throw new FS.ErrnoError(10);
        } else if (!root && !pseudo) {
          var lookup = FS.lookupPath(mountpoint, { follow_mount: false });
  
          mountpoint = lookup.path;  // use the absolute path
          node = lookup.node;
  
          if (FS.isMountpoint(node)) {
            throw new FS.ErrnoError(10);
          }
  
          if (!FS.isDir(node.mode)) {
            throw new FS.ErrnoError(54);
          }
        }
  
        var mount = {
          type,
          opts,
          mountpoint,
          mounts: []
        };
  
        // create a root node for the fs
        var mountRoot = type.mount(mount);
        mountRoot.mount = mount;
        mount.root = mountRoot;
  
        if (root) {
          FS.root = mountRoot;
        } else if (node) {
          // set as a mountpoint
          node.mounted = mount;
  
          // add the new mount to the current mount's children
          if (node.mount) {
            node.mount.mounts.push(mount);
          }
        }
  
        return mountRoot;
      },
  unmount(mountpoint) {
        var lookup = FS.lookupPath(mountpoint, { follow_mount: false });
  
        if (!FS.isMountpoint(lookup.node)) {
          throw new FS.ErrnoError(28);
        }
  
        // destroy the nodes for this mount, and all its child mounts
        var node = lookup.node;
        var mount = node.mounted;
        var mounts = FS.getMounts(mount);
  
        for (var [hash, current] of Object.entries(FS.nameTable)) {
          while (current) {
            var next = current.name_next;
  
            if (mounts.includes(current.mount)) {
              FS.destroyNode(current);
            }
  
            current = next;
          }
        }
  
        // no longer a mountpoint
        node.mounted = null;
  
        // remove this mount from the child mounts
        var idx = node.mount.mounts.indexOf(mount);
        assert(idx !== -1);
        node.mount.mounts.splice(idx, 1);
      },
  lookup(parent, name) {
        return parent.node_ops.lookup(parent, name);
      },
  mknod(path, mode, dev) {
        var lookup = FS.lookupPath(path, { parent: true });
        var parent = lookup.node;
        var name = PATH.basename(path);
        if (!name) {
          throw new FS.ErrnoError(28);
        }
        if (name === '.' || name === '..') {
          throw new FS.ErrnoError(20);
        }
        var errCode = FS.mayCreate(parent, name);
        if (errCode) {
          throw new FS.ErrnoError(errCode);
        }
        if (!parent.node_ops.mknod) {
          throw new FS.ErrnoError(63);
        }
        return parent.node_ops.mknod(parent, name, mode, dev);
      },
  statfs(path) {
        return FS.statfsNode(FS.lookupPath(path, {follow: true}).node);
      },
  statfsStream(stream) {
        // We keep a separate statfsStream function because noderawfs overrides
        // it. In noderawfs, stream.node is sometimes null. Instead, we need to
        // look at stream.path.
        return FS.statfsNode(stream.node);
      },
  statfsNode(node) {
        // NOTE: None of the defaults here are true. We're just returning safe and
        //       sane values. Currently nodefs and rawfs replace these defaults,
        //       other file systems leave them alone.
        var rtn = {
          bsize: 4096,
          frsize: 4096,
          blocks: 1e6,
          bfree: 5e5,
          bavail: 5e5,
          files: FS.nextInode,
          ffree: FS.nextInode - 1,
          fsid: 42,
          flags: 2,
          namelen: 255,
        };
  
        if (node.node_ops.statfs) {
          Object.assign(rtn, node.node_ops.statfs(node.mount.opts.root));
        }
        return rtn;
      },
  create(path, mode = 0o666) {
        mode &= 4095;
        mode |= 32768;
        return FS.mknod(path, mode, 0);
      },
  mkdir(path, mode = 0o777) {
        mode &= 511 | 512;
        mode |= 16384;
        return FS.mknod(path, mode, 0);
      },
  mkdirTree(path, mode) {
        var dirs = path.split('/');
        var d = '';
        for (var dir of dirs) {
          if (!dir) continue;
          if (d || PATH.isAbs(path)) d += '/';
          d += dir;
          try {
            FS.mkdir(d, mode);
          } catch(e) {
            if (e.errno != 20) throw e;
          }
        }
      },
  mkdev(path, mode, dev) {
        if (typeof dev == 'undefined') {
          dev = mode;
          mode = 0o666;
        }
        mode |= 8192;
        return FS.mknod(path, mode, dev);
      },
  symlink(oldpath, newpath) {
        if (!PATH_FS.resolve(oldpath)) {
          throw new FS.ErrnoError(44);
        }
        var lookup = FS.lookupPath(newpath, { parent: true });
        var parent = lookup.node;
        if (!parent) {
          throw new FS.ErrnoError(44);
        }
        var newname = PATH.basename(newpath);
        var errCode = FS.mayCreate(parent, newname);
        if (errCode) {
          throw new FS.ErrnoError(errCode);
        }
        if (!parent.node_ops.symlink) {
          throw new FS.ErrnoError(63);
        }
        return parent.node_ops.symlink(parent, newname, oldpath);
      },
  link(oldpath, newpath, flags) {
        var lookup = FS.lookupPath(newpath, { parent: true });
        var parent = lookup.node;
        if (!parent) {
          throw new FS.ErrnoError(44);
        }
        var newname = PATH.basename(newpath);
        var errCode = FS.mayCreate(parent, newname);
        if (errCode) {
          throw new FS.ErrnoError(errCode);
        }
        // Hardlinks are only supported by filesystem backends that provide a
        // `link` node op (e.g. NODERAWFS backed by the host). NODEFS omits it:
        // a host hardlink cannot be confined to the mount root.
        if (!parent.node_ops.link) {
          throw new FS.ErrnoError(34);
        }
        return parent.node_ops.link(parent, newname, oldpath, flags);
      },
  rename(old_path, new_path) {
        var old_dirname = PATH.dirname(old_path);
        var new_dirname = PATH.dirname(new_path);
        var old_name = PATH.basename(old_path);
        var new_name = PATH.basename(new_path);
        // parents must exist
        var lookup, old_dir, new_dir;
  
        // let the errors from non existent directories percolate up
        lookup = FS.lookupPath(old_path, { parent: true });
        old_dir = lookup.node;
        lookup = FS.lookupPath(new_path, { parent: true });
        new_dir = lookup.node;
  
        if (!old_dir || !new_dir) throw new FS.ErrnoError(44);
        // need to be part of the same mount
        if (old_dir.mount !== new_dir.mount) {
          throw new FS.ErrnoError(75);
        }
        // source must exist
        var old_node = FS.lookupNode(old_dir, old_name);
        // old path should not be an ancestor of the new path
        var relative = PATH_FS.relative(old_path, new_dirname);
        if (relative.charAt(0) !== '.') {
          throw new FS.ErrnoError(28);
        }
        // new path should not be an ancestor of the old path
        relative = PATH_FS.relative(new_path, old_dirname);
        if (relative.charAt(0) !== '.') {
          throw new FS.ErrnoError(55);
        }
        // see if the new path already exists
        var new_node;
        try {
          new_node = FS.lookupNode(new_dir, new_name);
        } catch (e) {
          // not fatal
        }
        // early out if nothing needs to change
        if (old_node === new_node) {
          return;
        }
        // we'll need to delete the old entry
        var isdir = FS.isDir(old_node.mode);
        var errCode = FS.mayDelete(old_dir, old_name, isdir);
        if (errCode) {
          throw new FS.ErrnoError(errCode);
        }
        // need delete permissions if we'll be overwriting.
        // need create permissions if new doesn't already exist.
        errCode = new_node ?
          FS.mayDelete(new_dir, new_name, isdir) :
          FS.mayCreate(new_dir, new_name);
        if (errCode) {
          throw new FS.ErrnoError(errCode);
        }
        if (!old_dir.node_ops.rename) {
          throw new FS.ErrnoError(63);
        }
        if (FS.isMountpoint(old_node) || (new_node && FS.isMountpoint(new_node))) {
          throw new FS.ErrnoError(10);
        }
        // if we are going to change the parent, check write permissions
        if (new_dir !== old_dir) {
          errCode = FS.nodePermissions(old_dir, 'w');
          if (errCode) {
            throw new FS.ErrnoError(errCode);
          }
        }
        // remove the node from the lookup hash
        FS.hashRemoveNode(old_node);
        // do the underlying fs rename
        try {
          old_dir.node_ops.rename(old_node, new_dir, new_name);
          // update old node (we do this here to avoid each backend
          // needing to)
          old_node.parent = new_dir;
        } catch (e) {
          throw e;
        } finally {
          // add the node back to the hash (in case node_ops.rename
          // changed its name)
          FS.hashAddNode(old_node);
        }
      },
  rmdir(path) {
        var lookup = FS.lookupPath(path, { parent: true });
        var parent = lookup.node;
        var name = PATH.basename(path);
        var node = FS.lookupNode(parent, name);
        var errCode = FS.mayDelete(parent, name, true);
        if (errCode) {
          throw new FS.ErrnoError(errCode);
        }
        if (!parent.node_ops.rmdir) {
          throw new FS.ErrnoError(63);
        }
        if (FS.isMountpoint(node)) {
          throw new FS.ErrnoError(10);
        }
        parent.node_ops.rmdir(parent, name);
        FS.destroyNode(node);
      },
  readdir(path) {
        var lookup = FS.lookupPath(path, { follow: true });
        var node = lookup.node;
        var readdir = FS.checkOpExists(node.node_ops.readdir, 54);
        return readdir(node);
      },
  unlink(path) {
        var lookup = FS.lookupPath(path, { parent: true });
        var parent = lookup.node;
        if (!parent) {
          throw new FS.ErrnoError(44);
        }
        var name = PATH.basename(path);
        var node = FS.lookupNode(parent, name);
        var errCode = FS.mayDelete(parent, name, false);
        if (errCode) {
          // According to POSIX, we should map EISDIR to EPERM, but
          // we instead do what Linux does (and we must, as we use
          // the musl linux libc).
          throw new FS.ErrnoError(errCode);
        }
        if (!parent.node_ops.unlink) {
          throw new FS.ErrnoError(63);
        }
        if (FS.isMountpoint(node)) {
          throw new FS.ErrnoError(10);
        }
        parent.node_ops.unlink(parent, name);
        FS.destroyNode(node);
      },
  readlink(path) {
        var lookup = FS.lookupPath(path);
        var link = lookup.node;
        if (!link) {
          throw new FS.ErrnoError(44);
        }
        if (!link.node_ops.readlink) {
          throw new FS.ErrnoError(28);
        }
        return link.node_ops.readlink(link);
      },
  stat(path, dontFollow) {
        var lookup = FS.lookupPath(path, { follow: !dontFollow });
        var node = lookup.node;
        var getattr = FS.checkOpExists(node.node_ops.getattr, 63);
        return getattr(node);
      },
  fstat(fd) {
        var stream = FS.getStreamChecked(fd);
        var node = stream.node;
        var getattr = stream.stream_ops.getattr;
        var arg = getattr ? stream : node;
        getattr ??= node.node_ops.getattr;
        FS.checkOpExists(getattr, 63)
        return getattr(arg);
      },
  lstat(path) {
        return FS.stat(path, true);
      },
  doChmod(stream, node, mode, dontFollow) {
        FS.doSetAttr(stream, node, {
          mode: (mode & 4095) | (node.mode & ~4095),
          ctime: Date.now(),
          dontFollow
        });
      },
  chmod(path, mode, dontFollow) {
        var node;
        if (typeof path == 'string') {
          var lookup = FS.lookupPath(path, { follow: !dontFollow });
          node = lookup.node;
        } else {
          node = path;
        }
        FS.doChmod(null, node, mode, dontFollow);
      },
  lchmod(path, mode) {
        FS.chmod(path, mode, true);
      },
  fchmod(fd, mode) {
        var stream = FS.getStreamChecked(fd);
        FS.doChmod(stream, stream.node, mode, false);
      },
  doChown(stream, node, dontFollow) {
        FS.doSetAttr(stream, node, {
          timestamp: Date.now(),
          dontFollow
          // we ignore the uid / gid for now
        });
      },
  chown(path, uid, gid, dontFollow) {
        var node;
        if (typeof path == 'string') {
          var lookup = FS.lookupPath(path, { follow: !dontFollow });
          node = lookup.node;
        } else {
          node = path;
        }
        FS.doChown(null, node, dontFollow);
      },
  lchown(path, uid, gid) {
        FS.chown(path, uid, gid, true);
      },
  fchown(fd, uid, gid) {
        var stream = FS.getStreamChecked(fd);
        FS.doChown(stream, stream.node, false);
      },
  doTruncate(stream, node, len) {
        if (FS.isDir(node.mode)) {
          throw new FS.ErrnoError(31);
        }
        if (!FS.isFile(node.mode)) {
          throw new FS.ErrnoError(28);
        }
        var errCode = FS.nodePermissions(node, 'w');
        if (errCode) {
          throw new FS.ErrnoError(errCode);
        }
        FS.doSetAttr(stream, node, {
          size: len,
          timestamp: Date.now()
        });
      },
  truncate(path, len) {
        if (len < 0) {
          throw new FS.ErrnoError(28);
        }
        var node;
        if (typeof path == 'string') {
          var lookup = FS.lookupPath(path, { follow: true });
          node = lookup.node;
        } else {
          node = path;
        }
        FS.doTruncate(null, node, len);
      },
  ftruncate(fd, len) {
        var stream = FS.getStreamChecked(fd);
        if (len < 0 || (stream.flags & 2097155) === 0) {
          throw new FS.ErrnoError(28);
        }
        FS.doTruncate(stream, stream.node, len);
      },
  utime(path, atime, mtime, dontFollow) {
        var lookup = FS.lookupPath(path, { follow: !dontFollow });
        FS.doSetAttr(null, lookup.node, {
          atime: atime,
          mtime: mtime,
          dontFollow
        });
      },
  open(path, flags, mode = 0o666) {
        if (path === '') {
          throw new FS.ErrnoError(44);
        }
        flags = FS_modeStringToFlags(flags);
        if ((flags & 64)) {
          mode = (mode & 4095) | 32768;
        } else {
          mode = 0;
        }
        var node;
        var isDirPath;
        if (typeof path == 'object') {
          node = path;
        } else {
          isDirPath = path.endsWith('/');
          // noent_okay makes it so that if the final component of the path
          // doesn't exist, lookupPath returns `node: undefined`. `path` will be
          // updated to point to the target of all symlinks.
          var lookup = FS.lookupPath(path, {
            follow: !(flags & 131072),
            noent_okay: true
          });
          node = lookup.node;
          path = lookup.path;
        }
        // perhaps we need to create the node
        var created = false;
        if ((flags & 64)) {
          if (node) {
            // if O_CREAT and O_EXCL are set, error out if the node already exists
            if ((flags & 128)) {
              throw new FS.ErrnoError(20);
            }
          } else if (isDirPath) {
            throw new FS.ErrnoError(31);
          } else {
            // node doesn't exist, try to create it
            // Ignore the permission bits here to ensure we can `open` this new
            // file below. We use chmod below to apply the permissions once the
            // file is open.
            node = FS.mknod(path, mode | 0o777, 0);
            created = true;
          }
        }
        if (!node) {
          throw new FS.ErrnoError(44);
        }
        // can't truncate a device
        if (FS.isChrdev(node.mode)) {
          flags &= ~512;
        }
        // if asked only for a directory, then this must be one
        if ((flags & 65536) && !FS.isDir(node.mode)) {
          throw new FS.ErrnoError(54);
        }
        // check permissions, if this is not a file we just created now (it is ok to
        // create and write to a file with read-only permissions; it is read-only
        // for later use)
        if (!created) {
          var errCode = FS.mayOpen(node, flags);
          if (errCode) {
            throw new FS.ErrnoError(errCode);
          }
        }
        // do truncation if necessary
        if ((flags & 512) && !created) {
          FS.truncate(node, 0);
        }
        // we've already handled these, don't pass down to the underlying vfs
        flags &= ~(128 | 512 | 131072);
  
        // register the stream with the filesystem
        var stream = FS.createStream({
          node,
          path: FS.getPath(node),  // we want the absolute path to the node
          flags,
          seekable: true,
          position: 0,
          stream_ops: node.stream_ops,
          // used by the file family libc calls (fopen, fwrite, ferror, etc.)
          ungotten: [],
          error: false
        });
        // call the new stream's open function
        if (stream.stream_ops.open) {
          stream.stream_ops.open(stream);
        }
        if (created) {
          FS.chmod(node, mode & 0o777);
        }
        return stream;
      },
  close(stream) {
        if (FS.isClosed(stream)) {
          throw new FS.ErrnoError(8);
        }
        if (stream.getdents) stream.getdents = null; // free readdir state
        // The fd is going away: wake anything waiting on it (poll/epoll) with
        // POLLNVAL so a blocking wait unblocks and an epoll registration is evicted
        // on its next derive. Only sockets/pipes/epoll ever carry a wait-queue, so
        // for every other stream (incl. nodeless noderawfs stdio) this is a no-op.
        stream.node?.notifyListeners(32);
        try {
          if (stream.stream_ops.close) {
            stream.stream_ops.close(stream);
          }
        } catch (e) {
          throw e;
        } finally {
          FS.closeStream(stream.fd);
        }
        stream.fd = null;
      },
  isClosed(stream) {
        return stream.fd === null;
      },
  llseek(stream, offset, whence) {
        if (FS.isClosed(stream)) {
          throw new FS.ErrnoError(8);
        }
        if (!stream.seekable || !stream.stream_ops.llseek) {
          throw new FS.ErrnoError(70);
        }
        if (whence != 0 && whence != 1 && whence != 2) {
          throw new FS.ErrnoError(28);
        }
        stream.position = stream.stream_ops.llseek(stream, offset, whence);
        stream.ungotten = [];
        return stream.position;
      },
  read(stream, buffer, offset, length, position) {
        assert(offset >= 0);
        if (length < 0 || position < 0) {
          throw new FS.ErrnoError(28);
        }
        if (FS.isClosed(stream)) {
          throw new FS.ErrnoError(8);
        }
        if ((stream.flags & 2097155) === 1) {
          throw new FS.ErrnoError(8);
        }
        if (FS.isDir(stream.node.mode)) {
          throw new FS.ErrnoError(31);
        }
        if (!stream.stream_ops.read) {
          throw new FS.ErrnoError(28);
        }
        var seeking = typeof position != 'undefined';
        if (!seeking) {
          position = stream.position;
        } else if (!stream.seekable) {
          throw new FS.ErrnoError(70);
        }
        var bytesRead = stream.stream_ops.read(stream, buffer, offset, length, position);
        if (!seeking) stream.position += bytesRead;
        return bytesRead;
      },
  write(stream, buffer, offset, length, position, canOwn) {
        assert(offset >= 0);
        assert(buffer.subarray, 'FS.write expects a TypedArray');
        if (length < 0 || position < 0) {
          throw new FS.ErrnoError(28);
        }
        if (FS.isClosed(stream)) {
          throw new FS.ErrnoError(8);
        }
        if ((stream.flags & 2097155) === 0) {
          throw new FS.ErrnoError(8);
        }
        if (FS.isDir(stream.node.mode)) {
          throw new FS.ErrnoError(31);
        }
        if (!stream.stream_ops.write) {
          throw new FS.ErrnoError(28);
        }
        if (stream.seekable && stream.flags & 1024) {
          // seek to the end before writing in append mode
          FS.llseek(stream, 0, 2);
        }
        var seeking = typeof position != 'undefined';
        if (!seeking) {
          position = stream.position;
        } else if (!stream.seekable) {
          throw new FS.ErrnoError(70);
        }
        var bytesWritten = stream.stream_ops.write(stream, buffer, offset, length, position, canOwn);
        if (!seeking) stream.position += bytesWritten;
        return bytesWritten;
      },
  mmap(stream, length, position, prot, flags) {
        // User requests writing to file (prot & PROT_WRITE != 0).
        // Checking if we have permissions to write to the file unless
        // MAP_PRIVATE flag is set. According to POSIX spec it is possible
        // to write to file opened in read-only mode with MAP_PRIVATE flag,
        // as all modifications will be visible only in the memory of
        // the current process.
        if ((prot & 2)
            && !(flags & 2)
            && (stream.flags & 2097155) !== 2) {
          throw new FS.ErrnoError(2);
        }
        if ((stream.flags & 2097155) === 1) {
          throw new FS.ErrnoError(2);
        }
        if (!stream.stream_ops.mmap) {
          throw new FS.ErrnoError(43);
        }
        if (!length) {
          throw new FS.ErrnoError(28);
        }
        return stream.stream_ops.mmap(stream, length, position, prot, flags);
      },
  msync(stream, buffer, offset, length, mmapFlags) {
        assert(offset >= 0);
        if (!stream.stream_ops.msync) {
          return 0;
        }
        return stream.stream_ops.msync(stream, buffer, offset, length, mmapFlags);
      },
  ioctl(stream, cmd, arg) {
        if (!stream.stream_ops.ioctl) {
          throw new FS.ErrnoError(59);
        }
        return stream.stream_ops.ioctl(stream, cmd, arg);
      },
  readFile(path, opts = {}) {
        opts.flags = opts.flags ?? 0;
        opts.encoding = opts.encoding ?? 'binary';
        if (opts.encoding !== 'utf8' && opts.encoding !== 'binary') {
          abort(`Invalid encoding type "${opts.encoding}"`);
        }
        var stream = FS.open(path, opts.flags);
        var stat = FS.stat(path);
        var length = stat.size;
        var buf = new Uint8Array(length);
        FS.read(stream, buf, 0, length, 0);
        if (opts.encoding === 'utf8') {
          buf = UTF8ArrayToString(buf);
        }
        FS.close(stream);
        return buf;
      },
  writeFile(path, data, opts = {}) {
        opts.flags = opts.flags ?? 577;
        var stream = FS.open(path, opts.flags, opts.mode);
        data = FS_fileDataToTypedArray(data);
        FS.write(stream, data, 0, data.byteLength, undefined, opts.canOwn);
        FS.close(stream);
      },
  cwd:() => FS.currentPath,
  chdir(path) {
        var lookup = FS.lookupPath(path, { follow: true });
        if (lookup.node === null) {
          throw new FS.ErrnoError(44);
        }
        if (!FS.isDir(lookup.node.mode)) {
          throw new FS.ErrnoError(54);
        }
        var errCode = FS.nodePermissions(lookup.node, 'x');
        if (errCode) {
          throw new FS.ErrnoError(errCode);
        }
        FS.currentPath = lookup.path;
      },
  createDefaultDirectories() {
        FS.mkdir('/tmp');
        FS.mkdir('/home');
        FS.mkdir('/home/web_user');
      },
  createDefaultDevices() {
        // create /dev
        FS.mkdir('/dev');
        // setup /dev/null
        FS.registerDevice(FS.makedev(1, 3), {
          read: () => 0,
          write: (stream, buffer, offset, length, pos) => length,
          llseek: () => 0,
        });
        FS.mkdev('/dev/null', FS.makedev(1, 3));
        // setup /dev/tty and /dev/tty1
        // stderr needs to print output using err() rather than out()
        // so we register a second tty just for it.
        TTY.register(FS.makedev(5, 0), TTY.default_tty_ops);
        TTY.register(FS.makedev(6, 0), TTY.default_tty1_ops);
        FS.mkdev('/dev/tty', FS.makedev(5, 0));
        FS.mkdev('/dev/tty1', FS.makedev(6, 0));
        // setup /dev/[u]random
        // use a buffer to avoid overhead of individual crypto calls per byte
        var randomBuffer = new Uint8Array(1024), randomLeft = 0;
        var randomByte = () => {
          if (!randomLeft) {
            randomFill(randomBuffer);
            randomLeft = randomBuffer.byteLength;
          }
          return randomBuffer[--randomLeft];
        };
        FS.createDevice('/dev', 'random', randomByte);
        FS.createDevice('/dev', 'urandom', randomByte);
        // we're not going to emulate the actual shm device,
        // just create the tmp dirs that reside in it commonly
        FS.mkdir('/dev/shm');
        FS.mkdir('/dev/shm/tmp');
      },
  createSpecialDirectories() {
        // create /proc/self/fd which allows /proc/self/fd/6 => readlink gives the
        // name of the stream for fd 6 (see test_unistd_ttyname)
        FS.mkdir('/proc');
        var proc_self = FS.mkdir('/proc/self');
        FS.mkdir('/proc/self/fd');
        FS.mount({
          mount() {
            var node = FS.createNode(proc_self, 'fd', 16895, 73);
            node.stream_ops = {
              llseek: MEMFS.stream_ops.llseek,
            };
            node.node_ops = {
              lookup(parent, name) {
                var fd = +name;
                var stream = FS.getStreamChecked(fd);
                var ret = {
                  parent: null,
                  mount: { mountpoint: 'fake' },
                  node_ops: { readlink: () => stream.path },
                  id: fd + 1,
                };
                ret.parent = ret; // make it look like a simple root node
                return ret;
              },
              readdir() {
                return Array.from(FS.streams.entries())
                  .filter(([k, v]) => v)
                  .map(([k, v]) => k.toString());
              }
            };
            return node;
          }
        }, {}, '/proc/self/fd');
      },
  createStandardStreams(input, output, error) {
        // TODO deprecate the old functionality of a single
        // input / output callback and that utilizes FS.createDevice
        // and instead require a unique set of stream ops
  
        // by default, we symlink the standard streams to the
        // default tty devices. however, if the standard streams
        // have been overwritten we create a unique device for
        // them instead.
        if (input) {
          FS.createDevice('/dev', 'stdin', input);
        } else {
          FS.symlink('/dev/tty', '/dev/stdin');
        }
        if (output) {
          FS.createDevice('/dev', 'stdout', null, output);
        } else {
          FS.symlink('/dev/tty', '/dev/stdout');
        }
        if (error) {
          FS.createDevice('/dev', 'stderr', null, error);
        } else {
          FS.symlink('/dev/tty1', '/dev/stderr');
        }
  
        // open default streams for the stdin, stdout and stderr devices
        var stdin = FS.open('/dev/stdin', 0);
        var stdout = FS.open('/dev/stdout', 1);
        var stderr = FS.open('/dev/stderr', 1);
        assert(stdin.fd === 0, `invalid handle for stdin (${stdin.fd})`);
        assert(stdout.fd === 1, `invalid handle for stdout (${stdout.fd})`);
        assert(stderr.fd === 2, `invalid handle for stderr (${stderr.fd})`);
      },
  staticInit() {
        FS.nameTable = new Array(4096);
  
        FS.mount(MEMFS, {}, '/');
  
        FS.createDefaultDirectories();
        FS.createDefaultDevices();
        FS.createSpecialDirectories();
  
        FS.filesystems = {
          'MEMFS': MEMFS,
        };
      },
  init(input, output, error) {
        assert(!FS.initialized, 'FS.init was previously called. If you want to initialize later with custom parameters, remove any earlier calls (note that one is automatically added to the generated code)');
        FS.initialized = true;
  
        // Allow Module.stdin etc. to provide defaults, if none explicitly passed to us here
        input ??= Module['stdin'];
        output ??= Module['stdout'];
        error ??= Module['stderr'];
  
        FS.createStandardStreams(input, output, error);
      },
  quit() {
        FS.initialized = false;
        // force-flush all streams, so we get musl std streams printed out
        _fflush(0);
        // close all of our streams
        for (var stream of FS.streams) {
          if (stream) {
            FS.close(stream);
          }
        }
      },
  findObject(path, dontResolveLastLink) {
        var ret = FS.analyzePath(path, dontResolveLastLink);
        if (!ret.exists) {
          return null;
        }
        return ret.object;
      },
  analyzePath(path, dontResolveLastLink) {
        // operate from within the context of the symlink's target
        try {
          var lookup = FS.lookupPath(path, { follow: !dontResolveLastLink });
          path = lookup.path;
        } catch (e) {
        }
        var ret = {
          isRoot: false, exists: false, error: 0, name: null, path: null, object: null,
          parentExists: false, parentPath: null, parentObject: null
        };
        try {
          var lookup = FS.lookupPath(path, { parent: true });
          ret.parentExists = true;
          ret.parentPath = lookup.path;
          ret.parentObject = lookup.node;
          ret.name = PATH.basename(path);
          lookup = FS.lookupPath(path, { follow: !dontResolveLastLink });
          ret.exists = true;
          ret.path = lookup.path;
          ret.object = lookup.node;
          ret.name = lookup.node.name;
          ret.isRoot = lookup.path === '/';
        } catch (e) {
          ret.error = e.errno;
        };
        return ret;
      },
  createPath(parent, path, canRead, canWrite) {
        parent = typeof parent == 'string' ? parent : FS.getPath(parent);
        var parts = path.split('/').reverse();
        while (parts.length) {
          var part = parts.pop();
          if (!part) continue;
          var current = PATH.join2(parent, part);
          try {
            FS.mkdir(current);
          } catch (e) {
            if (e.errno != 20) throw e;
          }
          parent = current;
        }
        return current;
      },
  createFile(parent, name, properties, canRead, canWrite) {
        var path = PATH.join2(typeof parent == 'string' ? parent : FS.getPath(parent), name);
        var mode = FS_getMode(canRead, canWrite);
        return FS.create(path, mode);
      },
  createDataFile(parent, name, data, canRead, canWrite, canOwn) {
        var path = name;
        if (parent) {
          parent = typeof parent == 'string' ? parent : FS.getPath(parent);
          path = name ? PATH.join2(parent, name) : parent;
        }
        var mode = FS_getMode(canRead, canWrite);
        var node = FS.create(path, mode);
        if (data) {
          data = FS_fileDataToTypedArray(data);
          // make sure we can write to the file
          FS.chmod(node, mode | 146);
          var stream = FS.open(node, 577);
          FS.write(stream, data, 0, data.length, 0, canOwn);
          FS.close(stream);
          FS.chmod(node, mode);
        }
      },
  createDevice(parent, name, input, output) {
        var path = PATH.join2(typeof parent == 'string' ? parent : FS.getPath(parent), name);
        var mode = FS_getMode(!!input, !!output);
        FS.createDevice.major ??= 64;
        var dev = FS.makedev(FS.createDevice.major++, 0);
        // Create a fake device that a set of stream ops to emulate
        // the old behavior.
        FS.registerDevice(dev, {
          open(stream) {
            stream.seekable = false;
          },
          close(stream) {
            // flush any pending line data
            if (output?.buffer?.length) {
              output(10);
            }
          },
          read(stream, buffer, offset, length, pos /* ignored */) {
            var bytesRead = 0;
            for (var i = 0; i < length; i++) {
              var result;
              try {
                result = input();
              } catch (e) {
                throw new FS.ErrnoError(29);
              }
              if (result === undefined && !bytesRead) {
                throw new FS.ErrnoError(6);
              }
              if (result === null || result === undefined) break;
              bytesRead++;
              buffer[offset+i] = result;
            }
            if (bytesRead) {
              stream.node.atime = Date.now();
            }
            return bytesRead;
          },
          write(stream, buffer, offset, length, pos) {
            for (var i = 0; i < length; i++) {
              try {
                output(buffer[offset+i]);
              } catch (e) {
                throw new FS.ErrnoError(29);
              }
            }
            if (length) {
              stream.node.mtime = stream.node.ctime = Date.now();
            }
            return i;
          }
        });
        return FS.mkdev(path, mode, dev);
      },
  forceLoadFile(obj) {
        if (obj.isDevice || obj.isFolder || obj.link || obj.contents) return true;
        if (globalThis.XMLHttpRequest) {
          abort('Lazy loading should have been performed (contents set) in createLazyFile, but it was not. Lazy loading only works in web workers. Use --embed-file or --preload-file in emcc on the main thread.');
        } else { // Command-line.
          try {
            obj.contents = readBinary(obj.url);
          } catch (e) {
            throw new FS.ErrnoError(29);
          }
        }
      },
  createLazyFile(parent, name, url, canRead, canWrite) {
        // Lazy chunked Uint8Array (implements get and length from Uint8Array).
        // Actual getting is abstracted away for eventual reuse.
        class LazyUint8Array {
          lengthKnown = false;
          chunks = []; // Loaded chunks. Index is the chunk number
          get(idx) {
            if (idx > this.length-1 || idx < 0) {
              return undefined;
            }
            var chunkOffset = idx % this.chunkSize;
            var chunkNum = (idx / this.chunkSize)|0;
            return this.getter(chunkNum)[chunkOffset];
          }
          setDataGetter(getter) {
            this.getter = getter;
          }
          cacheLength() {
            // Find length
            var xhr = new XMLHttpRequest();
            xhr.open('HEAD', url, false);
            xhr.send(null);
            if (!(xhr.status >= 200 && xhr.status < 300 || xhr.status === 304)) abort(`Couldn't load ${url}. Status: ${xhr.status}`);
            var datalength = Number(xhr.getResponseHeader('Content-length'));
            var header;
            var hasByteServing = (header = xhr.getResponseHeader('Accept-Ranges')) && header === 'bytes';
            var usesGzip = (header = xhr.getResponseHeader('Content-Encoding')) && header === 'gzip';
  
            var chunkSize = 1024*1024; // Chunk size in bytes
  
            if (!hasByteServing) chunkSize = datalength;
  
            // Function to get a range from the remote URL.
            var doXHR = (from, to) => {
              if (from > to) abort(`invalid range (${from}, ${to}) or no bytes requested!`);
              if (to > datalength-1) abort(`only ${datalength} bytes available! programmer error!`);
  
              // TODO: Use mozResponseArrayBuffer, responseStream, etc. if available.
              var xhr = new XMLHttpRequest();
              xhr.open('GET', url, false);
              if (datalength !== chunkSize) xhr.setRequestHeader('Range', `bytes=${from}-${to}`);
  
              // Some hints to the browser that we want binary data.
              xhr.responseType = 'arraybuffer';
              if (xhr.overrideMimeType) {
                xhr.overrideMimeType('text/plain; charset=x-user-defined');
              }
  
              xhr.send(null);
              if (!(xhr.status >= 200 && xhr.status < 300 || xhr.status === 304)) abort(`Couldn't load ${url}. Status: ${xhr.status}`);
              if (xhr.response !== undefined) {
                return new Uint8Array(/** @type{Array<number>} */(xhr.response || []));
              }
              return intArrayFromString(xhr.responseText ?? '', true);
            };
            var lazyArray = this;
            lazyArray.setDataGetter((chunkNum) => {
              var start = chunkNum * chunkSize;
              var end = (chunkNum+1) * chunkSize - 1; // including this byte
              end = Math.min(end, datalength-1); // if datalength-1 is selected, this is the last block
              if (typeof lazyArray.chunks[chunkNum] == 'undefined') {
                lazyArray.chunks[chunkNum] = doXHR(start, end);
              }
              if (typeof lazyArray.chunks[chunkNum] == 'undefined') abort('doXHR failed!');
              return lazyArray.chunks[chunkNum];
            });
  
            if (usesGzip || !datalength) {
              // if the server uses gzip or doesn't supply the length, we have to download the whole file to get the (uncompressed) length
              chunkSize = datalength = 1; // this will force getter(0)/doXHR do download the whole file
              datalength = this.getter(0).length;
              chunkSize = datalength;
              out('LazyFiles on gzip forces download of the whole file when length is accessed');
            }
  
            this._length = datalength;
            this._chunkSize = chunkSize;
            this.lengthKnown = true;
          }
          get length() {
            if (!this.lengthKnown) {
              this.cacheLength();
            }
            return this._length;
          }
          get chunkSize() {
            if (!this.lengthKnown) {
              this.cacheLength();
            }
            return this._chunkSize;
          }
        }
  
        if (globalThis.XMLHttpRequest) {
          if (!ENVIRONMENT_IS_WORKER) abort('Cannot do synchronous binary XHRs outside webworkers in modern browsers. Use --embed-file or --preload-file in emcc');
          var lazyArray = new LazyUint8Array();
          var properties = { isDevice: false, contents: lazyArray };
        } else {
          var properties = { isDevice: false, url: url };
        }
  
        var node = FS.createFile(parent, name, properties, canRead, canWrite);
        // This is a total hack, but I want to get this lazy file code out of the
        // core of MEMFS. If we want to keep this lazy file concept I feel it should
        // be its own thin LAZYFS proxying calls to MEMFS.
        if (properties.contents) {
          node.contents = properties.contents;
        } else if (properties.url) {
          node.contents = null;
          node.url = properties.url;
        }
        // Add a function that defers querying the file size until it is asked the first time.
        Object.defineProperties(node, {
          usedBytes: {
            get: function() { return this.contents.length; }
          }
        });
        // override each stream op with one that tries to force load the lazy file first
        var stream_ops = {};
        for (const [key, fn] of Object.entries(node.stream_ops)) {
          stream_ops[key] = (...args) => {
            FS.forceLoadFile(node);
            return fn(...args);
          };
        }
        function writeChunks(stream, buffer, offset, length, position) {
          var contents = stream.node.contents;
          if (position >= contents.length)
            return 0;
          var size = Math.min(contents.length - position, length);
          assert(size >= 0);
          if (contents.slice) { // normal array
            for (var i = 0; i < size; i++) {
              buffer[offset + i] = contents[position + i];
            }
          } else {
            for (var i = 0; i < size; i++) { // LazyUint8Array from sync binary XHR
              buffer[offset + i] = contents.get(position + i);
            }
          }
          return size;
        }
        // use a custom read function
        stream_ops.read = (stream, buffer, offset, length, position) => {
          FS.forceLoadFile(node);
          return writeChunks(stream, buffer, offset, length, position)
        };
        // use a custom mmap function
        stream_ops.mmap = (stream, length, position, prot, flags) => {
          FS.forceLoadFile(node);
          var ptr = mmapAlloc(length);
          if (!ptr) {
            throw new FS.ErrnoError(48);
          }
          writeChunks(stream, HEAP8, ptr, length, position);
          return { ptr, allocated: true };
        };
        node.stream_ops = stream_ops;
        return node;
      },
  };
  
  
  
  
  
  /** not-@type {!BigInt64Array} */
  var HEAP64;
  var SYSCALLS = {
  currentUmask:18,
  calculateAt(dirfd, path, allowEmpty) {
        if (PATH.isAbs(path)) {
          return path;
        }
        // relative path
        var dir;
        if (dirfd === -100) {
          dir = FS.cwd();
        } else {
          var dirstream = SYSCALLS.getStreamFromFD(dirfd);
          dir = dirstream.path;
        }
        if (path.length == 0) {
          if (!allowEmpty) {
            throw new FS.ErrnoError(44);;
          }
          return dir;
        }
        return dir + '/' + path;
      },
  writeStat(buf, stat) {
        HEAPU32[((buf)>>2)] = stat.dev;
        HEAPU32[(((buf)+(4))>>2)] = stat.mode;
        HEAPU32[(((buf)+(8))>>2)] = stat.nlink;
        HEAPU32[(((buf)+(12))>>2)] = stat.uid;
        HEAPU32[(((buf)+(16))>>2)] = stat.gid;
        HEAPU32[(((buf)+(20))>>2)] = stat.rdev;
        HEAP64[(((buf)+(24))>>3)] = BigInt(stat.size);
        HEAP32[(((buf)+(32))>>2)] = 4096;
        HEAP32[(((buf)+(36))>>2)] = stat.blocks;
        var atime = stat.atime.getTime();
        var mtime = stat.mtime.getTime();
        var ctime = stat.ctime.getTime();
        HEAP64[(((buf)+(40))>>3)] = BigInt(Math.floor(atime / 1000));
        HEAPU32[(((buf)+(48))>>2)] = (atime % 1000) * 1000 * 1000;
        HEAP64[(((buf)+(56))>>3)] = BigInt(Math.floor(mtime / 1000));
        HEAPU32[(((buf)+(64))>>2)] = (mtime % 1000) * 1000 * 1000;
        HEAP64[(((buf)+(72))>>3)] = BigInt(Math.floor(ctime / 1000));
        HEAPU32[(((buf)+(80))>>2)] = (ctime % 1000) * 1000 * 1000;
        HEAP64[(((buf)+(88))>>3)] = BigInt(stat.ino);
        return 0;
      },
  writeStatFs(buf, stats) {
        HEAPU32[(((buf)+(4))>>2)] = stats.bsize;
        HEAPU32[(((buf)+(60))>>2)] = stats.bsize;
        HEAP64[(((buf)+(8))>>3)] = BigInt(stats.blocks);
        HEAP64[(((buf)+(16))>>3)] = BigInt(stats.bfree);
        HEAP64[(((buf)+(24))>>3)] = BigInt(stats.bavail);
        HEAP64[(((buf)+(32))>>3)] = BigInt(stats.files);
        HEAP64[(((buf)+(40))>>3)] = BigInt(stats.ffree);
        HEAPU32[(((buf)+(48))>>2)] = stats.fsid;
        HEAPU32[(((buf)+(64))>>2)] = stats.flags;  // ST_NOSUID
        HEAPU32[(((buf)+(56))>>2)] = stats.namelen;
      },
  doMsync(addr, stream, len, flags, offset) {
        if (!FS.isFile(stream.node.mode)) {
          throw new FS.ErrnoError(43);
        }
        if (flags & 2) {
          // MAP_PRIVATE calls need not to be synced back to underlying fs
          return 0;
        }
        var buffer = HEAPU8.subarray(addr, addr + len);
        FS.msync(stream, buffer, offset, len, flags);
      },
  getStreamFromFD(fd) {
        var stream = FS.getStreamChecked(fd);
        return stream;
      },
  varargs:undefined,
  getStr(ptr) {
        var ret = UTF8ToString(ptr);
        return ret;
      },
  };
  
  /** @type {!Int16Array} */
  var HEAP16;
  function ___syscall_fcntl64(fd, cmd, varargs) {
  SYSCALLS.varargs = varargs;
  try {
  
      var stream = SYSCALLS.getStreamFromFD(fd);
      switch (cmd) {
        case 0: {
          var arg = syscallGetVarargI();
          if (arg < 0) {
            return -28;
          }
          while (FS.streams[arg]) {
            arg++;
          }
          var newStream;
          newStream = FS.dupStream(stream, arg);
          return newStream.fd;
        }
        case 1:
        case 2:
          return 0;  // FD_CLOEXEC makes no sense for a single process.
        case 3:
          return stream.flags;
        case 4: {
          var arg = syscallGetVarargI();
          var mask = 289792;
          stream.flags = (stream.flags & ~mask) | (arg & mask);
          return 0;
        }
        case 12: {
          var arg = syscallGetVarargP();
          var offset = 0;
          // We're always unlocked.
          HEAP16[(((arg)+(offset))>>1)] = 2;
          return 0;
        }
        case 13:
        case 14:
          // Pretend that the locking is successful. These are process-level locks,
          // and Emscripten programs are a single process. If we supported linking a
          // filesystem between programs, we'd need to do more here.
          // See https://github.com/emscripten-core/emscripten/issues/23697
          return 0;
      }
      return -28;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  }
  

  function ___syscall_fstat64(fd, buf) {
  try {
  
      return SYSCALLS.writeStat(buf, FS.fstat(fd));
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  }
  

  
  
  
  
  function ___syscall_ioctl(fd, op, varargs) {
  SYSCALLS.varargs = varargs;
  try {
  
      var stream = SYSCALLS.getStreamFromFD(fd);
      switch (op) {
        case 21509: {
          if (!stream.tty) return -59;
          return 0;
        }
        case 21505: {
          if (!stream.tty) return -59;
          if (stream.tty.ops.ioctl_tcgets) {
            var termios = stream.tty.ops.ioctl_tcgets(stream);
            var argp = syscallGetVarargP();
            HEAP32[((argp)>>2)] = termios.c_iflag || 0;
            HEAP32[(((argp)+(4))>>2)] = termios.c_oflag || 0;
            HEAP32[(((argp)+(8))>>2)] = termios.c_cflag || 0;
            HEAP32[(((argp)+(12))>>2)] = termios.c_lflag || 0;
            for (var i = 0; i < 32; i++) {
              HEAP8[(argp + i)+(17)] = termios.c_cc[i] || 0;
            }
            return 0;
          }
          return 0;
        }
        case 21510:
        case 21511:
        case 21512: {
          if (!stream.tty) return -59;
          return 0; // no-op, not actually adjusting terminal settings
        }
        case 21506:
        case 21507:
        case 21508: {
          if (!stream.tty) return -59;
          if (stream.tty.ops.ioctl_tcsets) {
            var argp = syscallGetVarargP();
            var c_iflag = HEAP32[((argp)>>2)];
            var c_oflag = HEAP32[(((argp)+(4))>>2)];
            var c_cflag = HEAP32[(((argp)+(8))>>2)];
            var c_lflag = HEAP32[(((argp)+(12))>>2)];
            var c_cc = []
            for (var i = 0; i < 32; i++) {
              c_cc.push(HEAP8[(argp + i)+(17)]);
            }
            return stream.tty.ops.ioctl_tcsets(stream.tty, op, { c_iflag, c_oflag, c_cflag, c_lflag, c_cc });
          }
          return 0; // no-op, not actually adjusting terminal settings
        }
        case 21519: {
          if (!stream.tty) return -59;
          var argp = syscallGetVarargP();
          HEAP32[((argp)>>2)] = 0;
          return 0;
        }
        case 21520: {
          if (!stream.tty) return -59;
          return -28; // not supported
        }
        case 21537:
        case 21531: {
          var argp = syscallGetVarargP();
          return FS.ioctl(stream, op, argp);
        }
        case 21523: {
          // TODO: in theory we should write to the winsize struct that gets
          // passed in, but for now musl doesn't read anything on it
          if (!stream.tty) return -59;
          if (stream.tty.ops.ioctl_tiocgwinsz) {
            var winsize = stream.tty.ops.ioctl_tiocgwinsz(stream.tty);
            var argp = syscallGetVarargP();
            HEAP16[((argp)>>1)] = winsize[0];
            HEAP16[(((argp)+(2))>>1)] = winsize[1];
          }
          return 0;
        }
        case 21524: {
          // TODO: technically, this ioctl call should change the window size.
          // but, since emscripten doesn't have any concept of a terminal window
          // yet, we'll just silently throw it away as we do TIOCGWINSZ
          if (!stream.tty) return -59;
          return 0;
        }
        case 21515: {
          if (!stream.tty) return -59;
          return 0;
        }
        default: return -28; // not supported
      }
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  }
  

  function ___syscall_lstat64(path, buf) {
  try {
  
      path = SYSCALLS.getStr(path);
      return SYSCALLS.writeStat(buf, FS.lstat(path));
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  }
  

  function ___syscall_newfstatat(dirfd, path, buf, flags) {
  try {
  
      path = SYSCALLS.getStr(path);
      var nofollow = flags & 256;
      var allowEmpty = flags & 4096;
      flags = flags & (~6400);
      assert(!flags, `unknown flags in __syscall_newfstatat: ${flags}`);
      path = SYSCALLS.calculateAt(dirfd, path, allowEmpty);
      return SYSCALLS.writeStat(buf, nofollow ? FS.lstat(path) : FS.stat(path));
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  }
  

  
  function ___syscall_openat(dirfd, path, flags, varargs) {
  SYSCALLS.varargs = varargs;
  try {
  
      path = SYSCALLS.getStr(path);
      path = SYSCALLS.calculateAt(dirfd, path);
      var mode = varargs ? syscallGetVarargI() : 0;
      if (flags & 64) {
        mode &= ~SYSCALLS.currentUmask;
      }
      return FS.open(path, flags, mode).fd;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  }
  

  function ___syscall_stat64(path, buf) {
  try {
  
      path = SYSCALLS.getStr(path);
      return SYSCALLS.writeStat(buf, FS.stat(path));
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  }
  

  var __abort_js = () =>
      abort('native code called abort()');

  var AsciiToString = (ptr) => {
      var str = '';
      while (1) {
        var ch = HEAPU8[ptr++];
        if (!ch) return str;
        str += String.fromCharCode(ch);
      }
    };
  
  var awaitingDependencies = {
  };
  
  var registeredTypes = {
  };
  
  var typeDependencies = {
  };
  
  class BindingError extends Error {
      constructor(message) {
        super(message);
        this.name = 'BindingError';
      }
    }
  var throwBindingError = (message) => { throw new BindingError(message); };
  /** @param {Object=} options */
  function sharedRegisterType(rawType, registeredInstance, options = {}) {
      var name = registeredInstance.name;
      if (!rawType) {
        throwBindingError(`type "${name}" must have a positive integer typeid pointer`);
      }
      if (registeredTypes.hasOwnProperty(rawType)) {
        if (options.ignoreDuplicateRegistrations) {
          return;
        } else {
          throwBindingError(`Cannot register type '${name}' twice`);
        }
      }
  
      registeredTypes[rawType] = registeredInstance;
      delete typeDependencies[rawType];
  
      if (awaitingDependencies.hasOwnProperty(rawType)) {
        var callbacks = awaitingDependencies[rawType];
        delete awaitingDependencies[rawType];
        callbacks.forEach((cb) => cb());
      }
    }
  /** @param {Object=} options */
  function registerType(rawType, registeredInstance, options = {}) {
      return sharedRegisterType(rawType, registeredInstance, options);
    }
  
  
  
  
  /** @type {!Uint16Array} */
  var HEAPU16;
  
  
  
  
  /** not-@type {!BigUint64Array} */
  var HEAPU64;
  var integerReadValueFromPointer = (name, width, signed) => {
      // integers are quite common, so generate very specialized functions
      switch (width) {
        case 1: return signed ?
          (pointer) => HEAP8[pointer] :
          (pointer) => HEAPU8[pointer];
        case 2: return signed ?
          (pointer) => HEAP16[((pointer)>>1)] :
          (pointer) => HEAPU16[((pointer)>>1)]
        case 4: return signed ?
          (pointer) => HEAP32[((pointer)>>2)] :
          (pointer) => HEAPU32[((pointer)>>2)]
        case 8: return signed ?
          (pointer) => HEAP64[((pointer)>>3)] :
          (pointer) => HEAPU64[((pointer)>>3)]
        default:
          throw new TypeError(`invalid integer width (${width}): ${name}`);
      }
    };
  
  var embindRepr = (v) => {
      if (v === null) {
          return 'null';
      }
      var t = typeof v;
      if (t === 'object' || t === 'array' || t === 'function') {
          return v.toString();
      } else {
          return '' + v;
      }
    };
  
  var assertIntegerRange = (typeName, value, minRange, maxRange) => {
      if (value < minRange || value > maxRange) {
        throw new TypeError(`Passing a number "${embindRepr(value)}" from JS side to C/C++ side to an argument of type "${typeName}", which is outside the valid range [${minRange}, ${maxRange}]!`);
      }
    };
  /** @suppress {globalThis} */
  var __embind_register_bigint = (primitiveType, name, size, minRange, maxRange) => {
      name = AsciiToString(name);
  
      const isUnsignedType = minRange === 0n;
  
      let fromWireType = (value) => value;
      if (isUnsignedType) {
        // uint64 get converted to int64 in ABI, fix them up like we do for 32-bit integers.
        const bitSize = size * 8;
        fromWireType = (value) => {
          return BigInt.asUintN(bitSize, value);
        }
        maxRange = fromWireType(maxRange);
      }
  
      registerType(primitiveType, {
        name,
        fromWireType: fromWireType,
        toWireType: (destructors, value) => {
          if (typeof value == 'number') {
            value = BigInt(value);
          }
          else if (typeof value != 'bigint') {
            throw new TypeError(`Cannot convert "${embindRepr(value)}" to ${name}`);
          }
          assertIntegerRange(name, value, minRange, maxRange);
          return value;
        },
        readValueFromPointer: integerReadValueFromPointer(name, size, !isUnsignedType),
        destructorFunction: null, // This type does not need a destructor
      });
    };

  
  
  /** @suppress {globalThis} */
  var __embind_register_bool = (rawType, name, trueValue, falseValue) => {
      name = AsciiToString(name);
      registerType(rawType, {
        name,
        fromWireType: function(wt) {
          // ambiguous emscripten ABI: sometimes return values are
          // true or false, and sometimes integers (0 or 1)
          return !!wt;
        },
        toWireType: function(destructors, o) {
          return o ? trueValue : falseValue;
        },
        readValueFromPointer: function(pointer) {
          return this.fromWireType(HEAPU8[pointer]);
        },
        destructorFunction: null, // This type does not need a destructor
      });
    };

  
  
  var shallowCopyInternalPointer = (o) => {
      return {
        count: o.count,
        deleteScheduled: o.deleteScheduled,
        preservePointerOnDelete: o.preservePointerOnDelete,
        ptr: o.ptr,
        ptrType: o.ptrType,
        smartPtr: o.smartPtr,
        smartPtrType: o.smartPtrType,
      };
    };
  
  var throwInstanceAlreadyDeleted = (obj) => {
      function getInstanceTypeName(handle) {
        return handle.$$.ptrType.registeredClass.name;
      }
      throwBindingError(getInstanceTypeName(obj) + ' instance already deleted');
    };
  
  var finalizationRegistry = false;
  
  var detachFinalizer = (handle) => {};
  
  var runDestructor = ($$) => {
      if ($$.smartPtr) {
        $$.smartPtrType.rawDestructor($$.smartPtr);
      } else {
        $$.ptrType.registeredClass.rawDestructor($$.ptr);
      }
    };
  var releaseClassHandle = ($$) => {
      $$.count.value -= 1;
      var toDelete = 0 === $$.count.value;
      if (toDelete) {
        runDestructor($$);
      }
    };
  
  var downcastPointer = (ptr, ptrClass, desiredClass) => {
      if (ptrClass === desiredClass) {
        return ptr;
      }
      if (undefined === desiredClass.baseClass) {
        return null; // no conversion
      }
  
      var rv = downcastPointer(ptr, ptrClass, desiredClass.baseClass);
      if (rv === null) {
        return null;
      }
      return desiredClass.downcast(rv);
    };
  
  var registeredPointers = {
  };
  
  var registeredInstances = {
  };
  
  var getBasestPointer = (class_, ptr) => {
      if (ptr === undefined) {
          throwBindingError('ptr should not be undefined');
      }
      while (class_.baseClass) {
          ptr = class_.upcast(ptr);
          class_ = class_.baseClass;
      }
      return ptr;
    };
  var getInheritedInstance = (class_, ptr) => {
      ptr = getBasestPointer(class_, ptr);
      return registeredInstances[ptr];
    };
  
  class InternalError extends Error {
      constructor(message) {
        super(message);
        this.name = 'InternalError';
      }
    }
  var throwInternalError = (message) => { throw new InternalError(message); };
  
  var makeClassHandle = (prototype, record) => {
      if (!record.ptrType || !record.ptr) {
        throwInternalError('makeClassHandle requires ptr and ptrType');
      }
      var hasSmartPtrType = !!record.smartPtrType;
      var hasSmartPtr = !!record.smartPtr;
      if (hasSmartPtrType !== hasSmartPtr) {
        throwInternalError('Both smartPtrType and smartPtr must be specified');
      }
      record.count = { value: 1 };
      return attachFinalizer(Object.create(prototype, {
        $$: {
          value: record,
          writable: true,
        },
      }));
    };
  /** @suppress {globalThis} */
  function RegisteredPointer_fromWireType(ptr) {
      // ptr is a raw pointer (or a raw smartpointer)
  
      // rawPointer is a maybe-null raw pointer
      var rawPointer = this.getPointee(ptr);
      if (!rawPointer) {
        this.destructor(ptr);
        return null;
      }
  
      var registeredInstance = getInheritedInstance(this.registeredClass, rawPointer);
      if (undefined !== registeredInstance) {
        // JS object has been neutered, time to repopulate it
        if (0 === registeredInstance.$$.count.value) {
          registeredInstance.$$.ptr = rawPointer;
          registeredInstance.$$.smartPtr = ptr;
          return registeredInstance['clone']();
        } else {
          // else, just increment reference count on existing object
          // it already has a reference to the smart pointer
          var rv = registeredInstance['clone']();
          this.destructor(ptr);
          return rv;
        }
      }
  
      function makeDefaultHandle() {
        if (this.isSmartPointer) {
          return makeClassHandle(this.registeredClass.instancePrototype, {
            ptrType: this.pointeeType,
            ptr: rawPointer,
            smartPtrType: this,
            smartPtr: ptr,
          });
        } else {
          return makeClassHandle(this.registeredClass.instancePrototype, {
            ptrType: this,
            ptr,
          });
        }
      }
  
      var actualType = this.registeredClass.getActualType(rawPointer);
      var registeredPointerRecord = registeredPointers[actualType];
      if (!registeredPointerRecord) {
        return makeDefaultHandle.call(this);
      }
  
      var toType;
      if (this.isConst) {
        toType = registeredPointerRecord.constPointerType;
      } else {
        toType = registeredPointerRecord.pointerType;
      }
      var dp = downcastPointer(
          rawPointer,
          this.registeredClass,
          toType.registeredClass);
      if (dp === null) {
        return makeDefaultHandle.call(this);
      }
      if (this.isSmartPointer) {
        return makeClassHandle(toType.registeredClass.instancePrototype, {
          ptrType: toType,
          ptr: dp,
          smartPtrType: this,
          smartPtr: ptr,
        });
      } else {
        return makeClassHandle(toType.registeredClass.instancePrototype, {
          ptrType: toType,
          ptr: dp,
        });
      }
    }
  var attachFinalizer = (handle) => {
      if (!globalThis.FinalizationRegistry) {
        attachFinalizer = (handle) => handle;
        return handle;
      }
      // If the running environment has a FinalizationRegistry (see
      // https://github.com/tc39/proposal-weakrefs), then attach finalizers
      // for class handles.  We check for the presence of FinalizationRegistry
      // at run-time, not build-time.
      finalizationRegistry = new FinalizationRegistry((info) => {
        console.warn(info.leakWarning);
        releaseClassHandle(info.$$);
      });
      attachFinalizer = (handle) => {
        var $$ = handle.$$;
        var hasSmartPtr = !!$$.smartPtr;
        if (hasSmartPtr) {
          // We should not call the destructor on raw pointers in case other code expects the pointee to live
          var info = { $$: $$ };
          // Create a warning as an Error instance in advance so that we can store
          // the current stacktrace and point to it when / if a leak is detected.
          // This is more useful than the empty stacktrace of `FinalizationRegistry`
          // callback.
          var cls = $$.ptrType.registeredClass;
          var err = new Error(`Embind found a leaked C++ instance ${cls.name} <${ptrToString($$.ptr)}>.
We'll free it automatically in this case, but this functionality is not reliable across various environments.
Make sure to invoke .delete() manually once you're done with the instance instead.
Originally allocated`); // `.stack` will add "at ..." after this sentence
          if ('captureStackTrace' in Error) {
            Error.captureStackTrace(err, RegisteredPointer_fromWireType);
          }
          info.leakWarning = err.stack.replace(/^Error: /, '');
          finalizationRegistry.register(handle, info, handle);
        }
        return handle;
      };
      detachFinalizer = (handle) => finalizationRegistry.unregister(handle);
      return attachFinalizer(handle);
    };
  
  
  
  
  var deletionQueue = [];
  var flushPendingDeletes = () => {
      while (deletionQueue.length) {
        var obj = deletionQueue.pop();
        obj.$$.deleteScheduled = false;
        obj['delete']();
      }
    };
  
  var delayFunction;
  var init_ClassHandle = () => {
      let proto = ClassHandle.prototype;
  
      Object.assign(proto, {
        'isAliasOf'(other) {
          if (!(this instanceof ClassHandle)) {
            return false;
          }
          if (!(other instanceof ClassHandle)) {
            return false;
          }
  
          var leftClass = this.$$.ptrType.registeredClass;
          var left = this.$$.ptr;
          other.$$ = /** @type {Object} */ (other.$$);
          var rightClass = other.$$.ptrType.registeredClass;
          var right = other.$$.ptr;
  
          while (leftClass.baseClass) {
            left = leftClass.upcast(left);
            leftClass = leftClass.baseClass;
          }
  
          while (rightClass.baseClass) {
            right = rightClass.upcast(right);
            rightClass = rightClass.baseClass;
          }
  
          return leftClass === rightClass && left === right;
        },
  
        'clone'() {
          if (!this.$$.ptr) {
            throwInstanceAlreadyDeleted(this);
          }
  
          if (this.$$.preservePointerOnDelete) {
            this.$$.count.value += 1;
            return this;
          } else {
            var clone = attachFinalizer(Object.create(Object.getPrototypeOf(this), {
              $$: {
                value: shallowCopyInternalPointer(this.$$),
              }
            }));
  
            clone.$$.count.value += 1;
            clone.$$.deleteScheduled = false;
            return clone;
          }
        },
  
        'delete'() {
          if (!this.$$.ptr) {
            throwInstanceAlreadyDeleted(this);
          }
  
          if (this.$$.deleteScheduled && !this.$$.preservePointerOnDelete) {
            throwBindingError('Object already scheduled for deletion');
          }
  
          detachFinalizer(this);
          releaseClassHandle(this.$$);
  
          if (!this.$$.preservePointerOnDelete) {
            this.$$.smartPtr = undefined;
            this.$$.ptr = undefined;
          }
        },
  
        'isDeleted'() {
          return !this.$$.ptr;
        },
  
        'deleteLater'() {
          if (!this.$$.ptr) {
            throwInstanceAlreadyDeleted(this);
          }
          if (this.$$.deleteScheduled && !this.$$.preservePointerOnDelete) {
            throwBindingError('Object already scheduled for deletion');
          }
          deletionQueue.push(this);
          if (deletionQueue.length === 1 && delayFunction) {
            delayFunction(flushPendingDeletes);
          }
          this.$$.deleteScheduled = true;
          return this;
        },
      });
  
      // Support `using ...` from https://github.com/tc39/proposal-explicit-resource-management.
      const symbolDispose = Symbol.dispose;
      if (symbolDispose) {
        proto[symbolDispose] = proto['delete'];
      }
    };
  /** @constructor */
  function ClassHandle() {
    }
  
  var createNamedFunction = (name, func) => Object.defineProperty(func, 'name', { value: name });
  
  
  var ensureOverloadTable = (proto, methodName, humanName) => {
      if (undefined === proto[methodName].overloadTable) {
        var prevFunc = proto[methodName];
        // Inject an overload resolver function that routes to the appropriate overload based on the number of arguments.
        proto[methodName] = function(...args) {
          // TODO This check can be removed in -O3 level "unsafe" optimizations.
          if (!proto[methodName].overloadTable.hasOwnProperty(args.length)) {
            throwBindingError(`Function '${humanName}' called with an invalid number of arguments (${args.length}) - expects one of (${proto[methodName].overloadTable})!`);
          }
          return proto[methodName].overloadTable[args.length].apply(this, args);
        };
        // Move the previous function into the overload table.
        proto[methodName].overloadTable = [];
        proto[methodName].overloadTable[prevFunc.argCount] = prevFunc;
      }
    };
  
  /** @param {number=} numArguments */
  var exposePublicSymbol = (name, value, numArguments) => {
      if (Module.hasOwnProperty(name)) {
        if (undefined === numArguments || (undefined !== Module[name].overloadTable && undefined !== Module[name].overloadTable[numArguments])) {
          throwBindingError(`Cannot register public name '${name}' twice`);
        }
  
        // We are exposing a function with the same name as an existing function. Create an overload table and a function selector
        // that routes between the two.
        ensureOverloadTable(Module, name, name);
        if (Module[name].overloadTable.hasOwnProperty(numArguments)) {
          throwBindingError(`Cannot register multiple overloads of a function with the same number of arguments (${numArguments})!`);
        }
        // Add the new function into the overload table.
        Module[name].overloadTable[numArguments] = value;
      } else {
        Module[name] = value;
        Module[name].argCount = numArguments;
      }
    };
  
  var char_0 = 48;
  
  var char_9 = 57;
  var makeLegalFunctionName = (name) => {
      assert(typeof name === 'string');
      name = name.replace(/[^a-zA-Z0-9_]/g, '$');
      var f = name.charCodeAt(0);
      if (f >= char_0 && f <= char_9) {
        return `_${name}`;
      }
      return name;
    };
  
  
  /** @constructor */
  function RegisteredClass(name,
                               constructor,
                               instancePrototype,
                               rawDestructor,
                               baseClass,
                               getActualType,
                               upcast,
                               downcast) {
      this.name = name;
      this.constructor = constructor;
      this.instancePrototype = instancePrototype;
      this.rawDestructor = rawDestructor;
      this.baseClass = baseClass;
      this.getActualType = getActualType;
      this.upcast = upcast;
      this.downcast = downcast;
      this.pureVirtualFunctions = [];
    }
  
  
  var upcastPointer = (ptr, ptrClass, desiredClass) => {
      while (ptrClass !== desiredClass) {
        if (!ptrClass.upcast) {
          throwBindingError(`Expected null or instance of ${desiredClass.name}, got an instance of ${ptrClass.name}`);
        }
        ptr = ptrClass.upcast(ptr);
        ptrClass = ptrClass.baseClass;
      }
      return ptr;
    };
  
  /** @suppress {globalThis} */
  function constNoSmartPtrRawPointerToWireType(destructors, handle) {
      if (handle === null) {
        if (this.isReference) {
          throwBindingError(`null is not a valid ${this.name}`);
        }
        return 0;
      }
  
      if (!handle.$$) {
        throwBindingError(`Cannot pass "${embindRepr(handle)}" as a ${this.name}`);
      }
      if (!handle.$$.ptr) {
        throwBindingError(`Cannot pass deleted object as a pointer of type ${this.name}`);
      }
      var handleClass = handle.$$.ptrType.registeredClass;
      var ptr = upcastPointer(handle.$$.ptr, handleClass, this.registeredClass);
      return ptr;
    }
  
  
  /** @suppress {globalThis} */
  function genericPointerToWireType(destructors, handle) {
      var ptr;
      if (handle === null) {
        if (this.isReference) {
          throwBindingError(`null is not a valid ${this.name}`);
        }
  
        if (this.isSmartPointer) {
          ptr = this.rawConstructor();
          if (destructors !== null) {
            destructors.push(this.rawDestructor, ptr);
          }
          return ptr;
        } else {
          return 0;
        }
      }
  
      if (!handle || !handle.$$) {
        throwBindingError(`Cannot pass "${embindRepr(handle)}" as a ${this.name}`);
      }
      if (!handle.$$.ptr) {
        throwBindingError(`Cannot pass deleted object as a pointer of type ${this.name}`);
      }
      if (!this.isConst && handle.$$.ptrType.isConst) {
        throwBindingError(`Cannot convert argument of type ${(handle.$$.smartPtrType ? handle.$$.smartPtrType.name : handle.$$.ptrType.name)} to parameter type ${this.name}`);
      }
      var handleClass = handle.$$.ptrType.registeredClass;
      ptr = upcastPointer(handle.$$.ptr, handleClass, this.registeredClass);
  
      if (this.isSmartPointer) {
        // TODO: this is not strictly true
        // We could support BY_EMVAL conversions from raw pointers to smart pointers
        // because the smart pointer can hold a reference to the handle
        if (undefined === handle.$$.smartPtr) {
          throwBindingError('Passing raw pointer to smart pointer is illegal');
        }
  
        switch (this.sharingPolicy) {
          case 0: // NONE
            // no upcasting
            if (handle.$$.smartPtrType === this) {
              ptr = handle.$$.smartPtr;
            } else {
              throwBindingError(`Cannot convert argument of type ${(handle.$$.smartPtrType ? handle.$$.smartPtrType.name : handle.$$.ptrType.name)} to parameter type ${this.name}`);
            }
            break;
  
          case 1: // INTRUSIVE
            ptr = handle.$$.smartPtr;
            break;
  
          case 2: // BY_EMVAL
            if (handle.$$.smartPtrType === this) {
              ptr = handle.$$.smartPtr;
            } else {
              var clonedHandle = handle['clone']();
              ptr = this.rawShare(
                ptr,
                Emval.toHandle(() => clonedHandle['delete']())
              );
              if (destructors !== null) {
                destructors.push(this.rawDestructor, ptr);
              }
            }
            break;
  
          default:
            throwBindingError('Unsupported sharing policy');
        }
      }
      return ptr;
    }
  
  
  
  /** @suppress {globalThis} */
  function nonConstNoSmartPtrRawPointerToWireType(destructors, handle) {
      if (handle === null) {
        if (this.isReference) {
          throwBindingError(`null is not a valid ${this.name}`);
        }
        return 0;
      }
  
      if (!handle.$$) {
        throwBindingError(`Cannot pass "${embindRepr(handle)}" as a ${this.name}`);
      }
      if (!handle.$$.ptr) {
        throwBindingError(`Cannot pass deleted object as a pointer of type ${this.name}`);
      }
      if (handle.$$.ptrType.isConst) {
        throwBindingError(`Cannot convert argument of type ${handle.$$.ptrType.name} to parameter type ${this.name}`);
      }
      var handleClass = handle.$$.ptrType.registeredClass;
      var ptr = upcastPointer(handle.$$.ptr, handleClass, this.registeredClass);
      return ptr;
    }
  
  
  /** @suppress {globalThis} */
  function readPointer(pointer) {
      return this.fromWireType(HEAPU32[((pointer)>>2)]);
    }
  
  var init_RegisteredPointer = () => {
      Object.assign(RegisteredPointer.prototype, {
        getPointee(ptr) {
          if (this.rawGetPointee) {
            ptr = this.rawGetPointee(ptr);
          }
          return ptr;
        },
        destructor(ptr) {
          this.rawDestructor?.(ptr);
        },
        readValueFromPointer: readPointer,
        fromWireType: RegisteredPointer_fromWireType,
      });
    };
  /** @constructor
    @param {*=} pointeeType,
    @param {*=} sharingPolicy,
    @param {*=} rawGetPointee,
    @param {*=} rawConstructor,
    @param {*=} rawShare,
    @param {*=} rawDestructor,
     */
  function RegisteredPointer(
      name,
      registeredClass,
      isReference,
      isConst,
  
      // smart pointer properties
      isSmartPointer,
      pointeeType,
      sharingPolicy,
      rawGetPointee,
      rawConstructor,
      rawShare,
      rawDestructor
    ) {
      this.name = name;
      this.registeredClass = registeredClass;
      this.isReference = isReference;
      this.isConst = isConst;
  
      // smart pointer properties
      this.isSmartPointer = isSmartPointer;
      this.pointeeType = pointeeType;
      this.sharingPolicy = sharingPolicy;
      this.rawGetPointee = rawGetPointee;
      this.rawConstructor = rawConstructor;
      this.rawShare = rawShare;
      this.rawDestructor = rawDestructor;
  
      if (!isSmartPointer && registeredClass.baseClass === undefined) {
        if (isConst) {
          this.toWireType = constNoSmartPtrRawPointerToWireType;
          this.destructorFunction = null;
        } else {
          this.toWireType = nonConstNoSmartPtrRawPointerToWireType;
          this.destructorFunction = null;
        }
      } else {
        this.toWireType = genericPointerToWireType;
        // Here we must leave this.destructorFunction undefined, since whether genericPointerToWireType returns
        // a pointer that needs to be freed up is runtime-dependent, and cannot be evaluated at registration time.
        // TODO: Create an alternative mechanism that allows removing the use of var destructors = []; array in
        //       craftInvokerFunction altogether.
      }
    }
  
  /** @param {number=} numArguments */
  var replacePublicSymbol = (name, value, numArguments) => {
      if (!Module.hasOwnProperty(name)) {
        throwInternalError('Replacing nonexistent public symbol');
      }
      // If there's an overload table for this symbol, replace the symbol in the overload table instead.
      if (undefined !== Module[name].overloadTable && undefined !== numArguments) {
        Module[name].overloadTable[numArguments] = value;
      } else {
        Module[name] = value;
        Module[name].argCount = numArguments;
      }
    };
  
  
  
  var getDynCaller = (sig, ptr, promising = false) => {
      return (...args) => dynCall(sig, ptr, args, promising);
    };
  
  var embind__requireFunction = (signature, rawFunction, isAsync = false) => {
      assert(!isAsync, 'async bindings are only supported with JSPI');
  
      signature = AsciiToString(signature);
  
      function makeDynCaller() {
        return getDynCaller(signature, rawFunction);
      }
  
      var fp = makeDynCaller();
      if (typeof fp != 'function') {
          throwBindingError(`unknown function pointer with signature ${signature}: ${rawFunction}`);
      }
      return fp;
    };
  
  
  
  class UnboundTypeError extends Error {}
  
  
  
  var getTypeName = (type) => {
      var ptr = ___getTypeName(type);
      var rv = AsciiToString(ptr);
      _free(ptr);
      return rv;
    };
  var throwUnboundTypeError = (message, types) => {
      var unboundTypes = [];
      var seen = {};
      function visit(type) {
        if (seen[type]) {
          return;
        }
        if (registeredTypes[type]) {
          return;
        }
        if (typeDependencies[type]) {
          typeDependencies[type].forEach(visit);
          return;
        }
        unboundTypes.push(type);
        seen[type] = true;
      }
      types.forEach(visit);
  
      throw new UnboundTypeError(`${message}: ` + unboundTypes.map(getTypeName).join([', ']));
    };
  
  
  
  
  var whenDependentTypesAreResolved = (myTypes, dependentTypes, getTypeConverters) => {
      myTypes.forEach((type) => typeDependencies[type] = dependentTypes);
  
      function onComplete(typeConverters) {
        var myTypeConverters = getTypeConverters(typeConverters);
        if (myTypeConverters.length !== myTypes.length) {
          throwInternalError('Mismatched type converter count');
        }
        for (var i = 0; i < myTypes.length; ++i) {
          registerType(myTypes[i], myTypeConverters[i]);
        }
      }
  
      var typeConverters = new Array(dependentTypes.length);
      var unregisteredTypes = [];
      var registered = 0;
      for (let [i, dt] of dependentTypes.entries()) {
        if (registeredTypes.hasOwnProperty(dt)) {
          typeConverters[i] = registeredTypes[dt];
        } else {
          unregisteredTypes.push(dt);
          if (!awaitingDependencies.hasOwnProperty(dt)) {
            awaitingDependencies[dt] = [];
          }
          awaitingDependencies[dt].push(() => {
            typeConverters[i] = registeredTypes[dt];
            ++registered;
            if (registered === unregisteredTypes.length) {
              onComplete(typeConverters);
            }
          });
        }
      }
      if (0 === unregisteredTypes.length) {
        onComplete(typeConverters);
      }
    };
  var __embind_register_class = (rawType,
                             rawPointerType,
                             rawConstPointerType,
                             baseClassRawType,
                             getActualTypeSignature,
                             getActualType,
                             upcastSignature,
                             upcast,
                             downcastSignature,
                             downcast,
                             name,
                             destructorSignature,
                             rawDestructor) => {
      name = AsciiToString(name);
      getActualType = embind__requireFunction(getActualTypeSignature, getActualType);
      upcast &&= embind__requireFunction(upcastSignature, upcast);
      downcast &&= embind__requireFunction(downcastSignature, downcast);
      rawDestructor = embind__requireFunction(destructorSignature, rawDestructor);
      var legalFunctionName = makeLegalFunctionName(name);
  
      exposePublicSymbol(legalFunctionName, function() {
        // this code cannot run if baseClassRawType is zero
        throwUnboundTypeError(`Cannot construct ${name} due to unbound types`, [baseClassRawType]);
      });
  
      whenDependentTypesAreResolved(
        [rawType, rawPointerType, rawConstPointerType],
        baseClassRawType ? [baseClassRawType] : [],
        (base) => {
          base = base[0];
  
          var baseClass;
          var basePrototype;
          if (baseClassRawType) {
            baseClass = base.registeredClass;
            basePrototype = baseClass.instancePrototype;
          } else {
            basePrototype = ClassHandle.prototype;
          }
  
          var constructor = createNamedFunction(name, function(...args) {
            if (Object.getPrototypeOf(this) !== instancePrototype) {
              throw new BindingError(`Use 'new' to construct ${name}`);
            }
            if (undefined === registeredClass.constructor_body) {
              throw new BindingError(`${name} has no accessible constructor`);
            }
            var body = registeredClass.constructor_body[args.length];
            if (undefined === body) {
              throw new BindingError(`Tried to invoke ctor of ${name} with invalid number of parameters (${args.length}) - expected (${Object.keys(registeredClass.constructor_body).toString()}) parameters instead!`);
            }
            return body.apply(this, args);
          });
  
          var instancePrototype = Object.create(basePrototype, {
            constructor: { value: constructor },
          });
  
          constructor.prototype = instancePrototype;
  
          var registeredClass = new RegisteredClass(name,
                                                    constructor,
                                                    instancePrototype,
                                                    rawDestructor,
                                                    baseClass,
                                                    getActualType,
                                                    upcast,
                                                    downcast);
  
          if (registeredClass.baseClass) {
            // Keep track of class hierarchy. Used to allow sub-classes to inherit class functions.
            registeredClass.baseClass.__derivedClasses ??= [];
  
            registeredClass.baseClass.__derivedClasses.push(registeredClass);
          }
  
          var referenceConverter = new RegisteredPointer(name,
                                                         registeredClass,
                                                         true,
                                                         false,
                                                         false);
  
          var pointerConverter = new RegisteredPointer(name + '*',
                                                       registeredClass,
                                                       false,
                                                       false,
                                                       false);
  
          var constPointerConverter = new RegisteredPointer(name + ' const*',
                                                            registeredClass,
                                                            false,
                                                            true,
                                                            false);
  
          registeredPointers[rawType] = {
            pointerType: pointerConverter,
            constPointerType: constPointerConverter
          };
  
          replacePublicSymbol(legalFunctionName, constructor);
  
          return [referenceConverter, pointerConverter, constPointerConverter];
        }
      );
    };

  var heap32VectorToArray = (count, firstElement) => {
      var array = [];
      for (var i = 0; i < count; i++) {
        // TODO(https://github.com/emscripten-core/emscripten/issues/17310):
        // Find a way to hoist the `>> 2` or `>> 3` out of this loop.
        array.push(HEAPU32[(((firstElement)+(i * 4))>>2)]);
      }
      return array;
    };
  
  
  
  
  var runDestructors = (destructors) => {
      while (destructors.length) {
        var ptr = destructors.pop();
        var del = destructors.pop();
        del(ptr);
      }
    };
  
  
  function usesDestructorStack(argTypes) {
      // Skip return value at index 0 - it's not deleted here.
      for (var i = 1; i < argTypes.length; ++i) {
        // The type does not define a destructor function - must use dynamic stack
        if (argTypes[i] !== null && argTypes[i].destructorFunction === undefined) {
          return true;
        }
      }
      return false;
    }
  
  function argsUseStackAlloc(argTypes) {
      // Skip return value at index 0 - only arguments stack-allocate.
      for (var i = 1; i < argTypes.length; ++i) {
        if (argTypes[i] !== null && argTypes[i].argStackAlloc) {
          return true;
        }
      }
      return false;
    }
  
  
  
  
  
  function checkArgCount(numArgs, minArgs, maxArgs, humanName, throwBindingError) {
      if (numArgs < minArgs || numArgs > maxArgs) {
        var argCountMessage = minArgs == maxArgs ? minArgs : `${minArgs} to ${maxArgs}`;
        throwBindingError(`function ${humanName} called with ${numArgs} arguments, expected ${argCountMessage}`);
      }
    }
  function createJsInvoker(argTypes, isClassMethodFunc, returns, isAsync) {
      var needsDestructorStack = usesDestructorStack(argTypes);
      var argsNeedStack = argsUseStackAlloc(argTypes);
      // Any call may suspend under Asyncify, and destructors run deferred in
      // onDone, after a stack frame would already be gone.
      var useStackFrame = false;
      if (argsNeedStack && !useStackFrame) {
        // A stack-allocating type must never see a null destructors argument
        // without a bracketing frame; route it through the destructors array
        // (it heap-allocates on that path).
        needsDestructorStack = true;
      }
      var argCount = argTypes.length - 2;
      var argsList = [];
      var argsListWired = ['fn'];
      if (isClassMethodFunc) {
        argsListWired.push('thisWired');
      }
      for (var i = 0; i < argCount; ++i) {
        argsList.push(`arg${i}`)
        argsListWired.push(`arg${i}Wired`)
      }
      argsList = argsList.join()
      argsListWired = argsListWired.join()
  
      var invokerFnBody = `return function (${argsList}) {\n`;
  
      invokerFnBody += 'checkArgCount(arguments.length, minArgs, maxArgs, humanName, throwBindingError);\n';
  
      if (needsDestructorStack) {
        invokerFnBody += 'var destructors = [];\n';
      }
      if (useStackFrame) {
        // The frame must be released on every completion, including a throwing
        // argument conversion or callee: a skipped stackRestore permanently
        // leaks wasm stack. `var` declarations hoist out of the try block.
        invokerFnBody += 'var sp = stackSave();\ntry {\n';
      }
  
      var dtorStack = needsDestructorStack ? 'destructors' : 'null';
      var args1 = ['humanName', 'throwBindingError', 'invoker', 'fn', 'runDestructors', 'fromRetWire', 'toClassParamWire'];
      if (useStackFrame) {
        args1.push('stackSave', 'stackRestore');
      }
  
      if (isClassMethodFunc) {
        invokerFnBody += `var thisWired = toClassParamWire(${dtorStack}, this);\n`;
      }
  
      for (var i = 0; i < argCount; ++i) {
        var argName = `toArg${i}Wire`;
        invokerFnBody += `var arg${i}Wired = ${argName}(${dtorStack}, arg${i});\n`;
        args1.push(argName);
      }
  
      invokerFnBody += (returns || isAsync ? 'var rv = ' : '') + `invoker(${argsListWired});\n`;
      if (useStackFrame) {
        // The callee has consumed the stack-allocated argument temporaries;
        // release the frame before any post-call work.
        invokerFnBody += '} finally {\nstackRestore(sp);\n}\n';
      }
  
      var returnVal = returns ? 'rv' : '';
      args1.push('Asyncify');
      invokerFnBody += `function onDone(${returnVal}) {\n`;
  
      if (needsDestructorStack) {
        invokerFnBody += 'runDestructors(destructors);\n';
      } else {
        for (var i = isClassMethodFunc?1:2; i < argTypes.length; ++i) { // Skip return value at index 0 - it's not deleted here. Also skip class type if not a method.
          var paramName = (i === 1 ? 'thisWired' : `arg${i - 2}Wired`);
          if (argTypes[i].destructorFunction !== null) {
            invokerFnBody += `${paramName}_dtor(${paramName});\n`;
            args1.push(`${paramName}_dtor`);
          }
        }
      }
  
      if (returns) {
        invokerFnBody += 'var ret = fromRetWire(rv);\n' +
                         'return ret;\n';
      } else {
      }
  
      invokerFnBody += '}\n';
      invokerFnBody += `return Asyncify.currData ? Asyncify.whenDone().then(onDone) : onDone(${returnVal});\n`
  
      invokerFnBody += '}\n';
  
      args1.push('checkArgCount', 'minArgs', 'maxArgs');
      invokerFnBody = `if (arguments.length !== ${args1.length}){ throw new Error(humanName + "Expected ${args1.length} closure arguments " + arguments.length + " given."); }\n${invokerFnBody}`;
      return new Function(args1, invokerFnBody);
    }
  
  var runAndAbortIfError = (func) => {
      try {
        return func();
      } catch (e) {
        abort(e);
      }
    };
  
  var handleException = (e) => {
      // Certain exception types we do not treat as errors since they are used for
      // internal control flow.
      // 1. ExitStatus, which is thrown by exit()
      // 2. "unwind", which is thrown by emscripten_unwind_to_js_event_loop() and others
      //    that wish to return to JS event loop.
      if (e instanceof ExitStatus || e == 'unwind') {
        return EXITSTATUS;
      }
      checkStackCookie();
      if (e instanceof WebAssembly.RuntimeError) {
        if (_emscripten_stack_get_current() <= 0) {
          err('Stack overflow detected.  You can try increasing -sSTACK_SIZE (currently set to 65536)');
        }
      }
      quit_(1, e);
    };
  
  
  var runtimeKeepaliveCounter = 0;
  var keepRuntimeAlive = () => noExitRuntime || runtimeKeepaliveCounter > 0;
  var _proc_exit = (code) => {
      EXITSTATUS = code;
      if (!keepRuntimeAlive()) {
        Module['onExit']?.(code);
        ABORT = true;
      }
      quit_(code, new ExitStatus(code));
    };
  
  
  /** @param {boolean|number=} implicit */
  var exitJS = (status, implicit) => {
      EXITSTATUS = status;
  
      checkUnflushedContent();
  
      // if exit() was called explicitly, warn the user if the runtime isn't actually being shut down
      if (keepRuntimeAlive() && !implicit) {
        var msg = `program exited (with status: ${status}), but keepRuntimeAlive() is set (counter=${runtimeKeepaliveCounter}) due to an async operation, so halting execution but not exiting the runtime or preventing further async execution (you can use emscripten_force_exit, if you want to force a true shutdown)`;
        err(msg);
      }
  
      _proc_exit(status);
    };
  var _exit = exitJS;
  
  
  var maybeExit = () => {
      if (!keepRuntimeAlive()) {
        try {
          _exit(EXITSTATUS);
        } catch (e) {
          handleException(e);
        }
      }
    };
  var callUserCallback = (func) => {
      if (ABORT) {
        err('user callback triggered after runtime exited or application aborted.  Ignoring.');
        return;
      }
      try {
        return func();
      } catch (e) {
        handleException(e);
      } finally {
        maybeExit();
      }
    };
  
  
  var runtimeKeepalivePush = () => {
      runtimeKeepaliveCounter += 1;
    };
  
  var runtimeKeepalivePop = () => {
      assert(runtimeKeepaliveCounter > 0);
      runtimeKeepaliveCounter -= 1;
    };
  
  
  
  
  var Asyncify = {
  instrumentWasmImports(imports) {
        var importPattern = /^(invoke_.*|__asyncjs__.*)$/;
  
        for (let [x, original] of Object.entries(imports)) {
          if (typeof original == 'function') {
            let isAsyncifyImport = original.isAsync || importPattern.test(x);
            imports[x] = (...args) => {
              var originalAsyncifyState = Asyncify.state;
              try {
                return original(...args);
              } finally {
                // Only asyncify-declared imports are allowed to change the
                // state.
                // Changing the state from normal to disabled is allowed (in any
                // function) as that is what shutdown does (and we don't have an
                // explicit list of shutdown imports).
                var changedToDisabled =
                      originalAsyncifyState === Asyncify.State.Normal &&
                      Asyncify.state        === Asyncify.State.Disabled;
                // invoke_* functions are allowed to change the state if we do
                // not ignore indirect calls.
                var ignoredInvoke = x.startsWith('invoke_') &&
                                    true;
                if (Asyncify.state !== originalAsyncifyState &&
                    !isAsyncifyImport &&
                    !changedToDisabled &&
                    !ignoredInvoke) {
                  abort(`import ${x} was not in ASYNCIFY_IMPORTS, but changed the state`);
                }
              }
            };
          }
        }
      },
  instrumentFunction(original) {
        var wrapper = (...args) => {
          Asyncify.exportCallStack.push(original);
          try {
            return original(...args);
          } finally {
            if (!ABORT) {
              var top = Asyncify.exportCallStack.pop();
              assert(top === original);
              Asyncify.maybeStopUnwind();
            }
          }
        };
        Asyncify.funcWrappers.set(original, wrapper);
        wrapper = createNamedFunction(`__asyncify_wrapper_${original.name}`, wrapper);
        return wrapper;
      },
  instrumentWasmExports(exports) {
        var ret = {};
        for (let [x, original] of Object.entries(exports)) {
          if (typeof original == 'function') {
            var wrapper = Asyncify.instrumentFunction(original);
            ret[x] = wrapper;
          } else {
            ret[x] = original;
          }
        }
        return ret;
      },
  State:{
  Normal:0,
  Unwinding:1,
  Rewinding:2,
  Disabled:3,
  },
  state:0,
  StackSize:65536,
  currData:null,
  handleSleepReturnValue:0,
  exportCallStack:[],
  callstackFuncToId:new Map,
  callStackIdToFunc:new Map,
  funcWrappers:new Map,
  callStackId:0,
  asyncPromiseHandlers:null,
  sleepCallbacks:[],
  getCallStackId(func) {
        assert(func);
        if (!Asyncify.callstackFuncToId.has(func)) {
          var id = Asyncify.callStackId++;
          Asyncify.callstackFuncToId.set(func, id);
          Asyncify.callStackIdToFunc.set(id, func);
        }
        return Asyncify.callstackFuncToId.get(func);
      },
  maybeStopUnwind() {
        if (Asyncify.currData &&
            Asyncify.state === Asyncify.State.Unwinding &&
            !Asyncify.exportCallStack.length) {
          // We just finished unwinding.
          // Be sure to set the state before calling any other functions to avoid
          // possible infinite recursion here (For example in debug pthread builds
          // the dbg() function itself can call back into WebAssembly to get the
          // current pthread_self() pointer).
          Asyncify.state = Asyncify.State.Normal;
          
          // Keep the runtime alive so that a re-wind can be done later.
          runAndAbortIfError(_asyncify_stop_unwind);
          if (typeof Fibers != 'undefined') {
            Fibers.trampoline();
          }
        }
      },
  whenDone() {
        assert(Asyncify.currData, 'tried to wait for an async operation when none is in progress');
        assert(!Asyncify.asyncPromiseHandlers, 'cannot have multiple async operations in flight at once');
        return new Promise((resolve, reject) => {
          Asyncify.asyncPromiseHandlers = { resolve, reject };
        });
      },
  allocateData() {
        // An asyncify data structure has three fields:
        //  0  current stack pos
        //  4  max stack pos
        //  8  id of function at bottom of the call stack (callStackIdToFunc[id] == wasm func)
        //
        // The Asyncify ABI only interprets the first two fields, the rest is for the runtime.
        // We also embed a stack in the same memory region here, right next to the structure.
        // This struct is also defined as asyncify_data_t in emscripten/fiber.h
        var ptr = _malloc(12 + Asyncify.StackSize);
        Asyncify.setDataHeader(ptr, ptr + 12, Asyncify.StackSize);
        Asyncify.setDataRewindFunc(ptr);
        return ptr;
      },
  setDataHeader(ptr, stack, stackSize) {
        HEAPU32[((ptr)>>2)] = stack;
        HEAPU32[(((ptr)+(4))>>2)] = stack + stackSize;
      },
  setDataRewindFunc(ptr) {
        var bottomOfCallStack = Asyncify.exportCallStack[0];
        assert(bottomOfCallStack, 'exportCallStack is empty');
        var rewindId = Asyncify.getCallStackId(bottomOfCallStack);
        HEAP32[(((ptr)+(8))>>2)] = rewindId;
      },
  getDataRewindFunc(ptr) {
        var id = HEAP32[(((ptr)+(8))>>2)];
        var func = Asyncify.callStackIdToFunc.get(id);
        assert(func, `id ${id} not found in callStackIdToFunc`);
        return func;
      },
  doRewind(ptr) {
        var original = Asyncify.getDataRewindFunc(ptr);
        var func = Asyncify.funcWrappers.get(original);
        assert(original);
        assert(func);
        // Once we have rewound and the stack we no longer need to artificially
        // keep the runtime alive.
        
        return callUserCallback(func);
      },
  handleSleep(startAsync) {
        assert(Asyncify.state !== Asyncify.State.Disabled, 'handleSleep called after Asyncify was shut down');
        if (ABORT) return;
        if (Asyncify.state === Asyncify.State.Normal) {
          // Prepare to sleep. Call startAsync, and see what happens:
          // if the code decided to call our callback synchronously,
          // then no async operation was in fact begun, and we don't
          // need to do anything.
          var reachedCallback = false;
          var reachedAfterCallback = false;
          startAsync((handleSleepReturnValue = 0) => {
            // old emterpretify API supported other stuff
            assert(['undefined', 'number', 'boolean', 'bigint'].includes(typeof handleSleepReturnValue), `invalid type for handleSleepReturnValue: '${typeof handleSleepReturnValue}'`);
            if (ABORT) return;
            Asyncify.handleSleepReturnValue = handleSleepReturnValue;
            reachedCallback = true;
            if (!reachedAfterCallback) {
              // We are happening synchronously, so no need for async.
              return;
            }
            // This async operation did not happen synchronously, so we did
            // unwind. In that case there can be no compiled code on the stack,
            // as it might break later operations (we can rewind ok now, but if
            // we unwind again, we would unwind through the extra compiled code
            // too).
            assert(!Asyncify.exportCallStack.length, 'waking up (starting to rewind) must be done from JS, without compiled code on the stack');
            Asyncify.state = Asyncify.State.Rewinding;
            runAndAbortIfError(() => _asyncify_start_rewind(Asyncify.currData));
            if (typeof MainLoop != 'undefined' && MainLoop.func) {
              MainLoop.resume();
            }
            var asyncWasmReturnValue, isError = false;
            try {
              asyncWasmReturnValue = Asyncify.doRewind(Asyncify.currData);
            } catch (err) {
              asyncWasmReturnValue = err;
              isError = true;
            }
            // Track whether the return value was handled by any promise handlers.
            var handled = false;
            if (!Asyncify.currData) {
              // All asynchronous execution has finished.
              // `asyncWasmReturnValue` now contains the final
              // return value of the exported async WASM function.
              //
              // Note: `asyncWasmReturnValue` is distinct from
              // `Asyncify.handleSleepReturnValue`.
              // `Asyncify.handleSleepReturnValue` contains the return
              // value of the last C function to have executed
              // `Asyncify.handleSleep()`, whereas `asyncWasmReturnValue`
              // contains the return value of the exported WASM function
              // that may have called C functions that
              // call `Asyncify.handleSleep()`.
              var asyncPromiseHandlers = Asyncify.asyncPromiseHandlers;
              if (asyncPromiseHandlers) {
                Asyncify.asyncPromiseHandlers = null;
                (isError ? asyncPromiseHandlers.reject : asyncPromiseHandlers.resolve)(asyncWasmReturnValue);
                handled = true;
              }
            }
            if (isError && !handled) {
              // If there was an error and it was not handled by now, we have no choice but to
              // rethrow that error into the global scope where it can be caught only by
              // `onerror` or `onunhandledpromiserejection`.
              throw asyncWasmReturnValue;
            }
          });
          reachedAfterCallback = true;
          if (!reachedCallback) {
            // A true async operation was begun; start a sleep.
            Asyncify.state = Asyncify.State.Unwinding;
            // TODO: reuse, don't alloc/free every sleep
            Asyncify.currData = Asyncify.allocateData();
            if (typeof MainLoop != 'undefined' && MainLoop.func) {
              MainLoop.pause();
            }
            runAndAbortIfError(() => _asyncify_start_unwind(Asyncify.currData));
          }
        } else if (Asyncify.state === Asyncify.State.Rewinding) {
          // Stop a resume.
          Asyncify.state = Asyncify.State.Normal;
          runAndAbortIfError(_asyncify_stop_rewind);
          _free(Asyncify.currData);
          Asyncify.currData = null;
          // Call all sleep callbacks now that the sleep-resume is all done.
          Asyncify.sleepCallbacks.forEach(callUserCallback);
        } else {
          abort(`invalid state: ${Asyncify.state}`);
        }
        return Asyncify.handleSleepReturnValue;
      },
  handleAsync:(startAsync) => Asyncify.handleSleep(async (wakeUp) => {
        // TODO: add error handling as a second param when handleSleep implements it.
        wakeUp(await startAsync());
      }),
  };
  
  function getRequiredArgCount(argTypes) {
      var requiredArgCount = argTypes.length - 2;
      for (var i = argTypes.length - 1; i >= 2; --i) {
        if (!argTypes[i].optional) {
          break;
        }
        requiredArgCount--;
      }
      return requiredArgCount;
    }
  
  function craftInvokerFunction(humanName, argTypes, classType, cppInvokerFunc, cppTargetFunc, /** boolean= */ isAsync) {
      // humanName: a human-readable string name for the function to be generated.
      // argTypes: An array that contains the embind type objects for all types in the function signature.
      //    argTypes[0] is the type object for the function return value.
      //    argTypes[1] is the type object for function this object/class type, or null if not crafting an invoker for a class method.
      //    argTypes[2...] are the actual function parameters.
      // classType: The embind type object for the class to be bound, or null if this is not a method of a class.
      // cppInvokerFunc: JS Function object to the C++-side function that interops into C++ code.
      // cppTargetFunc: Function pointer (an integer to FUNCTION_TABLE) to the target C++ function the cppInvokerFunc will end up calling.
      // isAsync: Optional. If true, returns an async function. Async bindings are only supported with JSPI.
      var argCount = argTypes.length;
  
      if (argCount < 2) {
        throwBindingError('argTypes array size mismatch! Must at least get return value and receiver (this) types!');
      }
  
      assert(!isAsync, 'async bindings are only supported with JSPI');
      var isClassMethodFunc = (argTypes[1] !== null && classType !== null);
  
      // Free functions with signature "void function()" do not need an invoker that marshalls between wire types.
      // TODO: This omits argument count check - enable only at -O3 or similar.
      //    if (ENABLE_UNSAFE_OPTS && argCount == 2 && argTypes[0].name == 'void' && !isClassMethodFunc) {
      //       return FUNCTION_TABLE[fn];
      //    }
  
      // Determine if we need to use a dynamic stack to store the destructors for the function parameters.
      // TODO: Remove this completely once all function invokers are being dynamically generated.
      var needsDestructorStack = usesDestructorStack(argTypes);
  
      // Stack-allocating trivial value types get a stackSave/stackRestore
      // bracket around the call; see createJsInvoker for the async carve-outs.
      var argsNeedStack = argsUseStackAlloc(argTypes);
      var useStackFrame = false;
      if (argsNeedStack && !useStackFrame) {
        needsDestructorStack = true;
      }
  
      var returns = !argTypes[0].isVoid;
  
      var expectedArgCount = argCount - 2;
      var minArgs = getRequiredArgCount(argTypes);
      // Build the arguments that will be passed into the closure around the invoker
      // function.
      var retType = argTypes[0];
      var instType = argTypes[1];
      var closureArgs = [humanName, throwBindingError, cppInvokerFunc, cppTargetFunc, runDestructors, retType.fromWireType.bind(retType), instType?.toWireType.bind(instType)];
      if (useStackFrame) {
        // Must mirror the `args1.push('stackSave', 'stackRestore')` in createJsInvoker.
        closureArgs.push(stackSave, stackRestore);
      }
      for (var i = 2; i < argCount; ++i) {
        var argType = argTypes[i];
        closureArgs.push(argType.toWireType.bind(argType));
      }
      closureArgs.push(Asyncify);
      if (!needsDestructorStack) {
        // Skip return value at index 0 - it's not deleted here. Also skip class type if not a method.
        for (var i = isClassMethodFunc?1:2; i < argTypes.length; ++i) {
          if (argTypes[i].destructorFunction !== null) {
            closureArgs.push(argTypes[i].destructorFunction);
          }
        }
      }
      closureArgs.push(checkArgCount, minArgs, expectedArgCount);
  
      let invokerFactory = createJsInvoker(argTypes, isClassMethodFunc, returns, isAsync);
      var invokerFn = invokerFactory(...closureArgs);
      return createNamedFunction(humanName, invokerFn);
    }
  var __embind_register_class_constructor = (
      rawClassType,
      argCount,
      rawArgTypesAddr,
      invokerSignature,
      invoker,
      rawConstructor
    ) => {
      assert(argCount > 0);
      var rawArgTypes = heap32VectorToArray(argCount, rawArgTypesAddr);
      invoker = embind__requireFunction(invokerSignature, invoker);
      var args = [rawConstructor];
      var destructors = [];
  
      whenDependentTypesAreResolved([], [rawClassType], (classType) => {
        classType = classType[0];
        var humanName = `constructor ${classType.name}`;
  
        if (undefined === classType.registeredClass.constructor_body) {
          classType.registeredClass.constructor_body = [];
        }
        if (undefined !== classType.registeredClass.constructor_body[argCount - 1]) {
          throw new BindingError(`Cannot register multiple constructors with identical number of parameters (${argCount-1}) for class '${classType.name}'! Overload resolution is currently only performed using the parameter count, not actual type info!`);
        }
        classType.registeredClass.constructor_body[argCount - 1] = () => {
          throwUnboundTypeError(`Cannot construct ${classType.name} due to unbound types`, rawArgTypes);
        };
  
        whenDependentTypesAreResolved([], rawArgTypes, (argTypes) => {
          // Insert empty slot for context type (argTypes[1]).
          argTypes.splice(1, 0, null);
          classType.registeredClass.constructor_body[argCount - 1] = craftInvokerFunction(humanName, argTypes, null, invoker, rawConstructor);
          return [];
        });
        return [];
      });
    };

  
  
  
  
  
  
  var getFunctionName = (signature) => {
      signature = signature.trim();
      const argsIndex = signature.indexOf('(');
      if (argsIndex === -1) return signature;
      assert(signature.endsWith(')'), 'Parentheses for argument names should match.');
      return signature.slice(0, argsIndex);
    };
  var __embind_register_class_function = (rawClassType,
                                      methodName,
                                      argCount,
                                      rawArgTypesAddr, // [ReturnType, ThisType, Args...]
                                      invokerSignature,
                                      rawInvoker,
                                      context,
                                      isPureVirtual,
                                      isAsync,
                                      isNonnullReturn) => {
      var rawArgTypes = heap32VectorToArray(argCount, rawArgTypesAddr);
      methodName = AsciiToString(methodName);
      methodName = getFunctionName(methodName);
      rawInvoker = embind__requireFunction(invokerSignature, rawInvoker, isAsync);
  
      whenDependentTypesAreResolved([], [rawClassType], (classType) => {
        classType = classType[0];
        var humanName = `${classType.name}.${methodName}`;
  
        if (methodName.startsWith('@@')) {
          methodName = Symbol[methodName.substring(2)];
        }
  
        if (isPureVirtual) {
          classType.registeredClass.pureVirtualFunctions.push(methodName);
        }
  
        function unboundTypesHandler() {
          throwUnboundTypeError(`Cannot call ${humanName} due to unbound types`, rawArgTypes);
        }
  
        var proto = classType.registeredClass.instancePrototype;
        var method = proto[methodName];
        if (undefined === method || (undefined === method.overloadTable && method.className !== classType.name && method.argCount === argCount - 2)) {
          // This is the first overload to be registered, OR we are replacing a
          // function in the base class with a function in the derived class.
          unboundTypesHandler.argCount = argCount - 2;
          unboundTypesHandler.className = classType.name;
          proto[methodName] = unboundTypesHandler;
        } else {
          // There was an existing function with the same name registered. Set up
          // a function overload routing table.
          ensureOverloadTable(proto, methodName, humanName);
          proto[methodName].overloadTable[argCount - 2] = unboundTypesHandler;
        }
  
        whenDependentTypesAreResolved([], rawArgTypes, (argTypes) => {
          var memberFunction = craftInvokerFunction(humanName, argTypes, classType, rawInvoker, context, isAsync);
  
          // Replace the initial unbound-handler-stub function with the
          // appropriate member function, now that all types are resolved. If
          // multiple overloads are registered for this function, the function
          // goes into an overload table.
          if (undefined === proto[methodName].overloadTable) {
            // Set argCount in case an overload is registered later
            memberFunction.argCount = argCount - 2;
            proto[methodName] = memberFunction;
          } else {
            proto[methodName].overloadTable[argCount - 2] = memberFunction;
          }
  
          return [];
        });
        return [];
      });
    };

  
  var emval_freelist = [];
  
  var emval_handles = [0,1,,1,null,1,true,1,false,1];
  
  var emval_exception_decrefs = [];
  var __emval_decref = (handle) => {
      if (handle > 9 && 0 === --emval_handles[handle + 1]) {
        assert(emval_handles[handle] !== undefined, `decref for unallocated handle`);
        var value = emval_handles[handle];
        emval_handles[handle] = undefined;
        // In case the value is a C++ exception, decrement the refcount, so the
        // memory can be freed correctly
        var destructor = emval_exception_decrefs[handle];
        if (destructor) {
          emval_exception_decrefs[handle] = undefined;
          destructor(value);
        }
        emval_freelist.push(handle);
      }
    };
  
  
  
  var Emval = {
  toValue:(handle) => {
        if (!handle) {
            throwBindingError(`Cannot use deleted val. handle = ${handle}`);
        }
        // handle 2 is supposed to be `undefined`.
        assert(handle === 2 || emval_handles[handle] !== undefined && handle % 2 === 0, `invalid handle: ${handle}`);
        return emval_handles[handle];
      },
  toHandle:(value) => {
        switch (value) {
          case undefined: return 2;
          case null: return 4;
          case true: return 6;
          case false: return 8;
          default:{
            const handle = emval_freelist.pop() || emval_handles.length;
            emval_handles[handle] = value;
            emval_handles[handle + 1] = 1;
            return handle;
          }
        }
      },
  };
  
  var EmValType = {
      name: 'emscripten::val',
      fromWireType: (handle) => {
        var rv = Emval.toValue(handle);
        __emval_decref(handle);
        return rv;
      },
      toWireType: (destructors, value) => Emval.toHandle(value),
      readValueFromPointer: readPointer,
      destructorFunction: null, // This type does not need a destructor
  
      // TODO: do we need a deleteObject here?  write a test where
      // emval is passed into JS via an interface
    };
  var __embind_register_emval = (rawType) => registerType(rawType, EmValType);

  /** @type {!Float32Array} */
  var HEAPF32;
  
  /** @type {!Float64Array} */
  var HEAPF64;
  var floatReadValueFromPointer = (name, width) => {
      switch (width) {
        case 4: return function(pointer) {
          return this.fromWireType(HEAPF32[((pointer)>>2)]);
        };
        case 8: return function(pointer) {
          return this.fromWireType(HEAPF64[((pointer)>>3)]);
        };
        default:
          throw new TypeError(`invalid float width (${width}): ${name}`);
      }
    };
  
  
  
  var __embind_register_float = (rawType, name, size) => {
      name = AsciiToString(name);
      registerType(rawType, {
        name,
        fromWireType: (value) => value,
        toWireType: (destructors, value) => {
          if (typeof value != 'number' && typeof value != 'boolean') {
            throw new TypeError(`Cannot convert ${embindRepr(value)} to ${name}`);
          }
          // The VM will perform JS to Wasm value conversion, according to the spec:
          // https://www.w3.org/TR/wasm-js-api-1/#towebassemblyvalue
          return value;
        },
        readValueFromPointer: floatReadValueFromPointer(name, size),
        destructorFunction: null, // This type does not need a destructor
      });
    };

  
  
  
  
  
  
  
  
  var __embind_register_function = (name, argCount, rawArgTypesAddr, signature, rawInvoker, fn, isAsync, isNonnullReturn) => {
      var argTypes = heap32VectorToArray(argCount, rawArgTypesAddr);
      name = AsciiToString(name);
      name = getFunctionName(name);
  
      rawInvoker = embind__requireFunction(signature, rawInvoker, isAsync);
  
      exposePublicSymbol(name, function() {
        throwUnboundTypeError(`Cannot call ${name} due to unbound types`, argTypes);
      }, argCount - 1);
  
      whenDependentTypesAreResolved([], argTypes, (argTypes) => {
        var invokerArgsArray = [argTypes[0] /* return value */, null /* no class 'this'*/].concat(argTypes.slice(1) /* actual params */);
        replacePublicSymbol(name, craftInvokerFunction(name, invokerArgsArray, null /* no class 'this'*/, rawInvoker, fn, isAsync), argCount - 1);
        return [];
      });
    };

  
  
  
  
  /** @suppress {globalThis} */
  var __embind_register_integer = (primitiveType, name, size, minRange, maxRange) => {
      name = AsciiToString(name);
  
      const isUnsignedType = minRange === 0;
  
      let fromWireType = (value) => value;
      if (isUnsignedType) {
        var bitshift = 32 - 8*size;
        fromWireType = (value) => (value << bitshift) >>> bitshift;
        maxRange = fromWireType(maxRange);
      }
  
      registerType(primitiveType, {
        name,
        fromWireType: fromWireType,
        toWireType: (destructors, value) => {
          if (typeof value != 'number' && typeof value != 'boolean') {
            throw new TypeError(`Cannot convert "${embindRepr(value)}" to ${name}`);
          }
          assertIntegerRange(name, value, minRange, maxRange);
          // The VM will perform JS to Wasm value conversion, according to the spec:
          // https://www.w3.org/TR/wasm-js-api-1/#towebassemblyvalue
          return value;
        },
        readValueFromPointer: integerReadValueFromPointer(name, size, minRange !== 0),
        destructorFunction: null, // This type does not need a destructor
      });
    };

  
  
  
  
  
  
  
  
    /**
   * @param {number} ptr
   * @param {string} type
   */
  function getValue(ptr, type = 'i8') {
    if (type.endsWith('*')) type = '*';
    switch (type) {
      case 'i1': return HEAP8[ptr];
      case 'i8': return HEAP8[ptr];
      case 'i16': return HEAP16[((ptr)>>1)];
      case 'i32': return HEAP32[((ptr)>>2)];
      case 'i64': return HEAP64[((ptr)>>3)];
      case 'float': return HEAPF32[((ptr)>>2)];
      case 'double': return HEAPF64[((ptr)>>3)];
      case '*': return HEAPU32[((ptr)>>2)];
      default: abort(`invalid type for getValue: ${type}`);
    }
  }
  var installIndexedIterator = (proto, sizeMethodName, getMethodName) => {
      const makeIterator = (size, getValue) => {
        let index = 0;
        return {
          next() {
            if (index >= size) {
              return { done: true };
            }
            const current = index;
            index++;
            const value = getValue(current);
            return { value, done: false };
          },
          [Symbol.iterator]() {
            return this;
          },
        };
      };
  
      if (!proto[Symbol.iterator]) {
        proto[Symbol.iterator] = function() {
          const size = this[sizeMethodName]();
          return makeIterator(size, (i) => this[getMethodName](i));
        };
      }
    };
  
  var __embind_register_iterable = (rawClassType, rawElementType, sizeMethodName, getMethodName) => {
      sizeMethodName = AsciiToString(sizeMethodName);
      getMethodName = AsciiToString(getMethodName);
      whenDependentTypesAreResolved([], [rawClassType, rawElementType], (types) => {
        const classType = types[0];
        installIndexedIterator(classType.registeredClass.instancePrototype, sizeMethodName, getMethodName);
        return [];
      });
    };

  
  
  
  var __embind_register_memory_view = (rawType, dataTypeIndex, name) => {
      var typeMapping = [
        Int8Array,
        Uint8Array,
        Int16Array,
        Uint16Array,
        Int32Array,
        Uint32Array,
        Float32Array,
        Float64Array,
        BigInt64Array,
        BigUint64Array,
      ];
  
      var TA = typeMapping[dataTypeIndex];
  
      function decodeMemoryView(handle) {
        var size = HEAPU32[((handle)>>2)];
        var data = HEAPU32[(((handle)+(4))>>2)];
        return new TA(HEAP8.buffer, data, size);
      }
  
      name = AsciiToString(name);
      registerType(rawType, {
        name,
        fromWireType: decodeMemoryView,
        readValueFromPointer: decodeMemoryView,
      }, {
        ignoreDuplicateRegistrations: true,
      });
    };

  
  var EmValOptionalType = Object.assign({optional: true}, EmValType);;
  var __embind_register_optional = (rawOptionalType, rawType) => {
      registerType(rawOptionalType, EmValOptionalType);
    };

  
  
  
  
  
  var stringToUTF8 = (str, outPtr, maxBytesToWrite) => {
      assert(typeof maxBytesToWrite == 'number', 'stringToUTF8 requires a third parameter that specifies the length of the output buffer');
      return stringToUTF8Array(str, HEAPU8, outPtr, maxBytesToWrite);
    };
  
  
  
  
  
  
  var __embind_register_std_string = (rawType, name) => {
      name = AsciiToString(name);
      var stdStringIsUTF8 = true;
  
      registerType(rawType, {
        name,
        // For some method names we use string keys here since they are part of
        // the public/external API and/or used by the runtime-generated code.
        fromWireType(value) {
          var length = HEAPU32[((value)>>2)];
          var payload = value + 4;
  
          var str;
          if (stdStringIsUTF8) {
            str = UTF8ToString(payload, length, true);
          } else {
            str = '';
            for (var i = 0; i < length; ++i) {
              str += String.fromCharCode(HEAPU8[payload + i]);
            }
          }
  
          _free(value);
  
          return str;
        },
        toWireType(destructors, value) {
          if (value instanceof ArrayBuffer) {
            value = new Uint8Array(value);
          }
  
          var length;
          var valueIsOfTypeString = (typeof value == 'string');
  
          // We accept `string` or array views with single byte elements
          if (!(valueIsOfTypeString || (ArrayBuffer.isView(value) && value.BYTES_PER_ELEMENT == 1))) {
            throwBindingError('Cannot pass non-string to std::string');
          }
          if (stdStringIsUTF8 && valueIsOfTypeString) {
            length = lengthBytesUTF8(value);
          } else {
            length = value.length;
          }
  
          // assumes POINTER_SIZE alignment
          var base = _malloc(4 + length + 1);
          var ptr = base + 4;
          HEAPU32[((base)>>2)] = length;
          if (valueIsOfTypeString) {
            if (stdStringIsUTF8) {
              stringToUTF8(value, ptr, length + 1);
            } else {
              for (var i = 0; i < length; ++i) {
                var charCode = value.charCodeAt(i);
                if (charCode > 255) {
                  _free(base);
                  throwBindingError('String has UTF-16 code units that do not fit in 8 bits');
                }
                HEAPU8[ptr + i] = charCode;
              }
            }
          } else {
            HEAPU8.set(value, ptr);
          }
  
          if (destructors !== null) {
            destructors.push(_free, base);
          }
          return base;
        },
        readValueFromPointer: readPointer,
        destructorFunction(ptr) {
          _free(ptr);
        },
      });
    };

  
  
  
  var UTF16Decoder = globalThis.TextDecoder ? new TextDecoder('utf-16le') : undefined;;
  
  
  var UTF16ToString = (ptr, maxBytesToRead, ignoreNul) => {
      assert(ptr % 2 == 0, 'pointer passed to UTF16ToString must be 2-byte aligned');
      var idx = ((ptr)>>1);
      var endIdx = findStringEnd(HEAPU16, idx, maxBytesToRead / 2, ignoreNul);
  
      // When using conditional TextDecoder, skip it for short strings as the overhead of the native call is not worth it.
      if (endIdx - idx > 16 && UTF16Decoder)
        return UTF16Decoder.decode(HEAPU16.subarray(idx, endIdx));
  
      // Fallback: decode without UTF16Decoder
      var str = '';
  
      // If maxBytesToRead is not passed explicitly, it will be undefined, and the
      // for-loop's condition will always evaluate to true. The loop is then
      // terminated on the first null char.
      for (var i = idx; i < endIdx; ++i) {
        var codeUnit = HEAPU16[i];
        // fromCharCode constructs a character from a UTF-16 code unit, so we can
        // pass the UTF16 string right through.
        str += String.fromCharCode(codeUnit);
      }
  
      return str;
    };
  
  var stringToUTF16 = (str, outPtr, maxBytesToWrite = 0x7FFFFFFF) => {
      assert(outPtr % 2 == 0, 'pointer passed to stringToUTF16 must be 2-byte aligned');
      assert(typeof maxBytesToWrite == 'number', 'stringToUTF16 requires a third parameter that specifies the length of the output buffer');
      if (maxBytesToWrite < 2) return 0;
      maxBytesToWrite -= 2; // Null terminator.
      var startPtr = outPtr;
      var numCharsToWrite = (maxBytesToWrite < str.length*2) ? (maxBytesToWrite / 2) : str.length;
      for (var i = 0; i < numCharsToWrite; ++i) {
        // charCodeAt returns a UTF-16 encoded code unit, so it can be directly written to the HEAP.
        var codeUnit = str.charCodeAt(i); // possibly a lead surrogate
        HEAP16[((outPtr)>>1)] = codeUnit;
        outPtr += 2;
      }
      // Null-terminate the pointer to the HEAP.
      HEAP16[((outPtr)>>1)] = 0;
      return outPtr - startPtr;
    };
  
  var lengthBytesUTF16 = (str) => str.length*2;
  
  var UTF32ToString = (ptr, maxBytesToRead, ignoreNul) => {
      assert(ptr % 4 == 0, 'pointer passed to UTF32ToString must be 2-byte aligned');
      var str = '';
      var startIdx = ((ptr)>>2);
      // If maxBytesToRead is not passed explicitly, it will be undefined, and this
      // will always evaluate to true. This saves on code size.
      for (var i = 0; !(i >= maxBytesToRead / 4); i++) {
        var utf32 = HEAPU32[startIdx + i];
        if (!utf32 && !ignoreNul) break;
        str += String.fromCodePoint(utf32);
      }
      return str;
    };
  
  var stringToUTF32 = (str, outPtr, maxBytesToWrite = 0x7FFFFFFF) => {
      assert(outPtr % 4 == 0, 'pointer passed to stringToUTF32 must be 4-byte aligned');
      assert(typeof maxBytesToWrite == 'number', 'stringToUTF32 requires a third parameter that specifies the length of the output buffer');
      if (maxBytesToWrite < 4) return 0;
      var startPtr = outPtr;
      var endPtr = startPtr + maxBytesToWrite - 4;
      for (var i = 0; i < str.length; ++i) {
        var codePoint = str.codePointAt(i);
        // Gotcha: if codePoint is over 0xFFFF, it is represented as a surrogate pair in UTF-16.
        // We need to manually skip over the second code unit for correct iteration.
        if (codePoint > 0xFFFF) {
          i++;
        }
        HEAP32[((outPtr)>>2)] = codePoint;
        outPtr += 4;
        if (outPtr + 4 > endPtr) break;
      }
      // Null-terminate the pointer to the HEAP.
      HEAP32[((outPtr)>>2)] = 0;
      return outPtr - startPtr;
    };
  
  var lengthBytesUTF32 = (str) => {
      var len = 0;
      for (var i = 0; i < str.length; ++i) {
        var codePoint = str.codePointAt(i);
        // Gotcha: if codePoint is over 0xFFFF, it is represented as a surrogate pair in UTF-16.
        // We need to manually skip over the second code unit for correct iteration.
        if (codePoint > 0xFFFF) {
          i++;
        }
        len += 4;
      }
  
      return len;
    };
  
  var __embind_register_std_wstring = (rawType, charSize, name) => {
      name = AsciiToString(name);
      var decodeString, encodeString, lengthBytesUTF;
      if (charSize === 2) {
        decodeString = UTF16ToString;
        encodeString = stringToUTF16;
        lengthBytesUTF = lengthBytesUTF16;
      } else {
        assert(charSize === 4, 'only 2-byte and 4-byte strings are currently supported');
        decodeString = UTF32ToString;
        encodeString = stringToUTF32;
        lengthBytesUTF = lengthBytesUTF32;
      }
      registerType(rawType, {
        name,
        fromWireType: (value) => {
          // Code mostly taken from _embind_register_std_string fromWireType
          var length = HEAPU32[((value)>>2)];
          var str = decodeString(value + 4, length * charSize, true);
  
          _free(value);
  
          return str;
        },
        toWireType: (destructors, value) => {
          if (!(typeof value == 'string')) {
            throwBindingError(`Cannot pass non-string to C++ string type ${name}`);
          }
  
          // assumes POINTER_SIZE alignment
          var length = lengthBytesUTF(value);
          var ptr = _malloc(4 + length + charSize);
          HEAPU32[((ptr)>>2)] = length / charSize;
  
          encodeString(value, ptr + 4, length + charSize);
  
          if (destructors !== null) {
            destructors.push(_free, ptr);
          }
          return ptr;
        },
        readValueFromPointer: readPointer,
        destructorFunction(ptr) {
          _free(ptr);
        }
      });
    };

  
  var __embind_register_void = (rawType, name) => {
      name = AsciiToString(name);
      registerType(rawType, {
        isVoid: true, // void return values can be optimized out sometimes
        name,
        fromWireType: () => undefined,
        // TODO: assert if anything else is given?
        toWireType: (destructors, o) => undefined,
      });
    };

  var __emscripten_throw_longjmp = () => {
      throw new EmscriptenSjLj;
    };

  var emval_methodCallers = [];
  var emval_addMethodCaller = (caller) => {
      var id = emval_methodCallers.length;
      emval_methodCallers.push(caller);
      return id;
    };
  
  
  
  var requireRegisteredType = (rawType, humanName) => {
      var impl = registeredTypes[rawType];
      if (undefined === impl) {
        throwBindingError(`${humanName} has unknown type ${getTypeName(rawType)}`);
      }
      return impl;
    };
  
  var emval_lookupTypes = (argCount, argTypes) => {
      var a = new Array(argCount);
      for (var i = 0; i < argCount; ++i) {
        a[i] = requireRegisteredType(HEAPU32[(((argTypes)+(i*4))>>2)],
                                     `parameter ${i}`);
      }
      return a;
    };
  
  
  
  var emval_returnValue = (toReturnWire, destructorsRef, handle) => {
      var destructors = [];
      var result = toReturnWire(destructors, handle);
      if (destructors.length) {
        // void, primitives and any other types w/o destructors don't need to allocate a handle
        HEAPU32[((destructorsRef)>>2)] = Emval.toHandle(destructors);
      }
      return result;
    };
  
  
  var emval_symbols = {
  };
  
  var getStringOrSymbol = (address) => {
      var symbol = emval_symbols[address];
      if (symbol === undefined) {
        return AsciiToString(address);
      }
      return symbol;
    };
  var __emval_create_invoker = (argCount, argTypesPtr, kind) => {
      var GenericWireTypeSize = 8;
  
      var [retType, ...argTypes] = emval_lookupTypes(argCount, argTypesPtr);
      var toReturnWire = retType.toWireType.bind(retType);
      var argFromPtr = argTypes.map(type => type.readValueFromPointer.bind(type));
      argCount--; // remove the extracted return type
  
      var captures = {'toValue': Emval.toValue};
      var args = argFromPtr.map((argFromPtr, i) => {
        var captureName = `argFromPtr${i}`;
        captures[captureName] = argFromPtr;
        return `${captureName}(args${i ? '+' + i * GenericWireTypeSize : ''})`;
      });
      var functionBody;
      switch (kind){
        case 0:
          functionBody = 'toValue(handle)';
          break;
        case 2:
          functionBody = 'new (toValue(handle))';
          break;
        case 3:
          functionBody = '';
          break;
        case 1:
          captures['getStringOrSymbol'] = getStringOrSymbol;
          functionBody = 'toValue(handle)[getStringOrSymbol(methodName)]';
          break;
      }
      functionBody += `(${args})`;
      if (!retType.isVoid) {
        captures['toReturnWire'] = toReturnWire;
        captures['emval_returnValue'] = emval_returnValue;
        functionBody = `return emval_returnValue(toReturnWire, destructorsRef, ${functionBody})`;
      }
      functionBody = `return function (handle, methodName, destructorsRef, args) {
${functionBody}
}`;
  
      var invokerFunction = new Function(Object.keys(captures), functionBody)(...Object.values(captures));
      var functionName = `methodCaller<(${argTypes.map(t => t.name)}) => ${retType.name}>`;
      return emval_addMethodCaller(createNamedFunction(functionName, invokerFunction));
    };


  
  
  var __emval_invoke = (caller, handle, methodName, destructorsRef, args) => {
      return emval_methodCallers[caller](handle, methodName, destructorsRef, args);
    };

  
  
  var __emval_run_destructors = (handle) => {
      var destructors = Emval.toValue(handle);
      runDestructors(destructors);
      __emval_decref(handle);
    };

  
  
  
  
  
  var INT53_MAX = 9007199254740992;
  
  var INT53_MIN = -9007199254740992;
  var bigintToI53Checked = (num) => (num < INT53_MIN || num > INT53_MAX) ? NaN : Number(num);
  
  
  function __mmap_js(len, prot, flags, fd, offset, allocated, addr) {
    offset = bigintToI53Checked(offset);
  
  
  try {
  
      // musl's mmap doesn't allow values over a certain limit
      // see OFF_MASK in mmap.c.
      assert(!isNaN(offset));
      var stream = SYSCALLS.getStreamFromFD(fd);
      var res = FS.mmap(stream, len, offset, prot, flags);
      var ptr = res.ptr;
      HEAP32[((allocated)>>2)] = res.allocated;
      HEAPU32[((addr)>>2)] = ptr;
      return 0;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  ;
  }

  
  function __munmap_js(addr, len, prot, flags, fd, offset) {
    offset = bigintToI53Checked(offset);
  
  
  try {
  
      var stream = SYSCALLS.getStreamFromFD(fd);
      if (prot & 2) {
        SYSCALLS.doMsync(addr, stream, len, flags, offset);
      }
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  ;
  }

  
  
  
  var __tzset_js = (timezone, daylight, std_name, dst_name) => {
      // TODO: Use (malleable) environment variables instead of system settings.
      var currentYear = new Date().getFullYear();
      var winter = new Date(currentYear, 0, 1);
      var summer = new Date(currentYear, 6, 1);
      var winterOffset = winter.getTimezoneOffset();
      var summerOffset = summer.getTimezoneOffset();
  
      // Local standard timezone offset. Local standard time is not adjusted for
      // daylight savings.  This code uses the fact that getTimezoneOffset returns
      // a greater value during Standard Time versus Daylight Saving Time (DST).
      // Thus it determines the expected output during Standard Time, and it
      // compares whether the output of the given date the same (Standard) or less
      // (DST).
      var stdTimezoneOffset = Math.max(winterOffset, summerOffset);
  
      // timezone is specified as seconds west of UTC ("The external variable
      // `timezone` shall be set to the difference, in seconds, between
      // Coordinated Universal Time (UTC) and local standard time."), the same
      // as returned by stdTimezoneOffset.
      // See http://pubs.opengroup.org/onlinepubs/009695399/functions/tzset.html
      HEAPU32[((timezone)>>2)] = stdTimezoneOffset * 60;
  
      HEAP32[((daylight)>>2)] = Number(winterOffset != summerOffset);
  
      var extractZone = (timezoneOffset) => {
        // Why inverse sign?
        // Read here https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/getTimezoneOffset
        var sign = timezoneOffset >= 0 ? '-' : '+';
  
        var absOffset = Math.abs(timezoneOffset)
        var hours = String(Math.floor(absOffset / 60)).padStart(2, '0');
        var minutes = String(absOffset % 60).padStart(2, '0');
  
        return `UTC${sign}${hours}${minutes}`;
      }
  
      var winterName = extractZone(winterOffset);
      var summerName = extractZone(summerOffset);
      assert(winterName);
      assert(summerName);
      assert(lengthBytesUTF8(winterName) <= 16, `timezone name truncated to fit in TZNAME_MAX (${winterName})`);
      assert(lengthBytesUTF8(summerName) <= 16, `timezone name truncated to fit in TZNAME_MAX (${summerName})`);
      if (summerOffset < winterOffset) {
        // Northern hemisphere
        stringToUTF8(winterName, std_name, 17);
        stringToUTF8(summerName, dst_name, 17);
      } else {
        stringToUTF8(winterName, dst_name, 17);
        stringToUTF8(summerName, std_name, 17);
      }
    };

  var _emscripten_get_now = () => performance.now();
  
  var _emscripten_date_now = () => Date.now();
  
  var nowIsMonotonic = 1;
  
  var checkWasiClock = (clock_id) => clock_id >= 0 && clock_id <= 3;
  
  
  function _clock_time_get(clk_id, ignored_precision, ptime) {
    ignored_precision = bigintToI53Checked(ignored_precision);
  
  
      if (!checkWasiClock(clk_id)) {
        return 28;
      }
      var now;
      // all wasi clocks but realtime are monotonic
      if (clk_id === 0) {
        now = _emscripten_date_now();
      } else if (nowIsMonotonic) {
        now = _emscripten_get_now();
      } else {
        return 52;
      }
      // "now" is in ms, and wasi times are in ns.
      var nsec = Math.round(now * 1000 * 1000);
      HEAP64[((ptime)>>3)] = BigInt(nsec);
      return 0;
    ;
  }

  
  function getFullscreenElement() {
      return document.fullscreenElement
             ?? document.webkitFullscreenElement
             ;
    }
  
  /** @param {number=} timeout */
  var safeSetTimeout = (func, timeout) => {
      
      // Slot 0 is reserved so that, like setTimeout, ids are always non-zero.
      safeSetTimeout.mapping ||= [0];
      var id = safeSetTimeout.mapping.length;
      safeSetTimeout.mapping[id] = setTimeout(() => {
        safeSetTimeout.mapping[id] = undefined;
        
        callUserCallback(func);
      }, timeout);
      return id;
    };
  
  
  
  
  
  var Browser = {
  useWebGL:false,
  isFullscreen:false,
  pointerLock:false,
  moduleContextCreatedCallbacks:[],
  preloadedImages:{
  },
  preloadedAudios:{
  },
  getCanvas:() => Module['canvas'],
  init() {
        if (Browser.initted) return;
        Browser.initted = true;
  
        // Support for plugins that can process preloaded files. You can add more of these to
        // your app by creating and appending to preloadPlugins.
        //
        // Each plugin is asked if it can handle a file based on the file's name. If it can,
        // it is given the file's raw data. When it is done, it calls a callback with the file's
        // (possibly modified) data. For example, a plugin might decompress a file, or it
        // might create some side data structure for use later (like an Image element, etc.).
  
        var imagePlugin = {};
        imagePlugin['canHandle'] = (name) => {
          return !Module['noImageDecoding'] && /\.(jpg|jpeg|png|bmp|webp)$/i.test(name);
        };
        imagePlugin['handle'] = async (byteArray, name) => {
          var b = new Blob([byteArray], { type: Browser.getMimetype(name) });
          if (b.size !== byteArray.length) { // Safari bug #118630
            // Safari's Blob can only take an ArrayBuffer
            b = new Blob([(new Uint8Array(byteArray)).buffer], { type: Browser.getMimetype(name) });
          }
          var url = URL.createObjectURL(b);
          return new Promise((resolve, reject) => {
            var img = new Image();
            img.onload = () => {
              assert(img.complete, `Image ${name} could not be decoded`);
              var canvas = /** @type {!HTMLCanvasElement} */ (document.createElement('canvas'));
              canvas.width = img.width;
              canvas.height = img.height;
              var ctx = canvas.getContext('2d');
              ctx.drawImage(img, 0, 0);
              Browser.preloadedImages[name] = canvas;
              URL.revokeObjectURL(url);
              resolve(byteArray);
            };
            img.onerror = (event) => {
              err(`Image ${url} could not be decoded`);
              reject();
            };
            img.src = url;
          });
        };
        preloadPlugins.push(imagePlugin);
  
        var audioPlugin = {};
        audioPlugin['canHandle'] = (name) => {
          return !Module['noAudioDecoding'] && name.slice(-4) in { '.ogg': 1, '.wav': 1, '.mp3': 1 };
        };
        audioPlugin['handle'] = async (byteArray, name) => {
          return new Promise((resolve, reject) => {
            var done = false;
            function finish(audio) {
              if (done) return;
              done = true;
              Browser.preloadedAudios[name] = audio;
              resolve(byteArray);
            }
            var b = new Blob([byteArray], { type: Browser.getMimetype(name) });
            var url = URL.createObjectURL(b); // XXX we never revoke this!
            var audio = new Audio();
            audio.addEventListener('canplaythrough', () => finish(audio)); // use addEventListener due to chromium bug 124926
            audio.onerror = (event) => {
              if (done) return;
              err(`warning: browser could not fully decode audio ${name}, trying slower base64 approach`);
              function encode64(data) {
                var BASE = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
                var PAD = '=';
                var ret = '';
                var leftchar = 0;
                var leftbits = 0;
                for (var byte of data) {
                  leftchar = (leftchar << 8) | byte;
                  leftbits += 8;
                  while (leftbits >= 6) {
                    var curr = (leftchar >> (leftbits-6)) & 0x3f;
                    leftbits -= 6;
                    ret += BASE[curr];
                  }
                }
                if (leftbits == 2) {
                  ret += BASE[(leftchar&3) << 4];
                  ret += PAD + PAD;
                } else if (leftbits == 4) {
                  ret += BASE[(leftchar&0xf) << 2];
                  ret += PAD;
                }
                return ret;
              }
              audio.src = 'data:audio/x-' + name.slice(-3) + ';base64,' + encode64(byteArray);
              finish(audio); // we don't wait for confirmation this worked - but it's worth trying
            };
            audio.src = url;
            // workaround for chrome bug 124926 - we do not always get oncanplaythrough or onerror
            safeSetTimeout(() => {
              finish(audio); // try to use it even though it is not necessarily ready to play
            }, 10000);
          });
        };
        preloadPlugins.push(audioPlugin);
  
        // Canvas event setup
  
        function pointerLockChange() {
          var canvas = Browser.getCanvas();
          Browser.pointerLock = document.pointerLockElement === canvas;
        }
        var canvas = Browser.getCanvas();
        if (canvas) {
          // forced aspect ratio can be enabled by defining 'forcedAspectRatio' on Module
          // Module['forcedAspectRatio'] = 4 / 3;
  
          document.addEventListener('pointerlockchange', pointerLockChange);
  
          if (Module['elementPointerLock']) {
            canvas.addEventListener('click', (ev) => {
              if (!Browser.pointerLock && Browser.getCanvas().requestPointerLock) {
                Browser.getCanvas().requestPointerLock();
                ev.preventDefault();
              }
            });
          }
        }
      },
  createContext(/** @type {HTMLCanvasElement} */ canvas, useWebGL, setInModule, webGLContextAttributes) {
        if (useWebGL && Module['ctx'] && canvas == Browser.getCanvas()) return Module['ctx']; // no need to recreate GL context if it's already been created for this canvas.
  
        var ctx;
        var contextHandle;
        if (useWebGL) {
          // For GLES2/desktop GL compatibility, adjust a few defaults to be different to WebGL defaults, so that they align better with the desktop defaults.
          var contextAttributes = {
            antialias: false,
            alpha: false,
            majorVersion: (typeof WebGL2RenderingContext != 'undefined') ? 2 : 1,
          };
  
          if (webGLContextAttributes) {
            for (var attribute in webGLContextAttributes) {
              contextAttributes[attribute] = webGLContextAttributes[attribute];
            }
          }
  
          // This check of existence of GL is here to satisfy Closure compiler, which yells if variable GL is referenced below but GL object is not
          // actually compiled in because application is not doing any GL operations. TODO: Ideally if GL is not being used, this function
          // Browser.createContext() should not even be emitted.
          if (typeof GL != 'undefined') {
            contextHandle = GL.createContext(canvas, contextAttributes);
            if (contextHandle) {
              ctx = GL.getContext(contextHandle).GLctx;
            }
          }
        } else {
          ctx = canvas.getContext('2d');
        }
  
        if (!ctx) return null;
  
        if (setInModule) {
          if (!useWebGL) assert(typeof GLctx == 'undefined', 'cannot set in module if GLctx is used, but we are a non-GL context that would replace it');
          Module['ctx'] = ctx;
          if (useWebGL) GL.makeContextCurrent(contextHandle);
          Browser.useWebGL = useWebGL;
          Browser.moduleContextCreatedCallbacks.forEach((callback) => callback());
          Browser.init();
        }
        return ctx;
      },
  fullscreenHandlersInstalled:false,
  lockPointer:undefined,
  resizeCanvas:undefined,
  requestFullscreen(lockPointer, resizeCanvas) {
        Browser.lockPointer = lockPointer;
        Browser.resizeCanvas = resizeCanvas;
        if (typeof Browser.lockPointer == 'undefined') Browser.lockPointer = true;
        if (typeof Browser.resizeCanvas == 'undefined') Browser.resizeCanvas = false;
  
        var canvas = Browser.getCanvas();
        function fullscreenChange() {
          Browser.isFullscreen = false;
          var canvasContainer = canvas.parentNode;
          if (getFullscreenElement() === canvasContainer) {
            canvas.exitFullscreen = Browser.exitFullscreen;
            if (Browser.lockPointer) canvas.requestPointerLock();
            Browser.isFullscreen = true;
            if (Browser.resizeCanvas) {
              Browser.setFullscreenCanvasSize();
            } else {
              Browser.updateCanvasDimensions(canvas);
            }
          } else {
            // remove the full screen specific parent of the canvas again to restore the HTML structure from before going full screen
            canvasContainer.parentNode.insertBefore(canvas, canvasContainer);
            canvasContainer.parentNode.removeChild(canvasContainer);
  
            if (Browser.resizeCanvas) {
              Browser.setWindowedCanvasSize();
            } else {
              Browser.updateCanvasDimensions(canvas);
            }
          }
        }
  
        if (!Browser.fullscreenHandlersInstalled) {
          Browser.fullscreenHandlersInstalled = true;
          document.addEventListener('fullscreenchange', fullscreenChange);
          document.addEventListener('webkitfullscreenchange', fullscreenChange);
        }
  
        // create a new parent to ensure the canvas has no siblings. this allows browsers to optimize full screen performance when its parent is the full screen root
        var canvasContainer = document.createElement('div');
        canvas.parentNode.insertBefore(canvasContainer, canvas);
        canvasContainer.appendChild(canvas);
  
        // use parent of canvas as full screen root to allow aspect ratio correction (Firefox stretches the root to screen size)
        // Safari didn't support Element.requestFullscreen until 16.4
        // See: https://developer.mozilla.org/en-US/docs/Web/API/Element/requestFullscreen
        /** @suppress {checkTypes} */
        canvasContainer.requestFullscreen ??= (canvasContainer['webkitRequestFullscreen'] ? () => canvasContainer['webkitRequestFullscreen'](Element.ALLOW_KEYBOARD_INPUT) : null) ??
                                              (canvasContainer['webkitRequestFullScreen'] ? () => canvasContainer['webkitRequestFullScreen'](Element.ALLOW_KEYBOARD_INPUT) : null);
  
        canvasContainer.requestFullscreen();
      },
  exitFullscreen() {
        // This is workaround for chrome. Trying to exit from fullscreen
        // not in fullscreen state will cause 'TypeError: Document not active'
        // in chrome. See https://github.com/emscripten-core/emscripten/pull/8236
        if (!Browser.isFullscreen) {
          return false;
        }
  
        var CFS = document.exitFullscreen ?? document['webkitCancelFullScreen'];
        CFS.apply(document, []);
        return true;
      },
  safeSetTimeout(func, timeout) {
        // Legacy function, this is used by the SDL2 port so we need to keep it
        // around at least until that is updated.
        // See https://github.com/libsdl-org/SDL/pull/6304
        return safeSetTimeout(func, timeout);
      },
  getMimetype(name) {
        return {
          'jpg': 'image/jpeg',
          'jpeg': 'image/jpeg',
          'png': 'image/png',
          'bmp': 'image/bmp',
          'ogg': 'audio/ogg',
          'wav': 'audio/wav',
          'mp3': 'audio/mpeg'
        }[name.slice(name.lastIndexOf('.')+1)];
      },
  getUserMedia(func) {
        return navigator.mediaDevices.getUserMedia(func);
      },
  getMouseWheelDelta(event) {
        var delta = 0;
        switch (event.type) {
          case 'DOMMouseScroll':
            // 3 lines make up a step
            delta = event.detail / 3;
            break;
          case 'mousewheel':
            // 120 units make up a step
            delta = event.wheelDelta / 120;
            break;
          case 'wheel':
            delta = event.deltaY
            switch (event.deltaMode) {
              case 0:
                // DOM_DELTA_PIXEL: 100 pixels make up a step
                delta /= 100;
                break;
              case 1:
                // DOM_DELTA_LINE: 3 lines make up a step
                delta /= 3;
                break;
              case 2:
                // DOM_DELTA_PAGE: A page makes up 80 steps
                delta *= 80;
                break;
              default:
                abort('unrecognized mouse wheel delta mode: ' + event.deltaMode);
            }
            break;
          default:
            abort('unrecognized mouse wheel event: ' + event.type);
        }
        return delta;
      },
  mouseX:0,
  mouseY:0,
  mouseMovementX:0,
  mouseMovementY:0,
  touches:{
  },
  lastTouches:{
  },
  calculateMouseCoords(pageX, pageY) {
        // Calculate the movement based on the changes
        // in the coordinates.
        var canvas = Browser.getCanvas();
        var rect = canvas.getBoundingClientRect();
  
        var adjustedX = pageX - (window.scrollX + rect.left);
        var adjustedY = pageY - (window.scrollY + rect.top);
  
        // the canvas might be CSS-scaled compared to its backbuffer;
        // SDL-using content will want mouse coordinates in terms
        // of backbuffer units.
        adjustedX = adjustedX * (canvas.width / rect.width);
        adjustedY = adjustedY * (canvas.height / rect.height);
  
        return { x: adjustedX, y: adjustedY };
      },
  setMouseCoords(pageX, pageY) {
        const {x, y} = Browser.calculateMouseCoords(pageX, pageY);
        Browser.mouseMovementX = x - Browser.mouseX;
        Browser.mouseMovementY = y - Browser.mouseY;
        Browser.mouseX = x;
        Browser.mouseY = y;
      },
  calculateMouseEvent(event) { // event should be mousemove, mousedown or mouseup
        if (Browser.pointerLock) {
          // When the pointer is locked, calculate the coordinates
          // based on the movement of the mouse.
          Browser.mouseMovementX = event.movementX;
          Browser.mouseMovementY = event.movementY;
  
          // add the mouse delta to the current absolute mouse position
          Browser.mouseX += Browser.mouseMovementX;
          Browser.mouseY += Browser.mouseMovementY;
        } else {
          if (event.type === 'touchstart' || event.type === 'touchend' || event.type === 'touchmove') {
            var touch = event.touch;
            if (touch === undefined) {
              return; // the 'touch' property is only defined in SDL
  
            }
            var coords = Browser.calculateMouseCoords(touch.pageX, touch.pageY);
  
            if (event.type === 'touchstart') {
              Browser.lastTouches[touch.identifier] = coords;
              Browser.touches[touch.identifier] = coords;
            } else if (event.type === 'touchend' || event.type === 'touchmove') {
              var last = Browser.touches[touch.identifier];
              last ||= coords;
              Browser.lastTouches[touch.identifier] = last;
              Browser.touches[touch.identifier] = coords;
            }
            return;
          }
  
          Browser.setMouseCoords(event.pageX, event.pageY);
        }
      },
  resizeListeners:[],
  updateResizeListeners() {
        var canvas = Browser.getCanvas();
        Browser.resizeListeners.forEach((listener) => listener(canvas.width, canvas.height));
      },
  setCanvasSize(width, height, noUpdates) {
        var canvas = Browser.getCanvas();
        Browser.updateCanvasDimensions(canvas, width, height);
        if (!noUpdates) Browser.updateResizeListeners();
      },
  windowedWidth:0,
  windowedHeight:0,
  setFullscreenCanvasSize() {
        // check if SDL is available
        if (typeof SDL != 'undefined') {
          var flags = HEAPU32[((SDL.screen)>>2)];
          flags = flags | 0x00800000; // set SDL_FULLSCREEN flag
          HEAP32[((SDL.screen)>>2)] = flags;
        }
        Browser.updateCanvasDimensions(Browser.getCanvas());
        Browser.updateResizeListeners();
      },
  setWindowedCanvasSize() {
        // check if SDL is available
        if (typeof SDL != 'undefined') {
          var flags = HEAPU32[((SDL.screen)>>2)];
          flags = flags & ~0x00800000; // clear SDL_FULLSCREEN flag
          HEAP32[((SDL.screen)>>2)] = flags;
        }
        Browser.updateCanvasDimensions(Browser.getCanvas());
        Browser.updateResizeListeners();
      },
  updateCanvasDimensions(canvas, wNative, hNative) {
        if (wNative && hNative) {
          canvas.widthNative = wNative;
          canvas.heightNative = hNative;
        } else {
          wNative = canvas.widthNative;
          hNative = canvas.heightNative;
        }
        var w = wNative;
        var h = hNative;
        if ((getFullscreenElement() === canvas.parentNode) && (typeof screen != 'undefined')) {
           var factor = Math.min(screen.width / w, screen.height / h);
           w = Math.round(w * factor);
           h = Math.round(h * factor);
        }
        if (Browser.resizeCanvas) {
          if (canvas.width  != w) canvas.width  = w;
          if (canvas.height != h) canvas.height = h;
          if (typeof canvas.style != 'undefined') {
            canvas.style.removeProperty( 'width');
            canvas.style.removeProperty('height');
          }
        } else {
          if (canvas.width  != wNative) canvas.width  = wNative;
          if (canvas.height != hNative) canvas.height = hNative;
          if (typeof canvas.style != 'undefined') {
            if (w != wNative || h != hNative) {
              canvas.style.setProperty( 'width', w + 'px', 'important');
              canvas.style.setProperty('height', h + 'px', 'important');
            } else {
              canvas.style.removeProperty( 'width');
              canvas.style.removeProperty('height');
            }
          }
        }
      },
  };
  
  
  
  var EGL = {
  errorCode:12288,
  defaultDisplayInitialized:false,
  currentContext:0,
  currentReadSurface:0,
  currentDrawSurface:0,
  contextAttributes:{
  alpha:false,
  depth:false,
  stencil:false,
  antialias:false,
  },
  stringCache:{
  },
  setErrorCode(code) {
        EGL.errorCode = code;
      },
  chooseConfig(display, attribList, config, config_size, numConfigs) {
        if (display != 62000) {
          EGL.setErrorCode(0x3008 /* EGL_BAD_DISPLAY */);
          return 0;
        }
  
        if (attribList) {
          // read attribList if it is non-null
          for (;;) {
            var param = HEAP32[((attribList)>>2)];
            if (param == 0x3021 /*EGL_ALPHA_SIZE*/) {
              var alphaSize = HEAP32[(((attribList)+(4))>>2)];
              EGL.contextAttributes.alpha = (alphaSize > 0);
            } else if (param == 0x3025 /*EGL_DEPTH_SIZE*/) {
              var depthSize = HEAP32[(((attribList)+(4))>>2)];
              EGL.contextAttributes.depth = (depthSize > 0);
            } else if (param == 0x3026 /*EGL_STENCIL_SIZE*/) {
              var stencilSize = HEAP32[(((attribList)+(4))>>2)];
              EGL.contextAttributes.stencil = (stencilSize > 0);
            } else if (param == 0x3031 /*EGL_SAMPLES*/) {
              var samples = HEAP32[(((attribList)+(4))>>2)];
              EGL.contextAttributes.antialias = (samples > 0);
            } else if (param == 0x3032 /*EGL_SAMPLE_BUFFERS*/) {
              var samples = HEAP32[(((attribList)+(4))>>2)];
              EGL.contextAttributes.antialias = (samples == 1);
            } else if (param == 0x3100 /*EGL_CONTEXT_PRIORITY_LEVEL_IMG*/) {
              var requestedPriority = HEAP32[(((attribList)+(4))>>2)];
              EGL.contextAttributes.lowLatency = (requestedPriority != 0x3103 /*EGL_CONTEXT_PRIORITY_LOW_IMG*/);
            } else if (param == 0x3038 /*EGL_NONE*/) {
                break;
            }
            attribList += 8;
          }
        }
  
        if ((!config || !config_size) && !numConfigs) {
          EGL.setErrorCode(0x300C /* EGL_BAD_PARAMETER */);
          return 0;
        }
        if (numConfigs) {
          HEAP32[((numConfigs)>>2)] = 1; // Total number of supported configs: 1.
        }
        if (config && config_size > 0) {
          HEAPU32[((config)>>2)] = 62002;
        }
  
        EGL.setErrorCode(0x3000 /* EGL_SUCCESS */);
        return 1;
      },
  };
  var _eglBindAPI = (api) => {
      if (api == 0x30A0 /* EGL_OPENGL_ES_API */) {
        EGL.setErrorCode(0x3000 /* EGL_SUCCESS */);
        return 1;
      }
      // if (api == 0x30A1 /* EGL_OPENVG_API */ || api == 0x30A2 /* EGL_OPENGL_API */) {
      EGL.setErrorCode(0x300C /* EGL_BAD_PARAMETER */);
      return 0;
    };

  var _eglChooseConfig = (display, attrib_list, configs, config_size, numConfigs) =>
      EGL.chooseConfig(display, attrib_list, configs, config_size, numConfigs);

  var GLctx;
  
  var webgl_enable_ANGLE_instanced_arrays = (ctx) => {
      // Extension available in WebGL 1 from Firefox 26 and Google Chrome 30 onwards. Core feature in WebGL 2.
      var ext = ctx.getExtension('ANGLE_instanced_arrays');
      // Because this extension is a core function in WebGL 2, assign the extension entry points in place of
      // where the core functions will reside in WebGL 2. This way the calling code can call these without
      // having to dynamically branch depending if running against WebGL 1 or WebGL 2.
      if (ext) {
        ctx['vertexAttribDivisor'] = (index, divisor) => ext['vertexAttribDivisorANGLE'](index, divisor);
        ctx['drawArraysInstanced'] = (mode, first, count, primcount) => ext['drawArraysInstancedANGLE'](mode, first, count, primcount);
        ctx['drawElementsInstanced'] = (mode, count, type, indices, primcount) => ext['drawElementsInstancedANGLE'](mode, count, type, indices, primcount);
        return 1;
      }
    };
  
  var webgl_enable_OES_vertex_array_object = (ctx) => {
      // Extension available in WebGL 1 from Firefox 25 and WebKit 536.28/desktop Safari 6.0.3 onwards. Core feature in WebGL 2.
      var ext = ctx.getExtension('OES_vertex_array_object');
      if (ext) {
        ctx['createVertexArray'] = () => ext['createVertexArrayOES']();
        ctx['deleteVertexArray'] = (vao) => ext['deleteVertexArrayOES'](vao);
        ctx['bindVertexArray'] = (vao) => ext['bindVertexArrayOES'](vao);
        ctx['isVertexArray'] = (vao) => ext['isVertexArrayOES'](vao);
        return 1;
      }
    };
  
  var webgl_enable_WEBGL_draw_buffers = (ctx) => {
      // Extension available in WebGL 1 from Firefox 28 onwards. Core feature in WebGL 2.
      var ext = ctx.getExtension('WEBGL_draw_buffers');
      if (ext) {
        ctx['drawBuffers'] = (n, bufs) => ext['drawBuffersWEBGL'](n, bufs);
        return 1;
      }
    };
  
  var webgl_enable_WEBGL_draw_instanced_base_vertex_base_instance = (ctx) =>
      // Closure is expected to be allowed to minify the '.dibvbi' property, so not accessing it quoted.
      !!(ctx.dibvbi = ctx.getExtension('WEBGL_draw_instanced_base_vertex_base_instance'));
  
  var webgl_enable_WEBGL_multi_draw_instanced_base_vertex_base_instance = (ctx) => {
      // Closure is expected to be allowed to minify the '.mdibvbi' property, so not accessing it quoted.
      return !!(ctx.mdibvbi = ctx.getExtension('WEBGL_multi_draw_instanced_base_vertex_base_instance'));
    };
  
  var webgl_enable_EXT_polygon_offset_clamp = (ctx) =>
      !!(ctx.extPolygonOffsetClamp = ctx.getExtension('EXT_polygon_offset_clamp'));
  
  var webgl_enable_EXT_clip_control = (ctx) =>
      !!(ctx.extClipControl = ctx.getExtension('EXT_clip_control'));
  
  var webgl_enable_WEBGL_polygon_mode = (ctx) =>
      !!(ctx.webglPolygonMode = ctx.getExtension('WEBGL_polygon_mode'));
  
  var webgl_enable_WEBGL_multi_draw = (ctx) =>
      // Closure is expected to be allowed to minify the '.multiDrawWebgl' property, so not accessing it quoted.
      !!(ctx.multiDrawWebgl = ctx.getExtension('WEBGL_multi_draw'));
  
  var getEmscriptenSupportedExtensions = (ctx) => {
      // Restrict the list of advertised extensions to those that we actually
      // support.
      var supportedExtensions = [
        // WebGL 1 extensions
        'ANGLE_instanced_arrays',
        'EXT_blend_minmax',
        'EXT_disjoint_timer_query',
        'EXT_frag_depth',
        'EXT_shader_texture_lod',
        'EXT_sRGB',
        'OES_element_index_uint',
        'OES_fbo_render_mipmap',
        'OES_standard_derivatives',
        'OES_texture_float',
        'OES_texture_half_float',
        'OES_texture_half_float_linear',
        'OES_vertex_array_object',
        'WEBGL_color_buffer_float',
        'WEBGL_depth_texture',
        'WEBGL_draw_buffers',
        // WebGL 2 extensions
        'EXT_color_buffer_float',
        'EXT_conservative_depth',
        'EXT_disjoint_timer_query_webgl2',
        'EXT_texture_norm16',
        'NV_shader_noperspective_interpolation',
        'WEBGL_clip_cull_distance',
        // WebGL 1 and WebGL 2 extensions
        'EXT_clip_control',
        'EXT_color_buffer_half_float',
        'EXT_depth_clamp',
        'EXT_float_blend',
        'EXT_polygon_offset_clamp',
        'EXT_texture_compression_bptc',
        'EXT_texture_compression_rgtc',
        'EXT_texture_filter_anisotropic',
        'KHR_parallel_shader_compile',
        'OES_texture_float_linear',
        'WEBGL_blend_func_extended',
        'WEBGL_compressed_texture_astc',
        'WEBGL_compressed_texture_etc',
        'WEBGL_compressed_texture_etc1',
        'WEBGL_compressed_texture_s3tc',
        'WEBGL_compressed_texture_s3tc_srgb',
        'WEBGL_debug_renderer_info',
        'WEBGL_debug_shaders',
        'WEBGL_lose_context',
        'WEBGL_multi_draw',
        'WEBGL_polygon_mode'
      ];
      // .getSupportedExtensions() can return null if context is lost, so coerce to empty array.
      return ctx.getSupportedExtensions()?.filter(ext => supportedExtensions.includes(ext)) ?? [];
    };
  
  var registerPreMainLoop = (f) => {
      // Does nothing unless $MainLoop is included/used.
      typeof MainLoop != 'undefined' && MainLoop.preMainLoop.push(f);
    };
  
  
  var webglBufferSubData = (target, offset, size, data, src = HEAPU8) => {
      if (GL.currentContext.version >= 2) {
        size && GLctx.bufferSubData(target, offset, src, data, size);
        return;
      }
      GLctx.bufferSubData(target, offset, src.subarray(data, data + size));
    };
  
  
  
  
  var GL = {
  counter:1,
  buffers:[],
  mappedBuffers:{
  },
  programs:[],
  framebuffers:[],
  renderbuffers:[],
  textures:[],
  shaders:[],
  vaos:[],
  contexts:[],
  offscreenCanvases:{
  },
  queries:[],
  samplers:[],
  transformFeedbacks:[],
  syncs:[],
  byteSizeByTypeRoot:5120,
  byteSizeByType:[1,1,2,2,4,4,4,2,3,4,8],
  stringCache:{
  },
  stringiCache:{
  },
  unpackAlignment:4,
  unpackRowLength:0,
  recordError:(errorCode) => {
        if (!GL.lastError) {
          GL.lastError = errorCode;
        }
      },
  getNewId:(table) => {
        var ret = GL.counter++;
        for (var i = table.length; i < ret; i++) {
          table[i] = null;
        }
        // Skip over any non-null elements that might have been created by
        // glBindBuffer.
        while (table[ret]) {
          ret = GL.counter++;
        }
        return ret;
      },
  genObject:(n, buffers, createFunction, objectTable
        ) => {
        for (var i = 0; i < n; i++) {
          var buffer = GLctx[createFunction]();
          var id = buffer && GL.getNewId(objectTable);
          if (buffer) {
            buffer.name = id;
            objectTable[id] = buffer;
          } else {
            GL.recordError(0x502 /* GL_INVALID_OPERATION */);
          }
          HEAP32[(((buffers)+(i*4))>>2)] = id;
        }
      },
  MAX_TEMP_BUFFER_SIZE:2097152,
  numTempVertexBuffersPerSize:64,
  log2ceilLookup:(i) => 32 - Math.clz32(i ? i - 1 : 0),
  generateTempBuffers:(quads, context) => {
        var largestIndex = GL.log2ceilLookup(GL.MAX_TEMP_BUFFER_SIZE);
        context.tempVertexBufferCounters1 = [];
        context.tempVertexBufferCounters2 = [];
        context.tempVertexBufferCounters1.length = context.tempVertexBufferCounters2.length = largestIndex+1;
        context.tempVertexBuffers1 = [];
        context.tempVertexBuffers2 = [];
        context.tempVertexBuffers1.length = context.tempVertexBuffers2.length = largestIndex+1;
        context.tempIndexBuffers = [];
        context.tempIndexBuffers.length = largestIndex+1;
        for (var i = 0; i <= largestIndex; ++i) {
          context.tempIndexBuffers[i] = null; // Created on-demand
          context.tempVertexBufferCounters1[i] = context.tempVertexBufferCounters2[i] = 0;
          var ringbufferLength = GL.numTempVertexBuffersPerSize;
          context.tempVertexBuffers1[i] = [];
          context.tempVertexBuffers2[i] = [];
          var ringbuffer1 = context.tempVertexBuffers1[i];
          var ringbuffer2 = context.tempVertexBuffers2[i];
          ringbuffer1.length = ringbuffer2.length = ringbufferLength;
          for (var j = 0; j < ringbufferLength; ++j) {
            ringbuffer1[j] = ringbuffer2[j] = null; // Created on-demand
          }
        }
  
        if (quads) {
          // GL_QUAD indexes can be precalculated
          context.tempQuadIndexBuffer = GLctx.createBuffer();
          context.GLctx.bindBuffer(0x8893 /*GL_ELEMENT_ARRAY_BUFFER*/, context.tempQuadIndexBuffer);
          var numIndexes = GL.MAX_TEMP_BUFFER_SIZE >> 1;
          var quadIndexes = new Uint16Array(numIndexes);
          var i = 0, v = 0;
          while (1) {
            quadIndexes[i++] = v;
            if (i >= numIndexes) break;
            quadIndexes[i++] = v+1;
            if (i >= numIndexes) break;
            quadIndexes[i++] = v+2;
            if (i >= numIndexes) break;
            quadIndexes[i++] = v;
            if (i >= numIndexes) break;
            quadIndexes[i++] = v+2;
            if (i >= numIndexes) break;
            quadIndexes[i++] = v+3;
            if (i >= numIndexes) break;
            v += 4;
          }
          context.GLctx.bufferData(0x8893 /*GL_ELEMENT_ARRAY_BUFFER*/, quadIndexes, 0x88E4 /*GL_STATIC_DRAW*/);
          context.GLctx.bindBuffer(0x8893 /*GL_ELEMENT_ARRAY_BUFFER*/, null);
        }
      },
  getTempVertexBuffer:(sizeBytes) => {
        var idx = GL.log2ceilLookup(sizeBytes);
        var ringbuffer = GL.currentContext.tempVertexBuffers1[idx];
        var nextFreeBufferIndex = GL.currentContext.tempVertexBufferCounters1[idx];
        GL.currentContext.tempVertexBufferCounters1[idx] = (GL.currentContext.tempVertexBufferCounters1[idx]+1) & (GL.numTempVertexBuffersPerSize-1);
        var vbo = ringbuffer[nextFreeBufferIndex];
        if (vbo) {
          return vbo;
        }
        var prevVBO = GLctx.getParameter(0x8894 /*GL_ARRAY_BUFFER_BINDING*/);
        ringbuffer[nextFreeBufferIndex] = GLctx.createBuffer();
        GLctx.bindBuffer(0x8892 /*GL_ARRAY_BUFFER*/, ringbuffer[nextFreeBufferIndex]);
        GLctx.bufferData(0x8892 /*GL_ARRAY_BUFFER*/, 1 << idx, 0x88E8 /*GL_DYNAMIC_DRAW*/);
        GLctx.bindBuffer(0x8892 /*GL_ARRAY_BUFFER*/, prevVBO);
        return ringbuffer[nextFreeBufferIndex];
      },
  getTempIndexBuffer:(sizeBytes) => {
        var idx = GL.log2ceilLookup(sizeBytes);
        var ibo = GL.currentContext.tempIndexBuffers[idx];
        if (ibo) {
          return ibo;
        }
        var prevIBO = GLctx.getParameter(0x8895 /*ELEMENT_ARRAY_BUFFER_BINDING*/);
        GL.currentContext.tempIndexBuffers[idx] = GLctx.createBuffer();
        GLctx.bindBuffer(0x8893 /*GL_ELEMENT_ARRAY_BUFFER*/, GL.currentContext.tempIndexBuffers[idx]);
        GLctx.bufferData(0x8893 /*GL_ELEMENT_ARRAY_BUFFER*/, 1 << idx, 0x88E8 /*GL_DYNAMIC_DRAW*/);
        GLctx.bindBuffer(0x8893 /*GL_ELEMENT_ARRAY_BUFFER*/, prevIBO);
        return GL.currentContext.tempIndexBuffers[idx];
      },
  newRenderingFrameStarted:() => {
        if (!GL.currentContext) {
          return;
        }
        var vb = GL.currentContext.tempVertexBuffers1;
        GL.currentContext.tempVertexBuffers1 = GL.currentContext.tempVertexBuffers2;
        GL.currentContext.tempVertexBuffers2 = vb;
        vb = GL.currentContext.tempVertexBufferCounters1;
        GL.currentContext.tempVertexBufferCounters1 = GL.currentContext.tempVertexBufferCounters2;
        GL.currentContext.tempVertexBufferCounters2 = vb;
        var largestIndex = GL.log2ceilLookup(GL.MAX_TEMP_BUFFER_SIZE);
        for (var i = 0; i <= largestIndex; ++i) {
          GL.currentContext.tempVertexBufferCounters1[i] = 0;
        }
      },
  getSource:(shader, count, string, length) => {
        var source = '';
        for (var i = 0; i < count; ++i) {
          var len = length ? HEAPU32[(((length)+(i*4))>>2)] : undefined;
          source += UTF8ToString(HEAPU32[(((string)+(i*4))>>2)], len);
        }
        return source;
      },
  calcBufLength:(size, type, stride, count) => {
        if (stride > 0) {
          return count * stride;  // XXXvlad this is not exactly correct I don't think
        }
        var typeSize = GL.byteSizeByType[type - GL.byteSizeByTypeRoot];
        return size * typeSize * count;
      },
  usedTempBuffers:[],
  preDrawHandleClientVertexAttribBindings:(count) => {
        GL.resetBufferBinding = false;
  
        // TODO: initial pass to detect ranges we need to upload, might not need
        // an upload per attrib
        for (var i = 0; i < GL.currentContext.maxVertexAttribs; ++i) {
          var cb = GL.currentContext.clientBuffers[i];
          if (!cb.clientside || !cb.enabled) continue;
  
          assert(count || !GLctx.currentElementArrayBufferBinding, 'must use array buffers when using element buffer');
  
          GL.resetBufferBinding = true;
  
          var size = GL.calcBufLength(cb.size, cb.type, cb.stride, count);
          var buf = GL.getTempVertexBuffer(size);
          GLctx.bindBuffer(0x8892 /*GL_ARRAY_BUFFER*/, buf);
          webglBufferSubData(0x8892 /*GL_ARRAY_BUFFER*/, 0, size, cb.ptr);
          cb.vertexAttribPointerAdaptor.call(GLctx, i, cb.size, cb.type, cb.normalized, cb.stride, 0);
        }
      },
  postDrawHandleClientVertexAttribBindings:() => {
        if (GL.resetBufferBinding) {
          GLctx.bindBuffer(0x8892 /*GL_ARRAY_BUFFER*/, GL.buffers[GLctx.currentArrayBufferBinding]);
        }
      },
  createContext:(/** @type {HTMLCanvasElement} */ canvas, webGLContextAttributes) => {
        // In proxied operation mode, rAF()/setTimeout() functions do not delimit
        // frame boundaries, so can't have WebGL implementation try to detect when
        // it's ok to discard contents of the rendered backbuffer.
        if (webGLContextAttributes.renderViaOffscreenBackBuffer) webGLContextAttributes['preserveDrawingBuffer'] = true;
  
        // If WebGL context has already been preinitialized for the page on the JS
        // side, reuse that context instead. This is useful for example when the
        // main page precompiles shaders for the application, in which case the
        // WebGL context is created already before any Emscripten compiled code
        // has been downloaded.
        if (Module['preinitializedWebGLContext']) {
          var ctx = Module['preinitializedWebGLContext'];
          // The ctx object may not be of a known class (e.g. it may be a debug
          // wrapper), so we ask it for its version rather than use instanceof.
          webGLContextAttributes.majorVersion = Number(ctx.getParameter(ctx.VERSION).match(/^WebGL (\d+).\d+/)[1]);
        } else {
  
        // BUG: Workaround Safari WebGL issue: After successfully acquiring WebGL
        // context on a canvas, calling .getContext() will always return that
        // context independent of which 'webgl' or 'webgl2'
        // context version was passed. See:
        //   https://webkit.org/b/222758
        // and:
        //   https://github.com/emscripten-core/emscripten/issues/13295.
        // TODO: Once the bug is fixed and shipped in Safari, adjust the Safari
        // version field in above check.
        if (!canvas.getContextSafariWebGL2Fixed) {
          canvas.getContextSafariWebGL2Fixed = canvas.getContext;
          /** @type {function(this:HTMLCanvasElement, string, (Object|null)=): (Object|null)} */
          function fixedGetContext(ver, attrs) {
            var gl = canvas.getContextSafariWebGL2Fixed(ver, attrs);
            return ((ver == 'webgl') == (gl instanceof WebGLRenderingContext)) ? gl : null;
          }
          canvas.getContext = fixedGetContext;
        }
  
        var ctx =
          (webGLContextAttributes.majorVersion > 1)
          ? canvas.getContext('webgl2', webGLContextAttributes) :
          canvas.getContext('webgl', webGLContextAttributes);
  
        }
  
        if (!ctx) return 0;
  
        var handle = GL.registerContext(ctx, webGLContextAttributes);
  
        return handle;
      },
  enableOffscreenFramebufferAttributes:(webGLContextAttributes) => {
        webGLContextAttributes.renderViaOffscreenBackBuffer = true;
        webGLContextAttributes.preserveDrawingBuffer = true;
      },
  createOffscreenFramebuffer:(context) => {
        var gl = context.GLctx;
  
        // Create FBO
        var fbo = gl.createFramebuffer();
        gl.bindFramebuffer(0x8D40 /*GL_FRAMEBUFFER*/, fbo);
        context.defaultFbo = fbo;
  
        context.defaultFboForbidBlitFramebuffer = false;
        if (gl.getContextAttributes().antialias) {
          context.defaultFboForbidBlitFramebuffer = true;
        }
  
        // Create render targets to the FBO
        context.defaultColorTarget = gl.createTexture();
        context.defaultDepthTarget = gl.createRenderbuffer();
        // Size them up correctly (use the same mechanism when resizing on demand)
        GL.resizeOffscreenFramebuffer(context);
  
        gl.bindTexture(0xDE1 /*GL_TEXTURE_2D*/, context.defaultColorTarget);
        gl.texParameteri(0xDE1 /*GL_TEXTURE_2D*/, 0x2801 /*GL_TEXTURE_MIN_FILTER*/, 0x2600 /*GL_NEAREST*/);
        gl.texParameteri(0xDE1 /*GL_TEXTURE_2D*/, 0x2800 /*GL_TEXTURE_MAG_FILTER*/, 0x2600 /*GL_NEAREST*/);
        gl.texParameteri(0xDE1 /*GL_TEXTURE_2D*/, 0x2802 /*GL_TEXTURE_WRAP_S*/, 0x812F /*GL_CLAMP_TO_EDGE*/);
        gl.texParameteri(0xDE1 /*GL_TEXTURE_2D*/, 0x2803 /*GL_TEXTURE_WRAP_T*/, 0x812F /*GL_CLAMP_TO_EDGE*/);
        gl.texImage2D(0xDE1 /*GL_TEXTURE_2D*/, 0, 0x1908 /*GL_RGBA*/, gl.canvas.width, gl.canvas.height, 0, 0x1908 /*GL_RGBA*/, 0x1401 /*GL_UNSIGNED_BYTE*/, null);
        gl.framebufferTexture2D(0x8D40 /*GL_FRAMEBUFFER*/, 0x8CE0 /*GL_COLOR_ATTACHMENT0*/, 0xDE1 /*GL_TEXTURE_2D*/, context.defaultColorTarget, 0);
        gl.bindTexture(0xDE1 /*GL_TEXTURE_2D*/, null);
  
        // Create depth render target to the FBO
        var depthTarget = gl.createRenderbuffer();
        gl.bindRenderbuffer(0x8D41 /*GL_RENDERBUFFER*/, context.defaultDepthTarget);
        gl.renderbufferStorage(0x8D41 /*GL_RENDERBUFFER*/, 0x81A5 /*GL_DEPTH_COMPONENT16*/, gl.canvas.width, gl.canvas.height);
        gl.framebufferRenderbuffer(0x8D40 /*GL_FRAMEBUFFER*/, 0x8D00 /*GL_DEPTH_ATTACHMENT*/, 0x8D41 /*GL_RENDERBUFFER*/, context.defaultDepthTarget);
        gl.bindRenderbuffer(0x8D41 /*GL_RENDERBUFFER*/, null);
  
        // Create blitter
        var vertices = [
          -1, -1,
          -1,  1,
           1, -1,
           1,  1
        ];
        var vb = gl.createBuffer();
        gl.bindBuffer(0x8892 /*GL_ARRAY_BUFFER*/, vb);
        gl.bufferData(0x8892 /*GL_ARRAY_BUFFER*/, new Float32Array(vertices), 0x88E4 /*GL_STATIC_DRAW*/);
        gl.bindBuffer(0x8892 /*GL_ARRAY_BUFFER*/, null);
        context.blitVB = vb;
  
        var vsCode =
          'attribute vec2 pos;' +
          'varying lowp vec2 tex;' +
          'void main() { tex = pos * 0.5 + vec2(0.5,0.5); gl_Position = vec4(pos, 0.0, 1.0); }';
        var vs = gl.createShader(0x8B31 /*GL_VERTEX_SHADER*/);
        gl.shaderSource(vs, vsCode);
        gl.compileShader(vs);
  
        var fsCode =
          'varying lowp vec2 tex;' +
          'uniform sampler2D sampler;' +
          'void main() { gl_FragColor = texture2D(sampler, tex); }';
        var fs = gl.createShader(0x8B30 /*GL_FRAGMENT_SHADER*/);
        gl.shaderSource(fs, fsCode);
        gl.compileShader(fs);
  
        var blitProgram = gl.createProgram();
        gl.attachShader(blitProgram, vs);
        gl.attachShader(blitProgram, fs);
        gl.linkProgram(blitProgram);
        context.blitProgram = blitProgram;
        context.blitPosLoc = gl.getAttribLocation(blitProgram, 'pos');
        gl.useProgram(blitProgram);
        gl.uniform1i(gl.getUniformLocation(blitProgram, 'sampler'), 0);
        gl.useProgram(null);
  
        if (gl.createVertexArray) {
          context.defaultVao = gl.createVertexArray();
          gl.bindVertexArray(context.defaultVao);
          gl.enableVertexAttribArray(context.blitPosLoc);
          gl.bindVertexArray(null);
        }
      },
  resizeOffscreenFramebuffer:(context) => {
        var gl = context.GLctx;
  
        // Resize color buffer
        if (context.defaultColorTarget) {
          var prevTextureBinding = gl.getParameter(0x8069 /*GL_TEXTURE_BINDING_2D*/);
          gl.bindTexture(0xDE1 /*GL_TEXTURE_2D*/, context.defaultColorTarget);
          gl.texImage2D(0xDE1 /*GL_TEXTURE_2D*/, 0, 0x1908 /*GL_RGBA*/, gl.drawingBufferWidth, gl.drawingBufferHeight, 0, 0x1908 /*GL_RGBA*/, 0x1401 /*GL_UNSIGNED_BYTE*/, null);
          gl.bindTexture(0xDE1 /*GL_TEXTURE_2D*/, prevTextureBinding);
        }
  
        // Resize depth buffer
        if (context.defaultDepthTarget) {
          var prevRenderBufferBinding = gl.getParameter(0x8CA7 /*GL_RENDERBUFFER_BINDING*/);
          gl.bindRenderbuffer(0x8D41 /*GL_RENDERBUFFER*/, context.defaultDepthTarget);
          gl.renderbufferStorage(0x8D41 /*GL_RENDERBUFFER*/, 0x81A5 /*GL_DEPTH_COMPONENT16*/, gl.drawingBufferWidth, gl.drawingBufferHeight); // TODO: Read context creation parameters for what type of depth and stencil to use
          gl.bindRenderbuffer(0x8D41 /*GL_RENDERBUFFER*/, prevRenderBufferBinding);
        }
      },
  blitOffscreenFramebuffer:(context) => {
        var gl = context.GLctx;
  
        var prevScissorTest = gl.getParameter(0xC11 /*GL_SCISSOR_TEST*/);
        if (prevScissorTest) gl.disable(0xC11 /*GL_SCISSOR_TEST*/);
  
        var prevFbo = gl.getParameter(0x8CA6 /*GL_FRAMEBUFFER_BINDING*/);
  
        if (gl.blitFramebuffer && !context.defaultFboForbidBlitFramebuffer) {
          gl.bindFramebuffer(0x8CA8 /*GL_READ_FRAMEBUFFER*/, context.defaultFbo);
          gl.bindFramebuffer(0x8CA9 /*GL_DRAW_FRAMEBUFFER*/, null);
          gl.blitFramebuffer(0, 0, gl.canvas.width, gl.canvas.height,
                             0, 0, gl.canvas.width, gl.canvas.height,
                             0x4000 /*GL_COLOR_BUFFER_BIT*/, 0x2600/*GL_NEAREST*/);
        }
        else
        {
          gl.bindFramebuffer(0x8D40 /*GL_FRAMEBUFFER*/, null);
  
          var prevProgram = gl.getParameter(0x8B8D /*GL_CURRENT_PROGRAM*/);
          gl.useProgram(context.blitProgram);
          // If prevProgram was already marked for deletion, then, since it was
          // still bound, it was not *actually* deleted. Binding a new program
          // just now, thus, deleted the old one. This makes it impossible to
          // restore. Hopefully the application didn't actually need it!
          if (!gl.isProgram(prevProgram)) prevProgram = null;
  
          var prevVB = gl.getParameter(0x8894 /*GL_ARRAY_BUFFER_BINDING*/);
          gl.bindBuffer(0x8892 /*GL_ARRAY_BUFFER*/, context.blitVB);
  
          var prevActiveTexture = gl.getParameter(0x84E0 /*GL_ACTIVE_TEXTURE*/);
          gl.activeTexture(0x84C0 /*GL_TEXTURE0*/);
  
          var prevTextureBinding = gl.getParameter(0x8069 /*GL_TEXTURE_BINDING_2D*/);
          gl.bindTexture(0xDE1 /*GL_TEXTURE_2D*/, context.defaultColorTarget);
  
          var prevBlend = gl.getParameter(0xBE2 /*GL_BLEND*/);
          if (prevBlend) gl.disable(0xBE2 /*GL_BLEND*/);
  
          var prevCullFace = gl.getParameter(0xB44 /*GL_CULL_FACE*/);
          if (prevCullFace) gl.disable(0xB44 /*GL_CULL_FACE*/);
  
          var prevDepthTest = gl.getParameter(0xB71 /*GL_DEPTH_TEST*/);
          if (prevDepthTest) gl.disable(0xB71 /*GL_DEPTH_TEST*/);
  
          var prevStencilTest = gl.getParameter(0xB90 /*GL_STENCIL_TEST*/);
          if (prevStencilTest) gl.disable(0xB90 /*GL_STENCIL_TEST*/);
  
          function draw() {
            gl.vertexAttribPointer(context.blitPosLoc, 2, 0x1406 /*GL_FLOAT*/, false, 0, 0);
            gl.drawArrays(5/*GL_TRIANGLE_STRIP*/, 0, 4);
          }
  
          if (context.defaultVao) {
            // WebGL 2 or OES_vertex_array_object
            var prevVAO = gl.getParameter(0x85B5 /*GL_VERTEX_ARRAY_BINDING*/);
            gl.bindVertexArray(context.defaultVao);
            draw();
            gl.bindVertexArray(prevVAO);
          } else {
            var prevVertexAttribPointer = {
              buffer: gl.getVertexAttrib(context.blitPosLoc, 0x889F /*GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING*/),
              size: gl.getVertexAttrib(context.blitPosLoc, 0x8623 /*GL_VERTEX_ATTRIB_ARRAY_SIZE*/),
              stride: gl.getVertexAttrib(context.blitPosLoc, 0x8624 /*GL_VERTEX_ATTRIB_ARRAY_STRIDE*/),
              type: gl.getVertexAttrib(context.blitPosLoc, 0x8625 /*GL_VERTEX_ATTRIB_ARRAY_TYPE*/),
              normalized: gl.getVertexAttrib(context.blitPosLoc, 0x886A /*GL_VERTEX_ATTRIB_ARRAY_NORMALIZED*/),
              pointer: gl.getVertexAttribOffset(context.blitPosLoc, 0x8645 /*GL_VERTEX_ATTRIB_ARRAY_POINTER*/),
            };
            var maxVertexAttribs = gl.getParameter(0x8869 /*GL_MAX_VERTEX_ATTRIBS*/);
            var prevVertexAttribEnables = [];
            for (var i = 0; i < maxVertexAttribs; ++i) {
              var prevEnabled = gl.getVertexAttrib(i, 0x8622 /*GL_VERTEX_ATTRIB_ARRAY_ENABLED*/);
              var wantEnabled = i == context.blitPosLoc;
              if (prevEnabled && !wantEnabled) {
                gl.disableVertexAttribArray(i);
              }
              if (!prevEnabled && wantEnabled) {
                gl.enableVertexAttribArray(i);
              }
              prevVertexAttribEnables[i] = prevEnabled;
            }
  
            draw();
  
            for (var i = 0; i < maxVertexAttribs; ++i) {
              var prevEnabled = prevVertexAttribEnables[i];
              var nowEnabled = i == context.blitPosLoc;
              if (prevEnabled && !nowEnabled) {
                gl.enableVertexAttribArray(i);
              }
              if (!prevEnabled && nowEnabled) {
                gl.disableVertexAttribArray(i);
              }
            }
            gl.bindBuffer(0x8892 /*GL_ARRAY_BUFFER*/, prevVertexAttribPointer.buffer);
            gl.vertexAttribPointer(context.blitPosLoc,
                                   prevVertexAttribPointer.size,
                                   prevVertexAttribPointer.type,
                                   prevVertexAttribPointer.normalized,
                                   prevVertexAttribPointer.stride,
                                   prevVertexAttribPointer.offset);
          }
  
          if (prevStencilTest) gl.enable(0xB90 /*GL_STENCIL_TEST*/);
          if (prevDepthTest) gl.enable(0xB71 /*GL_DEPTH_TEST*/);
          if (prevCullFace) gl.enable(0xB44 /*GL_CULL_FACE*/);
          if (prevBlend) gl.enable(0xBE2 /*GL_BLEND*/);
  
          gl.bindTexture(0xDE1 /*GL_TEXTURE_2D*/, prevTextureBinding);
          gl.activeTexture(prevActiveTexture);
          gl.bindBuffer(0x8892 /*GL_ARRAY_BUFFER*/, prevVB);
          gl.useProgram(prevProgram);
        }
        gl.bindFramebuffer(0x8D40 /*GL_FRAMEBUFFER*/, prevFbo);
        if (prevScissorTest) gl.enable(0xC11 /*GL_SCISSOR_TEST*/);
      },
  registerContext:(ctx, webGLContextAttributes) => {
        // without pthreads a context is just an integer ID
        var handle = GL.getNewId(GL.contexts);
  
        var context = {
          handle,
          attributes: webGLContextAttributes,
          version: webGLContextAttributes.majorVersion,
          GLctx: ctx
        };
  
        // Store the created context object so that we can access the context
        // given a canvas without having to pass the parameters again.
        if (ctx.canvas) ctx.canvas.GLctxObject = context;
        GL.contexts[handle] = context;
        if (typeof webGLContextAttributes.enableExtensionsByDefault == 'undefined' || webGLContextAttributes.enableExtensionsByDefault) {
          GL.initExtensions(context);
        }
  
        context.maxVertexAttribs = context.GLctx.getParameter(0x8869 /*GL_MAX_VERTEX_ATTRIBS*/);
        context.clientBuffers = [];
        for (var i = 0; i < context.maxVertexAttribs; i++) {
          context.clientBuffers[i] = {
            enabled: false,
            clientside: false,
            size: 0,
            type: 0,
            normalized: 0,
            stride: 0,
            ptr: 0,
            vertexAttribPointerAdaptor: null,
          };
        }
  
        GL.generateTempBuffers(false, context);
  
        if (webGLContextAttributes.renderViaOffscreenBackBuffer) GL.createOffscreenFramebuffer(context);
        return handle;
      },
  makeContextCurrent:(contextHandle) => {
  
        // Active Emscripten GL layer context object.
        GL.currentContext = GL.contexts[contextHandle];
        // Active WebGL context object.
        Module['ctx'] = GLctx = GL.currentContext?.GLctx;
        return !(contextHandle && !GLctx);
      },
  getContext:(contextHandle) => {
        return GL.contexts[contextHandle];
      },
  deleteContext:(contextHandle) => {
        if (GL.currentContext === GL.contexts[contextHandle]) {
          GL.currentContext = null;
        }
        if (typeof JSEvents == 'object') {
          // Release all JS event handlers on the DOM element that the GL context is
          // associated with since the context is now deleted.
          JSEvents.removeAllHandlersOnTarget(GL.contexts[contextHandle].GLctx.canvas);
        }
        // Make sure the canvas object no longer refers to the context object so
        // there are no GC surprises.
        if (GL.contexts[contextHandle]?.GLctx.canvas) {
          GL.contexts[contextHandle].GLctx.canvas.GLctxObject = undefined;
        }
        GL.contexts[contextHandle] = null;
      },
  initExtensions:(context) => {
        // If this function is called without a specific context object, init the
        // extensions of the currently active context.
        context ||= GL.currentContext;
  
        if (context.initExtensionsDone) return;
        context.initExtensionsDone = true;
  
        var GLctx = context.GLctx;
  
        // Detect the presence of a few extensions manually, since the GL interop
        // layer itself will need to know if they exist.
  
        // Extensions that are available in both WebGL 1 and WebGL 2
        webgl_enable_WEBGL_multi_draw(GLctx);
        webgl_enable_EXT_polygon_offset_clamp(GLctx);
        webgl_enable_EXT_clip_control(GLctx);
        webgl_enable_WEBGL_polygon_mode(GLctx);
        // Extensions that are only available in WebGL 1 (the calls will be no-ops
        // if called on a WebGL 2 context active)
        webgl_enable_ANGLE_instanced_arrays(GLctx);
        webgl_enable_OES_vertex_array_object(GLctx);
        webgl_enable_WEBGL_draw_buffers(GLctx);
        // Extensions that are available from WebGL >= 2 (no-op if called on a WebGL 1 context active)
        webgl_enable_WEBGL_draw_instanced_base_vertex_base_instance(GLctx);
        webgl_enable_WEBGL_multi_draw_instanced_base_vertex_base_instance(GLctx);
  
        // On WebGL 2, EXT_disjoint_timer_query is replaced with an alternative
        // that's based on core APIs, and exposes only the queryCounterEXT()
        // entrypoint.
        if (context.version >= 2) {
          GLctx.disjointTimerQueryExt = GLctx.getExtension('EXT_disjoint_timer_query_webgl2');
        }
  
        // However, Firefox exposes the WebGL 1 version on WebGL 2 as well and
        // thus we look for the WebGL 1 version again if the WebGL 2 version
        // isn't present. https://bugzil.la/1328882
        if (context.version < 2 || !GLctx.disjointTimerQueryExt)
        {
          GLctx.disjointTimerQueryExt = GLctx.getExtension('EXT_disjoint_timer_query');
        }
  
        for (var ext of getEmscriptenSupportedExtensions(GLctx)) {
          // WEBGL_lose_context, WEBGL_debug_renderer_info and WEBGL_debug_shaders
          // are not enabled by default.
          if (!ext.includes('lose_context') && !ext.includes('debug')) {
            // Call .getExtension() to enable that extension permanently.
            GLctx.getExtension(ext);
          }
        }
      },
  };
  
  
  var _eglCreateContext = (display, config, hmm, contextAttribs) => {
      if (display != 62000) {
        EGL.setErrorCode(0x3008 /* EGL_BAD_DISPLAY */);
        return 0;
      }
  
      // EGL 1.4 spec says default EGL_CONTEXT_CLIENT_VERSION is GLES1, but this is not supported by Emscripten.
      // So user must pass EGL_CONTEXT_CLIENT_VERSION == 2 to initialize EGL.
      var glesContextVersion = 1;
      for (;;) {
        var param = HEAP32[((contextAttribs)>>2)];
        if (param == 0x3098 /*EGL_CONTEXT_CLIENT_VERSION*/) {
          glesContextVersion = HEAP32[(((contextAttribs)+(4))>>2)];
        } else if (param == 0x3038 /*EGL_NONE*/) {
          break;
        } else {
          /* EGL1.4 specifies only EGL_CONTEXT_CLIENT_VERSION as supported attribute */
          EGL.setErrorCode(0x3004 /*EGL_BAD_ATTRIBUTE*/);
          return 0;
        }
        contextAttribs += 8;
      }
      if (glesContextVersion < 2 || glesContextVersion > 3) {
        EGL.setErrorCode(0x3005 /* EGL_BAD_CONFIG */);
        return 0; /* EGL_NO_CONTEXT */
      }
  
      EGL.contextAttributes.majorVersion = glesContextVersion - 1; // WebGL 1 is GLES 2, WebGL2 is GLES3
      EGL.contextAttributes.minorVersion = 0;
  
      EGL.context = GL.createContext(Browser.getCanvas(), EGL.contextAttributes);
  
      if (EGL.context != 0) {
        EGL.setErrorCode(0x3000 /* EGL_SUCCESS */);
  
        // Run callbacks so that GL emulation works
        GL.makeContextCurrent(EGL.context);
        Browser.useWebGL = true;
        Browser.moduleContextCreatedCallbacks.forEach((callback) => callback());
  
        // Note: This function only creates a context, but it shall not make it active.
        GL.makeContextCurrent(null);
        return 62004;
      } else {
        EGL.setErrorCode(0x3009 /* EGL_BAD_MATCH */); // By the EGL 1.4 spec, an implementation that does not support GLES2 (WebGL in this case), this error code is set.
        return 0; /* EGL_NO_CONTEXT */
      }
    };

  var _eglCreateWindowSurface = (display, config, win, attrib_list) => {
      if (display != 62000) {
        EGL.setErrorCode(0x3008 /* EGL_BAD_DISPLAY */);
        return 0;
      }
      if (config != 62002) {
        EGL.setErrorCode(0x3005 /* EGL_BAD_CONFIG */);
        return 0;
      }
      // TODO: Examine attrib_list! Parameters that can be present there are:
      // - EGL_RENDER_BUFFER (must be EGL_BACK_BUFFER)
      // - EGL_VG_COLORSPACE (can't be set)
      // - EGL_VG_ALPHA_FORMAT (can't be set)
      EGL.setErrorCode(0x3000 /* EGL_SUCCESS */);
      return 62006; /* Magic ID for Emscripten 'default surface' */
    };

  
  var _eglDestroyContext = (display, context) => {
      if (display != 62000) {
        EGL.setErrorCode(0x3008 /* EGL_BAD_DISPLAY */);
        return 0;
      }
      if (context != 62004) {
        EGL.setErrorCode(0x3006 /* EGL_BAD_CONTEXT */);
        return 0;
      }
  
      GL.deleteContext(EGL.context);
      EGL.setErrorCode(0x3000 /* EGL_SUCCESS */);
      if (EGL.currentContext == context) {
        EGL.currentContext = 0;
      }
      return 1 /* EGL_TRUE */;
    };

  var _eglDestroySurface = (display, surface) => {
      if (display != 62000) {
        EGL.setErrorCode(0x3008 /* EGL_BAD_DISPLAY */);
        return 0;
      }
      if (surface != 62006 /* Magic ID for the only EGLSurface supported by Emscripten */) {
        EGL.setErrorCode(0x300D /* EGL_BAD_SURFACE */);
        return 1;
      }
      if (EGL.currentReadSurface == surface) {
        EGL.currentReadSurface = 0;
      }
      if (EGL.currentDrawSurface == surface) {
        EGL.currentDrawSurface = 0;
      }
      EGL.setErrorCode(0x3000 /* EGL_SUCCESS */);
      return 1; /* Magic ID for Emscripten 'default surface' */
    };

  
  var _eglGetConfigAttrib = (display, config, attribute, value) => {
      if (display != 62000) {
        EGL.setErrorCode(0x3008 /* EGL_BAD_DISPLAY */);
        return 0;
      }
      if (config != 62002) {
        EGL.setErrorCode(0x3005 /* EGL_BAD_CONFIG */);
        return 0;
      }
      if (!value) {
        EGL.setErrorCode(0x300C /* EGL_BAD_PARAMETER */);
        return 0;
      }
      EGL.setErrorCode(0x3000 /* EGL_SUCCESS */);
      switch (attribute) {
      case 0x3020: // EGL_BUFFER_SIZE
        HEAP32[((value)>>2)] = EGL.contextAttributes.alpha ? 32 : 24;
        return 1;
      case 0x3021: // EGL_ALPHA_SIZE
        HEAP32[((value)>>2)] = EGL.contextAttributes.alpha ? 8 : 0;
        return 1;
      case 0x3022: // EGL_BLUE_SIZE
        HEAP32[((value)>>2)] = 8;
        return 1;
      case 0x3023: // EGL_GREEN_SIZE
        HEAP32[((value)>>2)] = 8;
        return 1;
      case 0x3024: // EGL_RED_SIZE
        HEAP32[((value)>>2)] = 8;
        return 1;
      case 0x3025: // EGL_DEPTH_SIZE
        HEAP32[((value)>>2)] = EGL.contextAttributes.depth ? 24 : 0;
        return 1;
      case 0x3026: // EGL_STENCIL_SIZE
        HEAP32[((value)>>2)] = EGL.contextAttributes.stencil ? 8 : 0;
        return 1;
      case 0x3027: // EGL_CONFIG_CAVEAT
        // We can return here one of EGL_NONE (0x3038), EGL_SLOW_CONFIG (0x3050) or EGL_NON_CONFORMANT_CONFIG (0x3051).
        HEAP32[((value)>>2)] = 0x3038;
        return 1;
      case 0x3028: // EGL_CONFIG_ID
        HEAP32[((value)>>2)] = 62002;
        return 1;
      case 0x3029: // EGL_LEVEL
        HEAP32[((value)>>2)] = 0;
        return 1;
      case 0x302A: // EGL_MAX_PBUFFER_HEIGHT
        HEAP32[((value)>>2)] = 4096;
        return 1;
      case 0x302B: // EGL_MAX_PBUFFER_PIXELS
        HEAP32[((value)>>2)] = 16777216;
        return 1;
      case 0x302C: // EGL_MAX_PBUFFER_WIDTH
        HEAP32[((value)>>2)] = 4096;
        return 1;
      case 0x302D: // EGL_NATIVE_RENDERABLE
        HEAP32[((value)>>2)] = 0;
        return 1;
      case 0x302E: // EGL_NATIVE_VISUAL_ID
        HEAP32[((value)>>2)] = 0;
        return 1;
      case 0x302F: // EGL_NATIVE_VISUAL_TYPE
        HEAP32[((value)>>2)] = 0x3038;
        return 1;
      case 0x3031: // EGL_SAMPLES
        HEAP32[((value)>>2)] = EGL.contextAttributes.antialias ? 4 : 0;
        return 1;
      case 0x3032: // EGL_SAMPLE_BUFFERS
        HEAP32[((value)>>2)] = EGL.contextAttributes.antialias ? 1 : 0;
        return 1;
      case 0x3033: // EGL_SURFACE_TYPE
        HEAP32[((value)>>2)] = 0x4;
        return 1;
      case 0x3034: // EGL_TRANSPARENT_TYPE
        // If this returns EGL_TRANSPARENT_RGB (0x3052), transparency is used through color-keying. No such thing applies to Emscripten canvas.
        HEAP32[((value)>>2)] = 0x3038;
        return 1;
      case 0x3035: // EGL_TRANSPARENT_BLUE_VALUE
      case 0x3036: // EGL_TRANSPARENT_GREEN_VALUE
      case 0x3037: // EGL_TRANSPARENT_RED_VALUE
        // "If EGL_TRANSPARENT_TYPE is EGL_NONE, then the values for EGL_TRANSPARENT_RED_VALUE, EGL_TRANSPARENT_GREEN_VALUE, and EGL_TRANSPARENT_BLUE_VALUE are undefined."
        HEAP32[((value)>>2)] = -1;
        return 1;
      case 0x3039: // EGL_BIND_TO_TEXTURE_RGB
      case 0x303A: // EGL_BIND_TO_TEXTURE_RGBA
        HEAP32[((value)>>2)] = 0;
        return 1;
      case 0x303B: // EGL_MIN_SWAP_INTERVAL
        HEAP32[((value)>>2)] = 0;
        return 1;
      case 0x303C: // EGL_MAX_SWAP_INTERVAL
        HEAP32[((value)>>2)] = 1;
        return 1;
      case 0x303D: // EGL_LUMINANCE_SIZE
      case 0x303E: // EGL_ALPHA_MASK_SIZE
        HEAP32[((value)>>2)] = 0;
        return 1;
      case 0x303F: // EGL_COLOR_BUFFER_TYPE
        // EGL has two types of buffers: EGL_RGB_BUFFER and EGL_LUMINANCE_BUFFER.
        HEAP32[((value)>>2)] = 0x308E;
        return 1;
      case 0x3040: // EGL_RENDERABLE_TYPE
        // A bit combination of EGL_OPENGL_ES_BIT,EGL_OPENVG_BIT,EGL_OPENGL_ES2_BIT and EGL_OPENGL_BIT.
        HEAP32[((value)>>2)] = 0x4;
        return 1;
      case 0x3042: // EGL_CONFORMANT
        // "EGL_CONFORMANT is a mask indicating if a client API context created with respect to the corresponding EGLConfig will pass the required conformance tests for that API."
        HEAP32[((value)>>2)] = 0;
        return 1;
      default:
        EGL.setErrorCode(0x3004 /* EGL_BAD_ATTRIBUTE */);
        return 0;
      }
    };

  var _eglGetDisplay = (nativeDisplayType) => {
      EGL.setErrorCode(0x3000 /* EGL_SUCCESS */);
      // Emscripten EGL implementation "emulates" X11, and eglGetDisplay is
      // expected to accept/receive a pointer to an X11 Display object (or
      // EGL_DEFAULT_DISPLAY).
      if (nativeDisplayType != 0 /* EGL_DEFAULT_DISPLAY */ && nativeDisplayType != 1 /* see library_xlib.js */) {
        return 0; // EGL_NO_DISPLAY
      }
      return 62000;
    };

  var _eglGetError = () => EGL.errorCode;

  
  var _eglInitialize = (display, majorVersion, minorVersion) => {
      if (display != 62000) {
        EGL.setErrorCode(0x3008 /* EGL_BAD_DISPLAY */);
        return 0;
      }
      if (majorVersion) {
        HEAP32[((majorVersion)>>2)] = 1; // Advertise EGL Major version: '1'
      }
      if (minorVersion) {
        HEAP32[((minorVersion)>>2)] = 4; // Advertise EGL Minor version: '4'
      }
      EGL.defaultDisplayInitialized = true;
      EGL.setErrorCode(0x3000 /* EGL_SUCCESS */);
      return 1;
    };

  
  var _eglMakeCurrent = (display, draw, read, context) => {
      if (display != 62000) {
        EGL.setErrorCode(0x3008 /* EGL_BAD_DISPLAY */);
        return 0 /* EGL_FALSE */;
      }
      //\todo An EGL_NOT_INITIALIZED error is generated if EGL is not initialized for dpy.
      if (context != 0 && context != 62004) {
        EGL.setErrorCode(0x3006 /* EGL_BAD_CONTEXT */);
        return 0;
      }
      if ((read != 0 && read != 62006) || (draw != 0 && draw != 62006 /* Magic ID for Emscripten 'default surface' */)) {
        EGL.setErrorCode(0x300D /* EGL_BAD_SURFACE */);
        return 0;
      }
  
      GL.makeContextCurrent(context ? EGL.context : null);
  
      EGL.currentContext = context;
      EGL.currentDrawSurface = draw;
      EGL.currentReadSurface = read;
      EGL.setErrorCode(0x3000 /* EGL_SUCCESS */);
      return 1 /* EGL_TRUE */;
    };

  
  
  var stringToNewUTF8 = (str) => {
      var size = lengthBytesUTF8(str) + 1;
      var ret = _malloc(size);
      if (ret) stringToUTF8(str, ret, size);
      return ret;
    };
  
  var _eglQueryString = (display, name) => {
      if (display != 62000) {
        EGL.setErrorCode(0x3008 /* EGL_BAD_DISPLAY */);
        return 0;
      }
      //\todo An EGL_NOT_INITIALIZED error is generated if EGL is not initialized for dpy.
      EGL.setErrorCode(0x3000 /* EGL_SUCCESS */);
      if (EGL.stringCache[name]) return EGL.stringCache[name];
      var ret;
      switch (name) {
        case 0x3053 /* EGL_VENDOR */: ret = stringToNewUTF8('Emscripten'); break;
        case 0x3054 /* EGL_VERSION */: ret = stringToNewUTF8('1.4 Emscripten EGL'); break;
        case 0x3055 /* EGL_EXTENSIONS */:  ret = stringToNewUTF8(''); break; // Currently not supporting any EGL extensions.
        case 0x308D /* EGL_CLIENT_APIS */: ret = stringToNewUTF8('OpenGL_ES'); break;
        default:
          EGL.setErrorCode(0x300C /* EGL_BAD_PARAMETER */);
          return 0;
      }
      EGL.stringCache[name] = ret;
      return ret;
    };

  
  var _eglSwapBuffers = (dpy, surface) => {
      if (!EGL.defaultDisplayInitialized) {
        EGL.setErrorCode(0x3001 /* EGL_NOT_INITIALIZED */);
      } else if (!GLctx) {
        EGL.setErrorCode(0x3002 /* EGL_BAD_ACCESS */);
      } else if (GLctx.isContextLost()) {
        EGL.setErrorCode(0x300E /* EGL_CONTEXT_LOST */);
      } else {
        // According to documentation this does an implicit flush.
        // Due to discussion at https://github.com/emscripten-core/emscripten/pull/1871
        // the flush was removed since this _may_ result in slowing code down.
        //_glFlush();
        EGL.setErrorCode(0x3000 /* EGL_SUCCESS */);
        return 1 /* EGL_TRUE */;
      }
      return 0 /* EGL_FALSE */;
    };

  
  
  
  
    /**
   * @param {number=} arg
   * @param {boolean=} noSetTiming
   */
  var setMainLoop = (iterFunc, fps, simulateInfiniteLoop, arg, noSetTiming) => {
      assert(!MainLoop.func, 'emscripten_set_main_loop: there can only be one main loop function at once')
      MainLoop.func = iterFunc;
      MainLoop.arg = arg;
  
      var thisMainLoopId = MainLoop.currentlyRunningMainloop;
      function checkIsRunning() {
        if (thisMainLoopId < MainLoop.currentlyRunningMainloop) {
          maybeExit();
          return false;
        }
        return true;
      }
  
      // We create the loop runner here but it is not actually running until
      // _emscripten_set_main_loop_timing is called (which might happen at a
      // later time).
      MainLoop.runner = function MainLoop_runner() {
        if (ABORT) return;
        if (MainLoop.queue.length > 0) {
          var start = Date.now();
          var blocker = MainLoop.queue.shift();
          blocker.func(blocker.arg);
          if (MainLoop.remainingBlockers) {
            var remaining = MainLoop.remainingBlockers;
            var next = remaining%1 == 0 ? remaining-1 : Math.floor(remaining);
            if (blocker.counted) {
              MainLoop.remainingBlockers = next;
            } else {
              // not counted, but move the progress along a tiny bit
              next = next + 0.5; // do not steal all the next one's progress
              MainLoop.remainingBlockers = (8*remaining + next)/9;
            }
          }
          MainLoop.updateStatus();
  
          // catches pause/resume main loop from blocker execution
          if (!checkIsRunning()) return;
  
          setTimeout(MainLoop.runner, 0);
          return;
        }
  
        // catch pauses from non-main loop sources
        if (!checkIsRunning()) return;
  
        // Implement very basic swap interval control
        MainLoop.currentFrameNumber = MainLoop.currentFrameNumber + 1 | 0;
        if (MainLoop.timingMode == 1 && MainLoop.timingValue > 1 && MainLoop.currentFrameNumber % MainLoop.timingValue != 0) {
          // Not the scheduled time to render this frame - skip.
          MainLoop.scheduler();
          return;
        } else if (MainLoop.timingMode == 0) {
          MainLoop.tickStartTime = _emscripten_get_now();
          if (Module['ctx']) {
            warnOnce('Looks like you are rendering without using requestAnimationFrame for the main loop. You should use 0 for the frame rate in emscripten_set_main_loop in order to use requestAnimationFrame, as that can greatly improve your frame rates!');
          }
        }
  
        MainLoop.runIter(iterFunc);
  
        // catch pauses from the main loop itself
        if (!checkIsRunning()) return;
  
        MainLoop.scheduler();
      }
  
      if (!noSetTiming) {
        if (fps > 0) {
          _emscripten_set_main_loop_timing(0, 1000.0 / fps);
        } else {
          // Do rAF by rendering each frame (no decimating)
          _emscripten_set_main_loop_timing(1, 1);
        }
  
        MainLoop.scheduler();
      }
  
      if (simulateInfiniteLoop) {
        throw 'unwind';
      }
    };
  
  
  var MainLoop = {
  func:null,
  scheduler:null,
  currentlyRunningMainloop:0,
  arg:0,
  timingMode:0,
  timingValue:0,
  currentFrameNumber:0,
  queue:[],
  preMainLoop:[],
  postMainLoop:[],
  pause() {
        if (MainLoop.scheduler) {
          MainLoop.scheduler = null;
          // Incrementing this signals the previous main loop that it's now become old, and it must return.
          MainLoop.currentlyRunningMainloop++;
          
        }
      },
  resume() {
        MainLoop.currentlyRunningMainloop++;
        var timingMode = MainLoop.timingMode;
        var timingValue = MainLoop.timingValue;
        var func = MainLoop.func;
        MainLoop.func = null;
        // do not set timing and call scheduler, we will do it on the next lines
        setMainLoop(func, 0, false, MainLoop.arg, true);
        _emscripten_set_main_loop_timing(timingMode, timingValue);
        MainLoop.scheduler();
      },
  updateStatus() {
        if (Module['setStatus']) {
          var message = Module['statusMessage'] || 'Please wait...';
          var remaining = MainLoop.remainingBlockers ?? 0;
          var expected = MainLoop.expectedBlockers ?? 0;
          if (remaining) {
            if (remaining < expected) {
              Module['setStatus'](`{message} ({expected - remaining}/{expected})`);
            } else {
              Module['setStatus'](message);
            }
          } else {
            Module['setStatus']('');
          }
        }
      },
  init() {
      },
  runIter(func) {
        if (ABORT) return;
        for (var pre of MainLoop.preMainLoop) {
          if (pre() === false) {
            return; // |return false| skips a frame
          }
        }
        callUserCallback(func);
        for (var post of MainLoop.postMainLoop) {
          post();
        }
        checkStackCookie();
      },
  nextRAF:0,
  fakeRequestAnimationFrame(func) {
        // try to keep 60fps between calls to here
        var now = Date.now();
        if (!MainLoop.nextRAF) {
          MainLoop.nextRAF = now + 1000/60;
        } else {
          while (now + 2 >= MainLoop.nextRAF) { // fudge a little, to avoid timer jitter causing us to do lots of delay:0
            MainLoop.nextRAF += 1000/60;
          }
        }
        var delay = Math.max(MainLoop.nextRAF - now, 0);
        setTimeout(func, delay);
      },
  requestAnimationFrame(func) {
        if (globalThis.requestAnimationFrame) {
          requestAnimationFrame(func);
        } else {
          MainLoop.fakeRequestAnimationFrame(func);
        }
      },
  };
  var _emscripten_set_main_loop_timing = (mode, value) => {
      MainLoop.timingMode = mode;
      MainLoop.timingValue = value;
  
      if (!MainLoop.func) {
        err('emscripten_set_main_loop_timing: Cannot set timing mode for main loop since a main loop does not exist! Call emscripten_set_main_loop first to set one up.');
        return 1; // Return non-zero on failure, can't set timing mode when there is no main loop.
      }
  
      if (mode == 0) {
        MainLoop.scheduler = function MainLoop_scheduler_setTimeout() {
          var timeUntilNextTick = Math.max(0, MainLoop.tickStartTime + value - _emscripten_get_now())|0;
          setTimeout(MainLoop.runner, timeUntilNextTick); // doing this each time means that on exception, we stop
        };
      } else if (mode == 1) {
        MainLoop.scheduler = function MainLoop_scheduler_rAF() {
          MainLoop.requestAnimationFrame(MainLoop.runner);
        };
      } else {
        assert(mode == 2);
        if (!MainLoop.setImmediate) {
          if (globalThis.scheduler) {
            // Some modern browsers implement scheduler.postTask, but not all.
            MainLoop.setImmediate = scheduler.postTask.bind(scheduler);
          } else {
            // Emulate setImmediate. (note: not a complete polyfill, we don't emulate clearImmediate() to keep code size to minimum, since not needed)
            var setImmediates = [];
            var emscriptenMainLoopMessageId = 'setimmediate';
            /** @param {Event} event */
            var MainLoop_setImmediate_messageHandler = (event) => {
              if (event.data === emscriptenMainLoopMessageId) {
                event.stopPropagation();
                setImmediates.shift()();
              }
            };
            addEventListener('message', MainLoop_setImmediate_messageHandler, true);
            MainLoop.setImmediate = /** @type{function(function(): ?, ...?): number} */((func) => {
              setImmediates.push(func);
              if (ENVIRONMENT_IS_WORKER) {
                // The postMessge API in a Worker, sends message to the main
                // thread and does not support the `targetOrigin` (*) argument.
                postMessage(emscriptenMainLoopMessageId);
              } else {
                postMessage(emscriptenMainLoopMessageId, '*');
              }
            });
          }
        }
        MainLoop.scheduler = function MainLoop_scheduler_setImmediate() {
          MainLoop.setImmediate(MainLoop.runner);
        };
      }
      return 0;
    };
  
  var _eglSwapInterval = (display, interval) => {
      if (display != 62000) {
        EGL.setErrorCode(0x3008 /* EGL_BAD_DISPLAY */);
        return 0;
      }
      if (interval == 0) _emscripten_set_main_loop_timing(0, 0);
      else _emscripten_set_main_loop_timing(1, interval);
  
      EGL.setErrorCode(0x3000 /* EGL_SUCCESS */);
      return 1;
    };

  var _eglTerminate = (display) => {
      if (display != 62000) {
        EGL.setErrorCode(0x3008 /* EGL_BAD_DISPLAY */);
        return 0;
      }
      EGL.currentContext = 0;
      EGL.currentReadSurface = 0;
      EGL.currentDrawSurface = 0;
      EGL.defaultDisplayInitialized = false;
      EGL.setErrorCode(0x3000 /* EGL_SUCCESS */);
      return 1;
    };

  
  var _eglWaitClient = () => {
      EGL.setErrorCode(0x3000 /* EGL_SUCCESS */);
      return 1;
    };
  var _eglWaitGL = _eglWaitClient;

  var _eglWaitNative = (nativeEngineId) => {
      EGL.setErrorCode(0x3000 /* EGL_SUCCESS */);
      return 1;
    };

  var readEmAsmArgsArray = [];
  
  
  
  
  
  var readEmAsmArgs = (sigPtr, buf) => {
      // Nobody should have mutated _readEmAsmArgsArray underneath us to be something else than an array.
      assert(Array.isArray(readEmAsmArgsArray));
      // The input buffer is allocated on the stack, so it must be stack-aligned.
      assert(buf % 16 == 0);
      readEmAsmArgsArray.length = 0;
      var ch;
      // Most arguments are i32s, so shift the buffer pointer so it is a plain
      // index into HEAP32.
      while (ch = HEAPU8[sigPtr++]) {
        var chr = String.fromCharCode(ch);
        var validChars = ['d', 'f', 'i', 'p'];
        // In WASM_BIGINT mode we support passing i64 values as bigint.
        validChars.push('j');
        assert(validChars.includes(chr), `Invalid character ${ch}("${chr}") in readEmAsmArgs! Use only [${validChars}], and do not specify "v" for void return argument.`);
        // Floats are always passed as doubles, so all types except for 'i'
        // are 8 bytes and require alignment.
        var wide = (ch != 105);
        wide &= (ch != 112);
        buf += wide && (buf % 8) ? 4 : 0;
        readEmAsmArgsArray.push(
          // Special case for pointers under wasm64 or CAN_ADDRESS_2GB mode.
          ch == 112 ? HEAPU32[((buf)>>2)] :
          ch == 106 ? HEAP64[((buf)>>3)] :
          ch == 105 ?
            HEAP32[((buf)>>2)] :
            HEAPF64[((buf)>>3)]
        );
        buf += wide ? 8 : 4;
      }
      return readEmAsmArgsArray;
    };
  var runEmAsmFunction = (code, sigPtr, argbuf) => {
      var args = readEmAsmArgs(sigPtr, argbuf);
      assert(ASM_CONSTS.hasOwnProperty(code), `No EM_ASM constant found at address ${code}.  The loaded WebAssembly file is likely out of sync with the generated JavaScript.`);
      return ASM_CONSTS[code](...args);
    };
  var _emscripten_asm_const_int = (code, sigPtr, argbuf) => {
      return runEmAsmFunction(code, sigPtr, argbuf);
    };

  var runMainThreadEmAsm = (emAsmAddr, sigPtr, argbuf, sync) => {
      var args = readEmAsmArgs(sigPtr, argbuf);
      assert(ASM_CONSTS.hasOwnProperty(emAsmAddr), `No EM_ASM constant found at address ${emAsmAddr}.  The loaded WebAssembly file is likely out of sync with the generated JavaScript.`);
      return ASM_CONSTS[emAsmAddr](...args);
    };
  var _emscripten_asm_const_int_sync_on_main_thread = (emAsmAddr, sigPtr, argbuf) => runMainThreadEmAsm(emAsmAddr, sigPtr, argbuf, 1);

  var _emscripten_asm_const_ptr_sync_on_main_thread = (emAsmAddr, sigPtr, argbuf) => runMainThreadEmAsm(emAsmAddr, sigPtr, argbuf, 1);

  
  var safeRequestAnimationFrame = (func) => {
      
      return MainLoop.requestAnimationFrame(() => {
        
        callUserCallback(func);
      });
    };
  var _emscripten_async_call = (func, arg, millis) => {
      var wrapper = () => ((a1) => dynCall_vi(func, a1))(arg);
  
      if (millis >= 0
      ) {
        safeSetTimeout(wrapper, millis);
      } else {
        safeRequestAnimationFrame(wrapper);
      }
    };


  var _emscripten_err = (str) => err(UTF8ToString(str));

  var onExits = [];
  var addOnExit = (cb) => onExits.push(cb);
  var JSEvents = {
  removeAllEventListeners() {
        while (JSEvents.eventHandlers.length) {
          JSEvents._removeHandler(JSEvents.eventHandlers.length - 1);
        }
        JSEvents.deferredCalls = [];
      },
  inEventHandler:0,
  deferredCalls:[],
  deferCall(targetFunction, precedence, argsList) {
        function arraysHaveEqualContent(arrA, arrB) {
          if (arrA.length != arrB.length) return false;
  
          for (var i = 0; i < arrA.length; i++) {
            if (arrA[i] != arrB[i]) return false;
          }
          return true;
        }
        // Test if the given call was already queued, and if so, don't add it again.
        for (var call of JSEvents.deferredCalls) {
          if (call.targetFunction == targetFunction && arraysHaveEqualContent(call.argsList, argsList)) {
            return;
          }
        }
        JSEvents.deferredCalls.push({
          targetFunction,
          precedence,
          argsList
        });
  
        JSEvents.deferredCalls.sort((x,y) => x.precedence - y.precedence);
      },
  removeDeferredCalls(targetFunction) {
        JSEvents.deferredCalls = JSEvents.deferredCalls.filter((call) => call.targetFunction != targetFunction);
      },
  canPerformEventHandlerRequests() {
        // Browsers that support navigator.userActivation.isActive: https://developer.mozilla.org/en-US/docs/Web/API/UserActivation/isActive
        if (navigator.userActivation) {
          // Verify against transient activation status from UserActivation API
          // whether it is possible to perform a request here without needing to defer. See
          // https://developer.mozilla.org/en-US/docs/Web/Security/User_activation#transient_activation
          // and https://caniuse.com/mdn-api_useractivation
          return navigator.userActivation.isActive;
        }
  
        return JSEvents.inEventHandler && JSEvents.currentEventHandler.allowsDeferredCalls;
      },
  runDeferredCalls() {
        if (!JSEvents.canPerformEventHandlerRequests()) {
          return;
        }
        var deferredCalls = JSEvents.deferredCalls;
        JSEvents.deferredCalls = [];
        for (var call of deferredCalls) {
          call.targetFunction(...call.argsList);
        }
      },
  eventHandlers:[],
  removeAllHandlersOnTarget:(target, eventTypeString) => {
        for (var i = 0; i < JSEvents.eventHandlers.length; ++i) {
          if (JSEvents.eventHandlers[i].target == target &&
            (!eventTypeString || eventTypeString == JSEvents.eventHandlers[i].eventTypeString)) {
             JSEvents._removeHandler(i--);
           }
        }
      },
  _removeHandler(i) {
        var h = JSEvents.eventHandlers[i];
        h.target.removeEventListener(h.eventTypeString, h.eventListenerFunc, h.useCapture);
        JSEvents.eventHandlers.splice(i, 1);
      },
  registerOrRemoveHandler(eventHandler) {
        if (!eventHandler.target) {
          err('registerOrRemoveHandler: the target element for event handler registration does not exist, when processing the following event handler registration:');
          console.dir(eventHandler);
          return -4;
        }
        if (eventHandler.callbackfunc) {
          eventHandler.eventListenerFunc = function(event) {
            // Increment nesting count for the event handler.
            ++JSEvents.inEventHandler;
            JSEvents.currentEventHandler = eventHandler;
            // Process any old deferred calls the user has placed.
            JSEvents.runDeferredCalls();
            // Process the actual event, calls back to user C code handler.
            eventHandler.handlerFunc(event);
            // Process any new deferred calls that were placed right now from this event handler.
            JSEvents.runDeferredCalls();
            // Out of event handler - restore nesting count.
            --JSEvents.inEventHandler;
          };
  
          eventHandler.target.addEventListener(eventHandler.eventTypeString,
                                               eventHandler.eventListenerFunc,
                                               eventHandler.useCapture);
          JSEvents.eventHandlers.push(eventHandler);
        } else {
          for (var i = 0; i < JSEvents.eventHandlers.length; ++i) {
            if (JSEvents.eventHandlers[i].target == eventHandler.target
             && JSEvents.eventHandlers[i].eventTypeString == eventHandler.eventTypeString) {
               JSEvents._removeHandler(i--);
             }
          }
        }
        return 0;
      },
  removeSingleHandler(eventHandler) {
        let success = false;
        for (let i = 0; i < JSEvents.eventHandlers.length; ++i) {
          const handler = JSEvents.eventHandlers[i];
          if (handler.target === eventHandler.target
            && handler.eventTypeId === eventHandler.eventTypeId
            && handler.callbackfunc === eventHandler.callbackfunc
            && handler.userData === eventHandler.userData) {
            // in some very rare cases (ex: Safari / fullscreen events), there is more than 1 handler (eventTypeString is different)
            JSEvents._removeHandler(i--);
            success = true;
          }
        }
        return success ? 0 : -5;
      },
  getNodeNameForTarget(target) {
        if (target == window) return '#window';
        if (target == screen) return '#screen';
        return target?.nodeName ?? '';
      },
  fullscreenEnabled() {
        return document.fullscreenEnabled
        // Safari 13.0.3 on macOS Catalina 10.15.1 still ships with prefixed webkitFullscreenEnabled.
        // TODO: If Safari at some point ships with unprefixed version, update the version check above.
        ?? document.webkitFullscreenEnabled
         ;
      },
  };
  
  /** @type {Object} */
  var specialHTMLTargets = [0, document, window];
  
  
  var maybeCStringToJsString = (cString) => {
      // 'cString > 2' checks if the input is a number, and isn't of the special
      // values we accept here, EMSCRIPTEN_EVENT_TARGET_* (which map to 0, 1, 2).
      // In other words, if cString > 2 then it's a pointer to a valid place in
      // memory, and points to a C string.
      return cString > 2 ? UTF8ToString(cString) : cString;
    };
  
  var findEventTarget = (target) => {
      target = maybeCStringToJsString(target);
      var domElement = specialHTMLTargets[target] || document.querySelector(target);
      return domElement;
    };
  var findCanvasEventTarget = findEventTarget;
  
  var _emscripten_get_canvas_element_size = (target, width, height) => {
      var canvas = findCanvasEventTarget(target);
      if (!canvas) return -4;
      HEAP32[((width)>>2)] = canvas.width;
      HEAP32[((height)>>2)] = canvas.height;
    };
  
  
  
  
  
  var stringToUTF8OnStack = (str) => {
      var size = lengthBytesUTF8(str) + 1;
      var ret = stackAlloc(size);
      stringToUTF8(str, ret, size);
      return ret;
    };
  
  var getCanvasElementSize = (target) => {
      var sp = stackSave();
      var w = stackAlloc(8);
      var h = w + 4;
  
      var targetInt = stringToUTF8OnStack(target.id);
      var ret = _emscripten_get_canvas_element_size(targetInt, w, h);
      var size = [HEAP32[((w)>>2)], HEAP32[((h)>>2)]];
      stackRestore(sp);
      return size;
    };
  
  var _emscripten_set_canvas_element_size = (target, width, height) => {
      var canvas = findCanvasEventTarget(target);
      if (!canvas) return -4;
      canvas.width = width;
      canvas.height = height;
      if (canvas.GLctxObject) GL.resizeOffscreenFramebuffer(canvas.GLctxObject);
      return 0;
    };
  
  
  
  var setCanvasElementSize = (target, width, height) => {
      if (!target.controlTransferredOffscreen) {
        target.width = width;
        target.height = height;
      } else {
        // This function is being called from high-level JavaScript code instead of asm.js/Wasm,
        // and it needs to synchronously proxy over to another thread, so marshal the string onto the heap to do the call.
        var sp = stackSave();
        var targetInt = stringToUTF8OnStack(target.id);
        _emscripten_set_canvas_element_size(targetInt, width, height);
        stackRestore(sp);
      }
    };
  
  var currentFullscreenStrategy = 0;
  
  var callCanvasResizedCallback = (strategy) => {
      if (strategy.canvasResizedCallback) {
        ((a1, a2, a3) => dynCall_iiii(strategy.canvasResizedCallback, a1, a2, a3))(37, 0, strategy.canvasResizedCallbackUserData);
      }
    };
  var registerRestoreOldStyle = (canvas) => {
      var canvasSize = getCanvasElementSize(canvas);
      var oldWidth = canvasSize[0];
      var oldHeight = canvasSize[1];
      var oldCssWidth = canvas.style.width;
      var oldCssHeight = canvas.style.height;
      var oldBackgroundColor = canvas.style.backgroundColor; // Chrome reads color from here.
      var oldDocumentBackgroundColor = document.body.style.backgroundColor; // IE11 reads color from here.
      // Firefox always has black background color.
      var oldPaddingLeft = canvas.style.paddingLeft; // Chrome, FF, Safari
      var oldPaddingRight = canvas.style.paddingRight;
      var oldPaddingTop = canvas.style.paddingTop;
      var oldPaddingBottom = canvas.style.paddingBottom;
      var oldMarginLeft = canvas.style.marginLeft; // IE11
      var oldMarginRight = canvas.style.marginRight;
      var oldMarginTop = canvas.style.marginTop;
      var oldMarginBottom = canvas.style.marginBottom;
      var oldDocumentBodyMargin = document.body.style.margin;
      var oldDocumentOverflow = document.documentElement.style.overflow; // Chrome, Firefox
      var oldDocumentScroll = document.body.scroll; // IE
      var oldImageRendering = canvas.style.imageRendering;
  
      function restoreOldStyle() {
        if (!getFullscreenElement()) {
          document.removeEventListener('fullscreenchange', restoreOldStyle);
  
          document.removeEventListener('webkitfullscreenchange', restoreOldStyle);
  
          setCanvasElementSize(canvas, oldWidth, oldHeight);
  
          canvas.style.width = oldCssWidth;
          canvas.style.height = oldCssHeight;
          canvas.style.backgroundColor = oldBackgroundColor; // Chrome
          // IE11 hack: assigning 'undefined' or an empty string to document.body.style.backgroundColor has no effect, so first assign back the default color
          // before setting the undefined value. Setting undefined value is also important, or otherwise we would later treat that as something that the user
          // had explicitly set so subsequent fullscreen transitions would not set background color properly.
          if (!oldDocumentBackgroundColor) document.body.style.backgroundColor = 'white';
          document.body.style.backgroundColor = oldDocumentBackgroundColor; // IE11
          canvas.style.paddingLeft = oldPaddingLeft; // Chrome, FF, Safari
          canvas.style.paddingRight = oldPaddingRight;
          canvas.style.paddingTop = oldPaddingTop;
          canvas.style.paddingBottom = oldPaddingBottom;
          canvas.style.marginLeft = oldMarginLeft; // IE11
          canvas.style.marginRight = oldMarginRight;
          canvas.style.marginTop = oldMarginTop;
          canvas.style.marginBottom = oldMarginBottom;
          document.body.style.margin = oldDocumentBodyMargin;
          document.documentElement.style.overflow = oldDocumentOverflow; // Chrome, Firefox
          document.body.scroll = oldDocumentScroll; // IE
          canvas.style.imageRendering = oldImageRendering;
          if (canvas.GLctxObject) canvas.GLctxObject.GLctx.viewport(0, 0, oldWidth, oldHeight);
  
          callCanvasResizedCallback(currentFullscreenStrategy);
        }
      }
      document.addEventListener('fullscreenchange', restoreOldStyle);
      document.addEventListener('webkitfullscreenchange', restoreOldStyle);
      return restoreOldStyle;
    };
  
  
  var setLetterbox = (element, topBottom, leftRight) => {
      // Cannot use margin to specify letterboxes in FF or Chrome, since those ignore margins in fullscreen mode.
      element.style.paddingLeft = element.style.paddingRight = leftRight + 'px';
      element.style.paddingTop = element.style.paddingBottom = topBottom + 'px';
    };
  
  
  var getBoundingClientRect = (e) => specialHTMLTargets.indexOf(e) < 0 ? e.getBoundingClientRect() : {'left':0,'top':0};
  var JSEvents_resizeCanvasForFullscreen = (target, strategy) => {
      var restoreOldStyle = registerRestoreOldStyle(target);
      var cssWidth = strategy.softFullscreen ? innerWidth : screen.width;
      var cssHeight = strategy.softFullscreen ? innerHeight : screen.height;
      var rect = getBoundingClientRect(target);
      var windowedCssWidth = rect.width;
      var windowedCssHeight = rect.height;
      var canvasSize = getCanvasElementSize(target);
      var windowedRttWidth = canvasSize[0];
      var windowedRttHeight = canvasSize[1];
  
      if (strategy.scaleMode == 3) {
        setLetterbox(target, (cssHeight - windowedCssHeight) / 2, (cssWidth - windowedCssWidth) / 2);
        cssWidth = windowedCssWidth;
        cssHeight = windowedCssHeight;
      } else if (strategy.scaleMode == 2) {
        if (cssWidth*windowedRttHeight < windowedRttWidth*cssHeight) {
          var desiredCssHeight = windowedRttHeight * cssWidth / windowedRttWidth;
          setLetterbox(target, (cssHeight - desiredCssHeight) / 2, 0);
          cssHeight = desiredCssHeight;
        } else {
          var desiredCssWidth = windowedRttWidth * cssHeight / windowedRttHeight;
          setLetterbox(target, 0, (cssWidth - desiredCssWidth) / 2);
          cssWidth = desiredCssWidth;
        }
      }
  
      // If we are adding padding, must choose a background color or otherwise Chrome will give the
      // padding a default white color. Do it only if user has not customized their own background color.
      target.style.backgroundColor ||= 'black';
      // IE11 does the same, but requires the color to be set in the document body.
      document.body.style.backgroundColor ||= 'black'; // IE11
      // Firefox always shows black letterboxes independent of style color.
  
      target.style.width = cssWidth + 'px';
      target.style.height = cssHeight + 'px';
  
      if (strategy.filteringMode == 1) {
        target.style.imageRendering = 'optimizeSpeed';
        target.style.imageRendering = '-moz-crisp-edges';
        target.style.imageRendering = '-o-crisp-edges';
        target.style.imageRendering = '-webkit-optimize-contrast';
        target.style.imageRendering = 'optimize-contrast';
        target.style.imageRendering = 'crisp-edges';
        target.style.imageRendering = 'pixelated';
      }
  
      var dpiScale = (strategy.canvasResolutionScaleMode == 2) ? devicePixelRatio : 1;
      if (strategy.canvasResolutionScaleMode != 0) {
        var newWidth = (cssWidth * dpiScale)|0;
        var newHeight = (cssHeight * dpiScale)|0;
        setCanvasElementSize(target, newWidth, newHeight);
        if (target.GLctxObject) target.GLctxObject.GLctx.viewport(0, 0, newWidth, newHeight);
      }
      return restoreOldStyle;
    };
  
  var JSEvents_requestFullscreen = (target, strategy) => {
      // EMSCRIPTEN_FULLSCREEN_SCALE_DEFAULT + EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_NONE is a mode where no extra logic is performed to the DOM elements.
      if (strategy.scaleMode != 0 || strategy.canvasResolutionScaleMode != 0) {
        JSEvents_resizeCanvasForFullscreen(target, strategy);
      }
  
      if (target.requestFullscreen) {
        target.requestFullscreen();
      } else if (target.webkitRequestFullscreen) {
        // Safari didn't Element.requestFullscreen support until 16.4
        // See: https://developer.mozilla.org/en-US/docs/Web/API/Element/requestFullscreen
        target.webkitRequestFullscreen(Element.ALLOW_KEYBOARD_INPUT);
      } else {
        return JSEvents.fullscreenEnabled() ? -3 : -1;
      }
  
      currentFullscreenStrategy = strategy;
      callCanvasResizedCallback(strategy);
      return 0;
    };
  var _emscripten_exit_fullscreen = () => {
      if (!JSEvents.fullscreenEnabled()) return -1;
      // Make sure no queued up calls will fire after this.
      JSEvents.removeDeferredCalls(JSEvents_requestFullscreen);
  
      var d = specialHTMLTargets[1];
      if (d.exitFullscreen) {
        d.fullscreenElement && d.exitFullscreen();
      } else if (d.webkitExitFullscreen) {
        d.webkitFullscreenElement && d.webkitExitFullscreen();
      } else {
        return -1;
      }
  
      return 0;
    };

  
  var requestPointerLock = (target) => {
      if (target.requestPointerLock) {
        target.requestPointerLock();
      } else {
        // document.body is known to accept pointer lock, so use that to differentiate if the user passed a bad element,
        // or if the whole browser just doesn't support the feature.
        if (document.body.requestPointerLock) {
          return -3;
        }
        return -1;
      }
      return 0;
    };
  var _emscripten_exit_pointerlock = () => {
      // Make sure no queued up calls will fire after this.
      JSEvents.removeDeferredCalls(requestPointerLock);
      if (!document.exitPointerLock) return -1;
      document.exitPointerLock();
      return 0;
    };


  var _emscripten_get_device_pixel_ratio = () => {
      return devicePixelRatio;
    };

  
  
  var _emscripten_get_element_css_size = (target, width, height) => {
      target = findEventTarget(target);
      if (!target) return -4;
  
      var rect = getBoundingClientRect(target);
      HEAPF64[((width)>>3)] = rect.width;
      HEAPF64[((height)>>3)] = rect.height;
  
      return 0;
    };

  
  
  
  
  var fillGamepadEventData = (eventStruct, e) => {
      HEAPF64[((eventStruct)>>3)] = e.timestamp;
      for (var i = 0; i < e.axes.length; ++i) {
        HEAPF64[(((eventStruct+i*8)+(16))>>3)] = e.axes[i];
      }
      for (var i = 0; i < e.buttons.length; ++i) {
        HEAP8[(eventStruct+i)+(1040)] = e.buttons[i].pressed;
        HEAPF64[(((eventStruct+i*8)+(528))>>3)] = e.buttons[i].value;
      }
      HEAP8[(eventStruct)+(1104)] = e.connected;
      HEAP32[(((eventStruct)+(1108))>>2)] = e.index;
      HEAP32[(((eventStruct)+(8))>>2)] = e.axes.length;
      HEAP32[(((eventStruct)+(12))>>2)] = e.buttons.length;
      stringToUTF8(e.id, eventStruct + 1112, 64);
      stringToUTF8(e.mapping, eventStruct + 1176, 64);
    };
  var _emscripten_get_gamepad_status = (index, gamepadState) => {
      assert(JSEvents.lastGamepadState, 'emscripten_get_gamepad_status() called before emscripten_sample_gamepad_data()');
      // INVALID_PARAM is returned on a Gamepad index that never was there.
      if (index < 0 || index >= JSEvents.lastGamepadState.length) return -5;
  
      // NO_DATA is returned on a Gamepad index that was removed.
      // For previously disconnected gamepads there should be an empty slot (null/undefined/false) at the index.
      // This is because gamepads must keep their original position in the array.
      // For example, removing the first of two gamepads produces [null/undefined/false, gamepad].
      if (!JSEvents.lastGamepadState[index]) return -7;
  
      fillGamepadEventData(gamepadState, JSEvents.lastGamepadState[index]);
      return 0;
    };

  var getHeapMax = () =>
      // Stay one Wasm page short of 4GB: while e.g. Chrome is able to allocate
      // full 4GB Wasm memories, the size will wrap back to 0 bytes in Wasm side
      // for any code that deals with heap sizes, which would require special
      // casing all heap size related code to treat 0 specially.
      2147483648;
  var _emscripten_get_heap_max = () => getHeapMax();


  var _emscripten_get_num_gamepads = () => {
      assert(JSEvents.lastGamepadState, 'emscripten_get_num_gamepads() called before emscripten_sample_gamepad_data()');
      // N.B. Do not call emscripten_get_num_gamepads() unless having first called emscripten_sample_gamepad_data(), and that has returned EMSCRIPTEN_RESULT_SUCCESS.
      // Otherwise the following line will throw an exception.
      return JSEvents.lastGamepadState.length;
    };

  
  
  var getPreloadedImageData = (path, w, h) => {
      path = PATH_FS.resolve(path);
  
      var canvas = /** @type {HTMLCanvasElement} */(Browser.preloadedImages[path]);
      if (!canvas) return 0;
  
      var ctx = canvas.getContext('2d');
      var image = ctx.getImageData(0, 0, canvas.width, canvas.height);
      var buf = _malloc(canvas.width * canvas.height * 4);
  
      HEAPU8.set(image.data, buf);
  
      HEAP32[((w)>>2)] = canvas.width;
      HEAP32[((h)>>2)] = canvas.height;
      return buf;
    };
  
  
  
  var _emscripten_get_preloaded_image_data = (path, w, h) => getPreloadedImageData(UTF8ToString(path), w, h);

  
  
  var _emscripten_get_preloaded_image_data_from_FILE = (file, w, h) => {
      var fd = _fileno(file);
      var stream = FS.getStream(fd);
      if (stream) {
        return getPreloadedImageData(stream.path, w, h);
      }
  
      return 0;
    };

  
  var _emscripten_get_screen_size = (width, height) => {
      HEAP32[((width)>>2)] = screen.width;
      HEAP32[((height)>>2)] = screen.height;
    };

  var _emscripten_glActiveTexture = (x0) => GLctx.activeTexture(x0);

  var _emscripten_glAttachShader = (program, shader) => {
      GLctx.attachShader(GL.programs[program], GL.shaders[shader]);
    };

  var _emscripten_glBeginQuery = (target, id) => {
      GLctx.beginQuery(target, GL.queries[id]);
    };

  var _emscripten_glBeginQueryEXT = (target, id) => {
      GLctx.disjointTimerQueryExt['beginQueryEXT'](target, GL.queries[id]);
    };

  var _emscripten_glBeginTransformFeedback = (x0) => GLctx.beginTransformFeedback(x0);

  
  var _emscripten_glBindAttribLocation = (program, index, name) => {
      GLctx.bindAttribLocation(GL.programs[program], index, UTF8ToString(name));
    };

  var _emscripten_glBindBuffer = (target, buffer) => {
      // Calling glBindBuffer with an unknown buffer will implicitly create a
      // new one.  Here we bypass `GL.counter` and directly using the ID passed
      // in.
      if (buffer && !GL.buffers[buffer]) {
        var b = GLctx.createBuffer();
        b.name = buffer;
        GL.buffers[buffer] = b;
      }
      if (target == 0x8892 /*GL_ARRAY_BUFFER*/) {
        GLctx.currentArrayBufferBinding = buffer;
      } else if (target == 0x8893 /*GL_ELEMENT_ARRAY_BUFFER*/) {
        GLctx.currentElementArrayBufferBinding = buffer;
      }
  
      if (target == 0x88EB /*GL_PIXEL_PACK_BUFFER*/) {
        // In WebGL 2 glReadPixels entry point, we need to use a different WebGL 2
        // API function call when a buffer is bound to
        // GL_PIXEL_PACK_BUFFER_BINDING point, so must keep track whether that
        // binding point is non-null to know what is the proper API function to
        // call.
        GLctx.currentPixelPackBufferBinding = buffer;
      } else if (target == 0x88EC /*GL_PIXEL_UNPACK_BUFFER*/) {
        // In WebGL 2 gl(Compressed)Tex(Sub)Image[23]D entry points, we need to
        // use a different WebGL 2 API function call when a buffer is bound to
        // GL_PIXEL_UNPACK_BUFFER_BINDING point, so must keep track whether that
        // binding point is non-null to know what is the proper API function to
        // call.
        GLctx.currentPixelUnpackBufferBinding = buffer;
      }
      GLctx.bindBuffer(target, GL.buffers[buffer]);
    };

  var _emscripten_glBindBufferBase = (target, index, buffer) => {
      GLctx.bindBufferBase(target, index, GL.buffers[buffer]);
    };

  var _emscripten_glBindBufferRange = (target, index, buffer, offset, ptrsize) => {
      GLctx.bindBufferRange(target, index, GL.buffers[buffer], offset, ptrsize);
    };

  var _emscripten_glBindFramebuffer = (target, framebuffer) => {
  
      // defaultFbo may not be present if 'renderViaOffscreenBackBuffer' was not enabled during context creation time,
      // i.e. setting -sOFFSCREEN_FRAMEBUFFER at compilation time does not yet mandate that offscreen back buffer
      // is being used, but that is ultimately decided at context creation time.
      GLctx.bindFramebuffer(target, framebuffer ? GL.framebuffers[framebuffer] : GL.currentContext.defaultFbo);
  
    };

  var _emscripten_glBindRenderbuffer = (target, renderbuffer) => {
      GLctx.bindRenderbuffer(target, GL.renderbuffers[renderbuffer]);
    };

  var _emscripten_glBindSampler = (unit, sampler) => {
      GLctx.bindSampler(unit, GL.samplers[sampler]);
    };

  var _emscripten_glBindTexture = (target, texture) => {
      GLctx.bindTexture(target, GL.textures[texture]);
    };

  var _emscripten_glBindTransformFeedback = (target, id) => {
      GLctx.bindTransformFeedback(target, GL.transformFeedbacks[id]);
    };

  var _emscripten_glBindVertexArray = (vao) => {
      GLctx.bindVertexArray(GL.vaos[vao]);
      var ibo = GLctx.getParameter(0x8895 /*ELEMENT_ARRAY_BUFFER_BINDING*/);
      GLctx.currentElementArrayBufferBinding = ibo ? (ibo.name | 0) : 0;
    };

  
  var _glBindVertexArray = _emscripten_glBindVertexArray;
  var _emscripten_glBindVertexArrayOES = _glBindVertexArray;

  var _emscripten_glBlendColor = (x0, x1, x2, x3) => GLctx.blendColor(x0, x1, x2, x3);

  var _emscripten_glBlendEquation = (x0) => GLctx.blendEquation(x0);

  var _emscripten_glBlendEquationSeparate = (x0, x1) => GLctx.blendEquationSeparate(x0, x1);

  var _emscripten_glBlendFunc = (x0, x1) => GLctx.blendFunc(x0, x1);

  var _emscripten_glBlendFuncSeparate = (x0, x1, x2, x3) => GLctx.blendFuncSeparate(x0, x1, x2, x3);

  var _emscripten_glBlitFramebuffer = (x0, x1, x2, x3, x4, x5, x6, x7, x8, x9) => GLctx.blitFramebuffer(x0, x1, x2, x3, x4, x5, x6, x7, x8, x9);

  
  var _emscripten_glBufferData = (target, size, data, usage) => {
  
      if (GL.currentContext.version >= 2) {
        // If size is zero, WebGL would interpret uploading the whole input
        // arraybuffer (starting from given offset), which would not make sense in
        // WebAssembly, so avoid uploading if size is zero. However we must still
        // call bufferData to establish a backing storage of zero bytes.
        if (data && size) {
          GLctx.bufferData(target, HEAPU8, usage, data, size);
        } else {
          GLctx.bufferData(target, size, usage);
        }
        return;
      }
      // N.b. here first form specifies a heap subarray, second form an integer
      // size, so the ?: code here is polymorphic. It is advised to avoid
      // randomly mixing both uses in calling code, to avoid any potential JS
      // engine JIT issues.
      GLctx.bufferData(target, data ? HEAPU8.subarray(data, data+size) : size, usage);
    };

  
  var _emscripten_glBufferSubData = (target, offset, size, data) => webglBufferSubData(target, offset, size, data);

  var _emscripten_glCheckFramebufferStatus = (x0) => GLctx.checkFramebufferStatus(x0);

  var _emscripten_glClear = (x0) => GLctx.clear(x0);

  var _emscripten_glClearBufferfi = (x0, x1, x2, x3) => GLctx.clearBufferfi(x0, x1, x2, x3);

  var _emscripten_glClearBufferfv = (buffer, drawbuffer, value) => {
  
      GLctx.clearBufferfv(buffer, drawbuffer, HEAPF32, ((value)>>2));
    };

  var _emscripten_glClearBufferiv = (buffer, drawbuffer, value) => {
  
      GLctx.clearBufferiv(buffer, drawbuffer, HEAP32, ((value)>>2));
    };

  var _emscripten_glClearBufferuiv = (buffer, drawbuffer, value) => {
  
      GLctx.clearBufferuiv(buffer, drawbuffer, HEAPU32, ((value)>>2));
    };

  var _emscripten_glClearColor = (x0, x1, x2, x3) => GLctx.clearColor(x0, x1, x2, x3);

  var _emscripten_glClearDepthf = (x0) => GLctx.clearDepth(x0);

  var _emscripten_glClearStencil = (x0) => GLctx.clearStencil(x0);

  var _emscripten_glClientWaitSync = (sync, flags, timeout) => {
      // WebGL2 vs GLES3 differences: in GLES3, the timeout parameter is a uint64, where 0xFFFFFFFFFFFFFFFFULL means GL_TIMEOUT_IGNORED.
      // In JS, there's no 64-bit value types, so instead timeout is taken to be signed, and GL_TIMEOUT_IGNORED is given value -1.
      // Inherently the value accepted in the timeout is lossy, and can't take in arbitrary u64 bit pattern (but most likely doesn't matter)
      // See https://www.khronos.org/registry/webgl/specs/latest/2.0/#5.15
      timeout = Number(timeout);
      return GLctx.clientWaitSync(GL.syncs[sync], flags, timeout);
    };

  var _emscripten_glClipControlEXT = (origin, depth) => {
      GLctx.extClipControl['clipControlEXT'](origin, depth);
    };

  var _emscripten_glColorMask = (red, green, blue, alpha) => {
      GLctx.colorMask(!!red, !!green, !!blue, !!alpha);
    };

  var _emscripten_glCompileShader = (shader) => {
      GLctx.compileShader(GL.shaders[shader]);
    };

  
  var _emscripten_glCompressedTexImage2D = (target, level, internalFormat, width, height, border, imageSize, data) => {
      // `data` may be null here, which means "allocate uninitialized space but
      // don't upload" in GLES parlance, but `compressedTexImage2D` requires the
      // final data parameter, so we simply pass a heap view starting at zero
      // effectively uploading whatever happens to be near address zero.  See
      // https://github.com/emscripten-core/emscripten/issues/19300.
      if (GL.currentContext.version >= 2) {
        if (GLctx.currentPixelUnpackBufferBinding || !imageSize) {
          GLctx.compressedTexImage2D(target, level, internalFormat, width, height, border, imageSize, data);
          return;
        }
        GLctx.compressedTexImage2D(target, level, internalFormat, width, height, border, HEAPU8, data, imageSize);
        return;
      }
      GLctx.compressedTexImage2D(target, level, internalFormat, width, height, border, HEAPU8.subarray(data, data + imageSize));
    };

  var _emscripten_glCompressedTexImage3D = (target, level, internalFormat, width, height, depth, border, imageSize, data) => {
      if (GLctx.currentPixelUnpackBufferBinding) {
        GLctx.compressedTexImage3D(target, level, internalFormat, width, height, depth, border, imageSize, data);
      } else {
        GLctx.compressedTexImage3D(target, level, internalFormat, width, height, depth, border, HEAPU8, data, imageSize);
      }
    };

  
  var _emscripten_glCompressedTexSubImage2D = (target, level, xoffset, yoffset, width, height, format, imageSize, data) => {
      if (GL.currentContext.version >= 2) {
        if (GLctx.currentPixelUnpackBufferBinding || !imageSize) {
          GLctx.compressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);
          return;
        }
        GLctx.compressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, HEAPU8, data, imageSize);
        return;
      }
      GLctx.compressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, HEAPU8.subarray(data, data + imageSize));
    };

  var _emscripten_glCompressedTexSubImage3D = (target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data) => {
      if (GLctx.currentPixelUnpackBufferBinding) {
        GLctx.compressedTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);
      } else {
        GLctx.compressedTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, HEAPU8, data, imageSize);
      }
    };

  var _emscripten_glCopyBufferSubData = (x0, x1, x2, x3, x4) => GLctx.copyBufferSubData(x0, x1, x2, x3, x4);

  var _emscripten_glCopyTexImage2D = (x0, x1, x2, x3, x4, x5, x6, x7) => GLctx.copyTexImage2D(x0, x1, x2, x3, x4, x5, x6, x7);

  var _emscripten_glCopyTexSubImage2D = (x0, x1, x2, x3, x4, x5, x6, x7) => GLctx.copyTexSubImage2D(x0, x1, x2, x3, x4, x5, x6, x7);

  var _emscripten_glCopyTexSubImage3D = (x0, x1, x2, x3, x4, x5, x6, x7, x8) => GLctx.copyTexSubImage3D(x0, x1, x2, x3, x4, x5, x6, x7, x8);

  var _emscripten_glCreateProgram = () => {
      var id = GL.getNewId(GL.programs);
      var program = GLctx.createProgram();
      // Store additional information needed for each shader program:
      program.name = id;
      // Lazy cache results of
      // glGetProgramiv(GL_ACTIVE_UNIFORM_MAX_LENGTH/GL_ACTIVE_ATTRIBUTE_MAX_LENGTH/GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH)
      program.maxUniformLength = program.maxAttributeLength = program.maxUniformBlockNameLength = 0;
      program.uniformIdCounter = 1;
      GL.programs[id] = program;
      return id;
    };

  var _emscripten_glCreateShader = (shaderType) => {
      var id = GL.getNewId(GL.shaders);
      GL.shaders[id] = GLctx.createShader(shaderType);
  
      return id;
    };

  var _emscripten_glCullFace = (x0) => GLctx.cullFace(x0);

  
  var _emscripten_glDeleteBuffers = (n, buffers) => {
      for (var i = 0; i < n; i++) {
        var id = HEAP32[(((buffers)+(i*4))>>2)];
        var buffer = GL.buffers[id];
  
        // From spec: "glDeleteBuffers silently ignores 0's and names that do not
        // correspond to existing buffer objects."
        if (!buffer) continue;
  
        GLctx.deleteBuffer(buffer);
        buffer.name = 0;
        GL.buffers[id] = null;
  
        if (id == GLctx.currentArrayBufferBinding) GLctx.currentArrayBufferBinding = 0;
        if (id == GLctx.currentElementArrayBufferBinding) GLctx.currentElementArrayBufferBinding = 0;
        if (id == GLctx.currentPixelPackBufferBinding) GLctx.currentPixelPackBufferBinding = 0;
        if (id == GLctx.currentPixelUnpackBufferBinding) GLctx.currentPixelUnpackBufferBinding = 0;
      }
    };

  
  var _emscripten_glDeleteFramebuffers = (n, framebuffers) => {
      for (var i = 0; i < n; ++i) {
        var id = HEAP32[(((framebuffers)+(i*4))>>2)];
        var framebuffer = GL.framebuffers[id];
        if (!framebuffer) continue; // GL spec: "glDeleteFramebuffers silently ignores 0s and names that do not correspond to existing framebuffer objects".
        GLctx.deleteFramebuffer(framebuffer);
        framebuffer.name = 0;
        GL.framebuffers[id] = null;
      }
    };

  var _emscripten_glDeleteProgram = (id) => {
      if (!id) return;
      var program = GL.programs[id];
      if (!program) {
        // glDeleteProgram actually signals an error when deleting a nonexisting
        // object, unlike some other GL delete functions.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      GLctx.deleteProgram(program);
      program.name = 0;
      GL.programs[id] = null;
    };

  var _emscripten_glDeleteQueries = (n, ids) => {
      for (var i = 0; i < n; i++) {
        var id = HEAP32[(((ids)+(i*4))>>2)];
        var query = GL.queries[id];
        if (!query) continue; // GL spec: "unused names in ids are ignored, as is the name zero."
        GLctx.deleteQuery(query);
        GL.queries[id] = null;
      }
    };

  
  var _emscripten_glDeleteQueriesEXT = (n, ids) => {
      for (var i = 0; i < n; i++) {
        var id = HEAP32[(((ids)+(i*4))>>2)];
        var query = GL.queries[id];
        if (!query) continue; // GL spec: "unused names in ids are ignored, as is the name zero."
        GLctx.disjointTimerQueryExt['deleteQueryEXT'](query);
        GL.queries[id] = null;
      }
    };

  
  var _emscripten_glDeleteRenderbuffers = (n, renderbuffers) => {
      for (var i = 0; i < n; i++) {
        var id = HEAP32[(((renderbuffers)+(i*4))>>2)];
        var renderbuffer = GL.renderbuffers[id];
        if (!renderbuffer) continue; // GL spec: "glDeleteRenderbuffers silently ignores 0s and names that do not correspond to existing renderbuffer objects".
        GLctx.deleteRenderbuffer(renderbuffer);
        renderbuffer.name = 0;
        GL.renderbuffers[id] = null;
      }
    };

  var _emscripten_glDeleteSamplers = (n, samplers) => {
      for (var i = 0; i < n; i++) {
        var id = HEAP32[(((samplers)+(i*4))>>2)];
        var sampler = GL.samplers[id];
        if (!sampler) continue;
        GLctx.deleteSampler(sampler);
        sampler.name = 0;
        GL.samplers[id] = null;
      }
    };

  var _emscripten_glDeleteShader = (id) => {
      if (!id) return;
      var shader = GL.shaders[id];
      if (!shader) {
        // glDeleteShader actually signals an error when deleting a nonexisting
        // object, unlike some other GL delete functions.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      GLctx.deleteShader(shader);
      GL.shaders[id] = null;
    };

  var _emscripten_glDeleteSync = (id) => {
      if (!id) return;
      var sync = GL.syncs[id];
      if (!sync) { // glDeleteSync signals an error when deleting a nonexisting object, unlike some other GL delete functions.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      GLctx.deleteSync(sync);
      sync.name = 0;
      GL.syncs[id] = null;
    };

  
  var _emscripten_glDeleteTextures = (n, textures) => {
      for (var i = 0; i < n; i++) {
        var id = HEAP32[(((textures)+(i*4))>>2)];
        var texture = GL.textures[id];
        // GL spec: "glDeleteTextures silently ignores 0s and names that do not
        // correspond to existing textures".
        if (!texture) continue;
        GLctx.deleteTexture(texture);
        texture.name = 0;
        GL.textures[id] = null;
      }
    };

  var _emscripten_glDeleteTransformFeedbacks = (n, ids) => {
      for (var i = 0; i < n; i++) {
        var id = HEAP32[(((ids)+(i*4))>>2)];
        var transformFeedback = GL.transformFeedbacks[id];
        if (!transformFeedback) continue; // GL spec: "unused names in ids are ignored, as is the name zero."
        GLctx.deleteTransformFeedback(transformFeedback);
        transformFeedback.name = 0;
        GL.transformFeedbacks[id] = null;
      }
    };

  
  var _emscripten_glDeleteVertexArrays = (n, vaos) => {
      for (var i = 0; i < n; i++) {
        var id = HEAP32[(((vaos)+(i*4))>>2)];
        GLctx.deleteVertexArray(GL.vaos[id]);
        GL.vaos[id] = null;
      }
    };

  
  var _glDeleteVertexArrays = _emscripten_glDeleteVertexArrays;
  var _emscripten_glDeleteVertexArraysOES = _glDeleteVertexArrays;

  var _emscripten_glDepthFunc = (x0) => GLctx.depthFunc(x0);

  var _emscripten_glDepthMask = (flag) => {
      GLctx.depthMask(!!flag);
    };

  var _emscripten_glDepthRangef = (x0, x1) => GLctx.depthRange(x0, x1);

  var _emscripten_glDetachShader = (program, shader) => {
      GLctx.detachShader(GL.programs[program], GL.shaders[shader]);
    };

  var _emscripten_glDisable = (x0) => GLctx.disable(x0);

  var _emscripten_glDisableVertexAttribArray = (index) => {
      var cb = GL.currentContext.clientBuffers[index];
      cb.enabled = false;
      GLctx.disableVertexAttribArray(index);
    };

  var _emscripten_glDrawArrays = (mode, first, count) => {
      // bind any client-side buffers
      GL.preDrawHandleClientVertexAttribBindings(first + count);
  
      GLctx.drawArrays(mode, first, count);
  
      GL.postDrawHandleClientVertexAttribBindings();
    };

  var _emscripten_glDrawArraysInstanced = (mode, first, count, primcount) => {
      GLctx.drawArraysInstanced(mode, first, count, primcount);
    };

  
  var _glDrawArraysInstanced = _emscripten_glDrawArraysInstanced;
  var _emscripten_glDrawArraysInstancedANGLE = _glDrawArraysInstanced;

  
  var _emscripten_glDrawArraysInstancedARB = _glDrawArraysInstanced;

  
  var _emscripten_glDrawArraysInstancedEXT = _glDrawArraysInstanced;

  
  var _emscripten_glDrawArraysInstancedNV = _glDrawArraysInstanced;

  var tempFixedLengthArray = [];
  
  
  var _emscripten_glDrawBuffers = (n, bufs) => {
  
      var bufArray = tempFixedLengthArray[n];
      for (var i = 0; i < n; i++) {
        bufArray[i] = HEAP32[(((bufs)+(i*4))>>2)];
      }
  
      GLctx.drawBuffers(bufArray);
    };

  
  var _glDrawBuffers = _emscripten_glDrawBuffers;
  var _emscripten_glDrawBuffersEXT = _glDrawBuffers;

  
  var _emscripten_glDrawBuffersWEBGL = _glDrawBuffers;

  
  
  var _emscripten_glDrawElements = (mode, count, type, indices) => {
      var buf;
      var vertexes = 0;
      if (!GLctx.currentElementArrayBufferBinding) {
        var size = GL.calcBufLength(1, type, 0, count);
        buf = GL.getTempIndexBuffer(size);
        GLctx.bindBuffer(0x8893 /*GL_ELEMENT_ARRAY_BUFFER*/, buf);
        webglBufferSubData(0x8893 /*GL_ELEMENT_ARRAY_BUFFER*/, 0, size, indices);
  
        // Calculating vertex count if shader's attribute data is on client side
        if (count > 0) {
          for (var i = 0; i < GL.currentContext.maxVertexAttribs; ++i) {
            var cb = GL.currentContext.clientBuffers[i];
            if (cb.clientside && cb.enabled) {
              let arrayClass;
              switch(type) {
                case 0x1401 /* GL_UNSIGNED_BYTE */: arrayClass = Uint8Array; break;
                case 0x1403 /* GL_UNSIGNED_SHORT */: arrayClass = Uint16Array; break;
                case 0x1405 /* GL_UNSIGNED_INT */: arrayClass = Uint32Array; break;
                default:
                  GL.recordError(0x502 /* GL_INVALID_OPERATION */);
                  return;
              }
  
              vertexes = new arrayClass(HEAPU8.buffer, indices, count).reduce((max, current) => Math.max(max, current)) + 1;
              break;
            }
          }
        }
  
        // the index is now 0
        indices = 0;
      }
  
      // bind any client-side buffers
      GL.preDrawHandleClientVertexAttribBindings(vertexes);
  
      GLctx.drawElements(mode, count, type, indices);
  
      GL.postDrawHandleClientVertexAttribBindings(count);
  
      if (!GLctx.currentElementArrayBufferBinding) {
        GLctx.bindBuffer(0x8893 /*GL_ELEMENT_ARRAY_BUFFER*/, null);
      }
    };

  var _emscripten_glDrawElementsInstanced = (mode, count, type, indices, primcount) => {
      GLctx.drawElementsInstanced(mode, count, type, indices, primcount);
    };

  
  var _glDrawElementsInstanced = _emscripten_glDrawElementsInstanced;
  var _emscripten_glDrawElementsInstancedANGLE = _glDrawElementsInstanced;

  
  var _emscripten_glDrawElementsInstancedARB = _glDrawElementsInstanced;

  
  var _emscripten_glDrawElementsInstancedEXT = _glDrawElementsInstanced;

  
  var _emscripten_glDrawElementsInstancedNV = _glDrawElementsInstanced;

  var _glDrawElements = _emscripten_glDrawElements;
  var _emscripten_glDrawRangeElements = (mode, start, end, count, type, indices) => {
      // TODO: This should be a trivial pass-through function registered at the bottom of this page as
      // glFuncs[6][1] += ' drawRangeElements';
      // but due to https://bugzil.la/1202427,
      // we work around by ignoring the range.
      _glDrawElements(mode, count, type, indices);
    };

  var _emscripten_glEnable = (x0) => GLctx.enable(x0);

  var _emscripten_glEnableVertexAttribArray = (index) => {
      var cb = GL.currentContext.clientBuffers[index];
      cb.enabled = true;
      GLctx.enableVertexAttribArray(index);
    };

  var _emscripten_glEndQuery = (x0) => GLctx.endQuery(x0);

  var _emscripten_glEndQueryEXT = (target) => {
      GLctx.disjointTimerQueryExt['endQueryEXT'](target);
    };

  var _emscripten_glEndTransformFeedback = () => GLctx.endTransformFeedback();

  var _emscripten_glFenceSync = (condition, flags) => {
      var sync = GLctx.fenceSync(condition, flags);
      if (sync) {
        var id = GL.getNewId(GL.syncs);
        sync.name = id;
        GL.syncs[id] = sync;
        return id;
      }
      return 0; // Failed to create a sync object
    };

  var _emscripten_glFinish = () => GLctx.finish();

  var _emscripten_glFlush = () => GLctx.flush();

  var emscriptenWebGLGetBufferBinding = (target) => {
      switch (target) {
        case 0x8892 /*GL_ARRAY_BUFFER*/: target = 0x8894 /*GL_ARRAY_BUFFER_BINDING*/; break;
        case 0x8893 /*GL_ELEMENT_ARRAY_BUFFER*/: target = 0x8895 /*GL_ELEMENT_ARRAY_BUFFER_BINDING*/; break;
        case 0x88EB /*GL_PIXEL_PACK_BUFFER*/: target = 0x88ED /*GL_PIXEL_PACK_BUFFER_BINDING*/; break;
        case 0x88EC /*GL_PIXEL_UNPACK_BUFFER*/: target = 0x88EF /*GL_PIXEL_UNPACK_BUFFER_BINDING*/; break;
        case 0x8C8E /*GL_TRANSFORM_FEEDBACK_BUFFER*/: target = 0x8C8F /*GL_TRANSFORM_FEEDBACK_BUFFER_BINDING*/; break;
        case 0x8F36 /*GL_COPY_READ_BUFFER*/: target = 0x8F36 /*GL_COPY_READ_BUFFER_BINDING*/; break;
        case 0x8F37 /*GL_COPY_WRITE_BUFFER*/: target = 0x8F37 /*GL_COPY_WRITE_BUFFER_BINDING*/; break;
        case 0x8A11 /*GL_UNIFORM_BUFFER*/: target = 0x8A28 /*GL_UNIFORM_BUFFER_BINDING*/; break;
        // In default case, fall through and assume passed one of the _BINDING enums directly.
      }
      var buffer = GLctx.getParameter(target);
      if (buffer) return buffer.name|0;
      else return 0;
    };
  
  var emscriptenWebGLValidateMapBufferTarget = (target) => {
      switch (target) {
        case 0x8892: // GL_ARRAY_BUFFER
        case 0x8893: // GL_ELEMENT_ARRAY_BUFFER
        case 0x8F36: // GL_COPY_READ_BUFFER
        case 0x8F37: // GL_COPY_WRITE_BUFFER
        case 0x88EB: // GL_PIXEL_PACK_BUFFER
        case 0x88EC: // GL_PIXEL_UNPACK_BUFFER
        case 0x8C2A: // GL_TEXTURE_BUFFER
        case 0x8C8E: // GL_TRANSFORM_FEEDBACK_BUFFER
        case 0x8A11: // GL_UNIFORM_BUFFER
          return true;
        default:
          return false;
      }
    };
  
  
  var _emscripten_glFlushMappedBufferRange = (target, offset, length) => {
      if (!emscriptenWebGLValidateMapBufferTarget(target)) {
        GL.recordError(0x500/*GL_INVALID_ENUM*/);
        err('GL_INVALID_ENUM in glFlushMappedBufferRange');
        return;
      }
  
      var mapping = GL.mappedBuffers[emscriptenWebGLGetBufferBinding(target)];
      if (!mapping) {
        GL.recordError(0x502 /* GL_INVALID_OPERATION */);
        err('buffer was never mapped in glFlushMappedBufferRange');
        return;
      }
  
      if (!(mapping.access & 0x10)) {
        GL.recordError(0x502 /* GL_INVALID_OPERATION */);
        err('buffer was not mapped with GL_MAP_FLUSH_EXPLICIT_BIT in glFlushMappedBufferRange');
        return;
      }
      if (offset < 0 || length < 0 || offset + length > mapping.length) {
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        err('invalid range in glFlushMappedBufferRange');
        return;
      }
  
      webglBufferSubData(target, mapping.offset, length, mapping.mem + offset);
    };

  var _emscripten_glFramebufferRenderbuffer = (target, attachment, renderbuffertarget, renderbuffer) => {
      GLctx.framebufferRenderbuffer(target, attachment, renderbuffertarget,
                                         GL.renderbuffers[renderbuffer]);
    };

  var _emscripten_glFramebufferTexture2D = (target, attachment, textarget, texture, level) => {
      GLctx.framebufferTexture2D(target, attachment, textarget,
                                      GL.textures[texture], level);
    };

  var _emscripten_glFramebufferTextureLayer = (target, attachment, texture, level, layer) => {
      GLctx.framebufferTextureLayer(target, attachment, GL.textures[texture], level, layer);
    };

  var _emscripten_glFrontFace = (x0) => GLctx.frontFace(x0);

  var _emscripten_glGenBuffers = (n, buffers) => {
      GL.genObject(n, buffers, 'createBuffer', GL.buffers
        );
    };

  var _emscripten_glGenFramebuffers = (n, ids) => {
      GL.genObject(n, ids, 'createFramebuffer', GL.framebuffers
        );
    };

  var _emscripten_glGenQueries = (n, ids) => {
      GL.genObject(n, ids, 'createQuery', GL.queries
        );
    };

  
  var _emscripten_glGenQueriesEXT = (n, ids) => {
      for (var i = 0; i < n; i++) {
        var query = GLctx.disjointTimerQueryExt['createQueryEXT']();
        if (!query) {
          GL.recordError(0x502 /* GL_INVALID_OPERATION */);
          while (i < n) HEAP32[(((ids)+(i++*4))>>2)] = 0;
          return;
        }
        var id = GL.getNewId(GL.queries);
        query.name = id;
        GL.queries[id] = query;
        HEAP32[(((ids)+(i*4))>>2)] = id;
      }
    };

  var _emscripten_glGenRenderbuffers = (n, renderbuffers) => {
      GL.genObject(n, renderbuffers, 'createRenderbuffer', GL.renderbuffers
        );
    };

  var _emscripten_glGenSamplers = (n, samplers) => {
      GL.genObject(n, samplers, 'createSampler', GL.samplers
        );
    };

  var _emscripten_glGenTextures = (n, textures) => {
      GL.genObject(n, textures, 'createTexture', GL.textures
        );
    };

  var _emscripten_glGenTransformFeedbacks = (n, ids) => {
      GL.genObject(n, ids, 'createTransformFeedback', GL.transformFeedbacks
        );
    };

  var _emscripten_glGenVertexArrays = (n, arrays) => {
      GL.genObject(n, arrays, 'createVertexArray', GL.vaos
        );
    };

  
  var _glGenVertexArrays = _emscripten_glGenVertexArrays;
  var _emscripten_glGenVertexArraysOES = _glGenVertexArrays;

  var _emscripten_glGenerateMipmap = (x0) => GLctx.generateMipmap(x0);

  
  
  var __glGetActiveAttribOrUniform = (funcName, program, index, bufSize, length, size, type, name) => {
      program = GL.programs[program];
      var info = GLctx[funcName](program, index);
      if (info) {
        // If an error occurs, nothing will be written to length, size and type and name.
        var numBytesWrittenExclNull = name && stringToUTF8(info.name, name, bufSize);
        if (length) HEAP32[((length)>>2)] = numBytesWrittenExclNull;
        if (size) HEAP32[((size)>>2)] = info.size;
        if (type) HEAP32[((type)>>2)] = info.type;
      }
    };
  
  var _emscripten_glGetActiveAttrib = (program, index, bufSize, length, size, type, name) =>
      __glGetActiveAttribOrUniform('getActiveAttrib', program, index, bufSize, length, size, type, name);

  
  var _emscripten_glGetActiveUniform = (program, index, bufSize, length, size, type, name) =>
      __glGetActiveAttribOrUniform('getActiveUniform', program, index, bufSize, length, size, type, name);

  var _emscripten_glGetActiveUniformBlockName = (program, uniformBlockIndex, bufSize, length, uniformBlockName) => {
      program = GL.programs[program];
  
      var result = GLctx.getActiveUniformBlockName(program, uniformBlockIndex);
      if (!result) return; // If an error occurs, nothing will be written to uniformBlockName or length.
      if (uniformBlockName && bufSize > 0) {
        var numBytesWrittenExclNull = stringToUTF8(result, uniformBlockName, bufSize);
        if (length) HEAP32[((length)>>2)] = numBytesWrittenExclNull;
      } else {
        if (length) HEAP32[((length)>>2)] = 0;
      }
    };

  var _emscripten_glGetActiveUniformBlockiv = (program, uniformBlockIndex, pname, params) => {
      if (!params) {
        // GLES2 specification does not specify how to behave if params is a null pointer. Since calling this function does not make sense
        // if params == null, issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      program = GL.programs[program];
  
      if (pname == 0x8A41 /* GL_UNIFORM_BLOCK_NAME_LENGTH */) {
        var name = GLctx.getActiveUniformBlockName(program, uniformBlockIndex);
        HEAP32[((params)>>2)] = name.length+1;
        return;
      }
  
      var result = GLctx.getActiveUniformBlockParameter(program, uniformBlockIndex, pname);
      if (result === null) return; // If an error occurs, nothing should be written to params.
      if (pname == 0x8A43 /*GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES*/) {
        for (var i = 0; i < result.length; i++) {
          HEAP32[(((params)+(i*4))>>2)] = result[i];
        }
      } else {
        HEAP32[((params)>>2)] = result;
      }
    };

  var _emscripten_glGetActiveUniformsiv = (program, uniformCount, uniformIndices, pname, params) => {
      if (!params) {
        // GLES2 specification does not specify how to behave if params is a null pointer. Since calling this function does not make sense
        // if params == null, issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      if (uniformCount > 0 && uniformIndices == 0) {
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      program = GL.programs[program];
      var ids = [];
      for (var i = 0; i < uniformCount; i++) {
        ids.push(HEAP32[(((uniformIndices)+(i*4))>>2)]);
      }
  
      var result = GLctx.getActiveUniforms(program, ids, pname);
      if (!result) return; // GL spec: If an error is generated, nothing is written out to params.
  
      var len = result.length;
      for (var i = 0; i < len; i++) {
        HEAP32[(((params)+(i*4))>>2)] = result[i];
      }
    };

  
  var _emscripten_glGetAttachedShaders = (program, maxCount, count, shaders) => {
      var result = GLctx.getAttachedShaders(GL.programs[program]);
      var len = result.length;
      if (len > maxCount) {
        len = maxCount;
      }
      HEAP32[((count)>>2)] = len;
      for (var i = 0; i < len; ++i) {
        var id = GL.shaders.indexOf(result[i]);
        HEAP32[(((shaders)+(i*4))>>2)] = id;
      }
    };

  
  var _emscripten_glGetAttribLocation = (program, name) =>
      GLctx.getAttribLocation(GL.programs[program], UTF8ToString(name));

  
  var readI53FromI64 = (ptr) => {
      return HEAPU32[((ptr)>>2)] + HEAP32[(((ptr)+(4))>>2)] * 4294967296;
    };
  
  var readI53FromU64 = (ptr) => {
      return HEAPU32[((ptr)>>2)] + HEAPU32[(((ptr)+(4))>>2)] * 4294967296;
    };
  
  var writeI53ToI64 = (ptr, num) => {
      HEAPU32[((ptr)>>2)] = num;
      var lower = HEAPU32[((ptr)>>2)];
      HEAPU32[(((ptr)+(4))>>2)] = (num - lower)/4294967296;
      var deserialized = (num >= 0) ? readI53FromU64(ptr) : readI53FromI64(ptr);
      var offset = ((ptr)>>2);
      if (deserialized != num) warnOnce(`writeI53ToI64() out of range: serialized JS Number ${num} to Wasm heap as bytes lo=${ptrToString(HEAPU32[offset])}, hi=${ptrToString(HEAPU32[offset+1])}, which deserializes back to ${deserialized} instead!`);
    };
  
  
  var webglGetExtensions = () => {
      var exts = getEmscriptenSupportedExtensions(GLctx);
      exts = exts.concat(exts.map((e) => 'GL_' + e));
      return exts;
    };
  
  
  
  
  var emscriptenWebGLGet = (name_, p, type) => {
      // Guard against user passing a null pointer.
      // Note that GLES2 spec does not say anything about how passing a null
      // pointer should be treated.  Testing on desktop core GL 3, the application
      // crashes on glGetIntegerv to a null pointer, but better to report an error
      // instead of doing anything random.
      if (!p) {
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      var ret = undefined;
      switch (name_) { // Handle a few trivial GLES values
        case 0x8DFA: // GL_SHADER_COMPILER
          ret = 1;
          break;
        case 0x8DF8: // GL_SHADER_BINARY_FORMATS
          if (type != 0 && type != 1) {
            GL.recordError(0x500); // GL_INVALID_ENUM
          }
          // Do not write anything to the out pointer, since no binary formats are
          // supported.
          return;
        case 0x87FE: // GL_NUM_PROGRAM_BINARY_FORMATS
        case 0x8DF9: // GL_NUM_SHADER_BINARY_FORMATS
          ret = 0;
          break;
        case 0x86A2: // GL_NUM_COMPRESSED_TEXTURE_FORMATS
          // WebGL doesn't have GL_NUM_COMPRESSED_TEXTURE_FORMATS (it's obsolete
          // since GL_COMPRESSED_TEXTURE_FORMATS returns a JS array that can be
          // queried for length), so implement it ourselves to allow C++ GLES2
          // code to get the length.
          var formats = GLctx.getParameter(0x86A3 /*GL_COMPRESSED_TEXTURE_FORMATS*/);
          ret = formats ? formats.length : 0;
          break;
  
        case 0x821D: // GL_NUM_EXTENSIONS
          if (GL.currentContext.version < 2) {
            // Calling GLES3/WebGL2 function with a GLES2/WebGL1 context
            GL.recordError(0x502 /* GL_INVALID_OPERATION */);
            return;
          }
          ret = webglGetExtensions().length;
          break;
        case 0x821B: // GL_MAJOR_VERSION
        case 0x821C: // GL_MINOR_VERSION
          if (GL.currentContext.version < 2) {
            GL.recordError(0x500); // GL_INVALID_ENUM
            return;
          }
          ret = name_ == 0x821B ? 3 : 0; // return version 3.0
          break;
      }
  
      if (ret === undefined) {
        var result = GLctx.getParameter(name_);
        switch (typeof result) {
          case 'number':
            ret = result;
            break;
          case 'boolean':
            ret = result ? 1 : 0;
            break;
          case 'string':
            GL.recordError(0x500); // GL_INVALID_ENUM
            return;
          case 'object':
            if (result === null) {
              // null is a valid result for some (e.g., which buffer is bound -
              // perhaps nothing is bound), but otherwise can mean an invalid
              // name_, which we need to report as an error
              switch (name_) {
                case 0x8894: // ARRAY_BUFFER_BINDING
                case 0x8B8D: // CURRENT_PROGRAM
                case 0x8895: // ELEMENT_ARRAY_BUFFER_BINDING
                case 0x8CA6: // FRAMEBUFFER_BINDING or DRAW_FRAMEBUFFER_BINDING
                case 0x8CA7: // RENDERBUFFER_BINDING
                case 0x8069: // TEXTURE_BINDING_2D
                case 0x85B5: // WebGL 2 GL_VERTEX_ARRAY_BINDING, or WebGL 1 extension OES_vertex_array_object GL_VERTEX_ARRAY_BINDING_OES
                case 0x8F36: // COPY_READ_BUFFER_BINDING or COPY_READ_BUFFER
                case 0x8F37: // COPY_WRITE_BUFFER_BINDING or COPY_WRITE_BUFFER
                case 0x88ED: // PIXEL_PACK_BUFFER_BINDING
                case 0x88EF: // PIXEL_UNPACK_BUFFER_BINDING
                case 0x8CAA: // READ_FRAMEBUFFER_BINDING
                case 0x8919: // SAMPLER_BINDING
                case 0x8C1D: // TEXTURE_BINDING_2D_ARRAY
                case 0x806A: // TEXTURE_BINDING_3D
                case 0x8E25: // TRANSFORM_FEEDBACK_BINDING
                case 0x8C8F: // TRANSFORM_FEEDBACK_BUFFER_BINDING
                case 0x8A28: // UNIFORM_BUFFER_BINDING
                case 0x8514: { // TEXTURE_BINDING_CUBE_MAP
                  ret = 0;
                  break;
                }
                default: {
                  GL.recordError(0x500); // GL_INVALID_ENUM
                  return;
                }
              }
            } else if (result instanceof Float32Array ||
                       result instanceof Uint32Array ||
                       result instanceof Int32Array ||
                       result instanceof Array) {
              for (var i = 0; i < result.length; ++i) {
                switch (type) {
                  case 0: HEAP32[(((p)+(i*4))>>2)] = result[i]; break;
                  case 2: HEAPF32[(((p)+(i*4))>>2)] = result[i]; break;
                  case 4: HEAP8[(p)+(i)] = result[i] ? 1 : 0; break;
                }
              }
              return;
            } else {
              try {
                ret = result.name | 0;
              } catch(e) {
                GL.recordError(0x500); // GL_INVALID_ENUM
                err(`GL_INVALID_ENUM in glGet${type}v: Unknown object returned from WebGL getParameter(${name_})! (error: ${e})`);
                return;
              }
            }
            break;
          default:
            GL.recordError(0x500); // GL_INVALID_ENUM
            err(`GL_INVALID_ENUM in glGet${type}v: Native code calling glGet${type}v(${name_}) and it returns ${result} of type ${typeof(result)}!`);
            return;
        }
      }
  
      switch (type) {
        case 1: writeI53ToI64(p, ret); break;
        case 0: HEAP32[((p)>>2)] = ret; break;
        case 2:   HEAPF32[((p)>>2)] = ret; break;
        case 4: HEAP8[p] = ret ? 1 : 0; break;
      }
    };
  
  var _emscripten_glGetBooleanv = (name_, p) => emscriptenWebGLGet(name_, p, 4);

  var _emscripten_glGetBufferParameteri64v = (target, value, data) => {
      if (!data) {
        // GLES2 specification does not specify how to behave if data is a null pointer. Since calling this function does not make sense
        // if data == null, issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      writeI53ToI64(data, GLctx.getBufferParameter(target, value));
    };

  
  var _emscripten_glGetBufferParameteriv = (target, value, data) => {
      if (!data) {
        // GLES2 specification does not specify how to behave if data is a null
        // pointer. Since calling this function does not make sense if data ==
        // null, issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      HEAP32[((data)>>2)] = GLctx.getBufferParameter(target, value);
    };

  
  
  var _emscripten_glGetBufferPointerv = (target, pname, params) => {
      if (pname == 0x88BD/*GL_BUFFER_MAP_POINTER*/) {
        var ptr = 0;
        var mappedBuffer = GL.mappedBuffers[emscriptenWebGLGetBufferBinding(target)];
        if (mappedBuffer) {
          ptr = mappedBuffer.mem;
        }
        HEAP32[((params)>>2)] = ptr;
      } else {
        GL.recordError(0x500/*GL_INVALID_ENUM*/);
        err('GL_INVALID_ENUM in glGetBufferPointerv');
      }
    };

  var _emscripten_glGetError = () => {
      var error = GLctx.getError() || GL.lastError;
      GL.lastError = 0/*GL_NO_ERROR*/;
      return error;
    };

  
  var _emscripten_glGetFloatv = (name_, p) => emscriptenWebGLGet(name_, p, 2);

  var _emscripten_glGetFragDataLocation = (program, name) => {
      return GLctx.getFragDataLocation(GL.programs[program], UTF8ToString(name));
    };

  
  var _emscripten_glGetFramebufferAttachmentParameteriv = (target, attachment, pname, params) => {
      var result = GLctx.getFramebufferAttachmentParameter(target, attachment, pname);
      if (result instanceof WebGLRenderbuffer ||
          result instanceof WebGLTexture) {
        result = result.name | 0;
      }
      HEAP32[((params)>>2)] = result;
    };

  
  
  
  var emscriptenWebGLGetIndexed = (target, index, data, type) => {
      if (!data) {
        // GLES2 specification does not specify how to behave if data is a null pointer. Since calling this function does not make sense
        // if data == null, issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      var result = GLctx.getIndexedParameter(target, index);
      var ret;
      switch (typeof result) {
        case 'boolean':
          ret = result ? 1 : 0;
          break;
        case 'number':
          ret = result;
          break;
        case 'object':
          if (result === null) {
            switch (target) {
              case 0x8C8F: // TRANSFORM_FEEDBACK_BUFFER_BINDING
              case 0x8A28: // UNIFORM_BUFFER_BINDING
                ret = 0;
                break;
              default: {
                GL.recordError(0x500); // GL_INVALID_ENUM
                return;
              }
            }
          } else if (result instanceof WebGLBuffer) {
            ret = result.name | 0;
          } else {
            GL.recordError(0x500); // GL_INVALID_ENUM
            return;
          }
          break;
        default:
          GL.recordError(0x500); // GL_INVALID_ENUM
          return;
      }
  
      switch (type) {
        case 1: writeI53ToI64(data, ret); break;
        case 0: HEAP32[((data)>>2)] = ret; break;
        case 2: HEAPF32[((data)>>2)] = ret; break;
        case 4: HEAP8[data] = ret ? 1 : 0; break;
        default: abort('internal emscriptenWebGLGetIndexed() error, bad type: ' + type);
      }
    };
  var _emscripten_glGetInteger64i_v = (target, index, data) =>
      emscriptenWebGLGetIndexed(target, index, data, 1);

  var _emscripten_glGetInteger64v = (name_, p) => {
      emscriptenWebGLGet(name_, p, 1);
    };

  var _emscripten_glGetIntegeri_v = (target, index, data) =>
      emscriptenWebGLGetIndexed(target, index, data, 0);

  
  var _emscripten_glGetIntegerv = (name_, p) => emscriptenWebGLGet(name_, p, 0);

  var _emscripten_glGetInternalformativ = (target, internalformat, pname, bufSize, params) => {
      if (bufSize < 0) {
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      if (!params) {
        // GLES3 specification does not specify how to behave if values is a null pointer. Since calling this function does not make sense
        // if values == null, issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      var ret = GLctx.getInternalformatParameter(target, internalformat, pname);
      if (ret === null) return;
      for (var i = 0; i < ret.length && i < bufSize; ++i) {
        HEAP32[(((params)+(i*4))>>2)] = ret[i];
      }
    };

  var _emscripten_glGetProgramBinary = (program, bufSize, length, binaryFormat, binary) => {
      GL.recordError(0x502/*GL_INVALID_OPERATION*/);
    };

  
  var _emscripten_glGetProgramInfoLog = (program, maxLength, length, infoLog) => {
      var log = GLctx.getProgramInfoLog(GL.programs[program]);
      if (log === null) log = '(unknown error)';
      var numBytesWrittenExclNull = (maxLength > 0 && infoLog) ? stringToUTF8(log, infoLog, maxLength) : 0;
      if (length) HEAP32[((length)>>2)] = numBytesWrittenExclNull;
    };

  
  var _emscripten_glGetProgramiv = (program, pname, p) => {
      if (!p) {
        // GLES2 specification does not specify how to behave if p is a null
        // pointer. Since calling this function does not make sense if p == null,
        // issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
  
      if (program >= GL.counter) {
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
  
      program = GL.programs[program];
  
      if (pname == 0x8B84) { // GL_INFO_LOG_LENGTH
        var log = GLctx.getProgramInfoLog(program);
        if (log === null) log = '(unknown error)';
        HEAP32[((p)>>2)] = log.length + 1;
      } else if (pname == 0x8B87 /* GL_ACTIVE_UNIFORM_MAX_LENGTH */) {
        if (!program.maxUniformLength) {
          var numActiveUniforms = GLctx.getProgramParameter(program, 0x8B86/*GL_ACTIVE_UNIFORMS*/);
          for (var i = 0; i < numActiveUniforms; ++i) {
            program.maxUniformLength = Math.max(program.maxUniformLength, GLctx.getActiveUniform(program, i).name.length+1);
          }
        }
        HEAP32[((p)>>2)] = program.maxUniformLength;
      } else if (pname == 0x8B8A /* GL_ACTIVE_ATTRIBUTE_MAX_LENGTH */) {
        if (!program.maxAttributeLength) {
          var numActiveAttributes = GLctx.getProgramParameter(program, 0x8B89/*GL_ACTIVE_ATTRIBUTES*/);
          for (var i = 0; i < numActiveAttributes; ++i) {
            program.maxAttributeLength = Math.max(program.maxAttributeLength, GLctx.getActiveAttrib(program, i).name.length+1);
          }
        }
        HEAP32[((p)>>2)] = program.maxAttributeLength;
      } else if (pname == 0x8A35 /* GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH */) {
        if (!program.maxUniformBlockNameLength) {
          var numActiveUniformBlocks = GLctx.getProgramParameter(program, 0x8A36/*GL_ACTIVE_UNIFORM_BLOCKS*/);
          for (var i = 0; i < numActiveUniformBlocks; ++i) {
            program.maxUniformBlockNameLength = Math.max(program.maxUniformBlockNameLength, GLctx.getActiveUniformBlockName(program, i).length+1);
          }
        }
        HEAP32[((p)>>2)] = program.maxUniformBlockNameLength;
      } else {
        HEAP32[((p)>>2)] = GLctx.getProgramParameter(program, pname);
      }
    };

  
  var _emscripten_glGetQueryObjecti64vEXT = (id, pname, params) => {
      if (!params) {
        // GLES2 specification does not specify how to behave if params is a null pointer. Since calling this function does not make sense
        // if p == null, issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      var query = GL.queries[id];
      var param;
      if (GL.currentContext.version < 2)
      {
        param = GLctx.disjointTimerQueryExt['getQueryObjectEXT'](query, pname);
      }
      else {
        param = GLctx.getQueryParameter(query, pname);
      }
      var ret;
      if (typeof param == 'boolean') {
        ret = param ? 1 : 0;
      } else {
        ret = param;
      }
      writeI53ToI64(params, ret);
    };

  
  var _emscripten_glGetQueryObjectivEXT = (id, pname, params) => {
      if (!params) {
        // GLES2 specification does not specify how to behave if params is a null pointer. Since calling this function does not make sense
        // if p == null, issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      var query = GL.queries[id];
      var param = GLctx.disjointTimerQueryExt['getQueryObjectEXT'](query, pname);
      var ret;
      if (typeof param == 'boolean') {
        ret = param ? 1 : 0;
      } else {
        ret = param;
      }
      HEAP32[((params)>>2)] = ret;
    };

  
  var _glGetQueryObjecti64vEXT = _emscripten_glGetQueryObjecti64vEXT;
  var _emscripten_glGetQueryObjectui64vEXT = _glGetQueryObjecti64vEXT;

  var _emscripten_glGetQueryObjectuiv = (id, pname, params) => {
      if (!params) {
        // GLES2 specification does not specify how to behave if params is a null pointer. Since calling this function does not make sense
        // if p == null, issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      var query = GL.queries[id];
      var param = GLctx.getQueryParameter(query, pname);
      var ret;
      if (typeof param == 'boolean') {
        ret = param ? 1 : 0;
      } else {
        ret = param;
      }
      HEAP32[((params)>>2)] = ret;
    };

  
  var _glGetQueryObjectivEXT = _emscripten_glGetQueryObjectivEXT;
  var _emscripten_glGetQueryObjectuivEXT = _glGetQueryObjectivEXT;

  var _emscripten_glGetQueryiv = (target, pname, params) => {
      if (!params) {
        // GLES2 specification does not specify how to behave if params is a null pointer. Since calling this function does not make sense
        // if p == null, issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      HEAP32[((params)>>2)] = GLctx.getQuery(target, pname);
    };

  
  var _emscripten_glGetQueryivEXT = (target, pname, params) => {
      if (!params) {
        // GLES2 specification does not specify how to behave if params is a null pointer. Since calling this function does not make sense
        // if p == null, issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      HEAP32[((params)>>2)] = GLctx.disjointTimerQueryExt['getQueryEXT'](target, pname);
    };

  
  var _emscripten_glGetRenderbufferParameteriv = (target, pname, params) => {
      if (!params) {
        // GLES2 specification does not specify how to behave if params is a null pointer. Since calling this function does not make sense
        // if params == null, issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      HEAP32[((params)>>2)] = GLctx.getRenderbufferParameter(target, pname);
    };

  var _emscripten_glGetSamplerParameterfv = (sampler, pname, params) => {
      if (!params) {
        // GLES3 specification does not specify how to behave if params is a null pointer. Since calling this function does not make sense
        // if p == null, issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      HEAPF32[((params)>>2)] = GLctx.getSamplerParameter(GL.samplers[sampler], pname);
    };

  var _emscripten_glGetSamplerParameteriv = (sampler, pname, params) => {
      if (!params) {
        // GLES3 specification does not specify how to behave if params is a null pointer. Since calling this function does not make sense
        // if p == null, issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      HEAP32[((params)>>2)] = GLctx.getSamplerParameter(GL.samplers[sampler], pname);
    };

  
  
  var _emscripten_glGetShaderInfoLog = (shader, maxLength, length, infoLog) => {
      var log = GLctx.getShaderInfoLog(GL.shaders[shader]);
      if (log === null) log = '(unknown error)';
      var numBytesWrittenExclNull = (maxLength > 0 && infoLog) ? stringToUTF8(log, infoLog, maxLength) : 0;
      if (length) HEAP32[((length)>>2)] = numBytesWrittenExclNull;
    };

  
  var _emscripten_glGetShaderPrecisionFormat = (shaderType, precisionType, range, precision) => {
      var result = GLctx.getShaderPrecisionFormat(shaderType, precisionType);
      HEAP32[((range)>>2)] = result.rangeMin;
      HEAP32[(((range)+(4))>>2)] = result.rangeMax;
      HEAP32[((precision)>>2)] = result.precision;
    };

  
  var _emscripten_glGetShaderSource = (shader, bufSize, length, source) => {
      var result = GLctx.getShaderSource(GL.shaders[shader]);
      if (!result) return; // If an error occurs, nothing will be written to length or source.
      var numBytesWrittenExclNull = (bufSize > 0 && source) ? stringToUTF8(result, source, bufSize) : 0;
      if (length) HEAP32[((length)>>2)] = numBytesWrittenExclNull;
    };

  
  var _emscripten_glGetShaderiv = (shader, pname, p) => {
      if (!p) {
        // GLES2 specification does not specify how to behave if p is a null
        // pointer. Since calling this function does not make sense if p == null,
        // issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      if (pname == 0x8B84) { // GL_INFO_LOG_LENGTH
        var log = GLctx.getShaderInfoLog(GL.shaders[shader]);
        if (log === null) log = '(unknown error)';
        // The GLES2 specification says that if the shader has an empty info log,
        // a value of 0 is returned. Otherwise the log has a null char appended.
        // (An empty string is falsey, so we can just check that instead of
        // looking at log.length.)
        var logLength = log ? log.length + 1 : 0;
        HEAP32[((p)>>2)] = logLength;
      } else if (pname == 0x8B88) { // GL_SHADER_SOURCE_LENGTH
        var source = GLctx.getShaderSource(GL.shaders[shader]);
        // source may be a null, or the empty string, both of which are falsey
        // values that we report a 0 length for.
        var sourceLength = source ? source.length + 1 : 0;
        HEAP32[((p)>>2)] = sourceLength;
      } else {
        HEAP32[((p)>>2)] = GLctx.getShaderParameter(GL.shaders[shader], pname);
      }
    };

  
  
  var _emscripten_glGetString = (name_) => {
      var ret = GL.stringCache[name_];
      if (!ret) {
        switch (name_) {
          case 0x1F03 /* GL_EXTENSIONS */:
            ret = stringToNewUTF8(webglGetExtensions().join(' '));
            break;
          case 0x1F00 /* GL_VENDOR */:
          case 0x1F01 /* GL_RENDERER */:
          case 0x9245 /* UNMASKED_VENDOR_WEBGL */:
          case 0x9246 /* UNMASKED_RENDERER_WEBGL */:
            var s = GLctx.getParameter(name_);
            if (!s) {
              GL.recordError(0x500/*GL_INVALID_ENUM*/);
            }
            ret = s ? stringToNewUTF8(s) : 0;
            break;
  
          case 0x1F02 /* GL_VERSION */:
            var webGLVersion = GLctx.getParameter(0x1F02 /*GL_VERSION*/);
            // return GLES version string corresponding to the version of the WebGL context
            var glVersion = `OpenGL ES 2.0 (${webGLVersion})`;
            if (GL.currentContext.version >= 2) glVersion = `OpenGL ES 3.0 (${webGLVersion})`;
            ret = stringToNewUTF8(glVersion);
            break;
          case 0x8B8C /* GL_SHADING_LANGUAGE_VERSION */:
            var glslVersion = GLctx.getParameter(0x8B8C /*GL_SHADING_LANGUAGE_VERSION*/);
            // extract the version number 'N.M' from the string 'WebGL GLSL ES N.M ...'
            var ver_re = /^WebGL GLSL ES ([0-9]\.[0-9][0-9]?)(?:$| .*)/;
            var ver_num = glslVersion.match(ver_re);
            if (ver_num !== null) {
              if (ver_num[1].length == 3) ver_num[1] = ver_num[1] + '0'; // ensure minor version has 2 digits
              glslVersion = `OpenGL ES GLSL ES ${ver_num[1]} (${glslVersion})`;
            }
            ret = stringToNewUTF8(glslVersion);
            break;
          default:
            GL.recordError(0x500/*GL_INVALID_ENUM*/);
            // fall through
        }
        GL.stringCache[name_] = ret;
      }
      return ret;
    };

  
  var _emscripten_glGetStringi = (name, index) => {
      if (GL.currentContext.version < 2) {
        GL.recordError(0x502 /* GL_INVALID_OPERATION */); // Calling GLES3/WebGL2 function with a GLES2/WebGL1 context
        return 0;
      }
      var stringiCache = GL.stringiCache[name];
      if (stringiCache) {
        if (index < 0 || index >= stringiCache.length) {
          GL.recordError(0x501/*GL_INVALID_VALUE*/);
          return 0;
        }
        return stringiCache[index];
      }
      switch (name) {
        case 0x1F03 /* GL_EXTENSIONS */:
          var exts = webglGetExtensions().map(stringToNewUTF8);
          stringiCache = GL.stringiCache[name] = exts;
          if (index < 0 || index >= stringiCache.length) {
            GL.recordError(0x501/*GL_INVALID_VALUE*/);
            return 0;
          }
          return stringiCache[index];
        default:
          GL.recordError(0x500/*GL_INVALID_ENUM*/);
          return 0;
      }
    };

  var _emscripten_glGetSynciv = (sync, pname, bufSize, length, values) => {
      if (bufSize < 0) {
        // GLES3 specification does not specify how to behave if bufSize < 0, however in the spec wording for glGetInternalformativ, it does say that GL_INVALID_VALUE should be raised,
        // so raise GL_INVALID_VALUE here as well.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      if (!values) {
        // GLES3 specification does not specify how to behave if values is a null pointer. Since calling this function does not make sense
        // if values == null, issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      var ret = GLctx.getSyncParameter(GL.syncs[sync], pname);
      if (ret !== null) {
        HEAP32[((values)>>2)] = ret;
        if (length) HEAP32[((length)>>2)] = 1; // Report a single value outputted.
      }
    };

  
  var _emscripten_glGetTexParameterfv = (target, pname, params) => {
      if (!params) {
        // GLES2 specification does not specify how to behave if params is a null
        // pointer. Since calling this function does not make sense if p == null,
        // issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      HEAPF32[((params)>>2)] = GLctx.getTexParameter(target, pname);
    };

  
  var _emscripten_glGetTexParameteriv = (target, pname, params) => {
      if (!params) {
        // GLES2 specification does not specify how to behave if params is a null
        // pointer. Since calling this function does not make sense if p == null,
        // issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      HEAP32[((params)>>2)] = GLctx.getTexParameter(target, pname);
    };

  var _emscripten_glGetTransformFeedbackVarying = (program, index, bufSize, length, size, type, name) => {
      program = GL.programs[program];
      var info = GLctx.getTransformFeedbackVarying(program, index);
      if (!info) return; // If an error occurred, the return parameters length, size, type and name will be unmodified.
  
      if (name && bufSize > 0) {
        var numBytesWrittenExclNull = stringToUTF8(info.name, name, bufSize);
        if (length) HEAP32[((length)>>2)] = numBytesWrittenExclNull;
      } else {
        if (length) HEAP32[((length)>>2)] = 0;
      }
  
      if (size) HEAP32[((size)>>2)] = info.size;
      if (type) HEAP32[((type)>>2)] = info.type;
    };

  var _emscripten_glGetUniformBlockIndex = (program, uniformBlockName) => {
      return GLctx.getUniformBlockIndex(GL.programs[program], UTF8ToString(uniformBlockName));
    };

  
  
  var _emscripten_glGetUniformIndices = (program, uniformCount, uniformNames, uniformIndices) => {
      if (!uniformIndices) {
        // GLES2 specification does not specify how to behave if uniformIndices is a null pointer. Since calling this function does not make sense
        // if uniformIndices == null, issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      if (uniformCount > 0 && (uniformNames == 0 || uniformIndices == 0)) {
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      program = GL.programs[program];
      var names = [];
      for (var i = 0; i < uniformCount; i++)
        names.push(UTF8ToString(HEAPU32[(((uniformNames)+(i*4))>>2)]));
  
      var result = GLctx.getUniformIndices(program, names);
      if (!result) return; // GL spec: If an error is generated, nothing is written out to uniformIndices.
  
      var len = result.length;
      for (var i = 0; i < len; i++) {
        HEAP32[(((uniformIndices)+(i*4))>>2)] = result[i];
      }
    };

  /** @suppress {checkTypes} */
  var jstoi_q = (str) => parseInt(str);
  
  /** @noinline */
  var webglGetLeftBracePos = (name) => name.slice(-1) == ']' && name.lastIndexOf('[');
  
  var webglPrepareUniformLocationsBeforeFirstUse = (program) => {
      var uniformLocsById = program.uniformLocsById, // Maps GLuint -> WebGLUniformLocation
        uniformSizeAndIdsByName = program.uniformSizeAndIdsByName, // Maps name -> [uniform array length, GLuint]
        i, j;
  
      // On the first time invocation of glGetUniformLocation on this shader program:
      // initialize cache data structures and discover which uniforms are arrays.
      if (!uniformLocsById) {
        // maps GLint integer locations to WebGLUniformLocations
        program.uniformLocsById = uniformLocsById = {};
        // maps integer locations back to uniform name strings, so that we can lazily fetch uniform array locations
        program.uniformArrayNamesById = {};
  
        var numActiveUniforms = GLctx.getProgramParameter(program, 0x8B86/*GL_ACTIVE_UNIFORMS*/);
        for (i = 0; i < numActiveUniforms; ++i) {
          var u = GLctx.getActiveUniform(program, i);
          var nm = u.name;
          var sz = u.size;
          var lb = webglGetLeftBracePos(nm);
          var arrayName = lb > 0 ? nm.slice(0, lb) : nm;
  
          // Assign a new location.
          var id = program.uniformIdCounter;
          program.uniformIdCounter += sz;
          // Eagerly get the location of the uniformArray[0] base element.
          // The remaining indices >0 will be left for lazy evaluation to
          // improve performance. Those may never be needed to fetch, if the
          // application fills arrays always in full starting from the first
          // element of the array.
          uniformSizeAndIdsByName[arrayName] = [sz, id];
  
          // Store placeholder integers in place that highlight that these
          // >0 index locations are array indices pending population.
          for (j = 0; j < sz; ++j) {
            uniformLocsById[id] = j;
            program.uniformArrayNamesById[id++] = arrayName;
          }
        }
      }
    };
  
  
  
  var _emscripten_glGetUniformLocation = (program, name) => {
  
      name = UTF8ToString(name);
  
      if (program = GL.programs[program]) {
        webglPrepareUniformLocationsBeforeFirstUse(program);
        var uniformLocsById = program.uniformLocsById; // Maps GLuint -> WebGLUniformLocation
        var arrayIndex = 0;
        var uniformBaseName = name;
  
        // Invariant: when populating integer IDs for uniform locations, we must
        // maintain the precondition that arrays reside in contiguous addresses,
        // i.e. for a 'vec4 colors[10];', colors[4] must be at location
        // colors[0]+4.  However, user might call glGetUniformLocation(program,
        // "colors") for an array, so we cannot discover based on the user input
        // arguments whether the uniform we are dealing with is an array. The only
        // way to discover which uniforms are arrays is to enumerate over all the
        // active uniforms in the program.
        var leftBrace = webglGetLeftBracePos(name);
  
        // If user passed an array accessor "[index]", parse the array index off the accessor.
        if (leftBrace > 0) {
          arrayIndex = jstoi_q(name.slice(leftBrace + 1)) >>> 0; // "index]", coerce parseInt(']') with >>>0 to treat "foo[]" as "foo[0]" and foo[-1] as unsigned out-of-bounds.
          uniformBaseName = name.slice(0, leftBrace);
        }
  
        // Have we cached the location of this uniform before?
        // A pair [array length, GLint of the uniform location]
        var sizeAndId = program.uniformSizeAndIdsByName[uniformBaseName];
  
        // If a uniform with this name exists, and if its index is within the
        // array limits (if it's even an array), query the WebGLlocation, or
        // return an existing cached location.
        if (sizeAndId && arrayIndex < sizeAndId[0]) {
          arrayIndex += sizeAndId[1]; // Add the base location of the uniform to the array index offset.
          if ((uniformLocsById[arrayIndex] = uniformLocsById[arrayIndex] || GLctx.getUniformLocation(program, name))) {
            return arrayIndex;
          }
        }
      }
      else {
        // N.b. we are currently unable to distinguish between GL program IDs that
        // never existed vs GL program IDs that have been deleted, so report
        // GL_INVALID_VALUE in both cases.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
      }
      return -1;
    };

  
  var webglGetProgramUniformLocation = (program, location) => {
  
      if (program) {
        var webglLoc = program.uniformLocsById[location];
        // program.uniformLocsById[location] stores either an integer, or a
        // WebGLUniformLocation.
        // If an integer, we have not yet bound the location, so do it now. The
        // integer value specifies the array index we should bind to.
        if (typeof webglLoc == 'number') {
          program.uniformLocsById[location] = webglLoc = GLctx.getUniformLocation(program, program.uniformArrayNamesById[location] + (webglLoc > 0 ? `[${webglLoc}]` : ''));
        }
        // Else an already cached WebGLUniformLocation, return it.
        return webglLoc;
      } else {
        GL.recordError(0x502/*GL_INVALID_OPERATION*/);
      }
    };
  
  
  
  
  /** @suppress{checkTypes} */
  var emscriptenWebGLGetUniform = (program, location, params, type) => {
      if (!params) {
        // GLES2 specification does not specify how to behave if params is a null
        // pointer. Since calling this function does not make sense if params ==
        // null, issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      program = GL.programs[program];
      webglPrepareUniformLocationsBeforeFirstUse(program);
      var data = GLctx.getUniform(program, webglGetProgramUniformLocation(program, location));
      if (typeof data == 'number' || typeof data == 'boolean') {
        switch (type) {
          case 0: HEAP32[((params)>>2)] = data; break;
          case 2: HEAPF32[((params)>>2)] = data; break;
        }
      } else {
        for (var i = 0; i < data.length; i++) {
          switch (type) {
            case 0: HEAP32[(((params)+(i*4))>>2)] = data[i]; break;
            case 2: HEAPF32[(((params)+(i*4))>>2)] = data[i]; break;
          }
        }
      }
    };
  
  var _emscripten_glGetUniformfv = (program, location, params) => {
      emscriptenWebGLGetUniform(program, location, params, 2);
    };

  
  var _emscripten_glGetUniformiv = (program, location, params) => {
      emscriptenWebGLGetUniform(program, location, params, 0);
    };

  var _emscripten_glGetUniformuiv = (program, location, params) =>
      emscriptenWebGLGetUniform(program, location, params, 0);

  
  
  /** @suppress{checkTypes} */
  var emscriptenWebGLGetVertexAttrib = (index, pname, params, type) => {
      if (!params) {
        // GLES2 specification does not specify how to behave if params is a null
        // pointer. Since calling this function does not make sense if params ==
        // null, issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      if (GL.currentContext.clientBuffers[index].enabled) {
        err('glGetVertexAttrib*v on client-side array: not supported, bad data returned');
      }
      var data = GLctx.getVertexAttrib(index, pname);
      if (pname == 0x889F/*VERTEX_ATTRIB_ARRAY_BUFFER_BINDING*/) {
        HEAP32[((params)>>2)] = data && data["name"];
      } else if (typeof data == 'number' || typeof data == 'boolean') {
        switch (type) {
          case 0: HEAP32[((params)>>2)] = data; break;
          case 2: HEAPF32[((params)>>2)] = data; break;
          case 5: HEAP32[((params)>>2)] = Math.fround(data); break;
        }
      } else {
        for (var i = 0; i < data.length; i++) {
          switch (type) {
            case 0: HEAP32[(((params)+(i*4))>>2)] = data[i]; break;
            case 2: HEAPF32[(((params)+(i*4))>>2)] = data[i]; break;
            case 5: HEAP32[(((params)+(i*4))>>2)] = Math.fround(data[i]); break;
          }
        }
      }
    };
  var _emscripten_glGetVertexAttribIiv = (index, pname, params) => {
      // N.B. This function may only be called if the vertex attribute was specified using the function glVertexAttribI4iv(),
      // otherwise the results are undefined. (GLES3 spec 6.1.12)
      emscriptenWebGLGetVertexAttrib(index, pname, params, 0);
    };

  
  var _glGetVertexAttribIiv = _emscripten_glGetVertexAttribIiv;
  var _emscripten_glGetVertexAttribIuiv = _glGetVertexAttribIiv;

  
  var _emscripten_glGetVertexAttribPointerv = (index, pname, pointer) => {
      if (!pointer) {
        // GLES2 specification does not specify how to behave if pointer is a null
        // pointer. Since calling this function does not make sense if pointer ==
        // null, issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      if (GL.currentContext.clientBuffers[index].enabled) {
        err('glGetVertexAttribPointer on client-side array: not supported, bad data returned');
      }
      HEAP32[((pointer)>>2)] = GLctx.getVertexAttribOffset(index, pname);
    };

  
  var _emscripten_glGetVertexAttribfv = (index, pname, params) => {
      // N.B. This function may only be called if the vertex attribute was
      // specified using the function glVertexAttrib*f(), otherwise the results
      // are undefined. (GLES3 spec 6.1.12)
      emscriptenWebGLGetVertexAttrib(index, pname, params, 2);
    };

  
  var _emscripten_glGetVertexAttribiv = (index, pname, params) => {
      // N.B. This function may only be called if the vertex attribute was
      // specified using the function glVertexAttrib*f(), otherwise the results
      // are undefined. (GLES3 spec 6.1.12)
      emscriptenWebGLGetVertexAttrib(index, pname, params, 5);
    };

  var _emscripten_glHint = (x0, x1) => GLctx.hint(x0, x1);

  
  var _emscripten_glInvalidateFramebuffer = (target, numAttachments, attachments) => {
      var list = tempFixedLengthArray[numAttachments];
      for (var i = 0; i < numAttachments; i++) {
        list[i] = HEAP32[(((attachments)+(i*4))>>2)];
      }
  
      GLctx.invalidateFramebuffer(target, list);
    };

  
  var _emscripten_glInvalidateSubFramebuffer = (target, numAttachments, attachments, x, y, width, height) => {
      var list = tempFixedLengthArray[numAttachments];
      for (var i = 0; i < numAttachments; i++) {
        list[i] = HEAP32[(((attachments)+(i*4))>>2)];
      }
  
      GLctx.invalidateSubFramebuffer(target, list, x, y, width, height);
    };

  var _emscripten_glIsBuffer = (buffer) => {
      var b = GL.buffers[buffer];
      if (!b) return 0;
      return GLctx.isBuffer(b);
    };

  var _emscripten_glIsEnabled = (x0) => GLctx.isEnabled(x0);

  var _emscripten_glIsFramebuffer = (framebuffer) => {
      var fb = GL.framebuffers[framebuffer];
      if (!fb) return 0;
      return GLctx.isFramebuffer(fb);
    };

  var _emscripten_glIsProgram = (program) => {
      program = GL.programs[program];
      if (!program) return 0;
      return GLctx.isProgram(program);
    };

  var _emscripten_glIsQuery = (id) => {
      var query = GL.queries[id];
      if (!query) return 0;
      return GLctx.isQuery(query);
    };

  var _emscripten_glIsQueryEXT = (id) => {
      var query = GL.queries[id];
      if (!query) return 0;
      return GLctx.disjointTimerQueryExt['isQueryEXT'](query);
    };

  var _emscripten_glIsRenderbuffer = (renderbuffer) => {
      var rb = GL.renderbuffers[renderbuffer];
      if (!rb) return 0;
      return GLctx.isRenderbuffer(rb);
    };

  var _emscripten_glIsSampler = (id) => {
      var sampler = GL.samplers[id];
      if (!sampler) return 0;
      return GLctx.isSampler(sampler);
    };

  var _emscripten_glIsShader = (shader) => {
      var s = GL.shaders[shader];
      if (!s) return 0;
      return GLctx.isShader(s);
    };

  var _emscripten_glIsSync = (sync) => GLctx.isSync(GL.syncs[sync]);

  var _emscripten_glIsTexture = (id) => {
      var texture = GL.textures[id];
      if (!texture) return 0;
      return GLctx.isTexture(texture);
    };

  var _emscripten_glIsTransformFeedback = (id) => GLctx.isTransformFeedback(GL.transformFeedbacks[id]);

  var _emscripten_glIsVertexArray = (array) => {
  
      var vao = GL.vaos[array];
      if (!vao) return 0;
      return GLctx.isVertexArray(vao);
    };

  
  var _glIsVertexArray = _emscripten_glIsVertexArray;
  var _emscripten_glIsVertexArrayOES = _glIsVertexArray;

  var _emscripten_glLineWidth = (x0) => GLctx.lineWidth(x0);

  var _emscripten_glLinkProgram = (program) => {
      program = GL.programs[program];
      GLctx.linkProgram(program);
      // Invalidate earlier computed uniform->ID mappings, those have now become stale
      program.uniformLocsById = 0; // Mark as null-like so that glGetUniformLocation() knows to populate this again.
      program.uniformSizeAndIdsByName = {};
  
    };

  
  
  
  var _emscripten_glMapBufferRange = (target, offset, length, access) => {
      if ((access & (0x1/*GL_MAP_READ_BIT*/ | 0x20/*GL_MAP_UNSYNCHRONIZED_BIT*/)) != 0) {
        err('glMapBufferRange access does not support MAP_READ or MAP_UNSYNCHRONIZED');
        return 0;
      }
  
      if ((access & 0x2/*GL_MAP_WRITE_BIT*/) == 0) {
        err('glMapBufferRange access must include MAP_WRITE');
        return 0;
      }
  
      if ((access & (0x4/*GL_MAP_INVALIDATE_BUFFER_BIT*/ | 0x8/*GL_MAP_INVALIDATE_RANGE_BIT*/)) == 0) {
        err('glMapBufferRange access must include INVALIDATE_BUFFER or INVALIDATE_RANGE');
        return 0;
      }
  
      if (!emscriptenWebGLValidateMapBufferTarget(target)) {
        GL.recordError(0x500/*GL_INVALID_ENUM*/);
        err('GL_INVALID_ENUM in glMapBufferRange');
        return 0;
      }
  
      var mem = _malloc(length), binding = emscriptenWebGLGetBufferBinding(target);
      if (!mem) return 0;
  
      binding = GL.mappedBuffers[binding] ??= {};
      binding.offset = offset;
      binding.length = length;
      binding.mem = mem;
      binding.access = access;
      return mem;
    };

  var _emscripten_glPauseTransformFeedback = () => GLctx.pauseTransformFeedback();

  var _emscripten_glPixelStorei = (pname, param) => {
      if (pname == 3317) {
        GL.unpackAlignment = param;
      } else if (pname == 3314) {
        GL.unpackRowLength = param;
      }
      GLctx.pixelStorei(pname, param);
    };

  var _emscripten_glPolygonModeWEBGL = (face, mode) => {
      GLctx.webglPolygonMode['polygonModeWEBGL'](face, mode);
    };

  var _emscripten_glPolygonOffset = (x0, x1) => GLctx.polygonOffset(x0, x1);

  var _emscripten_glPolygonOffsetClampEXT = (factor, units, clamp) => {
      GLctx.extPolygonOffsetClamp['polygonOffsetClampEXT'](factor, units, clamp);
    };

  var _emscripten_glProgramBinary = (program, binaryFormat, binary, length) => {
      GL.recordError(0x500/*GL_INVALID_ENUM*/);
    };

  var _emscripten_glProgramParameteri = (program, pname, value) => {
      GL.recordError(0x500/*GL_INVALID_ENUM*/);
    };

  var _emscripten_glQueryCounterEXT = (id, target) => {
      GLctx.disjointTimerQueryExt['queryCounterEXT'](GL.queries[id], target);
    };

  var _emscripten_glReadBuffer = (x0) => GLctx.readBuffer(x0);

  var computeUnpackAlignedImageSize = (width, height, sizePerPixel) => {
      function roundedToNextMultipleOf(x, y) {
        return (x + y - 1) & -y;
      }
      var plainRowSize = (GL.unpackRowLength || width) * sizePerPixel;
      var alignedRowSize = roundedToNextMultipleOf(plainRowSize, GL.unpackAlignment);
      return height * alignedRowSize;
    };
  
  var colorChannelsInGlTextureFormat = (format) => {
      // Micro-optimizations for size: map format to size by subtracting smallest
      // enum value (0x1902) from all values first.  Also omit the most common
      // size value (1) from the list, which is assumed by formats not on the
      // list.
      var colorChannels = {
        // 0x1902 /* GL_DEPTH_COMPONENT */ - 0x1902: 1,
        // 0x1906 /* GL_ALPHA */ - 0x1902: 1,
        5: 3,
        6: 4,
        // 0x1909 /* GL_LUMINANCE */ - 0x1902: 1,
        8: 2,
        29502: 3,
        29504: 4,
        // 0x1903 /* GL_RED */ - 0x1902: 1,
        26917: 2,
        26918: 2,
        // 0x8D94 /* GL_RED_INTEGER */ - 0x1902: 1,
        29846: 3,
        29847: 4
      };
      return colorChannels[format - 0x1902]||1;
    };
  
  
  
  
  
  
  
  
  var heapObjectForWebGLType = (type) => {
      // Micro-optimization for size: Subtract lowest GL enum number (0x1400/* GL_BYTE */) from type to compare
      // smaller values for the heap, for shorter generated code size.
      // Also the type HEAPU16 is not tested for explicitly, but any unrecognized type will return out HEAPU16.
      // (since most types are HEAPU16)
      type -= 0x1400;
      if (type == 0) return HEAP8;
  
      if (type == 1) return HEAPU8;
  
      if (type == 2) return HEAP16;
  
      if (type == 4) return HEAP32;
  
      if (type == 6) return HEAPF32;
  
      if (type == 5
        || type == 28922
        || type == 28520
        || type == 30779
        || type == 30782
        )
        return HEAPU32;
  
      return HEAPU16;
    };
  
  var toTypedArrayIndex = (pointer, heap) =>
      pointer >>> (31 - Math.clz32(heap.BYTES_PER_ELEMENT));
  
  var emscriptenWebGLGetTexPixelData = (type, format, width, height, pixels) => {
      var heap = heapObjectForWebGLType(type);
      var sizePerPixel = colorChannelsInGlTextureFormat(format) * heap.BYTES_PER_ELEMENT;
      var bytes = computeUnpackAlignedImageSize(width, height, sizePerPixel);
      return heap.subarray(toTypedArrayIndex(pixels, heap), toTypedArrayIndex(pixels + bytes, heap));
    };
  
  
  
  var _emscripten_glReadPixels = (x, y, width, height, format, type, pixels) => {
      if (GL.currentContext.version >= 2) {
        if (GLctx.currentPixelPackBufferBinding) {
          GLctx.readPixels(x, y, width, height, format, type, pixels);
          return;
        }
        var heap = heapObjectForWebGLType(type);
        var target = toTypedArrayIndex(pixels, heap);
        GLctx.readPixels(x, y, width, height, format, type, heap, target);
        return;
      }
      var pixelData = emscriptenWebGLGetTexPixelData(type, format, width, height, pixels);
      if (!pixelData) {
        GL.recordError(0x500/*GL_INVALID_ENUM*/);
        return;
      }
      GLctx.readPixels(x, y, width, height, format, type, pixelData);
    };

  var _emscripten_glReleaseShaderCompiler = () => {
      // NOP (as allowed by GLES 2.0 spec)
    };

  var _emscripten_glRenderbufferStorage = (x0, x1, x2, x3) => GLctx.renderbufferStorage(x0, x1, x2, x3);

  var _emscripten_glRenderbufferStorageMultisample = (x0, x1, x2, x3, x4) => GLctx.renderbufferStorageMultisample(x0, x1, x2, x3, x4);

  var _emscripten_glResumeTransformFeedback = () => GLctx.resumeTransformFeedback();

  var _emscripten_glSampleCoverage = (value, invert) => {
      GLctx.sampleCoverage(value, !!invert);
    };

  var _emscripten_glSamplerParameterf = (sampler, pname, param) => {
      GLctx.samplerParameterf(GL.samplers[sampler], pname, param);
    };

  var _emscripten_glSamplerParameterfv = (sampler, pname, params) => {
      var param = HEAPF32[((params)>>2)];
      GLctx.samplerParameterf(GL.samplers[sampler], pname, param);
    };

  var _emscripten_glSamplerParameteri = (sampler, pname, param) => {
      GLctx.samplerParameteri(GL.samplers[sampler], pname, param);
    };

  var _emscripten_glSamplerParameteriv = (sampler, pname, params) => {
      var param = HEAP32[((params)>>2)];
      GLctx.samplerParameteri(GL.samplers[sampler], pname, param);
    };

  var _emscripten_glScissor = (x0, x1, x2, x3) => GLctx.scissor(x0, x1, x2, x3);

  var _emscripten_glShaderBinary = (count, shaders, binaryformat, binary, length) => {
      GL.recordError(0x500/*GL_INVALID_ENUM*/);
    };

  var _emscripten_glShaderSource = (shader, count, string, length) => {
      var source = GL.getSource(shader, count, string, length);
  
      GLctx.shaderSource(GL.shaders[shader], source);
    };

  var _emscripten_glStencilFunc = (x0, x1, x2) => GLctx.stencilFunc(x0, x1, x2);

  var _emscripten_glStencilFuncSeparate = (x0, x1, x2, x3) => GLctx.stencilFuncSeparate(x0, x1, x2, x3);

  var _emscripten_glStencilMask = (x0) => GLctx.stencilMask(x0);

  var _emscripten_glStencilMaskSeparate = (x0, x1) => GLctx.stencilMaskSeparate(x0, x1);

  var _emscripten_glStencilOp = (x0, x1, x2) => GLctx.stencilOp(x0, x1, x2);

  var _emscripten_glStencilOpSeparate = (x0, x1, x2, x3) => GLctx.stencilOpSeparate(x0, x1, x2, x3);

  
  
  
  var _emscripten_glTexImage2D = (target, level, internalFormat, width, height, border, format, type, pixels) => {
      if (GL.currentContext.version >= 2) {
        if (GLctx.currentPixelUnpackBufferBinding) {
          GLctx.texImage2D(target, level, internalFormat, width, height, border, format, type, pixels);
          return;
        }
        if (pixels) {
          var heap = heapObjectForWebGLType(type);
          var index = toTypedArrayIndex(pixels, heap);
          GLctx.texImage2D(target, level, internalFormat, width, height, border, format, type, heap, index);
          return;
        }
      }
      var pixelData = pixels ? emscriptenWebGLGetTexPixelData(type, format, width, height, pixels) : null;
      GLctx.texImage2D(target, level, internalFormat, width, height, border, format, type, pixelData);
    };

  
  var _emscripten_glTexImage3D = (target, level, internalFormat, width, height, depth, border, format, type, pixels) => {
      if (GLctx.currentPixelUnpackBufferBinding) {
        GLctx.texImage3D(target, level, internalFormat, width, height, depth, border, format, type, pixels);
      } else if (pixels) {
        var heap = heapObjectForWebGLType(type);
        GLctx.texImage3D(target, level, internalFormat, width, height, depth, border, format, type, heap, toTypedArrayIndex(pixels, heap));
      } else {
        GLctx.texImage3D(target, level, internalFormat, width, height, depth, border, format, type, null);
      }
    };

  var _emscripten_glTexParameterf = (x0, x1, x2) => GLctx.texParameterf(x0, x1, x2);

  
  var _emscripten_glTexParameterfv = (target, pname, params) => {
      var param = HEAPF32[((params)>>2)];
      GLctx.texParameterf(target, pname, param);
    };

  var _emscripten_glTexParameteri = (x0, x1, x2) => GLctx.texParameteri(x0, x1, x2);

  
  var _emscripten_glTexParameteriv = (target, pname, params) => {
      var param = HEAP32[((params)>>2)];
      GLctx.texParameteri(target, pname, param);
    };

  var _emscripten_glTexStorage2D = (x0, x1, x2, x3, x4) => GLctx.texStorage2D(x0, x1, x2, x3, x4);

  var _emscripten_glTexStorage3D = (x0, x1, x2, x3, x4, x5) => GLctx.texStorage3D(x0, x1, x2, x3, x4, x5);

  
  
  
  var _emscripten_glTexSubImage2D = (target, level, xoffset, yoffset, width, height, format, type, pixels) => {
      if (GL.currentContext.version >= 2) {
        if (GLctx.currentPixelUnpackBufferBinding) {
          GLctx.texSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
          return;
        }
        if (pixels) {
          var heap = heapObjectForWebGLType(type);
          GLctx.texSubImage2D(target, level, xoffset, yoffset, width, height, format, type, heap, toTypedArrayIndex(pixels, heap));
          return;
        }
      }
      var pixelData = pixels ? emscriptenWebGLGetTexPixelData(type, format, width, height, pixels) : null;
      GLctx.texSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixelData);
    };

  
  var _emscripten_glTexSubImage3D = (target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels) => {
      if (GLctx.currentPixelUnpackBufferBinding) {
        GLctx.texSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
      } else if (pixels) {
        var heap = heapObjectForWebGLType(type);
        GLctx.texSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, heap, toTypedArrayIndex(pixels, heap));
      } else {
        GLctx.texSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, null);
      }
    };

  
  var _emscripten_glTransformFeedbackVaryings = (program, count, varyings, bufferMode) => {
      program = GL.programs[program];
      var vars = [];
      for (var i = 0; i < count; i++)
        vars.push(UTF8ToString(HEAPU32[(((varyings)+(i*4))>>2)]));
  
      GLctx.transformFeedbackVaryings(program, vars, bufferMode);
    };

  
  var webglGetUniformLocation = (location) => {
  
      return webglGetProgramUniformLocation(GLctx.currentProgram, location);
    };
  
  var _emscripten_glUniform1f = (location, v0) => {
      GLctx.uniform1f(webglGetUniformLocation(location), v0);
    };

  
  var miniTempWebGLFloatBuffers = [];
  
  
  var _emscripten_glUniform1fv = (location, count, value) => {
  
      if (GL.currentContext.version >= 2) {
        count && GLctx.uniform1fv(webglGetUniformLocation(location), HEAPF32, ((value)>>2), count);
        return;
      }
  
      if (count <= 288) {
        // avoid allocation when uploading few enough uniforms
        var view = miniTempWebGLFloatBuffers[count];
        for (var i = 0; i < count; ++i) {
          view[i] = HEAPF32[(((value)+(4*i))>>2)];
        }
      } else
      {
        var view = HEAPF32.subarray((((value)>>2)), ((value+count*4)>>2));
      }
      GLctx.uniform1fv(webglGetUniformLocation(location), view);
    };

  
  var _emscripten_glUniform1i = (location, v0) => {
      GLctx.uniform1i(webglGetUniformLocation(location), v0);
    };

  
  var miniTempWebGLIntBuffers = [];
  
  
  var _emscripten_glUniform1iv = (location, count, value) => {
  
      if (GL.currentContext.version >= 2) {
        count && GLctx.uniform1iv(webglGetUniformLocation(location), HEAP32, ((value)>>2), count);
        return;
      }
  
      if (count <= 288) {
        // avoid allocation when uploading few enough uniforms
        var view = miniTempWebGLIntBuffers[count];
        for (var i = 0; i < count; ++i) {
          view[i] = HEAP32[(((value)+(4*i))>>2)];
        }
      } else
      {
        var view = HEAP32.subarray((((value)>>2)), ((value+count*4)>>2));
      }
      GLctx.uniform1iv(webglGetUniformLocation(location), view);
    };

  var _emscripten_glUniform1ui = (location, v0) => {
      GLctx.uniform1ui(webglGetUniformLocation(location), v0);
    };

  
  var _emscripten_glUniform1uiv = (location, count, value) => {
      count && GLctx.uniform1uiv(webglGetUniformLocation(location), HEAPU32, ((value)>>2), count);
    };

  
  var _emscripten_glUniform2f = (location, v0, v1) => {
      GLctx.uniform2f(webglGetUniformLocation(location), v0, v1);
    };

  
  
  
  var _emscripten_glUniform2fv = (location, count, value) => {
  
      if (GL.currentContext.version >= 2) {
        count && GLctx.uniform2fv(webglGetUniformLocation(location), HEAPF32, ((value)>>2), count*2);
        return;
      }
  
      if (count <= 144) {
        // avoid allocation when uploading few enough uniforms
        count *= 2;
        var view = miniTempWebGLFloatBuffers[count];
        for (var i = 0; i < count; i += 2) {
          view[i] = HEAPF32[(((value)+(4*i))>>2)];
          view[i+1] = HEAPF32[(((value)+(4*i+4))>>2)];
        }
      } else
      {
        var view = HEAPF32.subarray((((value)>>2)), ((value+count*8)>>2));
      }
      GLctx.uniform2fv(webglGetUniformLocation(location), view);
    };

  
  var _emscripten_glUniform2i = (location, v0, v1) => {
      GLctx.uniform2i(webglGetUniformLocation(location), v0, v1);
    };

  
  
  
  var _emscripten_glUniform2iv = (location, count, value) => {
  
      if (GL.currentContext.version >= 2) {
        count && GLctx.uniform2iv(webglGetUniformLocation(location), HEAP32, ((value)>>2), count*2);
        return;
      }
  
      if (count <= 144) {
        // avoid allocation when uploading few enough uniforms
        count *= 2;
        var view = miniTempWebGLIntBuffers[count];
        for (var i = 0; i < count; i += 2) {
          view[i] = HEAP32[(((value)+(4*i))>>2)];
          view[i+1] = HEAP32[(((value)+(4*i+4))>>2)];
        }
      } else
      {
        var view = HEAP32.subarray((((value)>>2)), ((value+count*8)>>2));
      }
      GLctx.uniform2iv(webglGetUniformLocation(location), view);
    };

  var _emscripten_glUniform2ui = (location, v0, v1) => {
      GLctx.uniform2ui(webglGetUniformLocation(location), v0, v1);
    };

  
  var _emscripten_glUniform2uiv = (location, count, value) => {
      count && GLctx.uniform2uiv(webglGetUniformLocation(location), HEAPU32, ((value)>>2), count*2);
    };

  
  var _emscripten_glUniform3f = (location, v0, v1, v2) => {
      GLctx.uniform3f(webglGetUniformLocation(location), v0, v1, v2);
    };

  
  
  
  var _emscripten_glUniform3fv = (location, count, value) => {
  
      if (GL.currentContext.version >= 2) {
        count && GLctx.uniform3fv(webglGetUniformLocation(location), HEAPF32, ((value)>>2), count*3);
        return;
      }
  
      if (count <= 96) {
        // avoid allocation when uploading few enough uniforms
        count *= 3;
        var view = miniTempWebGLFloatBuffers[count];
        for (var i = 0; i < count; i += 3) {
          view[i] = HEAPF32[(((value)+(4*i))>>2)];
          view[i+1] = HEAPF32[(((value)+(4*i+4))>>2)];
          view[i+2] = HEAPF32[(((value)+(4*i+8))>>2)];
        }
      } else
      {
        var view = HEAPF32.subarray((((value)>>2)), ((value+count*12)>>2));
      }
      GLctx.uniform3fv(webglGetUniformLocation(location), view);
    };

  
  var _emscripten_glUniform3i = (location, v0, v1, v2) => {
      GLctx.uniform3i(webglGetUniformLocation(location), v0, v1, v2);
    };

  
  
  
  var _emscripten_glUniform3iv = (location, count, value) => {
  
      if (GL.currentContext.version >= 2) {
        count && GLctx.uniform3iv(webglGetUniformLocation(location), HEAP32, ((value)>>2), count*3);
        return;
      }
  
      if (count <= 96) {
        // avoid allocation when uploading few enough uniforms
        count *= 3;
        var view = miniTempWebGLIntBuffers[count];
        for (var i = 0; i < count; i += 3) {
          view[i] = HEAP32[(((value)+(4*i))>>2)];
          view[i+1] = HEAP32[(((value)+(4*i+4))>>2)];
          view[i+2] = HEAP32[(((value)+(4*i+8))>>2)];
        }
      } else
      {
        var view = HEAP32.subarray((((value)>>2)), ((value+count*12)>>2));
      }
      GLctx.uniform3iv(webglGetUniformLocation(location), view);
    };

  var _emscripten_glUniform3ui = (location, v0, v1, v2) => {
      GLctx.uniform3ui(webglGetUniformLocation(location), v0, v1, v2);
    };

  
  var _emscripten_glUniform3uiv = (location, count, value) => {
      count && GLctx.uniform3uiv(webglGetUniformLocation(location), HEAPU32, ((value)>>2), count*3);
    };

  
  var _emscripten_glUniform4f = (location, v0, v1, v2, v3) => {
      GLctx.uniform4f(webglGetUniformLocation(location), v0, v1, v2, v3);
    };

  
  
  
  var _emscripten_glUniform4fv = (location, count, value) => {
  
      if (GL.currentContext.version >= 2) {
        count && GLctx.uniform4fv(webglGetUniformLocation(location), HEAPF32, ((value)>>2), count*4);
        return;
      }
  
      if (count <= 72) {
        // avoid allocation when uploading few enough uniforms
        var view = miniTempWebGLFloatBuffers[4*count];
        // hoist the heap out of the loop for size and for pthreads+growth.
        var heap = HEAPF32;
        value = ((value)>>2);
        count *= 4;
        for (var i = 0; i < count; i += 4) {
          var dst = value + i;
          view[i] = heap[dst];
          view[i + 1] = heap[dst + 1];
          view[i + 2] = heap[dst + 2];
          view[i + 3] = heap[dst + 3];
        }
      } else
      {
        var view = HEAPF32.subarray((((value)>>2)), ((value+count*16)>>2));
      }
      GLctx.uniform4fv(webglGetUniformLocation(location), view);
    };

  
  var _emscripten_glUniform4i = (location, v0, v1, v2, v3) => {
      GLctx.uniform4i(webglGetUniformLocation(location), v0, v1, v2, v3);
    };

  
  
  
  var _emscripten_glUniform4iv = (location, count, value) => {
  
      if (GL.currentContext.version >= 2) {
        count && GLctx.uniform4iv(webglGetUniformLocation(location), HEAP32, ((value)>>2), count*4);
        return;
      }
  
      if (count <= 72) {
        // avoid allocation when uploading few enough uniforms
        count *= 4;
        var view = miniTempWebGLIntBuffers[count];
        for (var i = 0; i < count; i += 4) {
          view[i] = HEAP32[(((value)+(4*i))>>2)];
          view[i+1] = HEAP32[(((value)+(4*i+4))>>2)];
          view[i+2] = HEAP32[(((value)+(4*i+8))>>2)];
          view[i+3] = HEAP32[(((value)+(4*i+12))>>2)];
        }
      } else
      {
        var view = HEAP32.subarray((((value)>>2)), ((value+count*16)>>2));
      }
      GLctx.uniform4iv(webglGetUniformLocation(location), view);
    };

  var _emscripten_glUniform4ui = (location, v0, v1, v2, v3) => {
      GLctx.uniform4ui(webglGetUniformLocation(location), v0, v1, v2, v3);
    };

  
  var _emscripten_glUniform4uiv = (location, count, value) => {
      count && GLctx.uniform4uiv(webglGetUniformLocation(location), HEAPU32, ((value)>>2), count*4);
    };

  var _emscripten_glUniformBlockBinding = (program, uniformBlockIndex, uniformBlockBinding) => {
      program = GL.programs[program];
  
      GLctx.uniformBlockBinding(program, uniformBlockIndex, uniformBlockBinding);
    };

  
  
  
  var _emscripten_glUniformMatrix2fv = (location, count, transpose, value) => {
  
      if (GL.currentContext.version >= 2) {
        count && GLctx.uniformMatrix2fv(webglGetUniformLocation(location), !!transpose, HEAPF32, ((value)>>2), count*4);
        return;
      }
  
      if (count <= 72) {
        // avoid allocation when uploading few enough uniforms
        count *= 4;
        var view = miniTempWebGLFloatBuffers[count];
        for (var i = 0; i < count; i += 4) {
          view[i] = HEAPF32[(((value)+(4*i))>>2)];
          view[i+1] = HEAPF32[(((value)+(4*i+4))>>2)];
          view[i+2] = HEAPF32[(((value)+(4*i+8))>>2)];
          view[i+3] = HEAPF32[(((value)+(4*i+12))>>2)];
        }
      } else
      {
        var view = HEAPF32.subarray((((value)>>2)), ((value+count*16)>>2));
      }
      GLctx.uniformMatrix2fv(webglGetUniformLocation(location), !!transpose, view);
    };

  
  var _emscripten_glUniformMatrix2x3fv = (location, count, transpose, value) => {
      count && GLctx.uniformMatrix2x3fv(webglGetUniformLocation(location), !!transpose, HEAPF32, ((value)>>2), count*6);
    };

  
  var _emscripten_glUniformMatrix2x4fv = (location, count, transpose, value) => {
      count && GLctx.uniformMatrix2x4fv(webglGetUniformLocation(location), !!transpose, HEAPF32, ((value)>>2), count*8);
    };

  
  
  
  var _emscripten_glUniformMatrix3fv = (location, count, transpose, value) => {
  
      if (GL.currentContext.version >= 2) {
        count && GLctx.uniformMatrix3fv(webglGetUniformLocation(location), !!transpose, HEAPF32, ((value)>>2), count*9);
        return;
      }
  
      if (count <= 32) {
        // avoid allocation when uploading few enough uniforms
        count *= 9;
        var view = miniTempWebGLFloatBuffers[count];
        for (var i = 0; i < count; i += 9) {
          view[i] = HEAPF32[(((value)+(4*i))>>2)];
          view[i+1] = HEAPF32[(((value)+(4*i+4))>>2)];
          view[i+2] = HEAPF32[(((value)+(4*i+8))>>2)];
          view[i+3] = HEAPF32[(((value)+(4*i+12))>>2)];
          view[i+4] = HEAPF32[(((value)+(4*i+16))>>2)];
          view[i+5] = HEAPF32[(((value)+(4*i+20))>>2)];
          view[i+6] = HEAPF32[(((value)+(4*i+24))>>2)];
          view[i+7] = HEAPF32[(((value)+(4*i+28))>>2)];
          view[i+8] = HEAPF32[(((value)+(4*i+32))>>2)];
        }
      } else
      {
        var view = HEAPF32.subarray((((value)>>2)), ((value+count*36)>>2));
      }
      GLctx.uniformMatrix3fv(webglGetUniformLocation(location), !!transpose, view);
    };

  
  var _emscripten_glUniformMatrix3x2fv = (location, count, transpose, value) => {
      count && GLctx.uniformMatrix3x2fv(webglGetUniformLocation(location), !!transpose, HEAPF32, ((value)>>2), count*6);
    };

  
  var _emscripten_glUniformMatrix3x4fv = (location, count, transpose, value) => {
      count && GLctx.uniformMatrix3x4fv(webglGetUniformLocation(location), !!transpose, HEAPF32, ((value)>>2), count*12);
    };

  
  
  
  var _emscripten_glUniformMatrix4fv = (location, count, transpose, value) => {
  
      if (GL.currentContext.version >= 2) {
        count && GLctx.uniformMatrix4fv(webglGetUniformLocation(location), !!transpose, HEAPF32, ((value)>>2), count*16);
        return;
      }
  
      if (count <= 18) {
        // avoid allocation when uploading few enough uniforms
        var view = miniTempWebGLFloatBuffers[16*count];
        // hoist the heap out of the loop for size and for pthreads+growth.
        var heap = HEAPF32;
        value = ((value)>>2);
        count *= 16;
        for (var i = 0; i < count; i += 16) {
          var dst = value + i;
          view[i] = heap[dst];
          view[i + 1] = heap[dst + 1];
          view[i + 2] = heap[dst + 2];
          view[i + 3] = heap[dst + 3];
          view[i + 4] = heap[dst + 4];
          view[i + 5] = heap[dst + 5];
          view[i + 6] = heap[dst + 6];
          view[i + 7] = heap[dst + 7];
          view[i + 8] = heap[dst + 8];
          view[i + 9] = heap[dst + 9];
          view[i + 10] = heap[dst + 10];
          view[i + 11] = heap[dst + 11];
          view[i + 12] = heap[dst + 12];
          view[i + 13] = heap[dst + 13];
          view[i + 14] = heap[dst + 14];
          view[i + 15] = heap[dst + 15];
        }
      } else
      {
        var view = HEAPF32.subarray((((value)>>2)), ((value+count*64)>>2));
      }
      GLctx.uniformMatrix4fv(webglGetUniformLocation(location), !!transpose, view);
    };

  
  var _emscripten_glUniformMatrix4x2fv = (location, count, transpose, value) => {
      count && GLctx.uniformMatrix4x2fv(webglGetUniformLocation(location), !!transpose, HEAPF32, ((value)>>2), count*8);
    };

  
  var _emscripten_glUniformMatrix4x3fv = (location, count, transpose, value) => {
      count && GLctx.uniformMatrix4x3fv(webglGetUniformLocation(location), !!transpose, HEAPF32, ((value)>>2), count*12);
    };

  
  
  
  var _emscripten_glUnmapBuffer = (target) => {
      if (!emscriptenWebGLValidateMapBufferTarget(target)) {
        GL.recordError(0x500/*GL_INVALID_ENUM*/);
        err('GL_INVALID_ENUM in glUnmapBuffer');
        return 0;
      }
  
      var buffer = emscriptenWebGLGetBufferBinding(target);
      var mapping = GL.mappedBuffers[buffer];
      if (!mapping || !mapping.mem) {
        GL.recordError(0x502 /* GL_INVALID_OPERATION */);
        err('buffer was never mapped in glUnmapBuffer');
        return 0;
      }
  
      if (!(mapping.access & 0x10)) { /* GL_MAP_FLUSH_EXPLICIT_BIT */
        webglBufferSubData(target, mapping.offset, mapping.length, mapping.mem);
      }
  
      _free(mapping.mem);
      mapping.mem = 0;
      return 1;
    };

  var _emscripten_glUseProgram = (program) => {
      program = GL.programs[program];
      GLctx.useProgram(program);
      // Record the currently active program so that we can access the uniform
      // mapping table of that program.
      GLctx.currentProgram = program;
    };

  var _emscripten_glValidateProgram = (program) => {
      GLctx.validateProgram(GL.programs[program]);
    };

  var _emscripten_glVertexAttrib1f = (x0, x1) => GLctx.vertexAttrib1f(x0, x1);

  
  var _emscripten_glVertexAttrib1fv = (index, v) => {
  
      GLctx.vertexAttrib1f(index, HEAPF32[v>>2]);
    };

  var _emscripten_glVertexAttrib2f = (x0, x1, x2) => GLctx.vertexAttrib2f(x0, x1, x2);

  
  var _emscripten_glVertexAttrib2fv = (index, v) => {
  
      GLctx.vertexAttrib2f(index, HEAPF32[v>>2], HEAPF32[v+4>>2]);
    };

  var _emscripten_glVertexAttrib3f = (x0, x1, x2, x3) => GLctx.vertexAttrib3f(x0, x1, x2, x3);

  
  var _emscripten_glVertexAttrib3fv = (index, v) => {
  
      GLctx.vertexAttrib3f(index, HEAPF32[v>>2], HEAPF32[v+4>>2], HEAPF32[v+8>>2]);
    };

  var _emscripten_glVertexAttrib4f = (x0, x1, x2, x3, x4) => GLctx.vertexAttrib4f(x0, x1, x2, x3, x4);

  
  var _emscripten_glVertexAttrib4fv = (index, v) => {
  
      GLctx.vertexAttrib4f(index, HEAPF32[v>>2], HEAPF32[v+4>>2], HEAPF32[v+8>>2], HEAPF32[v+12>>2]);
    };

  var _emscripten_glVertexAttribDivisor = (index, divisor) => {
      GLctx.vertexAttribDivisor(index, divisor);
    };

  
  var _glVertexAttribDivisor = _emscripten_glVertexAttribDivisor;
  var _emscripten_glVertexAttribDivisorANGLE = _glVertexAttribDivisor;

  
  var _emscripten_glVertexAttribDivisorARB = _glVertexAttribDivisor;

  
  var _emscripten_glVertexAttribDivisorEXT = _glVertexAttribDivisor;

  
  var _emscripten_glVertexAttribDivisorNV = _glVertexAttribDivisor;

  var _emscripten_glVertexAttribI4i = (x0, x1, x2, x3, x4) => GLctx.vertexAttribI4i(x0, x1, x2, x3, x4);

  var _emscripten_glVertexAttribI4iv = (index, v) => {
      GLctx.vertexAttribI4i(index, HEAP32[v>>2], HEAP32[v+4>>2], HEAP32[v+8>>2], HEAP32[v+12>>2]);
    };

  var _emscripten_glVertexAttribI4ui = (x0, x1, x2, x3, x4) => GLctx.vertexAttribI4ui(x0, x1, x2, x3, x4);

  var _emscripten_glVertexAttribI4uiv = (index, v) => {
      GLctx.vertexAttribI4ui(index, HEAPU32[v>>2], HEAPU32[v+4>>2], HEAPU32[v+8>>2], HEAPU32[v+12>>2]);
    };

  var _emscripten_glVertexAttribIPointer = (index, size, type, stride, ptr) => {
      var cb = GL.currentContext.clientBuffers[index];
      if (!GLctx.currentArrayBufferBinding) {
        cb.size = size;
        cb.type = type;
        cb.normalized = false;
        cb.stride = stride;
        cb.ptr = ptr;
        cb.clientside = true;
        cb.vertexAttribPointerAdaptor = /** @this {WebGLRenderingContext} */ function(index, size, type, normalized, stride, ptr) {
          this.vertexAttribIPointer(index, size, type, stride, ptr);
        };
        return;
      }
      cb.clientside = false;
      GLctx.vertexAttribIPointer(index, size, type, stride, ptr);
    };

  var _emscripten_glVertexAttribPointer = (index, size, type, normalized, stride, ptr) => {
      var cb = GL.currentContext.clientBuffers[index];
      if (!GLctx.currentArrayBufferBinding) {
        cb.size = size;
        cb.type = type;
        cb.normalized = normalized;
        cb.stride = stride;
        cb.ptr = ptr;
        cb.clientside = true;
        cb.vertexAttribPointerAdaptor = /** @this {WebGLRenderingContext} */ function(index, size, type, normalized, stride, ptr) {
          this.vertexAttribPointer(index, size, type, normalized, stride, ptr);
        };
        return;
      }
      cb.clientside = false;
      GLctx.vertexAttribPointer(index, size, type, !!normalized, stride, ptr);
    };

  var _emscripten_glViewport = (x0, x1, x2, x3) => GLctx.viewport(x0, x1, x2, x3);

  var _emscripten_glWaitSync = (sync, flags, timeout) => {
      // See WebGL2 vs GLES3 difference on GL_TIMEOUT_IGNORED above (https://www.khronos.org/registry/webgl/specs/latest/2.0/#5.15)
      timeout = Number(timeout);
      GLctx.waitSync(GL.syncs[sync], flags, timeout);
    };

  var _emscripten_has_asyncify = () => 1;

  
  
  var doRequestFullscreen = (target, strategy) => {
      if (!JSEvents.fullscreenEnabled()) return -1;
      target = findEventTarget(target);
      if (!target) return -4;
  
      if (!target.requestFullscreen
        // Safari didn't Element.requestFullscreen support until 16.4
        // See: https://developer.mozilla.org/en-US/docs/Web/API/Element/requestFullscreen
        && !target.webkitRequestFullscreen
        ) {
        return -3;
      }
  
      // Queue this function call if we're not currently in an event handler and
      // the user saw it appropriate to do so.
      if (!JSEvents.canPerformEventHandlerRequests()) {
        if (strategy.deferUntilInEventHandler) {
          JSEvents.deferCall(JSEvents_requestFullscreen, 1 /* priority over pointer lock */, [target, strategy]);
          return 1;
        }
        return -2;
      }
  
      return JSEvents_requestFullscreen(target, strategy);
    };
  
  var _emscripten_request_fullscreen_strategy = (target, deferUntilInEventHandler, fullscreenStrategy) => {
      var strategy = {
        scaleMode: HEAP32[((fullscreenStrategy)>>2)],
        canvasResolutionScaleMode: HEAP32[(((fullscreenStrategy)+(4))>>2)],
        filteringMode: HEAP32[(((fullscreenStrategy)+(8))>>2)],
        deferUntilInEventHandler,
        canvasResizedCallback: HEAP32[(((fullscreenStrategy)+(12))>>2)],
        canvasResizedCallbackUserData: HEAP32[(((fullscreenStrategy)+(16))>>2)]
      };
  
      return doRequestFullscreen(target, strategy);
    };

  
  
  var _emscripten_request_pointerlock = (target, deferUntilInEventHandler) => {
      target = findEventTarget(target);
      if (!target) return -4;
      if (!target.requestPointerLock) {
        return -1;
      }
  
      // Queue this function call if we're not currently in an event handler and
      // the user saw it appropriate to do so.
      if (!JSEvents.canPerformEventHandlerRequests()) {
        if (deferUntilInEventHandler) {
          JSEvents.deferCall(requestPointerLock, 2 /* priority below fullscreen */, [target]);
          return 1;
        }
        return -2;
      }
  
      return requestPointerLock(target);
    };

  
  
  var growMemory = (size) => {
      var oldHeapSize = wasmMemory.buffer.byteLength;
      var pages = ((size - oldHeapSize + 65535) / 65536) | 0;
      try {
        // round size grow request up to wasm page size (fixed 64KB per spec)
        wasmMemory.grow(pages); // .grow() takes a delta compared to the previous size
        updateMemoryViews();
        return 1 /*success*/;
      } catch(e) {
        err(`growMemory: Attempted to grow heap from ${oldHeapSize} bytes to ${size} bytes, but got error: ${e}`);
      }
      // implicit 0 return to save code size (caller will cast 'undefined' into 0
      // anyhow)
    };
  
  var _emscripten_resize_heap = (requestedSize) => {
      var oldSize = HEAPU8.length;
      // With CAN_ADDRESS_2GB or MEMORY64, pointers are already unsigned.
      requestedSize >>>= 0;
      // With multithreaded builds, races can happen (another thread might increase the size
      // in between), so return a failure, and let the caller retry.
      assert(requestedSize > oldSize);
  
      // Memory resize rules:
      // 1.  Always increase heap size to at least the requested size, rounded up
      //     to next page multiple.
      // 2a. If MEMORY_GROWTH_LINEAR_STEP == -1, excessively resize the heap
      //     geometrically: increase the heap size according to
      //     MEMORY_GROWTH_GEOMETRIC_STEP factor (default +20%), At most
      //     overreserve by MEMORY_GROWTH_GEOMETRIC_CAP bytes (default 96MB).
      // 2b. If MEMORY_GROWTH_LINEAR_STEP != -1, excessively resize the heap
      //     linearly: increase the heap size by at least
      //     MEMORY_GROWTH_LINEAR_STEP bytes.
      // 3.  Max size for the heap is capped at 2048MB-WASM_PAGE_SIZE, or by
      //     MAXIMUM_MEMORY, or by ASAN limit, depending on which is smallest
      // 4.  If we were unable to allocate as much memory, it may be due to
      //     over-eager decision to excessively reserve due to (3) above.
      //     Hence if an allocation fails, cut down on the amount of excess
      //     growth, in an attempt to succeed to perform a smaller allocation.
  
      // A limit is set for how much we can grow. We should not exceed that
      // (the wasm binary specifies it, so if we tried, we'd fail anyhow).
      var maxHeapSize = getHeapMax();
      if (requestedSize > maxHeapSize) {
        err(`Cannot enlarge memory, requested ${requestedSize} bytes, but the limit is ${maxHeapSize} bytes!`);
        return false;
      }
  
      // Loop through potential heap size increases. If we attempt a too eager
      // reservation that fails, cut down on the attempted size and reserve a
      // smaller bump instead. (max 3 times, chosen somewhat arbitrarily)
      for (var cutDown = 1; cutDown <= 4; cutDown *= 2) {
        var overGrownHeapSize = oldSize * (1 + 0.2 / cutDown); // ensure geometric growth
        // but limit overreserving (default to capping at +96MB overgrowth at most)
        overGrownHeapSize = Math.min(overGrownHeapSize, requestedSize + 100663296 );
  
        var newSize = Math.min(maxHeapSize, alignMemory(Math.max(requestedSize, overGrownHeapSize), 65536));
  
        var replacement = growMemory(newSize);
        if (replacement) {
  
          return true;
        }
      }
      err(`Failed to grow the heap from ${oldSize} bytes to ${newSize} bytes, not enough memory!`);
      return false;
    };

  /** @suppress {checkTypes} */
  var _emscripten_sample_gamepad_data = () => {
      try {
        if (navigator.getGamepads) return (JSEvents.lastGamepadState = navigator.getGamepads())
          ? 0 : -1;
      } catch(e) {
        err(`navigator.getGamepads() exists, but failed to execute with exception ${e}. Disabling Gamepad access.`);
        navigator.getGamepads = null; // Disable getGamepads() so that it won't be attempted to be used again.
      }
      return -1;
    };

  
  
  var registerBeforeUnloadEventCallback = (target, userData, useCapture, callbackfunc, eventTypeId, eventTypeString) => {
      var beforeUnloadEventHandlerFunc = (e) => {
        // Note: This is always called on the main browser thread, since it needs synchronously return a value!
        var confirmationMessage = ((a1, a2, a3) => dynCall_iiii(callbackfunc, a1, a2, a3))(eventTypeId, 0, userData);
  
        if (confirmationMessage) {
          confirmationMessage = UTF8ToString(confirmationMessage);
        }
        if (confirmationMessage) {
          e.preventDefault();
          e.returnValue = confirmationMessage;
          return confirmationMessage;
        }
      };
  
      var eventHandler = {
        target: findEventTarget(target),
        eventTypeString,
        eventTypeId,
        userData,
        callbackfunc,
        handlerFunc: beforeUnloadEventHandlerFunc,
        useCapture
      };
      return JSEvents.registerOrRemoveHandler(eventHandler);
    };
  var _emscripten_set_beforeunload_callback_on_thread = (userData, callbackfunc, targetThread) => {
      if (typeof onbeforeunload == 'undefined') return -1;
      // beforeunload callback can only be registered on the main browser thread, because the page will go away immediately after returning from the handler,
      // and there is no time to start proxying it anywhere.
      if (targetThread !== 1) return -5;
      return registerBeforeUnloadEventCallback(2, userData, true, callbackfunc, 28, 'beforeunload');
    };

  
  
  
  var registerFocusEventCallback = (target, userData, useCapture, callbackfunc, eventTypeId, eventTypeString, targetThread) => {
      var eventSize = 256;
      JSEvents.focusEvent ||= _malloc(eventSize);
  
      var focusEventHandlerFunc = (e) => {
        var nodeName = JSEvents.getNodeNameForTarget(e.target);
        var id = e.target.id ?? '';
  
        var focusEvent = JSEvents.focusEvent;
        stringToUTF8(nodeName, focusEvent + 0, 128);
        stringToUTF8(id, focusEvent + 128, 128);
  
        if (((a1, a2, a3) => dynCall_iiii(callbackfunc, a1, a2, a3))(eventTypeId, focusEvent, userData)) e.preventDefault();
      };
  
      var eventHandler = {
        target: findEventTarget(target),
        eventTypeString,
        eventTypeId,
        userData,
        callbackfunc,
        handlerFunc: focusEventHandlerFunc,
        useCapture
      };
      return JSEvents.registerOrRemoveHandler(eventHandler);
    };
  var _emscripten_set_blur_callback_on_thread = (target, userData, useCapture, callbackfunc, targetThread) =>
      registerFocusEventCallback(target, userData, useCapture, callbackfunc, 12, 'blur', targetThread);


  var _emscripten_set_element_css_size = (target, width, height) => {
      target = findEventTarget(target);
      if (!target) return -4;
  
      target.style.width = width + 'px';
      target.style.height = height + 'px';
  
      return 0;
    };

  var _emscripten_set_focus_callback_on_thread = (target, userData, useCapture, callbackfunc, targetThread) =>
      registerFocusEventCallback(target, userData, useCapture, callbackfunc, 13, 'focus', targetThread);

  
  
  
  
  
  
  var fillFullscreenChangeEventData = (eventStruct) => {
      var fullscreenElement = getFullscreenElement();
      var isFullscreen = !!fullscreenElement;
      // Assigning a boolean to HEAP32 with expected type coercion.
      /** @suppress{checkTypes} */
      HEAP8[eventStruct] = isFullscreen;
      HEAP8[(eventStruct)+(1)] = JSEvents.fullscreenEnabled();
      // If transitioning to fullscreen, report info about the element that is now fullscreen.
      // If transitioning to windowed mode, report info about the element that just was fullscreen.
      var reportedElement = isFullscreen ? fullscreenElement : JSEvents.previousFullscreenElement;
      var nodeName = JSEvents.getNodeNameForTarget(reportedElement);
      var id = reportedElement?.id ?? '';
      stringToUTF8(nodeName, eventStruct + 2, 128);
      stringToUTF8(id, eventStruct + 130, 128);
      HEAP32[(((eventStruct)+(260))>>2)] = reportedElement?.clientWidth ?? 0;
      HEAP32[(((eventStruct)+(264))>>2)] = reportedElement?.clientHeight ?? 0;
      HEAP32[(((eventStruct)+(268))>>2)] = screen.width;
      HEAP32[(((eventStruct)+(272))>>2)] = screen.height;
      if (isFullscreen) {
        JSEvents.previousFullscreenElement = fullscreenElement;
      }
    };
  
  var registerFullscreenChangeEventCallback = (target, userData, useCapture, callbackfunc, eventTypeId, eventTypeString, targetThread) => {
      var eventSize = 276;
      JSEvents.fullscreenChangeEvent ||= _malloc(eventSize);
  
      var fullscreenChangeEventHandlerFunc = (e) => {
        var fullscreenChangeEvent = JSEvents.fullscreenChangeEvent;
        fillFullscreenChangeEventData(fullscreenChangeEvent);
  
        if (((a1, a2, a3) => dynCall_iiii(callbackfunc, a1, a2, a3))(eventTypeId, fullscreenChangeEvent, userData)) e.preventDefault();
      };
  
      var eventHandler = {
        target,
        eventTypeString,
        eventTypeId,
        userData,
        callbackfunc,
        handlerFunc: fullscreenChangeEventHandlerFunc,
        useCapture
      };
      return JSEvents.registerOrRemoveHandler(eventHandler);
    };
  
  var _emscripten_set_fullscreenchange_callback_on_thread = (target, userData, useCapture, callbackfunc, targetThread) => {
      if (!JSEvents.fullscreenEnabled()) return -1;
      target = findEventTarget(target);
      if (!target) return -4;
  
      // TODO: When this block is removed, also change test/test_html5_remove_event_listener.c test expectation on emscripten_set_fullscreenchange_callback().
      registerFullscreenChangeEventCallback(target, userData, useCapture, callbackfunc, 19, 'webkitfullscreenchange', targetThread);
  
      return registerFullscreenChangeEventCallback(target, userData, useCapture, callbackfunc, 19, 'fullscreenchange', targetThread);
    };

  
  
  
  var registerGamepadEventCallback = (target, userData, useCapture, callbackfunc, eventTypeId, eventTypeString, targetThread) => {
      var eventSize = 1240;
      JSEvents.gamepadEvent ||= _malloc(eventSize);
  
      var gamepadEventHandlerFunc = (e) => {
        var gamepadEvent = JSEvents.gamepadEvent;
        fillGamepadEventData(gamepadEvent, e['gamepad']);
  
        if (((a1, a2, a3) => dynCall_iiii(callbackfunc, a1, a2, a3))(eventTypeId, gamepadEvent, userData)) e.preventDefault();
      };
  
      var eventHandler = {
        target: findEventTarget(target),
        allowsDeferredCalls: true,
        eventTypeString,
        eventTypeId,
        userData,
        callbackfunc,
        handlerFunc: gamepadEventHandlerFunc,
        useCapture
      };
      return JSEvents.registerOrRemoveHandler(eventHandler);
    };
  
  var _emscripten_set_gamepadconnected_callback_on_thread = (userData, useCapture, callbackfunc, targetThread) => {
      if (_emscripten_sample_gamepad_data()) return -1;
      return registerGamepadEventCallback(2, userData, useCapture, callbackfunc, 26, 'gamepadconnected', targetThread);
    };

  
  var _emscripten_set_gamepaddisconnected_callback_on_thread = (userData, useCapture, callbackfunc, targetThread) => {
      if (_emscripten_sample_gamepad_data()) return -1;
      return registerGamepadEventCallback(2, userData, useCapture, callbackfunc, 27, 'gamepaddisconnected', targetThread);
    };

  
  
  
  
  
  
  var registerKeyEventCallback = (target, userData, useCapture, callbackfunc, eventTypeId, eventTypeString, targetThread) => {
      var eventSize = 160;
      JSEvents.keyEvent ||= _malloc(eventSize);
  
      var keyEventHandlerFunc = (e) => {
        assert(e);
  
        var keyEventData = JSEvents.keyEvent;
        HEAPF64[((keyEventData)>>3)] = e.timeStamp;
  
        var idx = ((keyEventData)>>2);
  
        HEAP32[idx + 2] = e.location;
        HEAP8[keyEventData + 12] = e.ctrlKey;
        HEAP8[keyEventData + 13] = e.shiftKey;
        HEAP8[keyEventData + 14] = e.altKey;
        HEAP8[keyEventData + 15] = e.metaKey;
        HEAP8[keyEventData + 16] = e.repeat;
        HEAP32[idx + 5] = e.charCode;
        HEAP32[idx + 6] = e.keyCode;
        HEAP32[idx + 7] = e.which;
        stringToUTF8(e.key ?? '', keyEventData + 32, 32);
        stringToUTF8(e.code ?? '', keyEventData + 64, 32);
        stringToUTF8(e.char ?? '', keyEventData + 96, 32);
        stringToUTF8(e.locale ?? '', keyEventData + 128, 32);
  
        if (((a1, a2, a3) => dynCall_iiii(callbackfunc, a1, a2, a3))(eventTypeId, keyEventData, userData)) e.preventDefault();
      };
  
      var eventHandler = {
        target: findEventTarget(target),
        eventTypeString,
        eventTypeId,
        userData,
        callbackfunc,
        handlerFunc: keyEventHandlerFunc,
        useCapture
      };
      return JSEvents.registerOrRemoveHandler(eventHandler);
    };
  var _emscripten_set_keydown_callback_on_thread = (target, userData, useCapture, callbackfunc, targetThread) =>
      registerKeyEventCallback(target, userData, useCapture, callbackfunc, 2, 'keydown', targetThread);

  var _emscripten_set_keypress_callback_on_thread = (target, userData, useCapture, callbackfunc, targetThread) =>
      registerKeyEventCallback(target, userData, useCapture, callbackfunc, 1, 'keypress', targetThread);

  var _emscripten_set_keyup_callback_on_thread = (target, userData, useCapture, callbackfunc, targetThread) =>
      registerKeyEventCallback(target, userData, useCapture, callbackfunc, 3, 'keyup', targetThread);

  var _emscripten_set_main_loop = (func, fps, simulateInfiniteLoop) => {
      var iterFunc = (() => dynCall_v(func));
      setMainLoop(iterFunc, fps, simulateInfiniteLoop);
    };

  
  
  
  
  
  var fillMouseEventData = (eventStruct, e, target) => {
      assert(eventStruct % 4 == 0);
      HEAPF64[((eventStruct)>>3)] = e.timeStamp;
      var idx = ((eventStruct)>>2);
      HEAP32[idx + 2] = e.screenX;
      HEAP32[idx + 3] = e.screenY;
      HEAP32[idx + 4] = e.clientX;
      HEAP32[idx + 5] = e.clientY;
      HEAP8[eventStruct + 24] = e.ctrlKey;
      HEAP8[eventStruct + 25] = e.shiftKey;
      HEAP8[eventStruct + 26] = e.altKey;
      HEAP8[eventStruct + 27] = e.metaKey;
      HEAP16[idx*2 + 14] = e.button;
      HEAP16[idx*2 + 15] = e.buttons;
      HEAP32[idx + 8] = e.movementX;
      HEAP32[idx + 9] = e.movementY;
  
      // Note: rect contains doubles (truncated to placate SAFE_HEAP, which is the same behaviour when writing to HEAP32 anyway)
      var rect = getBoundingClientRect(target);
      HEAP32[idx + 10] = e.clientX - (rect.left | 0);
      HEAP32[idx + 11] = e.clientY - (rect.top  | 0);
    };
  
  
  var registerMouseEventCallback = (target, userData, useCapture, callbackfunc, eventTypeId, eventTypeString, targetThread) => {
      var eventSize = 64;
      JSEvents.mouseEvent ||= _malloc(eventSize);
      target = findEventTarget(target);
  
      var mouseEventHandlerFunc = (e) => {
        // TODO: Make this access thread safe, or this could update live while app is reading it.
        fillMouseEventData(JSEvents.mouseEvent, e, target);
  
        if (((a1, a2, a3) => dynCall_iiii(callbackfunc, a1, a2, a3))(eventTypeId, JSEvents.mouseEvent, userData)) e.preventDefault();
      };
  
      var eventHandler = {
        target,
        allowsDeferredCalls: eventTypeString != 'mousemove' && eventTypeString != 'mouseenter' && eventTypeString != 'mouseleave', // Mouse move events do not allow fullscreen/pointer lock requests to be handled in them!
        eventTypeString,
        eventTypeId,
        userData,
        callbackfunc,
        handlerFunc: mouseEventHandlerFunc,
        useCapture
      };
      return JSEvents.registerOrRemoveHandler(eventHandler);
    };
  var _emscripten_set_mousedown_callback_on_thread = (target, userData, useCapture, callbackfunc, targetThread) =>
      registerMouseEventCallback(target, userData, useCapture, callbackfunc, 5, 'mousedown', targetThread);

  var _emscripten_set_mouseenter_callback_on_thread = (target, userData, useCapture, callbackfunc, targetThread) =>
      registerMouseEventCallback(target, userData, useCapture, callbackfunc, 33, 'mouseenter', targetThread);

  var _emscripten_set_mouseleave_callback_on_thread = (target, userData, useCapture, callbackfunc, targetThread) =>
      registerMouseEventCallback(target, userData, useCapture, callbackfunc, 34, 'mouseleave', targetThread);

  var _emscripten_set_mousemove_callback_on_thread = (target, userData, useCapture, callbackfunc, targetThread) =>
      registerMouseEventCallback(target, userData, useCapture, callbackfunc, 8, 'mousemove', targetThread);

  var _emscripten_set_mouseup_callback_on_thread = (target, userData, useCapture, callbackfunc, targetThread) =>
      registerMouseEventCallback(target, userData, useCapture, callbackfunc, 6, 'mouseup', targetThread);

  
  
  
  
  var fillPointerlockChangeEventData = (eventStruct) => {
      var pointerLockElement = document.pointerLockElement;
      var isPointerlocked = !!pointerLockElement;
      // Assigning a boolean to HEAP32 with expected type coercion.
      /** @suppress{checkTypes} */
      HEAP8[eventStruct] = isPointerlocked;
      var nodeName = JSEvents.getNodeNameForTarget(pointerLockElement);
      var id = pointerLockElement?.id ?? '';
      stringToUTF8(nodeName, eventStruct + 1, 128);
      stringToUTF8(id, eventStruct + 129, 128);
    };
  
  var registerPointerlockChangeEventCallback = (target, userData, useCapture, callbackfunc, eventTypeId, eventTypeString, targetThread) => {
      var eventSize = 257;
      JSEvents.pointerlockChangeEvent ||= _malloc(eventSize);
  
      var pointerlockChangeEventHandlerFunc = (e) => {
        var pointerlockChangeEvent = JSEvents.pointerlockChangeEvent;
        fillPointerlockChangeEventData(pointerlockChangeEvent);
  
        if (((a1, a2, a3) => dynCall_iiii(callbackfunc, a1, a2, a3))(eventTypeId, pointerlockChangeEvent, userData)) e.preventDefault();
      };
  
      var eventHandler = {
        target,
        eventTypeString,
        eventTypeId,
        userData,
        callbackfunc,
        handlerFunc: pointerlockChangeEventHandlerFunc,
        useCapture
      };
      return JSEvents.registerOrRemoveHandler(eventHandler);
    };
  
  var _emscripten_set_pointerlockchange_callback_on_thread = (target, userData, useCapture, callbackfunc, targetThread) => {
      if (!document.body?.requestPointerLock) {
        return -1;
      }
  
      target = findEventTarget(target);
      if (!target) return -4;
      return registerPointerlockChangeEventCallback(target, userData, useCapture, callbackfunc, 20, 'pointerlockchange', targetThread);
    };

  
  
  
  var registerUiEventCallback = (target, userData, useCapture, callbackfunc, eventTypeId, eventTypeString, targetThread) => {
      var eventSize = 36;
      JSEvents.uiEvent ||= _malloc(eventSize);
  
      target = findEventTarget(target);
  
      var uiEventHandlerFunc = (e) => {
        if (e.target != target) {
          // Never take ui events such as scroll via a 'bubbled' route, but always from the direct element that
          // was targeted. Otherwise e.g. if app logs a message in response to a page scroll, the Emscripten log
          // message box could cause to scroll, generating a new (bubbled) scroll message, causing a new log print,
          // causing a new scroll, etc..
          return;
        }
        var b = document.body; // Take document.body to a variable, Closure compiler does not outline access to it on its own.
        if (!b) {
          // During a page unload 'body' can be null, with "Cannot read property 'clientWidth' of null" being thrown
          return;
        }
        var uiEvent = JSEvents.uiEvent;
        HEAP32[((uiEvent)>>2)] = 0; // always zero for resize and scroll
        HEAP32[(((uiEvent)+(4))>>2)] = b.clientWidth;
        HEAP32[(((uiEvent)+(8))>>2)] = b.clientHeight;
        HEAP32[(((uiEvent)+(12))>>2)] = innerWidth;
        HEAP32[(((uiEvent)+(16))>>2)] = innerHeight;
        HEAP32[(((uiEvent)+(20))>>2)] = outerWidth;
        HEAP32[(((uiEvent)+(24))>>2)] = outerHeight;
        HEAP32[(((uiEvent)+(28))>>2)] = pageXOffset | 0; // scroll offsets are float
        HEAP32[(((uiEvent)+(32))>>2)] = pageYOffset | 0;
        if (((a1, a2, a3) => dynCall_iiii(callbackfunc, a1, a2, a3))(eventTypeId, uiEvent, userData)) e.preventDefault();
      };
  
      var eventHandler = {
        target,
        eventTypeString,
        eventTypeId,
        userData,
        callbackfunc,
        handlerFunc: uiEventHandlerFunc,
        useCapture
      };
      return JSEvents.registerOrRemoveHandler(eventHandler);
    };
  var _emscripten_set_resize_callback_on_thread = (target, userData, useCapture, callbackfunc, targetThread) =>
      registerUiEventCallback(target, userData, useCapture, callbackfunc, 10, 'resize', targetThread);

  
  
  
  
  
  
  var registerTouchEventCallback = (target, userData, useCapture, callbackfunc, eventTypeId, eventTypeString, targetThread) => {
      var eventSize = 1552;
      JSEvents.touchEvent ||= _malloc(eventSize);
  
      target = findEventTarget(target);
  
      var touchEventHandlerFunc = (e) => {
        assert(e);
        var t, touches = {}, et = e.touches;
        // To ease marshalling different kinds of touches that browser reports (all touches are listed in e.touches,
        // only changed touches in e.changedTouches, and touches on target at a.targetTouches), mark a boolean in
        // each Touch object so that we can later loop only once over all touches we see to marshall over to Wasm.
  
        for (let t of et) {
          // Browser might recycle the generated Touch objects between each frame (Firefox on Android), so reset any
          // changed/target states we may have set from previous frame.
          t.isChanged = t.onTarget = 0;
          touches[t.identifier] = t;
        }
        // Mark which touches are part of the changedTouches list.
        for (let t of e.changedTouches) {
          t.isChanged = 1;
          touches[t.identifier] = t;
        }
        // Mark which touches are part of the targetTouches list.
        for (let t of e.targetTouches) {
          touches[t.identifier].onTarget = 1;
        }
  
        var touchEvent = JSEvents.touchEvent;
        HEAPF64[((touchEvent)>>3)] = e.timeStamp;
        HEAP8[touchEvent + 12] = e.ctrlKey;
        HEAP8[touchEvent + 13] = e.shiftKey;
        HEAP8[touchEvent + 14] = e.altKey;
        HEAP8[touchEvent + 15] = e.metaKey;
        var idx = touchEvent + 16;
        var targetRect = getBoundingClientRect(target);
        var numTouches = 0;
        for (let t of Object.values(touches)) {
          var idx32 = ((idx)>>2); // Pre-shift the ptr to index to HEAP32 to save code size
          HEAP32[idx32 + 0] = t.identifier;
          HEAP32[idx32 + 1] = t.screenX;
          HEAP32[idx32 + 2] = t.screenY;
          HEAP32[idx32 + 3] = t.clientX;
          HEAP32[idx32 + 4] = t.clientY;
          HEAP32[idx32 + 5] = t.pageX;
          HEAP32[idx32 + 6] = t.pageY;
          HEAP8[idx + 28] = t.isChanged;
          HEAP8[idx + 29] = t.onTarget;
          HEAP32[idx32 + 8] = t.clientX - (targetRect.left | 0);
          HEAP32[idx32 + 9] = t.clientY - (targetRect.top  | 0);
  
          idx += 48;
  
          if (++numTouches > 31) {
            break;
          }
        }
        HEAP32[(((touchEvent)+(8))>>2)] = numTouches;
  
        if (((a1, a2, a3) => dynCall_iiii(callbackfunc, a1, a2, a3))(eventTypeId, touchEvent, userData)) e.preventDefault();
      };
  
      var eventHandler = {
        target,
        allowsDeferredCalls: eventTypeString == 'touchstart' || eventTypeString == 'touchend',
        eventTypeString,
        eventTypeId,
        userData,
        callbackfunc,
        handlerFunc: touchEventHandlerFunc,
        useCapture
      };
      return JSEvents.registerOrRemoveHandler(eventHandler);
    };
  var _emscripten_set_touchcancel_callback_on_thread = (target, userData, useCapture, callbackfunc, targetThread) =>
      registerTouchEventCallback(target, userData, useCapture, callbackfunc, 25, 'touchcancel', targetThread);

  var _emscripten_set_touchend_callback_on_thread = (target, userData, useCapture, callbackfunc, targetThread) =>
      registerTouchEventCallback(target, userData, useCapture, callbackfunc, 23, 'touchend', targetThread);

  var _emscripten_set_touchmove_callback_on_thread = (target, userData, useCapture, callbackfunc, targetThread) =>
      registerTouchEventCallback(target, userData, useCapture, callbackfunc, 24, 'touchmove', targetThread);

  var _emscripten_set_touchstart_callback_on_thread = (target, userData, useCapture, callbackfunc, targetThread) =>
      registerTouchEventCallback(target, userData, useCapture, callbackfunc, 22, 'touchstart', targetThread);

  
  
  var fillVisibilityChangeEventData = (eventStruct) => {
      var visibilityStates = [ 'hidden', 'visible', 'prerender', 'unloaded' ];
      var visibilityState = visibilityStates.indexOf(document.visibilityState);
  
      // Assigning a boolean to HEAP32 with expected type coercion.
      /** @suppress{checkTypes} */
      HEAP8[eventStruct] = document.hidden;
      HEAP32[(((eventStruct)+(4))>>2)] = visibilityState;
    };
  
  var registerVisibilityChangeEventCallback = (target, userData, useCapture, callbackfunc, eventTypeId, eventTypeString, targetThread) => {
      var eventSize = 8;
      JSEvents.visibilityChangeEvent ||= _malloc(eventSize);
  
      var visibilityChangeEventHandlerFunc = (e) => {
        var visibilityChangeEvent = JSEvents.visibilityChangeEvent;
        fillVisibilityChangeEventData(visibilityChangeEvent);
  
        if (((a1, a2, a3) => dynCall_iiii(callbackfunc, a1, a2, a3))(eventTypeId, visibilityChangeEvent, userData)) e.preventDefault();
      };
  
      var eventHandler = {
        target,
        eventTypeString,
        eventTypeId,
        userData,
        callbackfunc,
        handlerFunc: visibilityChangeEventHandlerFunc,
        useCapture
      };
      return JSEvents.registerOrRemoveHandler(eventHandler);
    };
  
  var _emscripten_set_visibilitychange_callback_on_thread = (userData, useCapture, callbackfunc, targetThread) => {
      return registerVisibilityChangeEventCallback(specialHTMLTargets[1], userData, useCapture, callbackfunc, 21, 'visibilitychange', targetThread);
    };

  
  
  
  
  var registerWheelEventCallback = (target, userData, useCapture, callbackfunc, eventTypeId, eventTypeString, targetThread) => {
      var eventSize = 96;
      JSEvents.wheelEvent ||= _malloc(eventSize)
  
      // The DOM Level 3 events spec event 'wheel'
      var wheelHandlerFunc = (e) => {
        var wheelEvent = JSEvents.wheelEvent;
        fillMouseEventData(wheelEvent, e, target);
        HEAPF64[(((wheelEvent)+(64))>>3)] = e["deltaX"];
        HEAPF64[(((wheelEvent)+(72))>>3)] = e["deltaY"];
        HEAPF64[(((wheelEvent)+(80))>>3)] = e["deltaZ"];
        HEAP32[(((wheelEvent)+(88))>>2)] = e["deltaMode"];
        if (((a1, a2, a3) => dynCall_iiii(callbackfunc, a1, a2, a3))(eventTypeId, wheelEvent, userData)) e.preventDefault();
      };
  
      var eventHandler = {
        target,
        allowsDeferredCalls: true,
        eventTypeString,
        eventTypeId,
        userData,
        callbackfunc,
        handlerFunc: wheelHandlerFunc,
        useCapture
      };
      return JSEvents.registerOrRemoveHandler(eventHandler);
    };
  
  var _emscripten_set_wheel_callback_on_thread = (target, userData, useCapture, callbackfunc, targetThread) => {
      target = findEventTarget(target);
      if (!target) return -4;
      if (typeof target.onwheel != 'undefined') {
        return registerWheelEventCallback(target, userData, useCapture, callbackfunc, 9, 'wheel', targetThread);
      } else {
        return -1;
      }
    };

  
  var _emscripten_set_window_title = (title) => document.title = UTF8ToString(title);

  var _emscripten_sleep = function(ms) {
    let innerFunc =  () => new Promise((resolve) => setTimeout(resolve, ms));
    return Asyncify.handleAsync(innerFunc);
  }
  ;
  _emscripten_sleep.isAsync = true;

  
  var webglPowerPreferences = ["default","low-power","high-performance"];
  
  
  
  
  var _emscripten_webgl_do_create_context = (target, attributes) => {
      assert(attributes);
      var attr32 = ((attributes)>>2);
      var powerPreference = HEAP32[attr32 + (8>>2)];
      var contextAttributes = {
        'alpha': !!HEAP8[attributes + 0],
        'depth': !!HEAP8[attributes + 1],
        'stencil': !!HEAP8[attributes + 2],
        'antialias': !!HEAP8[attributes + 3],
        'premultipliedAlpha': !!HEAP8[attributes + 4],
        'preserveDrawingBuffer': !!HEAP8[attributes + 5],
        'powerPreference': webglPowerPreferences[powerPreference],
        'failIfMajorPerformanceCaveat': !!HEAP8[attributes + 12],
        'desynchronized': !!HEAP8[attributes + 33],
        // The following are not predefined WebGL context attributes in the WebGL specification, so the property names can be minified by Closure.
        majorVersion: HEAP32[attr32 + (16>>2)],
        minorVersion: HEAP32[attr32 + (20>>2)],
        enableExtensionsByDefault: HEAP8[attributes + 24],
        explicitSwapControl: HEAP8[attributes + 25],
        proxyContextToMainThread: HEAP32[attr32 + (28>>2)],
        renderViaOffscreenBackBuffer: HEAP8[attributes + 32]
      };
  
      //  TODO: Make these into hard errors at some point in the future
      if (contextAttributes.majorVersion !== 1 && contextAttributes.majorVersion !== 2) {
        err(`Invalid WebGL version requested: ${contextAttributes.majorVersion}`);
      }
  
      var canvas = findCanvasEventTarget(target);
  
      if (!canvas) {
        return 0;
      }
  
      if (contextAttributes.explicitSwapControl && !contextAttributes.renderViaOffscreenBackBuffer) {
        contextAttributes.renderViaOffscreenBackBuffer = true;
      }
  
      var contextHandle = GL.createContext(canvas, contextAttributes);
      return contextHandle;
    };
  var _emscripten_webgl_create_context = _emscripten_webgl_do_create_context;

  
  
  
  
  
  
  
  
  
  
  var _emscripten_webgl_enable_extension = (contextHandle, extension) => {
      var context = GL.getContext(contextHandle);
      var extString = UTF8ToString(extension);
      if (extString.startsWith('GL_')) extString = extString.slice(3); // Allow enabling extensions both with "GL_" prefix and without.
  
      // Switch-board that pulls in code for all GL extensions, even if those are not used :/
      // Build with -sGL_SUPPORT_SIMPLE_ENABLE_EXTENSIONS=0 to avoid this.
  
      // Obtain function entry points to WebGL 1 extension related functions.
      if (extString == 'ANGLE_instanced_arrays') webgl_enable_ANGLE_instanced_arrays(GLctx);
      if (extString == 'OES_vertex_array_object') webgl_enable_OES_vertex_array_object(GLctx);
      if (extString == 'WEBGL_draw_buffers') webgl_enable_WEBGL_draw_buffers(GLctx);
  
      if (extString == 'WEBGL_draw_instanced_base_vertex_base_instance') webgl_enable_WEBGL_draw_instanced_base_vertex_base_instance(GLctx);
      if (extString == 'WEBGL_multi_draw_instanced_base_vertex_base_instance') webgl_enable_WEBGL_multi_draw_instanced_base_vertex_base_instance(GLctx);
  
      if (extString == 'WEBGL_multi_draw') webgl_enable_WEBGL_multi_draw(GLctx);
      if (extString == 'EXT_polygon_offset_clamp') webgl_enable_EXT_polygon_offset_clamp(GLctx);
      if (extString == 'EXT_clip_control') webgl_enable_EXT_clip_control(GLctx);
      if (extString == 'WEBGL_polygon_mode') webgl_enable_WEBGL_polygon_mode(GLctx);
  
      var ext = context.GLctx.getExtension(extString);
      return !!ext;
    };

  
  var _emscripten_webgl_do_get_current_context = () => GL.currentContext ? GL.currentContext.handle : 0;
  var _emscripten_webgl_get_current_context = _emscripten_webgl_do_get_current_context;

  var _emscripten_webgl_make_context_current = (contextHandle) => {
      var success = GL.makeContextCurrent(contextHandle);
      return success ? 0 : -5;
    };

  var ENV = {
  };
  
  var getExecutableName = () => thisProgram;
  var getEnvStrings = () => {
      if (!getEnvStrings.strings) {
        // Default values.
        var lang = (globalThis.navigator?.language ?? 'C').replace('-', '_') + '.UTF-8';
        var env = {
          'USER': 'web_user',
          'LOGNAME': 'web_user',
          'PATH': '/',
          'PWD': '/',
          'HOME': '/home/web_user',
          'LANG': lang,
          '_': getExecutableName()
        };
        // Apply the user-provided values, if any.
        for (var x in ENV) {
          // x is a key in ENV; if ENV[x] is undefined, that means it was
          // explicitly set to be so. We allow user code to do that to
          // force variables with default values to remain unset.
          if (ENV[x] === undefined) delete env[x];
          else env[x] = ENV[x];
        }
        var strings = [];
        for (var x in env) {
          strings.push(`${x}=${env[x]}`);
        }
        getEnvStrings.strings = strings;
      }
      return getEnvStrings.strings;
    };
  
  
  var _environ_get = (__environ, environ_buf) => {
      var bufSize = 0;
      var envp = 0;
      for (var string of getEnvStrings()) {
        var ptr = environ_buf + bufSize;
        HEAPU32[(((__environ)+(envp))>>2)] = ptr;
        bufSize += stringToUTF8(string, ptr, Infinity) + 1;
        envp += 4;
      }
      return 0;
    };

  
  
  var _environ_sizes_get = (penviron_count, penviron_buf_size) => {
      var strings = getEnvStrings();
      HEAPU32[((penviron_count)>>2)] = strings.length;
      var bufSize = 0;
      for (var string of strings) {
        bufSize += lengthBytesUTF8(string) + 1;
      }
      HEAPU32[((penviron_buf_size)>>2)] = bufSize;
      return 0;
    };


  function _fd_close(fd) {
  try {
  
      var stream = SYSCALLS.getStreamFromFD(fd);
      FS.close(stream);
      return 0;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return e.errno;
  }
  }
  

  
  /** @param {number=} offset */
  var doReadv = (stream, iov, iovcnt, offset) => {
      var ret = 0;
      for (var i = 0; i < iovcnt; i++) {
        var ptr = HEAPU32[((iov)>>2)];
        var len = HEAPU32[(((iov)+(4))>>2)];
        iov += 8;
        try {
          var curr = FS.read(stream, HEAP8, ptr, len, offset);
        } catch (e) {
          // On a non-blocking stream a subsequent read may would-block after we
          // already gathered data. POSIX readv is a single gather-read: return
          // what we have rather than failing the whole call.
          if (ret > 0 && e instanceof FS.ErrnoError &&
              (e.errno == 6 || e.errno == 6)) {
            break;
          }
          throw e;
        }
        if (curr < 0) return -1;
        ret += curr;
        if (curr < len) break; // nothing more to read
        if (typeof offset != 'undefined') {
          offset += curr;
        }
      }
      return ret;
    };
  
  
  function _fd_read(fd, iov, iovcnt, pnum) {
  try {
  
      var stream = SYSCALLS.getStreamFromFD(fd);
      var num = doReadv(stream, iov, iovcnt);
      HEAPU32[((pnum)>>2)] = num;
      return 0;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return e.errno;
  }
  }
  

  
  
  function _fd_seek(fd, offset, whence, newOffset) {
    offset = bigintToI53Checked(offset);
  
  
  try {
  
      if (isNaN(offset)) return 22;
      var stream = SYSCALLS.getStreamFromFD(fd);
      FS.llseek(stream, offset, whence);
      HEAP64[((newOffset)>>3)] = BigInt(stream.position);
      if (stream.getdents && !offset && whence === 0) stream.getdents = null; // reset readdir state
      return 0;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return e.errno;
  }
  ;
  }

  
  
  /** @param {number=} offset */
  var doWritev = (stream, iov, iovcnt, offset) => {
      // Gather all iovecs into one contiguous buffer and issue a single
      // FS.write, matching POSIX writev's single gather-write semantics (as
      // __syscall_sendmsg already does). Per-iovec writes fragment a stream
      // socket send into multiple segments, breaking stream byte semantics.
      if (iovcnt == 1) {
        // Single iovec: write directly from HEAP8, no gather buffer needed.
        return FS.write(stream, HEAP8, HEAPU32[((iov)>>2)], HEAPU32[(((iov)+(4))>>2)], offset);
      }
      var total = 0;
      for (var i = 0, p = iov; i < iovcnt; i++, p += 8) {
        total += HEAPU32[(((p)+(4))>>2)];
      }
      var view = new Uint8Array(total);
      var voff = 0;
      for (var i = 0; i < iovcnt; i++, iov += 8) {
        var ptr = HEAPU32[((iov)>>2)];
        var len = HEAPU32[(((iov)+(4))>>2)];
        view.set(HEAPU8.subarray(ptr, ptr + len), voff);
        voff += len;
      }
      return FS.write(stream, view, 0, total, offset);
    };
  
  
  function _fd_write(fd, iov, iovcnt, pnum) {
  try {
  
      var stream = SYSCALLS.getStreamFromFD(fd);
      var num = doWritev(stream, iov, iovcnt);
      HEAPU32[((pnum)>>2)] = num;
      return 0;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return e.errno;
  }
  }
  

  var _glActiveTexture = _emscripten_glActiveTexture;

  var _glAttachShader = _emscripten_glAttachShader;

  var _glBindBuffer = _emscripten_glBindBuffer;

  var _glBindFramebuffer = _emscripten_glBindFramebuffer;

  var _glBindTexture = _emscripten_glBindTexture;


  var _glBlendFunc = _emscripten_glBlendFunc;

  var _glBufferData = _emscripten_glBufferData;

  var _glBufferSubData = _emscripten_glBufferSubData;

  var _glCheckFramebufferStatus = _emscripten_glCheckFramebufferStatus;

  var _glClear = _emscripten_glClear;

  var _glClearColor = _emscripten_glClearColor;

  var _glCompileShader = _emscripten_glCompileShader;

  var _glCreateProgram = _emscripten_glCreateProgram;

  var _glCreateShader = _emscripten_glCreateShader;

  var _glDeleteBuffers = _emscripten_glDeleteBuffers;

  var _glDeleteFramebuffers = _emscripten_glDeleteFramebuffers;

  var _glDeleteProgram = _emscripten_glDeleteProgram;

  var _glDeleteShader = _emscripten_glDeleteShader;

  var _glDeleteTextures = _emscripten_glDeleteTextures;


  var _glDepthFunc = _emscripten_glDepthFunc;

  var _glDepthMask = _emscripten_glDepthMask;

  var _glDetachShader = _emscripten_glDetachShader;

  var _glDisable = _emscripten_glDisable;

  var _glDrawArrays = _emscripten_glDrawArrays;


  var _glEnable = _emscripten_glEnable;

  var _glEnableVertexAttribArray = _emscripten_glEnableVertexAttribArray;

  var _glFramebufferTexture2D = _emscripten_glFramebufferTexture2D;

  var _glFrontFace = _emscripten_glFrontFace;

  var _glGenBuffers = _emscripten_glGenBuffers;

  var _glGenFramebuffers = _emscripten_glGenFramebuffers;

  var _glGenTextures = _emscripten_glGenTextures;


  var _glGetError = _emscripten_glGetError;

  var _glGetProgramInfoLog = _emscripten_glGetProgramInfoLog;

  var _glGetProgramiv = _emscripten_glGetProgramiv;

  var _glGetShaderInfoLog = _emscripten_glGetShaderInfoLog;

  var _glGetShaderiv = _emscripten_glGetShaderiv;

  var _glGetUniformLocation = _emscripten_glGetUniformLocation;

  var _glIsBuffer = _emscripten_glIsBuffer;

  var _glIsProgram = _emscripten_glIsProgram;

  var _glIsShader = _emscripten_glIsShader;

  var _glIsTexture = _emscripten_glIsTexture;


  var _glLinkProgram = _emscripten_glLinkProgram;

  var _glReadPixels = _emscripten_glReadPixels;

  var _glShaderSource = _emscripten_glShaderSource;

  var _glTexImage2D = _emscripten_glTexImage2D;

  var _glTexParameteri = _emscripten_glTexParameteri;

  var _glTexSubImage2D = _emscripten_glTexSubImage2D;

  var _glUniform1f = _emscripten_glUniform1f;

  var _glUniform1i = _emscripten_glUniform1i;

  var _glUniform2fv = _emscripten_glUniform2fv;

  var _glUniform3fv = _emscripten_glUniform3fv;

  var _glUniform4fv = _emscripten_glUniform4fv;

  var _glUniformMatrix4fv = _emscripten_glUniformMatrix4fv;

  var _glUseProgram = _emscripten_glUseProgram;

  var _glVertexAttribPointer = _emscripten_glVertexAttribPointer;

  var _glViewport = _emscripten_glViewport;

  var _llvm_eh_typeid_for = (type) => type;





  var autoResumeAudioContext = (ctx) => {
      for (var event of ['keydown', 'mousedown', 'touchstart']) {
        for (var element of [document, document.getElementById('canvas')]) {
          element?.addEventListener(event, () => {
            if (ctx.state === 'suspended') ctx.resume();
          }, { 'once': true });
        }
      }
    };




  var wasmTableMirror = [];
  
  
  var getWasmTableEntry = (funcPtr) => {
      var func = wasmTableMirror[funcPtr];
      if (!func) {
        /** @suppress {checkTypes} */
        wasmTableMirror[funcPtr] = func = wasmTable.get(funcPtr);
      }
      /** @suppress {checkTypes} */
      assert(wasmTable.get(funcPtr) == func, 'table mirror is out of date');
      return func;
    };






  
  
  
  
  
  
  
    /**
   * @param {number} ptr
   * @param {number} value
   * @param {string} type
   */
  function setValue(ptr, value, type = 'i8') {
    if (type.endsWith('*')) type = '*';
    switch (type) {
      case 'i1': HEAP8[ptr] = value; break;
      case 'i8': HEAP8[ptr] = value; break;
      case 'i16': HEAP16[((ptr)>>1)] = value; break;
      case 'i32': HEAP32[((ptr)>>2)] = value; break;
      case 'i64': HEAP64[((ptr)>>3)] = BigInt(value); break;
      case 'float': HEAPF32[((ptr)>>2)] = value; break;
      case 'double': HEAPF64[((ptr)>>3)] = value; break;
      case '*': HEAPU32[((ptr)>>2)] = value; break;
      default: abort(`invalid type for setValue: ${type}`);
    }
  }







  var requestFullscreen = Browser.requestFullscreen;

  var FS_createPath = (...args) => FS.createPath(...args);



  var FS_unlink = (...args) => FS.unlink(...args);

  var FS_createLazyFile = (...args) => FS.createLazyFile(...args);

  var FS_createDevice = (...args) => FS.createDevice(...args);



  var createContext = Browser.createContext;

  FS.createPreloadedFile = FS_createPreloadedFile;
  FS.preloadFile = FS_preloadFile;
  FS.staticInit();;
init_ClassHandle();
init_RegisteredPointer();
assert(emval_handles.length === 5 * 2);

      // Signal GL rendering layer that processing of a new frame is about to
      // start. This helps it optimize VBO double-buffering and reduce GPU stalls.
      registerPreMainLoop(() => GL.newRenderingFrameStarted());
    ;

      Module['requestAnimationFrame'] = MainLoop.requestAnimationFrame;
      Module['pauseMainLoop'] = MainLoop.pause;
      Module['resumeMainLoop'] = MainLoop.resume;
      MainLoop.init();;
for (let i = 0; i < 32; ++i) tempFixedLengthArray.push(new Array(i));;
var miniTempWebGLFloatBuffersStorage = new Float32Array(288);
  // Create GL_POOL_TEMP_BUFFERS_SIZE+1 temporary buffers, for uploads of size 0 through GL_POOL_TEMP_BUFFERS_SIZE inclusive
  for (/**@suppress{duplicate}*/var i = 0; i <= 288; ++i) {
    miniTempWebGLFloatBuffers[i] = miniTempWebGLFloatBuffersStorage.subarray(0, i);
  };
var miniTempWebGLIntBuffersStorage = new Int32Array(288);
  // Create GL_POOL_TEMP_BUFFERS_SIZE+1 temporary buffers, for uploads of size 0 through GL_POOL_TEMP_BUFFERS_SIZE inclusive
  for (/**@suppress{duplicate}*/var i = 0; i <= 288; ++i) {
    miniTempWebGLIntBuffers[i] = miniTempWebGLIntBuffersStorage.subarray(0, i);
  };
// End JS library code

// include: postlibrary.js
// This file is included after the automatically-generated JS library code
// but before the wasm module is created.

{

  // Begin ATMODULES hooks
  if (Module['noExitRuntime']) noExitRuntime = Module['noExitRuntime'];

if (Module['print']) out = Module['print'];
if (Module['printErr']) err = Module['printErr'];
if (Module['wasmBinary']) wasmBinary = Module['wasmBinary'];
  // End ATMODULES hooks

  checkIncomingModuleAPI();

  if (Module['arguments']) programArgs = Module['arguments'];
  if (Module['thisProgram']) thisProgram = Module['thisProgram'];

  // Assertions on removed incoming Module JS APIs.
  assert(typeof Module['memoryInitializerPrefixURL'] == 'undefined', 'Module.memoryInitializerPrefixURL option was removed, use Module.locateFile instead');
  assert(typeof Module['pthreadMainPrefixURL'] == 'undefined', 'Module.pthreadMainPrefixURL option was removed, use Module.locateFile instead');
  assert(typeof Module['cdInitializerPrefixURL'] == 'undefined', 'Module.cdInitializerPrefixURL option was removed, use Module.locateFile instead');
  assert(typeof Module['filePackagePrefixURL'] == 'undefined', 'Module.filePackagePrefixURL option was removed, use Module.locateFile instead');
  assert(typeof Module['read'] == 'undefined', 'Module.read option was removed');
  assert(typeof Module['readAsync'] == 'undefined', 'Module.readAsync option was removed (modify readAsync in JS)');
  assert(typeof Module['readBinary'] == 'undefined', 'Module.readBinary option was removed (modify readBinary in JS)');
  assert(typeof Module['setWindowTitle'] == 'undefined', 'Module.setWindowTitle option was removed (modify emscripten_set_window_title in JS)');
  assert(typeof Module['TOTAL_MEMORY'] == 'undefined', 'Module.TOTAL_MEMORY has been renamed Module.INITIAL_MEMORY');
  assert(typeof Module['ENVIRONMENT'] == 'undefined', 'Module.ENVIRONMENT has been deprecated. To force the environment, use the ENVIRONMENT compile-time option (for example, -sENVIRONMENT=web or -sENVIRONMENT=node)');
  assert(typeof Module['STACK_SIZE'] == 'undefined', 'STACK_SIZE can no longer be set at runtime.  Use -sSTACK_SIZE at link time')
  // If memory is defined in wasm, the user can't provide it, or set INITIAL_MEMORY
  assert(typeof Module['wasmMemory'] == 'undefined', 'Use of `wasmMemory` detected.  Use -sIMPORTED_MEMORY to define wasmMemory externally');
  assert(typeof Module['INITIAL_MEMORY'] == 'undefined', 'Detected runtime INITIAL_MEMORY setting.  Use -sIMPORTED_MEMORY to define wasmMemory dynamically');

  var preInit = Module['preInit'];
  if (preInit) {
    if (typeof preInit == 'function') Module['preInit'] = preInit = [preInit];
    // Written as a loop so that preInit functions that themselves add more
    // preInit functions.  Is this actually needed?
    while (preInit.length > 0) {
      preInit.shift()();
    }
  }
  consumedModuleProp('preInit');
}

// Begin runtime exports
  Module['addRunDependency'] = addRunDependency;
  Module['removeRunDependency'] = removeRunDependency;
  Module['requestFullscreen'] = requestFullscreen;
  Module['createContext'] = createContext;
  Module['FS_preloadFile'] = FS_preloadFile;
  Module['FS_unlink'] = FS_unlink;
  Module['FS_createPath'] = FS_createPath;
  Module['FS_createDevice'] = FS_createDevice;
  Module['FS'] = FS;
  Module['FS_createDataFile'] = FS_createDataFile;
  Module['FS_createLazyFile'] = FS_createLazyFile;
  var missingLibrarySymbols = [
  'writeI53ToI64Clamped',
  'writeI53ToI64Signaling',
  'writeI53ToU64Clamped',
  'writeI53ToU64Signaling',
  'convertI32PairToI53',
  'convertI32PairToI53Checked',
  'convertU32PairToI53',
  'getTempRet0',
  'withStackSave',
  'inetPton4',
  'inetNtop4',
  'inetPton6',
  'inetNtop6',
  'readSockaddr',
  'writeSockaddr',
  'asmjsMangle',
  'HandleAllocator',
  'addOnInit',
  'addOnPostCtor',
  'addOnPreMain',
  'STACK_SIZE',
  'STACK_ALIGN',
  'POINTER_SIZE',
  'ASSERTIONS',
  'ccall',
  'cwrap',
  'convertJsFunctionToWasm',
  'getEmptyTableSlot',
  'updateTableMap',
  'getFunctionAddress',
  'addFunction',
  'removeFunction',
  'intArrayToString',
  'stringToAscii',
  'writeArrayToMemory',
  'fillDeviceOrientationEventData',
  'registerDeviceOrientationEventCallback',
  'fillDeviceMotionEventData',
  'registerDeviceMotionEventCallback',
  'screenOrientation',
  'fillOrientationChangeEventData',
  'registerOrientationChangeEventCallback',
  'hideEverythingExceptGivenElement',
  'restoreHiddenElements',
  'softFullscreenResizeWebGLRenderTarget',
  'registerPointerlockErrorEventCallback',
  'fillBatteryEventData',
  'registerBatteryEventCallback',
  'jsStackTrace',
  'getCallstack',
  'convertPCtoSourceLocation',
  'wasiRightsToMuslOFlags',
  'wasiOFlagsToMuslOFlags',
  'safeClearTimeout',
  'setImmediateWrapped',
  'clearImmediateWrapped',
  'registerPostMainLoop',
  'getPromise',
  'makePromise',
  'addPromise',
  'idsToPromises',
  'makePromiseCallback',
  'incrementUncaughtExceptionCount',
  'decrementUncaughtExceptionCount',
  'Browser_asyncPrepareDataCounter',
  'isLeapYear',
  'ydayFromDate',
  'arraySum',
  'addDays',
  'getSocketFromFD',
  'getSocketAddress',
  'FS_mkdirTree',
  '_setNetworkCallback',
  'writeGLArray',
  'registerWebGlEventCallback',
  'writeStringToMemory',
  'writeAsciiToMemory',
  'allocateUTF8',
  'allocateUTF8OnStack',
  'demangle',
  'stackTrace',
  'getNativeTypeSize',
  'getFunctionArgsName',
  'createJsInvokerSignature',
  'getEnumValueType',
  'PureVirtualError',
  'registerInheritedInstance',
  'unregisterInheritedInstance',
  'getInheritedInstanceCount',
  'getLiveInheritedInstances',
  'enumReadValueFromPointer',
  'setDelayFunction',
  'validateThis',
  'count_emval_handles',
  'isCppExceptionObject',
];
missingLibrarySymbols.forEach(missingLibrarySymbol)

  var unexportedSymbols = [
  'run',
  'out',
  'err',
  'callMain',
  'abort',
  'wasmExports',
  'writeStackCookie',
  'checkStackCookie',
  'writeI53ToI64',
  'readI53FromI64',
  'readI53FromU64',
  'INT53_MAX',
  'INT53_MIN',
  'bigintToI53Checked',
  'HEAP8',
  'HEAP16',
  'HEAPU16',
  'HEAP32',
  'HEAPU32',
  'HEAPF32',
  'HEAPF64',
  'HEAP64',
  'HEAPU64',
  'stackSave',
  'stackRestore',
  'stackAlloc',
  'setTempRet0',
  'createNamedFunction',
  'ptrToString',
  'zeroMemory',
  'exitJS',
  'getHeapMax',
  'growMemory',
  'ENV',
  'ERRNO_CODES',
  'strError',
  'DNS',
  'Protocols',
  'Sockets',
  'timers',
  'warnOnce',
  'readEmAsmArgsArray',
  'readEmAsmArgs',
  'runEmAsmFunction',
  'runMainThreadEmAsm',
  'jstoi_q',
  'getExecutableName',
  'autoResumeAudioContext',
  'dynCallLegacy',
  'getDynCaller',
  'dynCall',
  'handleException',
  'keepRuntimeAlive',
  'runtimeKeepalivePush',
  'runtimeKeepalivePop',
  'callUserCallback',
  'maybeExit',
  'asyncLoad',
  'alignMemory',
  'mmapAlloc',
  'wasmTable',
  'wasmMemory',
  'getUniqueRunDependency',
  'noExitRuntime',
  'addOnPreRun',
  'addOnExit',
  'addOnPostRun',
  'freeTableIndexes',
  'functionsInTableMap',
  'setValue',
  'getValue',
  'PATH',
  'PATH_FS',
  'UTF8Decoder',
  'UTF8ArrayToString',
  'UTF8ToString',
  'stringToUTF8Array',
  'stringToUTF8',
  'lengthBytesUTF8',
  'intArrayFromString',
  'AsciiToString',
  'UTF16Decoder',
  'UTF16ToString',
  'stringToUTF16',
  'lengthBytesUTF16',
  'UTF32ToString',
  'stringToUTF32',
  'lengthBytesUTF32',
  'stringToNewUTF8',
  'stringToUTF8OnStack',
  'JSEvents',
  'registerKeyEventCallback',
  'specialHTMLTargets',
  'maybeCStringToJsString',
  'findEventTarget',
  'findCanvasEventTarget',
  'getBoundingClientRect',
  'fillMouseEventData',
  'registerMouseEventCallback',
  'registerWheelEventCallback',
  'registerUiEventCallback',
  'registerFocusEventCallback',
  'fillFullscreenChangeEventData',
  'registerFullscreenChangeEventCallback',
  'callCanvasResizedCallback',
  'JSEvents_requestFullscreen',
  'JSEvents_resizeCanvasForFullscreen',
  'registerRestoreOldStyle',
  'setLetterbox',
  'currentFullscreenStrategy',
  'restoreOldWindowedStyle',
  'doRequestFullscreen',
  'fillPointerlockChangeEventData',
  'registerPointerlockChangeEventCallback',
  'requestPointerLock',
  'fillVisibilityChangeEventData',
  'registerVisibilityChangeEventCallback',
  'registerTouchEventCallback',
  'fillGamepadEventData',
  'registerGamepadEventCallback',
  'registerBeforeUnloadEventCallback',
  'setCanvasElementSize',
  'getCanvasElementSize',
  'UNWIND_CACHE',
  'ExitStatus',
  'getEnvStrings',
  'checkWasiClock',
  'doReadv',
  'doWritev',
  'initRandomFill',
  'randomFill',
  'safeSetTimeout',
  'safeRequestAnimationFrame',
  'emSetImmediate',
  'emClearImmediate_deps',
  'emClearImmediate',
  'registerPreMainLoop',
  'promiseMap',
  'uncaughtExceptionCount',
  'exceptionLast',
  'exceptionCaught',
  'ExceptionInfo',
  'findMatchingCatch',
  'getExceptionMessageCommon',
  'incrementExceptionRefcount',
  'decrementExceptionRefcount',
  'getExceptionMessage',
  'Browser',
  'setCanvasSize',
  'getUserMedia',
  'getPreloadedImageData__data',
  'wget',
  'MONTH_DAYS_REGULAR',
  'MONTH_DAYS_LEAP',
  'MONTH_DAYS_REGULAR_CUMULATIVE',
  'MONTH_DAYS_LEAP_CUMULATIVE',
  'SYSCALLS',
  'preloadPlugins',
  'FS_createPreloadedFile',
  'FS_modeStringToFlags',
  'FS_getMode',
  'FS_fileDataToTypedArray',
  'FS_stdin_getChar_buffer',
  'FS_stdin_getChar',
  'FS_readFile',
  'FS_root',
  'FS_mounts',
  'FS_devices',
  'FS_streams',
  'FS_nextInode',
  'FS_nameTable',
  'FS_currentPath',
  'FS_initialized',
  'FS_ignorePermissions',
  'FS_filesystems',
  'FS_syncFSRequests',
  'FS_lookupPath',
  'FS_getPath',
  'FS_hashName',
  'FS_hashAddNode',
  'FS_hashRemoveNode',
  'FS_lookupNode',
  'FS_createNode',
  'FS_destroyNode',
  'FS_isRoot',
  'FS_isMountpoint',
  'FS_isFile',
  'FS_isDir',
  'FS_isLink',
  'FS_isChrdev',
  'FS_isBlkdev',
  'FS_isFIFO',
  'FS_isSocket',
  'FS_flagsToPermissionString',
  'FS_nodePermissions',
  'FS_mayLookup',
  'FS_mayCreate',
  'FS_mayDelete',
  'FS_mayOpen',
  'FS_checkOpExists',
  'FS_nextfd',
  'FS_getStreamChecked',
  'FS_getStream',
  'FS_createStream',
  'FS_closeStream',
  'FS_dupStream',
  'FS_doSetAttr',
  'FS_chrdev_stream_ops',
  'FS_major',
  'FS_minor',
  'FS_makedev',
  'FS_registerDevice',
  'FS_getDevice',
  'FS_getMounts',
  'FS_syncfs',
  'FS_mount',
  'FS_unmount',
  'FS_lookup',
  'FS_mknod',
  'FS_statfs',
  'FS_statfsStream',
  'FS_statfsNode',
  'FS_create',
  'FS_mkdir',
  'FS_mkdev',
  'FS_symlink',
  'FS_link',
  'FS_rename',
  'FS_rmdir',
  'FS_readdir',
  'FS_readlink',
  'FS_stat',
  'FS_fstat',
  'FS_lstat',
  'FS_doChmod',
  'FS_chmod',
  'FS_lchmod',
  'FS_fchmod',
  'FS_doChown',
  'FS_chown',
  'FS_lchown',
  'FS_fchown',
  'FS_doTruncate',
  'FS_truncate',
  'FS_ftruncate',
  'FS_utime',
  'FS_open',
  'FS_close',
  'FS_isClosed',
  'FS_llseek',
  'FS_read',
  'FS_write',
  'FS_mmap',
  'FS_msync',
  'FS_ioctl',
  'FS_writeFile',
  'FS_cwd',
  'FS_chdir',
  'FS_createDefaultDirectories',
  'FS_createDefaultDevices',
  'FS_createSpecialDirectories',
  'FS_createStandardStreams',
  'FS_staticInit',
  'FS_init',
  'FS_quit',
  'FS_findObject',
  'FS_analyzePath',
  'FS_createFile',
  'FS_forceLoadFile',
  'MEMFS',
  'TTY',
  'PIPEFS',
  'SOCKFS',
  'tempFixedLengthArray',
  'miniTempWebGLFloatBuffers',
  'miniTempWebGLIntBuffers',
  'heapObjectForWebGLType',
  'toTypedArrayIndex',
  'webgl_enable_ANGLE_instanced_arrays',
  'webgl_enable_OES_vertex_array_object',
  'webgl_enable_WEBGL_draw_buffers',
  'webgl_enable_WEBGL_multi_draw',
  'webgl_enable_EXT_polygon_offset_clamp',
  'webgl_enable_EXT_clip_control',
  'webgl_enable_WEBGL_polygon_mode',
  'GL',
  'emscriptenWebGLGet',
  'computeUnpackAlignedImageSize',
  'colorChannelsInGlTextureFormat',
  'emscriptenWebGLGetTexPixelData',
  'emscriptenWebGLGetUniform',
  'webglGetProgramUniformLocation',
  'webglGetUniformLocation',
  'webglPrepareUniformLocationsBeforeFirstUse',
  'webglGetLeftBracePos',
  'emscriptenWebGLGetVertexAttrib',
  '__glGetActiveAttribOrUniform',
  'emscriptenWebGLGetBufferBinding',
  'emscriptenWebGLValidateMapBufferTarget',
  'AL',
  'GLUT',
  'EGL',
  'GLEW',
  'IDBStore',
  'runAndAbortIfError',
  'Asyncify',
  'Fibers',
  'emscriptenWebGLGetIndexed',
  'webgl_enable_WEBGL_draw_instanced_base_vertex_base_instance',
  'webgl_enable_WEBGL_multi_draw_instanced_base_vertex_base_instance',
  'print',
  'printErr',
  'jstoi_s',
  'InternalError',
  'BindingError',
  'throwInternalError',
  'throwBindingError',
  'registeredTypes',
  'awaitingDependencies',
  'typeDependencies',
  'tupleRegistrations',
  'structRegistrations',
  'sharedRegisterType',
  'whenDependentTypesAreResolved',
  'getTypeName',
  'getFunctionName',
  'heap32VectorToArray',
  'requireRegisteredType',
  'usesDestructorStack',
  'argsUseStackAlloc',
  'checkArgCount',
  'getRequiredArgCount',
  'createJsInvoker',
  'UnboundTypeError',
  'EmValType',
  'EmValOptionalType',
  'throwUnboundTypeError',
  'ensureOverloadTable',
  'exposePublicSymbol',
  'replacePublicSymbol',
  'embindRepr',
  'registeredInstances',
  'getBasestPointer',
  'getInheritedInstance',
  'registeredPointers',
  'registerType',
  'integerReadValueFromPointer',
  'floatReadValueFromPointer',
  'assertIntegerRange',
  'readPointer',
  'installIndexedIterator',
  'runDestructors',
  'craftInvokerFunction',
  'embind__requireFunction',
  'genericPointerToWireType',
  'constNoSmartPtrRawPointerToWireType',
  'nonConstNoSmartPtrRawPointerToWireType',
  'init_RegisteredPointer',
  'RegisteredPointer',
  'RegisteredPointer_fromWireType',
  'runDestructor',
  'releaseClassHandle',
  'finalizationRegistry',
  'detachFinalizer_deps',
  'detachFinalizer',
  'attachFinalizer',
  'makeClassHandle',
  'init_ClassHandle',
  'ClassHandle',
  'throwInstanceAlreadyDeleted',
  'deletionQueue',
  'flushPendingDeletes',
  'delayFunction',
  'RegisteredClass',
  'shallowCopyInternalPointer',
  'downcastPointer',
  'upcastPointer',
  'char_0',
  'char_9',
  'makeLegalFunctionName',
  'emval_freelist',
  'emval_exception_decrefs',
  'emval_handles',
  'emval_symbols',
  'getStringOrSymbol',
  'Emval',
  'emval_returnValue',
  'emval_lookupTypes',
  'emval_methodCallers',
  'emval_addMethodCaller',
];
unexportedSymbols.forEach(unexportedRuntimeSymbol);

  // End runtime exports
  // Begin JS library exports
  // End JS library exports

// end include: postlibrary.js

function checkIncomingModuleAPI() {
  ignoredModuleProp('fetchSettings');
  ignoredModuleProp('logReadFiles');
  ignoredModuleProp('loadSplitModule');
  ignoredModuleProp('onMalloc');
  ignoredModuleProp('onRealloc');
  ignoredModuleProp('onFree');
  ignoredModuleProp('onSbrkGrow');
  ignoredModuleProp('onCOSCacheHit');
  ignoredModuleProp('onCOSCacheMiss');
  ignoredModuleProp('onCOSStore');
  ignoredModuleProp('GL_MAX_TEXTURE_IMAGE_UNITS');
  ignoredModuleProp('SDL_canPlayWithWebAudio');
  ignoredModuleProp('SDL_numSimultaneouslyQueuedBuffers');
  ignoredModuleProp('freePreloadedMediaOnUse');
  ignoredModuleProp('keyboardListeningElement');
  ignoredModuleProp('doNotCaptureKeyboard');
  ignoredModuleProp('extraStackTrace');
  ignoredModuleProp('preloadPlugins');
  ignoredModuleProp('preMainLoop');
  ignoredModuleProp('postMainLoop');
  ignoredModuleProp('forcedAspectRatio');
  ignoredModuleProp('mainScriptUrlOrBlob');
  ignoredModuleProp('onFullScreen');
  ignoredModuleProp('INITIAL_MEMORY');
  ignoredModuleProp('wasmMemory');
}
var ASM_CONSTS = {
  1660932: ($0, $1, $2, $3, $4, $5, $6) => { if (typeof window.onShaderQueueProgress === 'function') window.onShaderQueueProgress($0, $1, $2, $3, $4, !!$5, $6); },  
 1661052: ($0) => { if (typeof window.onAllShadersCompiled === 'function') window.onAllShadersCompiled($0); },  
 1661144: ($0, $1) => { if (typeof window.addLoadingMessage === 'function') { window.addLoadingMessage('MX2 Graphics Demo', 'info'); window.addLoadingMessage('OpenGL initialized: ' + $0 + 'x' + $1, 'success'); window.addLoadingMessage(' ', 'normal'); window.addLoadingMessage('Compiling initial shaders...', 'info'); window.addLoadingMessage(' ', 'normal'); } },  
 1661484: ($0, $1, $2, $3, $4, $5, $6) => { if (typeof window.onShaderPrepared === 'function') window.onShaderPrepared($0, UTF8ToString($1), UTF8ToString($2), !!$3, !!$4, UTF8ToString($5), !!$6); },  
 1661640: ($0, $1) => { if (typeof window.addLoadingMessage === 'function') { window.addLoadingMessage('Texture loaded: ' + $0 + 'x' + $1, 'success'); window.addLoadingMessage(' ', 'normal'); window.addLoadingMessage('Initialization complete!', 'success'); } },  
 1661879: () => { setTimeout( function() { if (typeof window.onShaderLibraryReady === 'function') window.onShaderLibraryReady(); if (typeof window.hideLoadingScreen === 'function') { window.hideLoadingScreen(); } }, 0); },  
 1662085: () => { return typeof window.consumeShaderIdleTime === 'function' && window.consumeShaderIdleTime() ? 1 : 0; },  
 1662190: ($0) => { if (typeof window.onShaderSelectionActivated === 'function') window.onShaderSelectionActivated($0); },  
 1662294: ($0, $1, $2, $3, $4) => { var cropW = $0; var cropH = $1; var dataPtr = $2; var finalW = $3; var finalH = $4; try { var srcCanvas = document.createElement('canvas'); srcCanvas.width = cropW; srcCanvas.height = cropH; var srcCtx = srcCanvas.getContext('2d'); var pixelArray = new Uint8ClampedArray(cropW * cropH * 4); for (var i = 0; i < cropW * cropH * 4; i++) { pixelArray[i] = Module.HEAPU8[dataPtr + i]; } var imageData = new ImageData(pixelArray, cropW, cropH); srcCtx.putImageData(imageData, 0, 0); var outCanvas = document.createElement('canvas'); outCanvas.width = finalW; outCanvas.height = finalH; var outCtx = outCanvas.getContext('2d'); outCtx.imageSmoothingEnabled = true; outCtx.imageSmoothingQuality = 'high'; outCtx.drawImage(srcCanvas, 0, 0, finalW, finalH); var now = new Date(); var timestamp = now.getFullYear() + '-' + String(now.getMonth() + 1).padStart(2, '0') + '-' + String(now.getDate()).padStart(2, '0') + 'T' + String(now.getHours()).padStart(2, '0') + '-' + String(now.getMinutes()).padStart(2, '0') + '-' + String(now.getSeconds()).padStart(2, '0'); var dataUrl = outCanvas.toDataURL('image/png'); var filename = 'acmx2.visualizer.' + finalW + 'x' + finalH + '.' + timestamp + '.png'; if (typeof window.AndroidInterface !== 'undefined' &&typeof window.AndroidInterface.saveImage === 'function') { window.AndroidInterface.saveImage(dataUrl); } else { var link = document.createElement('a'); link.download = filename; link.href = dataUrl; document.body.appendChild(link); link.click(); document.body.removeChild(link); } console.log('Image saved:', finalW, 'x', finalH); } catch (e) { console.error('Save error:', e); } },  
 1663919: ($0) => { var str = UTF8ToString($0) + '\n\n' + 'Abort/Retry/Ignore/AlwaysIgnore? [ariA] :'; var reply = window.prompt(str, "i"); if (reply === null) { reply = "i"; } return reply.length === 1 ? reply.charCodeAt(0) : -1; },  
 1664134: () => { if (typeof(AudioContext) !== 'undefined') { return true; } else if (typeof(webkitAudioContext) !== 'undefined') { return true; } return false; },  
 1664281: () => { if ((typeof(navigator.mediaDevices) !== 'undefined') && (typeof(navigator.mediaDevices.getUserMedia) !== 'undefined')) { return true; } else if (typeof(navigator.webkitGetUserMedia) !== 'undefined') { return true; } return false; },  
 1664515: ($0) => { if(typeof(Module['SDL2']) === 'undefined') { Module['SDL2'] = {}; } var SDL2 = Module['SDL2']; if (!$0) { SDL2.audio = {}; } else { SDL2.capture = {}; } if (!SDL2.audioContext) { if (typeof(AudioContext) !== 'undefined') { SDL2.audioContext = new AudioContext(); } else if (typeof(webkitAudioContext) !== 'undefined') { SDL2.audioContext = new webkitAudioContext(); } if (SDL2.audioContext) { if ((typeof navigator.userActivation) === 'undefined') { autoResumeAudioContext(SDL2.audioContext); } } } return SDL2.audioContext === undefined ? -1 : 0; },  
 1665067: () => { var SDL2 = Module['SDL2']; return SDL2.audioContext.sampleRate; },  
 1665135: ($0, $1, $2, $3) => { var SDL2 = Module['SDL2']; var have_microphone = function(stream) { if (SDL2.capture.silenceTimer !== undefined) { clearInterval(SDL2.capture.silenceTimer); SDL2.capture.silenceTimer = undefined; SDL2.capture.silenceBuffer = undefined } SDL2.capture.mediaStreamNode = SDL2.audioContext.createMediaStreamSource(stream); SDL2.capture.scriptProcessorNode = SDL2.audioContext.createScriptProcessor($1, $0, 1); SDL2.capture.scriptProcessorNode.onaudioprocess = function(audioProcessingEvent) { if ((SDL2 === undefined) || (SDL2.capture === undefined)) { return; } audioProcessingEvent.outputBuffer.getChannelData(0).fill(0.0); SDL2.capture.currentCaptureBuffer = audioProcessingEvent.inputBuffer; dynCall('vp', $2, [$3]); }; SDL2.capture.mediaStreamNode.connect(SDL2.capture.scriptProcessorNode); SDL2.capture.scriptProcessorNode.connect(SDL2.audioContext.destination); SDL2.capture.stream = stream; }; var no_microphone = function(error) { }; SDL2.capture.silenceBuffer = SDL2.audioContext.createBuffer($0, $1, SDL2.audioContext.sampleRate); SDL2.capture.silenceBuffer.getChannelData(0).fill(0.0); var silence_callback = function() { SDL2.capture.currentCaptureBuffer = SDL2.capture.silenceBuffer; dynCall('vp', $2, [$3]); }; SDL2.capture.silenceTimer = setInterval(silence_callback, ($1 / SDL2.audioContext.sampleRate) * 1000); if ((navigator.mediaDevices !== undefined) && (navigator.mediaDevices.getUserMedia !== undefined)) { navigator.mediaDevices.getUserMedia({ audio: true, video: false }).then(have_microphone).catch(no_microphone); } else if (navigator.webkitGetUserMedia !== undefined) { navigator.webkitGetUserMedia({ audio: true, video: false }, have_microphone, no_microphone); } },  
 1666828: ($0, $1, $2, $3) => { var SDL2 = Module['SDL2']; SDL2.audio.scriptProcessorNode = SDL2.audioContext['createScriptProcessor']($1, 0, $0); SDL2.audio.scriptProcessorNode['onaudioprocess'] = function (e) { if ((SDL2 === undefined) || (SDL2.audio === undefined)) { return; } if (SDL2.audio.silenceTimer !== undefined) { clearInterval(SDL2.audio.silenceTimer); SDL2.audio.silenceTimer = undefined; SDL2.audio.silenceBuffer = undefined; } SDL2.audio.currentOutputBuffer = e['outputBuffer']; dynCall('vp', $2, [$3]); }; SDL2.audio.scriptProcessorNode['connect'](SDL2.audioContext['destination']); if (SDL2.audioContext.state === 'suspended') { SDL2.audio.silenceBuffer = SDL2.audioContext.createBuffer($0, $1, SDL2.audioContext.sampleRate); SDL2.audio.silenceBuffer.getChannelData(0).fill(0.0); var silence_callback = function() { if ((typeof navigator.userActivation) !== 'undefined') { if (navigator.userActivation.hasBeenActive) { SDL2.audioContext.resume(); } } SDL2.audio.currentOutputBuffer = SDL2.audio.silenceBuffer; dynCall('vp', $2, [$3]); SDL2.audio.currentOutputBuffer = undefined; }; SDL2.audio.silenceTimer = setInterval(silence_callback, ($1 / SDL2.audioContext.sampleRate) * 1000); } },  
 1668003: ($0, $1) => { var SDL2 = Module['SDL2']; var numChannels = SDL2.capture.currentCaptureBuffer.numberOfChannels; for (var c = 0; c < numChannels; ++c) { var channelData = SDL2.capture.currentCaptureBuffer.getChannelData(c); if (channelData.length != $1) { throw 'Web Audio capture buffer length mismatch! Destination size: ' + channelData.length + ' samples vs expected ' + $1 + ' samples!'; } if (numChannels == 1) { for (var j = 0; j < $1; ++j) { setValue($0 + (j * 4), channelData[j], 'float'); } } else { for (var j = 0; j < $1; ++j) { setValue($0 + (((j * numChannels) + c) * 4), channelData[j], 'float'); } } } },  
 1668608: ($0, $1) => { var SDL2 = Module['SDL2']; var buf = $0 >>> 2; var numChannels = SDL2.audio.currentOutputBuffer['numberOfChannels']; for (var c = 0; c < numChannels; ++c) { var channelData = SDL2.audio.currentOutputBuffer['getChannelData'](c); if (channelData.length != $1) { throw 'Web Audio output buffer length mismatch! Destination size: ' + channelData.length + ' samples vs expected ' + $1 + ' samples!'; } for (var j = 0; j < $1; ++j) { channelData[j] = HEAPF32[buf + (j*numChannels + c)]; } } },  
 1669097: ($0) => { var SDL2 = Module['SDL2']; if ($0) { if (SDL2.capture.silenceTimer !== undefined) { clearInterval(SDL2.capture.silenceTimer); } if (SDL2.capture.stream !== undefined) { var tracks = SDL2.capture.stream.getAudioTracks(); for (var i = 0; i < tracks.length; i++) { SDL2.capture.stream.removeTrack(tracks[i]); } } if (SDL2.capture.scriptProcessorNode !== undefined) { SDL2.capture.scriptProcessorNode.onaudioprocess = function(audioProcessingEvent) {}; SDL2.capture.scriptProcessorNode.disconnect(); } if (SDL2.capture.mediaStreamNode !== undefined) { SDL2.capture.mediaStreamNode.disconnect(); } SDL2.capture = undefined; } else { if (SDL2.audio.scriptProcessorNode != undefined) { SDL2.audio.scriptProcessorNode.disconnect(); } if (SDL2.audio.silenceTimer !== undefined) { clearInterval(SDL2.audio.silenceTimer); } SDL2.audio = undefined; } if ((SDL2.audioContext !== undefined) && (SDL2.audio === undefined) && (SDL2.capture === undefined)) { SDL2.audioContext.close(); SDL2.audioContext = undefined; } },  
 1670103: ($0, $1, $2) => { var w = $0; var h = $1; var pixels = $2; if (!Module['SDL2']) Module['SDL2'] = {}; var SDL2 = Module['SDL2']; if (SDL2.ctxCanvas !== Module['canvas']) { SDL2.ctx = Browser.createContext(Module['canvas'], false, true); SDL2.ctxCanvas = Module['canvas']; } if (SDL2.w !== w || SDL2.h !== h || SDL2.imageCtx !== SDL2.ctx) { SDL2.image = SDL2.ctx.createImageData(w, h); SDL2.w = w; SDL2.h = h; SDL2.imageCtx = SDL2.ctx; } var data = SDL2.image.data; var src = pixels / 4; var dst = 0; var num; if (typeof CanvasPixelArray !== 'undefined' && data instanceof CanvasPixelArray) { num = data.length; while (dst < num) { var val = HEAP32[src]; data[dst ] = val & 0xff; data[dst+1] = (val >> 8) & 0xff; data[dst+2] = (val >> 16) & 0xff; data[dst+3] = 0xff; src++; dst += 4; } } else { if (SDL2.data32Data !== data) { SDL2.data32 = new Int32Array(data.buffer); SDL2.data8 = new Uint8Array(data.buffer); SDL2.data32Data = data; } var data32 = SDL2.data32; num = data32.length; data32.set(HEAP32.subarray(src, src + num)); var data8 = SDL2.data8; var i = 3; var j = i + 4*num; if (num % 8 == 0) { while (i < j) { data8[i] = 0xff; i = i + 4 | 0; data8[i] = 0xff; i = i + 4 | 0; data8[i] = 0xff; i = i + 4 | 0; data8[i] = 0xff; i = i + 4 | 0; data8[i] = 0xff; i = i + 4 | 0; data8[i] = 0xff; i = i + 4 | 0; data8[i] = 0xff; i = i + 4 | 0; data8[i] = 0xff; i = i + 4 | 0; } } else { while (i < j) { data8[i] = 0xff; i = i + 4 | 0; } } } SDL2.ctx.putImageData(SDL2.image, 0, 0); },  
 1671569: ($0, $1, $2, $3, $4) => { var w = $0; var h = $1; var hot_x = $2; var hot_y = $3; var pixels = $4; var canvas = document.createElement("canvas"); canvas.width = w; canvas.height = h; var ctx = canvas.getContext("2d"); var image = ctx.createImageData(w, h); var data = image.data; var src = pixels / 4; var dst = 0; var num; if (typeof CanvasPixelArray !== 'undefined' && data instanceof CanvasPixelArray) { num = data.length; while (dst < num) { var val = HEAP32[src]; data[dst ] = val & 0xff; data[dst+1] = (val >> 8) & 0xff; data[dst+2] = (val >> 16) & 0xff; data[dst+3] = (val >> 24) & 0xff; src++; dst += 4; } } else { var data32 = new Int32Array(data.buffer); num = data32.length; data32.set(HEAP32.subarray(src, src + num)); } ctx.putImageData(image, 0, 0); var url = hot_x === 0 && hot_y === 0 ? "url(" + canvas.toDataURL() + "), auto" : "url(" + canvas.toDataURL() + ") " + hot_x + " " + hot_y + ", auto"; var urlBuf = _malloc(url.length + 1); stringToUTF8(url, urlBuf, url.length + 1); return urlBuf; },  
 1672557: ($0) => { if (Module['canvas']) { Module['canvas'].style['cursor'] = UTF8ToString($0); } },  
 1672640: () => { if (Module['canvas']) { Module['canvas'].style['cursor'] = 'none'; } },  
 1672709: () => { return window.innerWidth; },  
 1672739: () => { return window.innerHeight; }
};

// Imports from the Wasm binary.
var _main = Module['_main'] = makeInvalidEarlyAccess('_main');
var _malloc = Module['_malloc'] = makeInvalidEarlyAccess('_malloc');
var _fflush = makeInvalidEarlyAccess('_fflush');
var _free = Module['_free'] = makeInvalidEarlyAccess('_free');
var ___getTypeName = makeInvalidEarlyAccess('___getTypeName');
var _strerror = makeInvalidEarlyAccess('_strerror');
var _fileno = makeInvalidEarlyAccess('_fileno');
var _emscripten_stack_get_end = makeInvalidEarlyAccess('_emscripten_stack_get_end');
var _emscripten_stack_get_base = makeInvalidEarlyAccess('_emscripten_stack_get_base');
var _emscripten_builtin_memalign = makeInvalidEarlyAccess('_emscripten_builtin_memalign');
var _setThrew = makeInvalidEarlyAccess('_setThrew');
var __emscripten_tempret_set = makeInvalidEarlyAccess('__emscripten_tempret_set');
var _emscripten_stack_init = makeInvalidEarlyAccess('_emscripten_stack_init');
var _emscripten_stack_get_free = makeInvalidEarlyAccess('_emscripten_stack_get_free');
var __emscripten_stack_restore = makeInvalidEarlyAccess('__emscripten_stack_restore');
var __emscripten_stack_alloc = makeInvalidEarlyAccess('__emscripten_stack_alloc');
var _emscripten_stack_get_current = makeInvalidEarlyAccess('_emscripten_stack_get_current');
var ___cxa_decrement_exception_refcount = makeInvalidEarlyAccess('___cxa_decrement_exception_refcount');
var ___cxa_increment_exception_refcount = makeInvalidEarlyAccess('___cxa_increment_exception_refcount');
var ___get_exception_message = makeInvalidEarlyAccess('___get_exception_message');
var ___cxa_can_catch = makeInvalidEarlyAccess('___cxa_can_catch');
var ___cxa_get_exception_ptr = makeInvalidEarlyAccess('___cxa_get_exception_ptr');
var dynCall_iii = makeInvalidEarlyAccess('dynCall_iii');
var dynCall_vi = makeInvalidEarlyAccess('dynCall_vi');
var dynCall_vii = makeInvalidEarlyAccess('dynCall_vii');
var dynCall_viiii = makeInvalidEarlyAccess('dynCall_viiii');
var dynCall_viif = makeInvalidEarlyAccess('dynCall_viif');
var dynCall_viii = makeInvalidEarlyAccess('dynCall_viii');
var dynCall_ii = makeInvalidEarlyAccess('dynCall_ii');
var dynCall_iiii = makeInvalidEarlyAccess('dynCall_iiii');
var dynCall_iiiiiii = makeInvalidEarlyAccess('dynCall_iiiiiii');
var dynCall_v = makeInvalidEarlyAccess('dynCall_v');
var dynCall_i = makeInvalidEarlyAccess('dynCall_i');
var dynCall_viiiii = makeInvalidEarlyAccess('dynCall_viiiii');
var dynCall_vf = makeInvalidEarlyAccess('dynCall_vf');
var dynCall_f = makeInvalidEarlyAccess('dynCall_f');
var dynCall_vffi = makeInvalidEarlyAccess('dynCall_vffi');
var dynCall_iiiii = makeInvalidEarlyAccess('dynCall_iiiii');
var dynCall_iiiiiiiii = makeInvalidEarlyAccess('dynCall_iiiiiiiii');
var dynCall_viiiiiii = makeInvalidEarlyAccess('dynCall_viiiiiii');
var dynCall_iiiiii = makeInvalidEarlyAccess('dynCall_iiiiii');
var dynCall_iiiiiiiiii = makeInvalidEarlyAccess('dynCall_iiiiiiiiii');
var dynCall_viiiiii = makeInvalidEarlyAccess('dynCall_viiiiii');
var dynCall_vif = makeInvalidEarlyAccess('dynCall_vif');
var dynCall_fi = makeInvalidEarlyAccess('dynCall_fi');
var dynCall_viffi = makeInvalidEarlyAccess('dynCall_viffi');
var dynCall_viff = makeInvalidEarlyAccess('dynCall_viff');
var dynCall_iif = makeInvalidEarlyAccess('dynCall_iif');
var dynCall_viijii = makeInvalidEarlyAccess('dynCall_viijii');
var dynCall_ji = makeInvalidEarlyAccess('dynCall_ji');
var dynCall_jiji = makeInvalidEarlyAccess('dynCall_jiji');
var dynCall_viiiiiiiii = makeInvalidEarlyAccess('dynCall_viiiiiiiii');
var dynCall_iiji = makeInvalidEarlyAccess('dynCall_iiji');
var dynCall_iid = makeInvalidEarlyAccess('dynCall_iid');
var dynCall_di = makeInvalidEarlyAccess('dynCall_di');
var dynCall_viiiiiiii = makeInvalidEarlyAccess('dynCall_viiiiiiii');
var dynCall_iiiiiiii = makeInvalidEarlyAccess('dynCall_iiiiiiii');
var dynCall_vffff = makeInvalidEarlyAccess('dynCall_vffff');
var dynCall_vff = makeInvalidEarlyAccess('dynCall_vff');
var dynCall_vfi = makeInvalidEarlyAccess('dynCall_vfi');
var dynCall_vifff = makeInvalidEarlyAccess('dynCall_vifff');
var dynCall_viffff = makeInvalidEarlyAccess('dynCall_viffff');
var dynCall_vfff = makeInvalidEarlyAccess('dynCall_vfff');
var dynCall_viiiiiiiiii = makeInvalidEarlyAccess('dynCall_viiiiiiiiii');
var dynCall_viiiiiiiiiii = makeInvalidEarlyAccess('dynCall_viiiiiiiiiii');
var dynCall_viifi = makeInvalidEarlyAccess('dynCall_viifi');
var dynCall_iiij = makeInvalidEarlyAccess('dynCall_iiij');
var dynCall_viij = makeInvalidEarlyAccess('dynCall_viij');
var dynCall_iidiiiii = makeInvalidEarlyAccess('dynCall_iidiiiii');
var dynCall_viiifi = makeInvalidEarlyAccess('dynCall_viiifi');
var dynCall_viiidi = makeInvalidEarlyAccess('dynCall_viiidi');
var dynCall_iiiiiiiiiiiii = makeInvalidEarlyAccess('dynCall_iiiiiiiiiiiii');
var dynCall_fiii = makeInvalidEarlyAccess('dynCall_fiii');
var dynCall_diii = makeInvalidEarlyAccess('dynCall_diii');
var dynCall_iiiiiiiiiiii = makeInvalidEarlyAccess('dynCall_iiiiiiiiiiii');
var dynCall_viiiiiiiiiiiiiii = makeInvalidEarlyAccess('dynCall_viiiiiiiiiiiiiii');
var dynCall_iiiiij = makeInvalidEarlyAccess('dynCall_iiiiij');
var dynCall_iiiiid = makeInvalidEarlyAccess('dynCall_iiiiid');
var dynCall_iiiiijj = makeInvalidEarlyAccess('dynCall_iiiiijj');
var dynCall_iiiiiijj = makeInvalidEarlyAccess('dynCall_iiiiiijj');
var _asyncify_start_unwind = makeInvalidEarlyAccess('_asyncify_start_unwind');
var _asyncify_stop_unwind = makeInvalidEarlyAccess('_asyncify_stop_unwind');
var _asyncify_start_rewind = makeInvalidEarlyAccess('_asyncify_start_rewind');
var _asyncify_stop_rewind = makeInvalidEarlyAccess('_asyncify_stop_rewind');
var memory = makeInvalidEarlyAccess('memory');
var __indirect_function_table = makeInvalidEarlyAccess('__indirect_function_table');
var wasmMemory = makeInvalidEarlyAccess('wasmMemory');
var wasmTable = makeInvalidEarlyAccess('wasmTable');

function assignWasmExports(wasmExports) {
  assert(typeof wasmExports['__main_argc_argv'] != 'undefined', 'missing Wasm export: __main_argc_argv');
  assert(typeof wasmExports['malloc'] != 'undefined', 'missing Wasm export: malloc');
  assert(typeof wasmExports['fflush'] != 'undefined', 'missing Wasm export: fflush');
  assert(typeof wasmExports['free'] != 'undefined', 'missing Wasm export: free');
  assert(typeof wasmExports['__getTypeName'] != 'undefined', 'missing Wasm export: __getTypeName');
  assert(typeof wasmExports['strerror'] != 'undefined', 'missing Wasm export: strerror');
  assert(typeof wasmExports['fileno'] != 'undefined', 'missing Wasm export: fileno');
  assert(typeof wasmExports['emscripten_stack_get_end'] != 'undefined', 'missing Wasm export: emscripten_stack_get_end');
  assert(typeof wasmExports['emscripten_stack_get_base'] != 'undefined', 'missing Wasm export: emscripten_stack_get_base');
  assert(typeof wasmExports['emscripten_builtin_memalign'] != 'undefined', 'missing Wasm export: emscripten_builtin_memalign');
  assert(typeof wasmExports['setThrew'] != 'undefined', 'missing Wasm export: setThrew');
  assert(typeof wasmExports['_emscripten_tempret_set'] != 'undefined', 'missing Wasm export: _emscripten_tempret_set');
  assert(typeof wasmExports['emscripten_stack_init'] != 'undefined', 'missing Wasm export: emscripten_stack_init');
  assert(typeof wasmExports['emscripten_stack_get_free'] != 'undefined', 'missing Wasm export: emscripten_stack_get_free');
  assert(typeof wasmExports['_emscripten_stack_restore'] != 'undefined', 'missing Wasm export: _emscripten_stack_restore');
  assert(typeof wasmExports['_emscripten_stack_alloc'] != 'undefined', 'missing Wasm export: _emscripten_stack_alloc');
  assert(typeof wasmExports['emscripten_stack_get_current'] != 'undefined', 'missing Wasm export: emscripten_stack_get_current');
  assert(typeof wasmExports['__cxa_decrement_exception_refcount'] != 'undefined', 'missing Wasm export: __cxa_decrement_exception_refcount');
  assert(typeof wasmExports['__cxa_increment_exception_refcount'] != 'undefined', 'missing Wasm export: __cxa_increment_exception_refcount');
  assert(typeof wasmExports['__get_exception_message'] != 'undefined', 'missing Wasm export: __get_exception_message');
  assert(typeof wasmExports['__cxa_can_catch'] != 'undefined', 'missing Wasm export: __cxa_can_catch');
  assert(typeof wasmExports['__cxa_get_exception_ptr'] != 'undefined', 'missing Wasm export: __cxa_get_exception_ptr');
  assert(typeof wasmExports['dynCall_iii'] != 'undefined', 'missing Wasm export: dynCall_iii');
  assert(typeof wasmExports['dynCall_vi'] != 'undefined', 'missing Wasm export: dynCall_vi');
  assert(typeof wasmExports['dynCall_vii'] != 'undefined', 'missing Wasm export: dynCall_vii');
  assert(typeof wasmExports['dynCall_viiii'] != 'undefined', 'missing Wasm export: dynCall_viiii');
  assert(typeof wasmExports['dynCall_viif'] != 'undefined', 'missing Wasm export: dynCall_viif');
  assert(typeof wasmExports['dynCall_viii'] != 'undefined', 'missing Wasm export: dynCall_viii');
  assert(typeof wasmExports['dynCall_ii'] != 'undefined', 'missing Wasm export: dynCall_ii');
  assert(typeof wasmExports['dynCall_iiii'] != 'undefined', 'missing Wasm export: dynCall_iiii');
  assert(typeof wasmExports['dynCall_iiiiiii'] != 'undefined', 'missing Wasm export: dynCall_iiiiiii');
  assert(typeof wasmExports['dynCall_v'] != 'undefined', 'missing Wasm export: dynCall_v');
  assert(typeof wasmExports['dynCall_i'] != 'undefined', 'missing Wasm export: dynCall_i');
  assert(typeof wasmExports['dynCall_viiiii'] != 'undefined', 'missing Wasm export: dynCall_viiiii');
  assert(typeof wasmExports['dynCall_vf'] != 'undefined', 'missing Wasm export: dynCall_vf');
  assert(typeof wasmExports['dynCall_f'] != 'undefined', 'missing Wasm export: dynCall_f');
  assert(typeof wasmExports['dynCall_vffi'] != 'undefined', 'missing Wasm export: dynCall_vffi');
  assert(typeof wasmExports['dynCall_iiiii'] != 'undefined', 'missing Wasm export: dynCall_iiiii');
  assert(typeof wasmExports['dynCall_iiiiiiiii'] != 'undefined', 'missing Wasm export: dynCall_iiiiiiiii');
  assert(typeof wasmExports['dynCall_viiiiiii'] != 'undefined', 'missing Wasm export: dynCall_viiiiiii');
  assert(typeof wasmExports['dynCall_iiiiii'] != 'undefined', 'missing Wasm export: dynCall_iiiiii');
  assert(typeof wasmExports['dynCall_iiiiiiiiii'] != 'undefined', 'missing Wasm export: dynCall_iiiiiiiiii');
  assert(typeof wasmExports['dynCall_viiiiii'] != 'undefined', 'missing Wasm export: dynCall_viiiiii');
  assert(typeof wasmExports['dynCall_vif'] != 'undefined', 'missing Wasm export: dynCall_vif');
  assert(typeof wasmExports['dynCall_fi'] != 'undefined', 'missing Wasm export: dynCall_fi');
  assert(typeof wasmExports['dynCall_viffi'] != 'undefined', 'missing Wasm export: dynCall_viffi');
  assert(typeof wasmExports['dynCall_viff'] != 'undefined', 'missing Wasm export: dynCall_viff');
  assert(typeof wasmExports['dynCall_iif'] != 'undefined', 'missing Wasm export: dynCall_iif');
  assert(typeof wasmExports['dynCall_viijii'] != 'undefined', 'missing Wasm export: dynCall_viijii');
  assert(typeof wasmExports['dynCall_ji'] != 'undefined', 'missing Wasm export: dynCall_ji');
  assert(typeof wasmExports['dynCall_jiji'] != 'undefined', 'missing Wasm export: dynCall_jiji');
  assert(typeof wasmExports['dynCall_viiiiiiiii'] != 'undefined', 'missing Wasm export: dynCall_viiiiiiiii');
  assert(typeof wasmExports['dynCall_iiji'] != 'undefined', 'missing Wasm export: dynCall_iiji');
  assert(typeof wasmExports['dynCall_iid'] != 'undefined', 'missing Wasm export: dynCall_iid');
  assert(typeof wasmExports['dynCall_di'] != 'undefined', 'missing Wasm export: dynCall_di');
  assert(typeof wasmExports['dynCall_viiiiiiii'] != 'undefined', 'missing Wasm export: dynCall_viiiiiiii');
  assert(typeof wasmExports['dynCall_iiiiiiii'] != 'undefined', 'missing Wasm export: dynCall_iiiiiiii');
  assert(typeof wasmExports['dynCall_vffff'] != 'undefined', 'missing Wasm export: dynCall_vffff');
  assert(typeof wasmExports['dynCall_vff'] != 'undefined', 'missing Wasm export: dynCall_vff');
  assert(typeof wasmExports['dynCall_vfi'] != 'undefined', 'missing Wasm export: dynCall_vfi');
  assert(typeof wasmExports['dynCall_vifff'] != 'undefined', 'missing Wasm export: dynCall_vifff');
  assert(typeof wasmExports['dynCall_viffff'] != 'undefined', 'missing Wasm export: dynCall_viffff');
  assert(typeof wasmExports['dynCall_vfff'] != 'undefined', 'missing Wasm export: dynCall_vfff');
  assert(typeof wasmExports['dynCall_viiiiiiiiii'] != 'undefined', 'missing Wasm export: dynCall_viiiiiiiiii');
  assert(typeof wasmExports['dynCall_viiiiiiiiiii'] != 'undefined', 'missing Wasm export: dynCall_viiiiiiiiiii');
  assert(typeof wasmExports['dynCall_viifi'] != 'undefined', 'missing Wasm export: dynCall_viifi');
  assert(typeof wasmExports['dynCall_iiij'] != 'undefined', 'missing Wasm export: dynCall_iiij');
  assert(typeof wasmExports['dynCall_viij'] != 'undefined', 'missing Wasm export: dynCall_viij');
  assert(typeof wasmExports['dynCall_iidiiiii'] != 'undefined', 'missing Wasm export: dynCall_iidiiiii');
  assert(typeof wasmExports['dynCall_viiifi'] != 'undefined', 'missing Wasm export: dynCall_viiifi');
  assert(typeof wasmExports['dynCall_viiidi'] != 'undefined', 'missing Wasm export: dynCall_viiidi');
  assert(typeof wasmExports['dynCall_iiiiiiiiiiiii'] != 'undefined', 'missing Wasm export: dynCall_iiiiiiiiiiiii');
  assert(typeof wasmExports['dynCall_fiii'] != 'undefined', 'missing Wasm export: dynCall_fiii');
  assert(typeof wasmExports['dynCall_diii'] != 'undefined', 'missing Wasm export: dynCall_diii');
  assert(typeof wasmExports['dynCall_iiiiiiiiiiii'] != 'undefined', 'missing Wasm export: dynCall_iiiiiiiiiiii');
  assert(typeof wasmExports['dynCall_viiiiiiiiiiiiiii'] != 'undefined', 'missing Wasm export: dynCall_viiiiiiiiiiiiiii');
  assert(typeof wasmExports['dynCall_iiiiij'] != 'undefined', 'missing Wasm export: dynCall_iiiiij');
  assert(typeof wasmExports['dynCall_iiiiid'] != 'undefined', 'missing Wasm export: dynCall_iiiiid');
  assert(typeof wasmExports['dynCall_iiiiijj'] != 'undefined', 'missing Wasm export: dynCall_iiiiijj');
  assert(typeof wasmExports['dynCall_iiiiiijj'] != 'undefined', 'missing Wasm export: dynCall_iiiiiijj');
  assert(typeof wasmExports['asyncify_start_unwind'] != 'undefined', 'missing Wasm export: asyncify_start_unwind');
  assert(typeof wasmExports['asyncify_stop_unwind'] != 'undefined', 'missing Wasm export: asyncify_stop_unwind');
  assert(typeof wasmExports['asyncify_start_rewind'] != 'undefined', 'missing Wasm export: asyncify_start_rewind');
  assert(typeof wasmExports['asyncify_stop_rewind'] != 'undefined', 'missing Wasm export: asyncify_stop_rewind');
  assert(typeof wasmExports['memory'] != 'undefined', 'missing Wasm export: memory');
  assert(typeof wasmExports['__indirect_function_table'] != 'undefined', 'missing Wasm export: __indirect_function_table');
  _main = Module['_main'] = createExportWrapper('__main_argc_argv', wasmExports['__main_argc_argv'], 2);
  _malloc = Module['_malloc'] = createExportWrapper('malloc', wasmExports['malloc'], 1);
  _fflush = createExportWrapper('fflush', wasmExports['fflush'], 1);
  _free = Module['_free'] = createExportWrapper('free', wasmExports['free'], 1);
  ___getTypeName = createExportWrapper('__getTypeName', wasmExports['__getTypeName'], 1);
  _strerror = createExportWrapper('strerror', wasmExports['strerror'], 1);
  _fileno = createExportWrapper('fileno', wasmExports['fileno'], 1);
  _emscripten_stack_get_end = wasmExports['emscripten_stack_get_end'];
  _emscripten_stack_get_base = wasmExports['emscripten_stack_get_base'];
  _emscripten_builtin_memalign = createExportWrapper('emscripten_builtin_memalign', wasmExports['emscripten_builtin_memalign'], 2);
  _setThrew = createExportWrapper('setThrew', wasmExports['setThrew'], 2);
  __emscripten_tempret_set = createExportWrapper('_emscripten_tempret_set', wasmExports['_emscripten_tempret_set'], 1);
  _emscripten_stack_init = wasmExports['emscripten_stack_init'];
  _emscripten_stack_get_free = wasmExports['emscripten_stack_get_free'];
  __emscripten_stack_restore = wasmExports['_emscripten_stack_restore'];
  __emscripten_stack_alloc = wasmExports['_emscripten_stack_alloc'];
  _emscripten_stack_get_current = wasmExports['emscripten_stack_get_current'];
  ___cxa_decrement_exception_refcount = createExportWrapper('__cxa_decrement_exception_refcount', wasmExports['__cxa_decrement_exception_refcount'], 1);
  ___cxa_increment_exception_refcount = createExportWrapper('__cxa_increment_exception_refcount', wasmExports['__cxa_increment_exception_refcount'], 1);
  ___get_exception_message = createExportWrapper('__get_exception_message', wasmExports['__get_exception_message'], 3);
  ___cxa_can_catch = createExportWrapper('__cxa_can_catch', wasmExports['__cxa_can_catch'], 3);
  ___cxa_get_exception_ptr = createExportWrapper('__cxa_get_exception_ptr', wasmExports['__cxa_get_exception_ptr'], 1);
  dynCall_iii = dynCalls['iii'] = createExportWrapper('dynCall_iii', wasmExports['dynCall_iii'], 3);
  dynCall_vi = dynCalls['vi'] = createExportWrapper('dynCall_vi', wasmExports['dynCall_vi'], 2);
  dynCall_vii = dynCalls['vii'] = createExportWrapper('dynCall_vii', wasmExports['dynCall_vii'], 3);
  dynCall_viiii = dynCalls['viiii'] = createExportWrapper('dynCall_viiii', wasmExports['dynCall_viiii'], 5);
  dynCall_viif = dynCalls['viif'] = createExportWrapper('dynCall_viif', wasmExports['dynCall_viif'], 4);
  dynCall_viii = dynCalls['viii'] = createExportWrapper('dynCall_viii', wasmExports['dynCall_viii'], 4);
  dynCall_ii = dynCalls['ii'] = createExportWrapper('dynCall_ii', wasmExports['dynCall_ii'], 2);
  dynCall_iiii = dynCalls['iiii'] = createExportWrapper('dynCall_iiii', wasmExports['dynCall_iiii'], 4);
  dynCall_iiiiiii = dynCalls['iiiiiii'] = createExportWrapper('dynCall_iiiiiii', wasmExports['dynCall_iiiiiii'], 7);
  dynCall_v = dynCalls['v'] = createExportWrapper('dynCall_v', wasmExports['dynCall_v'], 1);
  dynCall_i = dynCalls['i'] = createExportWrapper('dynCall_i', wasmExports['dynCall_i'], 1);
  dynCall_viiiii = dynCalls['viiiii'] = createExportWrapper('dynCall_viiiii', wasmExports['dynCall_viiiii'], 6);
  dynCall_vf = dynCalls['vf'] = createExportWrapper('dynCall_vf', wasmExports['dynCall_vf'], 2);
  dynCall_f = dynCalls['f'] = createExportWrapper('dynCall_f', wasmExports['dynCall_f'], 1);
  dynCall_vffi = dynCalls['vffi'] = createExportWrapper('dynCall_vffi', wasmExports['dynCall_vffi'], 4);
  dynCall_iiiii = dynCalls['iiiii'] = createExportWrapper('dynCall_iiiii', wasmExports['dynCall_iiiii'], 5);
  dynCall_iiiiiiiii = dynCalls['iiiiiiiii'] = createExportWrapper('dynCall_iiiiiiiii', wasmExports['dynCall_iiiiiiiii'], 9);
  dynCall_viiiiiii = dynCalls['viiiiiii'] = createExportWrapper('dynCall_viiiiiii', wasmExports['dynCall_viiiiiii'], 8);
  dynCall_iiiiii = dynCalls['iiiiii'] = createExportWrapper('dynCall_iiiiii', wasmExports['dynCall_iiiiii'], 6);
  dynCall_iiiiiiiiii = dynCalls['iiiiiiiiii'] = createExportWrapper('dynCall_iiiiiiiiii', wasmExports['dynCall_iiiiiiiiii'], 10);
  dynCall_viiiiii = dynCalls['viiiiii'] = createExportWrapper('dynCall_viiiiii', wasmExports['dynCall_viiiiii'], 7);
  dynCall_vif = dynCalls['vif'] = createExportWrapper('dynCall_vif', wasmExports['dynCall_vif'], 3);
  dynCall_fi = dynCalls['fi'] = createExportWrapper('dynCall_fi', wasmExports['dynCall_fi'], 2);
  dynCall_viffi = dynCalls['viffi'] = createExportWrapper('dynCall_viffi', wasmExports['dynCall_viffi'], 5);
  dynCall_viff = dynCalls['viff'] = createExportWrapper('dynCall_viff', wasmExports['dynCall_viff'], 4);
  dynCall_iif = dynCalls['iif'] = createExportWrapper('dynCall_iif', wasmExports['dynCall_iif'], 3);
  dynCall_viijii = dynCalls['viijii'] = createExportWrapper('dynCall_viijii', wasmExports['dynCall_viijii'], 6);
  dynCall_ji = dynCalls['ji'] = createExportWrapper('dynCall_ji', wasmExports['dynCall_ji'], 2);
  dynCall_jiji = dynCalls['jiji'] = createExportWrapper('dynCall_jiji', wasmExports['dynCall_jiji'], 4);
  dynCall_viiiiiiiii = dynCalls['viiiiiiiii'] = createExportWrapper('dynCall_viiiiiiiii', wasmExports['dynCall_viiiiiiiii'], 10);
  dynCall_iiji = dynCalls['iiji'] = createExportWrapper('dynCall_iiji', wasmExports['dynCall_iiji'], 4);
  dynCall_iid = dynCalls['iid'] = createExportWrapper('dynCall_iid', wasmExports['dynCall_iid'], 3);
  dynCall_di = dynCalls['di'] = createExportWrapper('dynCall_di', wasmExports['dynCall_di'], 2);
  dynCall_viiiiiiii = dynCalls['viiiiiiii'] = createExportWrapper('dynCall_viiiiiiii', wasmExports['dynCall_viiiiiiii'], 9);
  dynCall_iiiiiiii = dynCalls['iiiiiiii'] = createExportWrapper('dynCall_iiiiiiii', wasmExports['dynCall_iiiiiiii'], 8);
  dynCall_vffff = dynCalls['vffff'] = createExportWrapper('dynCall_vffff', wasmExports['dynCall_vffff'], 5);
  dynCall_vff = dynCalls['vff'] = createExportWrapper('dynCall_vff', wasmExports['dynCall_vff'], 3);
  dynCall_vfi = dynCalls['vfi'] = createExportWrapper('dynCall_vfi', wasmExports['dynCall_vfi'], 3);
  dynCall_vifff = dynCalls['vifff'] = createExportWrapper('dynCall_vifff', wasmExports['dynCall_vifff'], 5);
  dynCall_viffff = dynCalls['viffff'] = createExportWrapper('dynCall_viffff', wasmExports['dynCall_viffff'], 6);
  dynCall_vfff = dynCalls['vfff'] = createExportWrapper('dynCall_vfff', wasmExports['dynCall_vfff'], 4);
  dynCall_viiiiiiiiii = dynCalls['viiiiiiiiii'] = createExportWrapper('dynCall_viiiiiiiiii', wasmExports['dynCall_viiiiiiiiii'], 11);
  dynCall_viiiiiiiiiii = dynCalls['viiiiiiiiiii'] = createExportWrapper('dynCall_viiiiiiiiiii', wasmExports['dynCall_viiiiiiiiiii'], 12);
  dynCall_viifi = dynCalls['viifi'] = createExportWrapper('dynCall_viifi', wasmExports['dynCall_viifi'], 5);
  dynCall_iiij = dynCalls['iiij'] = createExportWrapper('dynCall_iiij', wasmExports['dynCall_iiij'], 4);
  dynCall_viij = dynCalls['viij'] = createExportWrapper('dynCall_viij', wasmExports['dynCall_viij'], 4);
  dynCall_iidiiiii = dynCalls['iidiiiii'] = createExportWrapper('dynCall_iidiiiii', wasmExports['dynCall_iidiiiii'], 8);
  dynCall_viiifi = dynCalls['viiifi'] = createExportWrapper('dynCall_viiifi', wasmExports['dynCall_viiifi'], 6);
  dynCall_viiidi = dynCalls['viiidi'] = createExportWrapper('dynCall_viiidi', wasmExports['dynCall_viiidi'], 6);
  dynCall_iiiiiiiiiiiii = dynCalls['iiiiiiiiiiiii'] = createExportWrapper('dynCall_iiiiiiiiiiiii', wasmExports['dynCall_iiiiiiiiiiiii'], 13);
  dynCall_fiii = dynCalls['fiii'] = createExportWrapper('dynCall_fiii', wasmExports['dynCall_fiii'], 4);
  dynCall_diii = dynCalls['diii'] = createExportWrapper('dynCall_diii', wasmExports['dynCall_diii'], 4);
  dynCall_iiiiiiiiiiii = dynCalls['iiiiiiiiiiii'] = createExportWrapper('dynCall_iiiiiiiiiiii', wasmExports['dynCall_iiiiiiiiiiii'], 12);
  dynCall_viiiiiiiiiiiiiii = dynCalls['viiiiiiiiiiiiiii'] = createExportWrapper('dynCall_viiiiiiiiiiiiiii', wasmExports['dynCall_viiiiiiiiiiiiiii'], 16);
  dynCall_iiiiij = dynCalls['iiiiij'] = createExportWrapper('dynCall_iiiiij', wasmExports['dynCall_iiiiij'], 6);
  dynCall_iiiiid = dynCalls['iiiiid'] = createExportWrapper('dynCall_iiiiid', wasmExports['dynCall_iiiiid'], 6);
  dynCall_iiiiijj = dynCalls['iiiiijj'] = createExportWrapper('dynCall_iiiiijj', wasmExports['dynCall_iiiiijj'], 7);
  dynCall_iiiiiijj = dynCalls['iiiiiijj'] = createExportWrapper('dynCall_iiiiiijj', wasmExports['dynCall_iiiiiijj'], 8);
  _asyncify_start_unwind = createExportWrapper('asyncify_start_unwind', wasmExports['asyncify_start_unwind'], 1);
  _asyncify_stop_unwind = createExportWrapper('asyncify_stop_unwind', wasmExports['asyncify_stop_unwind'], 0);
  _asyncify_start_rewind = createExportWrapper('asyncify_start_rewind', wasmExports['asyncify_start_rewind'], 1);
  _asyncify_stop_rewind = createExportWrapper('asyncify_stop_rewind', wasmExports['asyncify_stop_rewind'], 0);
  memory = wasmMemory = wasmExports['memory'];
  __indirect_function_table = wasmTable = wasmExports['__indirect_function_table'];
}

var wasmImports = {
  /** @export */
  __assert_fail: ___assert_fail,
  /** @export */
  __cxa_begin_catch: ___cxa_begin_catch,
  /** @export */
  __cxa_end_catch: ___cxa_end_catch,
  /** @export */
  __cxa_find_matching_catch_2: ___cxa_find_matching_catch_2,
  /** @export */
  __cxa_find_matching_catch_3: ___cxa_find_matching_catch_3,
  /** @export */
  __cxa_rethrow: ___cxa_rethrow,
  /** @export */
  __cxa_throw: ___cxa_throw,
  /** @export */
  __cxa_uncaught_exceptions: ___cxa_uncaught_exceptions,
  /** @export */
  __resumeException: ___resumeException,
  /** @export */
  __syscall_fcntl64: ___syscall_fcntl64,
  /** @export */
  __syscall_fstat64: ___syscall_fstat64,
  /** @export */
  __syscall_ioctl: ___syscall_ioctl,
  /** @export */
  __syscall_lstat64: ___syscall_lstat64,
  /** @export */
  __syscall_newfstatat: ___syscall_newfstatat,
  /** @export */
  __syscall_openat: ___syscall_openat,
  /** @export */
  __syscall_stat64: ___syscall_stat64,
  /** @export */
  _abort_js: __abort_js,
  /** @export */
  _embind_register_bigint: __embind_register_bigint,
  /** @export */
  _embind_register_bool: __embind_register_bool,
  /** @export */
  _embind_register_class: __embind_register_class,
  /** @export */
  _embind_register_class_constructor: __embind_register_class_constructor,
  /** @export */
  _embind_register_class_function: __embind_register_class_function,
  /** @export */
  _embind_register_emval: __embind_register_emval,
  /** @export */
  _embind_register_float: __embind_register_float,
  /** @export */
  _embind_register_function: __embind_register_function,
  /** @export */
  _embind_register_integer: __embind_register_integer,
  /** @export */
  _embind_register_iterable: __embind_register_iterable,
  /** @export */
  _embind_register_memory_view: __embind_register_memory_view,
  /** @export */
  _embind_register_optional: __embind_register_optional,
  /** @export */
  _embind_register_std_string: __embind_register_std_string,
  /** @export */
  _embind_register_std_wstring: __embind_register_std_wstring,
  /** @export */
  _embind_register_void: __embind_register_void,
  /** @export */
  _emscripten_throw_longjmp: __emscripten_throw_longjmp,
  /** @export */
  _emval_create_invoker: __emval_create_invoker,
  /** @export */
  _emval_decref: __emval_decref,
  /** @export */
  _emval_invoke: __emval_invoke,
  /** @export */
  _emval_run_destructors: __emval_run_destructors,
  /** @export */
  _mmap_js: __mmap_js,
  /** @export */
  _munmap_js: __munmap_js,
  /** @export */
  _tzset_js: __tzset_js,
  /** @export */
  clock_time_get: _clock_time_get,
  /** @export */
  eglBindAPI: _eglBindAPI,
  /** @export */
  eglChooseConfig: _eglChooseConfig,
  /** @export */
  eglCreateContext: _eglCreateContext,
  /** @export */
  eglCreateWindowSurface: _eglCreateWindowSurface,
  /** @export */
  eglDestroyContext: _eglDestroyContext,
  /** @export */
  eglDestroySurface: _eglDestroySurface,
  /** @export */
  eglGetConfigAttrib: _eglGetConfigAttrib,
  /** @export */
  eglGetDisplay: _eglGetDisplay,
  /** @export */
  eglGetError: _eglGetError,
  /** @export */
  eglInitialize: _eglInitialize,
  /** @export */
  eglMakeCurrent: _eglMakeCurrent,
  /** @export */
  eglQueryString: _eglQueryString,
  /** @export */
  eglSwapBuffers: _eglSwapBuffers,
  /** @export */
  eglSwapInterval: _eglSwapInterval,
  /** @export */
  eglTerminate: _eglTerminate,
  /** @export */
  eglWaitGL: _eglWaitGL,
  /** @export */
  eglWaitNative: _eglWaitNative,
  /** @export */
  emscripten_asm_const_int: _emscripten_asm_const_int,
  /** @export */
  emscripten_asm_const_int_sync_on_main_thread: _emscripten_asm_const_int_sync_on_main_thread,
  /** @export */
  emscripten_asm_const_ptr_sync_on_main_thread: _emscripten_asm_const_ptr_sync_on_main_thread,
  /** @export */
  emscripten_async_call: _emscripten_async_call,
  /** @export */
  emscripten_date_now: _emscripten_date_now,
  /** @export */
  emscripten_err: _emscripten_err,
  /** @export */
  emscripten_exit_fullscreen: _emscripten_exit_fullscreen,
  /** @export */
  emscripten_exit_pointerlock: _emscripten_exit_pointerlock,
  /** @export */
  emscripten_get_canvas_element_size: _emscripten_get_canvas_element_size,
  /** @export */
  emscripten_get_device_pixel_ratio: _emscripten_get_device_pixel_ratio,
  /** @export */
  emscripten_get_element_css_size: _emscripten_get_element_css_size,
  /** @export */
  emscripten_get_gamepad_status: _emscripten_get_gamepad_status,
  /** @export */
  emscripten_get_heap_max: _emscripten_get_heap_max,
  /** @export */
  emscripten_get_now: _emscripten_get_now,
  /** @export */
  emscripten_get_num_gamepads: _emscripten_get_num_gamepads,
  /** @export */
  emscripten_get_preloaded_image_data: _emscripten_get_preloaded_image_data,
  /** @export */
  emscripten_get_preloaded_image_data_from_FILE: _emscripten_get_preloaded_image_data_from_FILE,
  /** @export */
  emscripten_get_screen_size: _emscripten_get_screen_size,
  /** @export */
  emscripten_glActiveTexture: _emscripten_glActiveTexture,
  /** @export */
  emscripten_glAttachShader: _emscripten_glAttachShader,
  /** @export */
  emscripten_glBeginQuery: _emscripten_glBeginQuery,
  /** @export */
  emscripten_glBeginQueryEXT: _emscripten_glBeginQueryEXT,
  /** @export */
  emscripten_glBeginTransformFeedback: _emscripten_glBeginTransformFeedback,
  /** @export */
  emscripten_glBindAttribLocation: _emscripten_glBindAttribLocation,
  /** @export */
  emscripten_glBindBuffer: _emscripten_glBindBuffer,
  /** @export */
  emscripten_glBindBufferBase: _emscripten_glBindBufferBase,
  /** @export */
  emscripten_glBindBufferRange: _emscripten_glBindBufferRange,
  /** @export */
  emscripten_glBindFramebuffer: _emscripten_glBindFramebuffer,
  /** @export */
  emscripten_glBindRenderbuffer: _emscripten_glBindRenderbuffer,
  /** @export */
  emscripten_glBindSampler: _emscripten_glBindSampler,
  /** @export */
  emscripten_glBindTexture: _emscripten_glBindTexture,
  /** @export */
  emscripten_glBindTransformFeedback: _emscripten_glBindTransformFeedback,
  /** @export */
  emscripten_glBindVertexArray: _emscripten_glBindVertexArray,
  /** @export */
  emscripten_glBindVertexArrayOES: _emscripten_glBindVertexArrayOES,
  /** @export */
  emscripten_glBlendColor: _emscripten_glBlendColor,
  /** @export */
  emscripten_glBlendEquation: _emscripten_glBlendEquation,
  /** @export */
  emscripten_glBlendEquationSeparate: _emscripten_glBlendEquationSeparate,
  /** @export */
  emscripten_glBlendFunc: _emscripten_glBlendFunc,
  /** @export */
  emscripten_glBlendFuncSeparate: _emscripten_glBlendFuncSeparate,
  /** @export */
  emscripten_glBlitFramebuffer: _emscripten_glBlitFramebuffer,
  /** @export */
  emscripten_glBufferData: _emscripten_glBufferData,
  /** @export */
  emscripten_glBufferSubData: _emscripten_glBufferSubData,
  /** @export */
  emscripten_glCheckFramebufferStatus: _emscripten_glCheckFramebufferStatus,
  /** @export */
  emscripten_glClear: _emscripten_glClear,
  /** @export */
  emscripten_glClearBufferfi: _emscripten_glClearBufferfi,
  /** @export */
  emscripten_glClearBufferfv: _emscripten_glClearBufferfv,
  /** @export */
  emscripten_glClearBufferiv: _emscripten_glClearBufferiv,
  /** @export */
  emscripten_glClearBufferuiv: _emscripten_glClearBufferuiv,
  /** @export */
  emscripten_glClearColor: _emscripten_glClearColor,
  /** @export */
  emscripten_glClearDepthf: _emscripten_glClearDepthf,
  /** @export */
  emscripten_glClearStencil: _emscripten_glClearStencil,
  /** @export */
  emscripten_glClientWaitSync: _emscripten_glClientWaitSync,
  /** @export */
  emscripten_glClipControlEXT: _emscripten_glClipControlEXT,
  /** @export */
  emscripten_glColorMask: _emscripten_glColorMask,
  /** @export */
  emscripten_glCompileShader: _emscripten_glCompileShader,
  /** @export */
  emscripten_glCompressedTexImage2D: _emscripten_glCompressedTexImage2D,
  /** @export */
  emscripten_glCompressedTexImage3D: _emscripten_glCompressedTexImage3D,
  /** @export */
  emscripten_glCompressedTexSubImage2D: _emscripten_glCompressedTexSubImage2D,
  /** @export */
  emscripten_glCompressedTexSubImage3D: _emscripten_glCompressedTexSubImage3D,
  /** @export */
  emscripten_glCopyBufferSubData: _emscripten_glCopyBufferSubData,
  /** @export */
  emscripten_glCopyTexImage2D: _emscripten_glCopyTexImage2D,
  /** @export */
  emscripten_glCopyTexSubImage2D: _emscripten_glCopyTexSubImage2D,
  /** @export */
  emscripten_glCopyTexSubImage3D: _emscripten_glCopyTexSubImage3D,
  /** @export */
  emscripten_glCreateProgram: _emscripten_glCreateProgram,
  /** @export */
  emscripten_glCreateShader: _emscripten_glCreateShader,
  /** @export */
  emscripten_glCullFace: _emscripten_glCullFace,
  /** @export */
  emscripten_glDeleteBuffers: _emscripten_glDeleteBuffers,
  /** @export */
  emscripten_glDeleteFramebuffers: _emscripten_glDeleteFramebuffers,
  /** @export */
  emscripten_glDeleteProgram: _emscripten_glDeleteProgram,
  /** @export */
  emscripten_glDeleteQueries: _emscripten_glDeleteQueries,
  /** @export */
  emscripten_glDeleteQueriesEXT: _emscripten_glDeleteQueriesEXT,
  /** @export */
  emscripten_glDeleteRenderbuffers: _emscripten_glDeleteRenderbuffers,
  /** @export */
  emscripten_glDeleteSamplers: _emscripten_glDeleteSamplers,
  /** @export */
  emscripten_glDeleteShader: _emscripten_glDeleteShader,
  /** @export */
  emscripten_glDeleteSync: _emscripten_glDeleteSync,
  /** @export */
  emscripten_glDeleteTextures: _emscripten_glDeleteTextures,
  /** @export */
  emscripten_glDeleteTransformFeedbacks: _emscripten_glDeleteTransformFeedbacks,
  /** @export */
  emscripten_glDeleteVertexArrays: _emscripten_glDeleteVertexArrays,
  /** @export */
  emscripten_glDeleteVertexArraysOES: _emscripten_glDeleteVertexArraysOES,
  /** @export */
  emscripten_glDepthFunc: _emscripten_glDepthFunc,
  /** @export */
  emscripten_glDepthMask: _emscripten_glDepthMask,
  /** @export */
  emscripten_glDepthRangef: _emscripten_glDepthRangef,
  /** @export */
  emscripten_glDetachShader: _emscripten_glDetachShader,
  /** @export */
  emscripten_glDisable: _emscripten_glDisable,
  /** @export */
  emscripten_glDisableVertexAttribArray: _emscripten_glDisableVertexAttribArray,
  /** @export */
  emscripten_glDrawArrays: _emscripten_glDrawArrays,
  /** @export */
  emscripten_glDrawArraysInstanced: _emscripten_glDrawArraysInstanced,
  /** @export */
  emscripten_glDrawArraysInstancedANGLE: _emscripten_glDrawArraysInstancedANGLE,
  /** @export */
  emscripten_glDrawArraysInstancedARB: _emscripten_glDrawArraysInstancedARB,
  /** @export */
  emscripten_glDrawArraysInstancedEXT: _emscripten_glDrawArraysInstancedEXT,
  /** @export */
  emscripten_glDrawArraysInstancedNV: _emscripten_glDrawArraysInstancedNV,
  /** @export */
  emscripten_glDrawBuffers: _emscripten_glDrawBuffers,
  /** @export */
  emscripten_glDrawBuffersEXT: _emscripten_glDrawBuffersEXT,
  /** @export */
  emscripten_glDrawBuffersWEBGL: _emscripten_glDrawBuffersWEBGL,
  /** @export */
  emscripten_glDrawElements: _emscripten_glDrawElements,
  /** @export */
  emscripten_glDrawElementsInstanced: _emscripten_glDrawElementsInstanced,
  /** @export */
  emscripten_glDrawElementsInstancedANGLE: _emscripten_glDrawElementsInstancedANGLE,
  /** @export */
  emscripten_glDrawElementsInstancedARB: _emscripten_glDrawElementsInstancedARB,
  /** @export */
  emscripten_glDrawElementsInstancedEXT: _emscripten_glDrawElementsInstancedEXT,
  /** @export */
  emscripten_glDrawElementsInstancedNV: _emscripten_glDrawElementsInstancedNV,
  /** @export */
  emscripten_glDrawRangeElements: _emscripten_glDrawRangeElements,
  /** @export */
  emscripten_glEnable: _emscripten_glEnable,
  /** @export */
  emscripten_glEnableVertexAttribArray: _emscripten_glEnableVertexAttribArray,
  /** @export */
  emscripten_glEndQuery: _emscripten_glEndQuery,
  /** @export */
  emscripten_glEndQueryEXT: _emscripten_glEndQueryEXT,
  /** @export */
  emscripten_glEndTransformFeedback: _emscripten_glEndTransformFeedback,
  /** @export */
  emscripten_glFenceSync: _emscripten_glFenceSync,
  /** @export */
  emscripten_glFinish: _emscripten_glFinish,
  /** @export */
  emscripten_glFlush: _emscripten_glFlush,
  /** @export */
  emscripten_glFlushMappedBufferRange: _emscripten_glFlushMappedBufferRange,
  /** @export */
  emscripten_glFramebufferRenderbuffer: _emscripten_glFramebufferRenderbuffer,
  /** @export */
  emscripten_glFramebufferTexture2D: _emscripten_glFramebufferTexture2D,
  /** @export */
  emscripten_glFramebufferTextureLayer: _emscripten_glFramebufferTextureLayer,
  /** @export */
  emscripten_glFrontFace: _emscripten_glFrontFace,
  /** @export */
  emscripten_glGenBuffers: _emscripten_glGenBuffers,
  /** @export */
  emscripten_glGenFramebuffers: _emscripten_glGenFramebuffers,
  /** @export */
  emscripten_glGenQueries: _emscripten_glGenQueries,
  /** @export */
  emscripten_glGenQueriesEXT: _emscripten_glGenQueriesEXT,
  /** @export */
  emscripten_glGenRenderbuffers: _emscripten_glGenRenderbuffers,
  /** @export */
  emscripten_glGenSamplers: _emscripten_glGenSamplers,
  /** @export */
  emscripten_glGenTextures: _emscripten_glGenTextures,
  /** @export */
  emscripten_glGenTransformFeedbacks: _emscripten_glGenTransformFeedbacks,
  /** @export */
  emscripten_glGenVertexArrays: _emscripten_glGenVertexArrays,
  /** @export */
  emscripten_glGenVertexArraysOES: _emscripten_glGenVertexArraysOES,
  /** @export */
  emscripten_glGenerateMipmap: _emscripten_glGenerateMipmap,
  /** @export */
  emscripten_glGetActiveAttrib: _emscripten_glGetActiveAttrib,
  /** @export */
  emscripten_glGetActiveUniform: _emscripten_glGetActiveUniform,
  /** @export */
  emscripten_glGetActiveUniformBlockName: _emscripten_glGetActiveUniformBlockName,
  /** @export */
  emscripten_glGetActiveUniformBlockiv: _emscripten_glGetActiveUniformBlockiv,
  /** @export */
  emscripten_glGetActiveUniformsiv: _emscripten_glGetActiveUniformsiv,
  /** @export */
  emscripten_glGetAttachedShaders: _emscripten_glGetAttachedShaders,
  /** @export */
  emscripten_glGetAttribLocation: _emscripten_glGetAttribLocation,
  /** @export */
  emscripten_glGetBooleanv: _emscripten_glGetBooleanv,
  /** @export */
  emscripten_glGetBufferParameteri64v: _emscripten_glGetBufferParameteri64v,
  /** @export */
  emscripten_glGetBufferParameteriv: _emscripten_glGetBufferParameteriv,
  /** @export */
  emscripten_glGetBufferPointerv: _emscripten_glGetBufferPointerv,
  /** @export */
  emscripten_glGetError: _emscripten_glGetError,
  /** @export */
  emscripten_glGetFloatv: _emscripten_glGetFloatv,
  /** @export */
  emscripten_glGetFragDataLocation: _emscripten_glGetFragDataLocation,
  /** @export */
  emscripten_glGetFramebufferAttachmentParameteriv: _emscripten_glGetFramebufferAttachmentParameteriv,
  /** @export */
  emscripten_glGetInteger64i_v: _emscripten_glGetInteger64i_v,
  /** @export */
  emscripten_glGetInteger64v: _emscripten_glGetInteger64v,
  /** @export */
  emscripten_glGetIntegeri_v: _emscripten_glGetIntegeri_v,
  /** @export */
  emscripten_glGetIntegerv: _emscripten_glGetIntegerv,
  /** @export */
  emscripten_glGetInternalformativ: _emscripten_glGetInternalformativ,
  /** @export */
  emscripten_glGetProgramBinary: _emscripten_glGetProgramBinary,
  /** @export */
  emscripten_glGetProgramInfoLog: _emscripten_glGetProgramInfoLog,
  /** @export */
  emscripten_glGetProgramiv: _emscripten_glGetProgramiv,
  /** @export */
  emscripten_glGetQueryObjecti64vEXT: _emscripten_glGetQueryObjecti64vEXT,
  /** @export */
  emscripten_glGetQueryObjectivEXT: _emscripten_glGetQueryObjectivEXT,
  /** @export */
  emscripten_glGetQueryObjectui64vEXT: _emscripten_glGetQueryObjectui64vEXT,
  /** @export */
  emscripten_glGetQueryObjectuiv: _emscripten_glGetQueryObjectuiv,
  /** @export */
  emscripten_glGetQueryObjectuivEXT: _emscripten_glGetQueryObjectuivEXT,
  /** @export */
  emscripten_glGetQueryiv: _emscripten_glGetQueryiv,
  /** @export */
  emscripten_glGetQueryivEXT: _emscripten_glGetQueryivEXT,
  /** @export */
  emscripten_glGetRenderbufferParameteriv: _emscripten_glGetRenderbufferParameteriv,
  /** @export */
  emscripten_glGetSamplerParameterfv: _emscripten_glGetSamplerParameterfv,
  /** @export */
  emscripten_glGetSamplerParameteriv: _emscripten_glGetSamplerParameteriv,
  /** @export */
  emscripten_glGetShaderInfoLog: _emscripten_glGetShaderInfoLog,
  /** @export */
  emscripten_glGetShaderPrecisionFormat: _emscripten_glGetShaderPrecisionFormat,
  /** @export */
  emscripten_glGetShaderSource: _emscripten_glGetShaderSource,
  /** @export */
  emscripten_glGetShaderiv: _emscripten_glGetShaderiv,
  /** @export */
  emscripten_glGetString: _emscripten_glGetString,
  /** @export */
  emscripten_glGetStringi: _emscripten_glGetStringi,
  /** @export */
  emscripten_glGetSynciv: _emscripten_glGetSynciv,
  /** @export */
  emscripten_glGetTexParameterfv: _emscripten_glGetTexParameterfv,
  /** @export */
  emscripten_glGetTexParameteriv: _emscripten_glGetTexParameteriv,
  /** @export */
  emscripten_glGetTransformFeedbackVarying: _emscripten_glGetTransformFeedbackVarying,
  /** @export */
  emscripten_glGetUniformBlockIndex: _emscripten_glGetUniformBlockIndex,
  /** @export */
  emscripten_glGetUniformIndices: _emscripten_glGetUniformIndices,
  /** @export */
  emscripten_glGetUniformLocation: _emscripten_glGetUniformLocation,
  /** @export */
  emscripten_glGetUniformfv: _emscripten_glGetUniformfv,
  /** @export */
  emscripten_glGetUniformiv: _emscripten_glGetUniformiv,
  /** @export */
  emscripten_glGetUniformuiv: _emscripten_glGetUniformuiv,
  /** @export */
  emscripten_glGetVertexAttribIiv: _emscripten_glGetVertexAttribIiv,
  /** @export */
  emscripten_glGetVertexAttribIuiv: _emscripten_glGetVertexAttribIuiv,
  /** @export */
  emscripten_glGetVertexAttribPointerv: _emscripten_glGetVertexAttribPointerv,
  /** @export */
  emscripten_glGetVertexAttribfv: _emscripten_glGetVertexAttribfv,
  /** @export */
  emscripten_glGetVertexAttribiv: _emscripten_glGetVertexAttribiv,
  /** @export */
  emscripten_glHint: _emscripten_glHint,
  /** @export */
  emscripten_glInvalidateFramebuffer: _emscripten_glInvalidateFramebuffer,
  /** @export */
  emscripten_glInvalidateSubFramebuffer: _emscripten_glInvalidateSubFramebuffer,
  /** @export */
  emscripten_glIsBuffer: _emscripten_glIsBuffer,
  /** @export */
  emscripten_glIsEnabled: _emscripten_glIsEnabled,
  /** @export */
  emscripten_glIsFramebuffer: _emscripten_glIsFramebuffer,
  /** @export */
  emscripten_glIsProgram: _emscripten_glIsProgram,
  /** @export */
  emscripten_glIsQuery: _emscripten_glIsQuery,
  /** @export */
  emscripten_glIsQueryEXT: _emscripten_glIsQueryEXT,
  /** @export */
  emscripten_glIsRenderbuffer: _emscripten_glIsRenderbuffer,
  /** @export */
  emscripten_glIsSampler: _emscripten_glIsSampler,
  /** @export */
  emscripten_glIsShader: _emscripten_glIsShader,
  /** @export */
  emscripten_glIsSync: _emscripten_glIsSync,
  /** @export */
  emscripten_glIsTexture: _emscripten_glIsTexture,
  /** @export */
  emscripten_glIsTransformFeedback: _emscripten_glIsTransformFeedback,
  /** @export */
  emscripten_glIsVertexArray: _emscripten_glIsVertexArray,
  /** @export */
  emscripten_glIsVertexArrayOES: _emscripten_glIsVertexArrayOES,
  /** @export */
  emscripten_glLineWidth: _emscripten_glLineWidth,
  /** @export */
  emscripten_glLinkProgram: _emscripten_glLinkProgram,
  /** @export */
  emscripten_glMapBufferRange: _emscripten_glMapBufferRange,
  /** @export */
  emscripten_glPauseTransformFeedback: _emscripten_glPauseTransformFeedback,
  /** @export */
  emscripten_glPixelStorei: _emscripten_glPixelStorei,
  /** @export */
  emscripten_glPolygonModeWEBGL: _emscripten_glPolygonModeWEBGL,
  /** @export */
  emscripten_glPolygonOffset: _emscripten_glPolygonOffset,
  /** @export */
  emscripten_glPolygonOffsetClampEXT: _emscripten_glPolygonOffsetClampEXT,
  /** @export */
  emscripten_glProgramBinary: _emscripten_glProgramBinary,
  /** @export */
  emscripten_glProgramParameteri: _emscripten_glProgramParameteri,
  /** @export */
  emscripten_glQueryCounterEXT: _emscripten_glQueryCounterEXT,
  /** @export */
  emscripten_glReadBuffer: _emscripten_glReadBuffer,
  /** @export */
  emscripten_glReadPixels: _emscripten_glReadPixels,
  /** @export */
  emscripten_glReleaseShaderCompiler: _emscripten_glReleaseShaderCompiler,
  /** @export */
  emscripten_glRenderbufferStorage: _emscripten_glRenderbufferStorage,
  /** @export */
  emscripten_glRenderbufferStorageMultisample: _emscripten_glRenderbufferStorageMultisample,
  /** @export */
  emscripten_glResumeTransformFeedback: _emscripten_glResumeTransformFeedback,
  /** @export */
  emscripten_glSampleCoverage: _emscripten_glSampleCoverage,
  /** @export */
  emscripten_glSamplerParameterf: _emscripten_glSamplerParameterf,
  /** @export */
  emscripten_glSamplerParameterfv: _emscripten_glSamplerParameterfv,
  /** @export */
  emscripten_glSamplerParameteri: _emscripten_glSamplerParameteri,
  /** @export */
  emscripten_glSamplerParameteriv: _emscripten_glSamplerParameteriv,
  /** @export */
  emscripten_glScissor: _emscripten_glScissor,
  /** @export */
  emscripten_glShaderBinary: _emscripten_glShaderBinary,
  /** @export */
  emscripten_glShaderSource: _emscripten_glShaderSource,
  /** @export */
  emscripten_glStencilFunc: _emscripten_glStencilFunc,
  /** @export */
  emscripten_glStencilFuncSeparate: _emscripten_glStencilFuncSeparate,
  /** @export */
  emscripten_glStencilMask: _emscripten_glStencilMask,
  /** @export */
  emscripten_glStencilMaskSeparate: _emscripten_glStencilMaskSeparate,
  /** @export */
  emscripten_glStencilOp: _emscripten_glStencilOp,
  /** @export */
  emscripten_glStencilOpSeparate: _emscripten_glStencilOpSeparate,
  /** @export */
  emscripten_glTexImage2D: _emscripten_glTexImage2D,
  /** @export */
  emscripten_glTexImage3D: _emscripten_glTexImage3D,
  /** @export */
  emscripten_glTexParameterf: _emscripten_glTexParameterf,
  /** @export */
  emscripten_glTexParameterfv: _emscripten_glTexParameterfv,
  /** @export */
  emscripten_glTexParameteri: _emscripten_glTexParameteri,
  /** @export */
  emscripten_glTexParameteriv: _emscripten_glTexParameteriv,
  /** @export */
  emscripten_glTexStorage2D: _emscripten_glTexStorage2D,
  /** @export */
  emscripten_glTexStorage3D: _emscripten_glTexStorage3D,
  /** @export */
  emscripten_glTexSubImage2D: _emscripten_glTexSubImage2D,
  /** @export */
  emscripten_glTexSubImage3D: _emscripten_glTexSubImage3D,
  /** @export */
  emscripten_glTransformFeedbackVaryings: _emscripten_glTransformFeedbackVaryings,
  /** @export */
  emscripten_glUniform1f: _emscripten_glUniform1f,
  /** @export */
  emscripten_glUniform1fv: _emscripten_glUniform1fv,
  /** @export */
  emscripten_glUniform1i: _emscripten_glUniform1i,
  /** @export */
  emscripten_glUniform1iv: _emscripten_glUniform1iv,
  /** @export */
  emscripten_glUniform1ui: _emscripten_glUniform1ui,
  /** @export */
  emscripten_glUniform1uiv: _emscripten_glUniform1uiv,
  /** @export */
  emscripten_glUniform2f: _emscripten_glUniform2f,
  /** @export */
  emscripten_glUniform2fv: _emscripten_glUniform2fv,
  /** @export */
  emscripten_glUniform2i: _emscripten_glUniform2i,
  /** @export */
  emscripten_glUniform2iv: _emscripten_glUniform2iv,
  /** @export */
  emscripten_glUniform2ui: _emscripten_glUniform2ui,
  /** @export */
  emscripten_glUniform2uiv: _emscripten_glUniform2uiv,
  /** @export */
  emscripten_glUniform3f: _emscripten_glUniform3f,
  /** @export */
  emscripten_glUniform3fv: _emscripten_glUniform3fv,
  /** @export */
  emscripten_glUniform3i: _emscripten_glUniform3i,
  /** @export */
  emscripten_glUniform3iv: _emscripten_glUniform3iv,
  /** @export */
  emscripten_glUniform3ui: _emscripten_glUniform3ui,
  /** @export */
  emscripten_glUniform3uiv: _emscripten_glUniform3uiv,
  /** @export */
  emscripten_glUniform4f: _emscripten_glUniform4f,
  /** @export */
  emscripten_glUniform4fv: _emscripten_glUniform4fv,
  /** @export */
  emscripten_glUniform4i: _emscripten_glUniform4i,
  /** @export */
  emscripten_glUniform4iv: _emscripten_glUniform4iv,
  /** @export */
  emscripten_glUniform4ui: _emscripten_glUniform4ui,
  /** @export */
  emscripten_glUniform4uiv: _emscripten_glUniform4uiv,
  /** @export */
  emscripten_glUniformBlockBinding: _emscripten_glUniformBlockBinding,
  /** @export */
  emscripten_glUniformMatrix2fv: _emscripten_glUniformMatrix2fv,
  /** @export */
  emscripten_glUniformMatrix2x3fv: _emscripten_glUniformMatrix2x3fv,
  /** @export */
  emscripten_glUniformMatrix2x4fv: _emscripten_glUniformMatrix2x4fv,
  /** @export */
  emscripten_glUniformMatrix3fv: _emscripten_glUniformMatrix3fv,
  /** @export */
  emscripten_glUniformMatrix3x2fv: _emscripten_glUniformMatrix3x2fv,
  /** @export */
  emscripten_glUniformMatrix3x4fv: _emscripten_glUniformMatrix3x4fv,
  /** @export */
  emscripten_glUniformMatrix4fv: _emscripten_glUniformMatrix4fv,
  /** @export */
  emscripten_glUniformMatrix4x2fv: _emscripten_glUniformMatrix4x2fv,
  /** @export */
  emscripten_glUniformMatrix4x3fv: _emscripten_glUniformMatrix4x3fv,
  /** @export */
  emscripten_glUnmapBuffer: _emscripten_glUnmapBuffer,
  /** @export */
  emscripten_glUseProgram: _emscripten_glUseProgram,
  /** @export */
  emscripten_glValidateProgram: _emscripten_glValidateProgram,
  /** @export */
  emscripten_glVertexAttrib1f: _emscripten_glVertexAttrib1f,
  /** @export */
  emscripten_glVertexAttrib1fv: _emscripten_glVertexAttrib1fv,
  /** @export */
  emscripten_glVertexAttrib2f: _emscripten_glVertexAttrib2f,
  /** @export */
  emscripten_glVertexAttrib2fv: _emscripten_glVertexAttrib2fv,
  /** @export */
  emscripten_glVertexAttrib3f: _emscripten_glVertexAttrib3f,
  /** @export */
  emscripten_glVertexAttrib3fv: _emscripten_glVertexAttrib3fv,
  /** @export */
  emscripten_glVertexAttrib4f: _emscripten_glVertexAttrib4f,
  /** @export */
  emscripten_glVertexAttrib4fv: _emscripten_glVertexAttrib4fv,
  /** @export */
  emscripten_glVertexAttribDivisor: _emscripten_glVertexAttribDivisor,
  /** @export */
  emscripten_glVertexAttribDivisorANGLE: _emscripten_glVertexAttribDivisorANGLE,
  /** @export */
  emscripten_glVertexAttribDivisorARB: _emscripten_glVertexAttribDivisorARB,
  /** @export */
  emscripten_glVertexAttribDivisorEXT: _emscripten_glVertexAttribDivisorEXT,
  /** @export */
  emscripten_glVertexAttribDivisorNV: _emscripten_glVertexAttribDivisorNV,
  /** @export */
  emscripten_glVertexAttribI4i: _emscripten_glVertexAttribI4i,
  /** @export */
  emscripten_glVertexAttribI4iv: _emscripten_glVertexAttribI4iv,
  /** @export */
  emscripten_glVertexAttribI4ui: _emscripten_glVertexAttribI4ui,
  /** @export */
  emscripten_glVertexAttribI4uiv: _emscripten_glVertexAttribI4uiv,
  /** @export */
  emscripten_glVertexAttribIPointer: _emscripten_glVertexAttribIPointer,
  /** @export */
  emscripten_glVertexAttribPointer: _emscripten_glVertexAttribPointer,
  /** @export */
  emscripten_glViewport: _emscripten_glViewport,
  /** @export */
  emscripten_glWaitSync: _emscripten_glWaitSync,
  /** @export */
  emscripten_has_asyncify: _emscripten_has_asyncify,
  /** @export */
  emscripten_request_fullscreen_strategy: _emscripten_request_fullscreen_strategy,
  /** @export */
  emscripten_request_pointerlock: _emscripten_request_pointerlock,
  /** @export */
  emscripten_resize_heap: _emscripten_resize_heap,
  /** @export */
  emscripten_sample_gamepad_data: _emscripten_sample_gamepad_data,
  /** @export */
  emscripten_set_beforeunload_callback_on_thread: _emscripten_set_beforeunload_callback_on_thread,
  /** @export */
  emscripten_set_blur_callback_on_thread: _emscripten_set_blur_callback_on_thread,
  /** @export */
  emscripten_set_canvas_element_size: _emscripten_set_canvas_element_size,
  /** @export */
  emscripten_set_element_css_size: _emscripten_set_element_css_size,
  /** @export */
  emscripten_set_focus_callback_on_thread: _emscripten_set_focus_callback_on_thread,
  /** @export */
  emscripten_set_fullscreenchange_callback_on_thread: _emscripten_set_fullscreenchange_callback_on_thread,
  /** @export */
  emscripten_set_gamepadconnected_callback_on_thread: _emscripten_set_gamepadconnected_callback_on_thread,
  /** @export */
  emscripten_set_gamepaddisconnected_callback_on_thread: _emscripten_set_gamepaddisconnected_callback_on_thread,
  /** @export */
  emscripten_set_keydown_callback_on_thread: _emscripten_set_keydown_callback_on_thread,
  /** @export */
  emscripten_set_keypress_callback_on_thread: _emscripten_set_keypress_callback_on_thread,
  /** @export */
  emscripten_set_keyup_callback_on_thread: _emscripten_set_keyup_callback_on_thread,
  /** @export */
  emscripten_set_main_loop: _emscripten_set_main_loop,
  /** @export */
  emscripten_set_mousedown_callback_on_thread: _emscripten_set_mousedown_callback_on_thread,
  /** @export */
  emscripten_set_mouseenter_callback_on_thread: _emscripten_set_mouseenter_callback_on_thread,
  /** @export */
  emscripten_set_mouseleave_callback_on_thread: _emscripten_set_mouseleave_callback_on_thread,
  /** @export */
  emscripten_set_mousemove_callback_on_thread: _emscripten_set_mousemove_callback_on_thread,
  /** @export */
  emscripten_set_mouseup_callback_on_thread: _emscripten_set_mouseup_callback_on_thread,
  /** @export */
  emscripten_set_pointerlockchange_callback_on_thread: _emscripten_set_pointerlockchange_callback_on_thread,
  /** @export */
  emscripten_set_resize_callback_on_thread: _emscripten_set_resize_callback_on_thread,
  /** @export */
  emscripten_set_touchcancel_callback_on_thread: _emscripten_set_touchcancel_callback_on_thread,
  /** @export */
  emscripten_set_touchend_callback_on_thread: _emscripten_set_touchend_callback_on_thread,
  /** @export */
  emscripten_set_touchmove_callback_on_thread: _emscripten_set_touchmove_callback_on_thread,
  /** @export */
  emscripten_set_touchstart_callback_on_thread: _emscripten_set_touchstart_callback_on_thread,
  /** @export */
  emscripten_set_visibilitychange_callback_on_thread: _emscripten_set_visibilitychange_callback_on_thread,
  /** @export */
  emscripten_set_wheel_callback_on_thread: _emscripten_set_wheel_callback_on_thread,
  /** @export */
  emscripten_set_window_title: _emscripten_set_window_title,
  /** @export */
  emscripten_sleep: _emscripten_sleep,
  /** @export */
  emscripten_webgl_create_context: _emscripten_webgl_create_context,
  /** @export */
  emscripten_webgl_enable_extension: _emscripten_webgl_enable_extension,
  /** @export */
  emscripten_webgl_get_current_context: _emscripten_webgl_get_current_context,
  /** @export */
  emscripten_webgl_make_context_current: _emscripten_webgl_make_context_current,
  /** @export */
  environ_get: _environ_get,
  /** @export */
  environ_sizes_get: _environ_sizes_get,
  /** @export */
  exit: _exit,
  /** @export */
  fd_close: _fd_close,
  /** @export */
  fd_read: _fd_read,
  /** @export */
  fd_seek: _fd_seek,
  /** @export */
  fd_write: _fd_write,
  /** @export */
  glActiveTexture: _glActiveTexture,
  /** @export */
  glAttachShader: _glAttachShader,
  /** @export */
  glBindBuffer: _glBindBuffer,
  /** @export */
  glBindFramebuffer: _glBindFramebuffer,
  /** @export */
  glBindTexture: _glBindTexture,
  /** @export */
  glBindVertexArray: _glBindVertexArray,
  /** @export */
  glBlendFunc: _glBlendFunc,
  /** @export */
  glBufferData: _glBufferData,
  /** @export */
  glBufferSubData: _glBufferSubData,
  /** @export */
  glCheckFramebufferStatus: _glCheckFramebufferStatus,
  /** @export */
  glClear: _glClear,
  /** @export */
  glClearColor: _glClearColor,
  /** @export */
  glCompileShader: _glCompileShader,
  /** @export */
  glCreateProgram: _glCreateProgram,
  /** @export */
  glCreateShader: _glCreateShader,
  /** @export */
  glDeleteBuffers: _glDeleteBuffers,
  /** @export */
  glDeleteFramebuffers: _glDeleteFramebuffers,
  /** @export */
  glDeleteProgram: _glDeleteProgram,
  /** @export */
  glDeleteShader: _glDeleteShader,
  /** @export */
  glDeleteTextures: _glDeleteTextures,
  /** @export */
  glDeleteVertexArrays: _glDeleteVertexArrays,
  /** @export */
  glDepthFunc: _glDepthFunc,
  /** @export */
  glDepthMask: _glDepthMask,
  /** @export */
  glDetachShader: _glDetachShader,
  /** @export */
  glDisable: _glDisable,
  /** @export */
  glDrawArrays: _glDrawArrays,
  /** @export */
  glDrawElements: _glDrawElements,
  /** @export */
  glEnable: _glEnable,
  /** @export */
  glEnableVertexAttribArray: _glEnableVertexAttribArray,
  /** @export */
  glFramebufferTexture2D: _glFramebufferTexture2D,
  /** @export */
  glFrontFace: _glFrontFace,
  /** @export */
  glGenBuffers: _glGenBuffers,
  /** @export */
  glGenFramebuffers: _glGenFramebuffers,
  /** @export */
  glGenTextures: _glGenTextures,
  /** @export */
  glGenVertexArrays: _glGenVertexArrays,
  /** @export */
  glGetError: _glGetError,
  /** @export */
  glGetProgramInfoLog: _glGetProgramInfoLog,
  /** @export */
  glGetProgramiv: _glGetProgramiv,
  /** @export */
  glGetShaderInfoLog: _glGetShaderInfoLog,
  /** @export */
  glGetShaderiv: _glGetShaderiv,
  /** @export */
  glGetUniformLocation: _glGetUniformLocation,
  /** @export */
  glIsBuffer: _glIsBuffer,
  /** @export */
  glIsProgram: _glIsProgram,
  /** @export */
  glIsShader: _glIsShader,
  /** @export */
  glIsTexture: _glIsTexture,
  /** @export */
  glIsVertexArray: _glIsVertexArray,
  /** @export */
  glLinkProgram: _glLinkProgram,
  /** @export */
  glReadPixels: _glReadPixels,
  /** @export */
  glShaderSource: _glShaderSource,
  /** @export */
  glTexImage2D: _glTexImage2D,
  /** @export */
  glTexParameteri: _glTexParameteri,
  /** @export */
  glTexSubImage2D: _glTexSubImage2D,
  /** @export */
  glUniform1f: _glUniform1f,
  /** @export */
  glUniform1i: _glUniform1i,
  /** @export */
  glUniform2fv: _glUniform2fv,
  /** @export */
  glUniform3fv: _glUniform3fv,
  /** @export */
  glUniform4fv: _glUniform4fv,
  /** @export */
  glUniformMatrix4fv: _glUniformMatrix4fv,
  /** @export */
  glUseProgram: _glUseProgram,
  /** @export */
  glVertexAttribPointer: _glVertexAttribPointer,
  /** @export */
  glViewport: _glViewport,
  /** @export */
  invoke_diii,
  /** @export */
  invoke_fiii,
  /** @export */
  invoke_i,
  /** @export */
  invoke_ii,
  /** @export */
  invoke_iif,
  /** @export */
  invoke_iii,
  /** @export */
  invoke_iiii,
  /** @export */
  invoke_iiiii,
  /** @export */
  invoke_iiiiid,
  /** @export */
  invoke_iiiiii,
  /** @export */
  invoke_iiiiiii,
  /** @export */
  invoke_iiiiiiii,
  /** @export */
  invoke_iiiiiiiii,
  /** @export */
  invoke_iiiiiiiiii,
  /** @export */
  invoke_iiiiiiiiiiii,
  /** @export */
  invoke_iiiiiiiiiiiii,
  /** @export */
  invoke_ji,
  /** @export */
  invoke_jiji,
  /** @export */
  invoke_v,
  /** @export */
  invoke_vi,
  /** @export */
  invoke_viff,
  /** @export */
  invoke_vii,
  /** @export */
  invoke_viif,
  /** @export */
  invoke_viii,
  /** @export */
  invoke_viiidi,
  /** @export */
  invoke_viiifi,
  /** @export */
  invoke_viiii,
  /** @export */
  invoke_viiiii,
  /** @export */
  invoke_viiiiii,
  /** @export */
  invoke_viiiiiii,
  /** @export */
  invoke_viiiiiiiiii,
  /** @export */
  invoke_viiiiiiiiiiiiiii,
  /** @export */
  invoke_viijii,
  /** @export */
  llvm_eh_typeid_for: _llvm_eh_typeid_for
};

function invoke_iii(index,a1,a2) {
  var sp = stackSave();
  try {
    return dynCall_iii(index,a1,a2);
  } catch(e) {
    stackRestore(sp);
    if (!(e instanceof EmscriptenEH)) throw e;
    _setThrew(1, 0);
  }
}

function invoke_viif(index,a1,a2,a3) {
  var sp = stackSave();
  try {
    dynCall_viif(index,a1,a2,a3);
  } catch(e) {
    stackRestore(sp);
    if (!(e instanceof EmscriptenEH)) throw e;
    _setThrew(1, 0);
  }
}

function invoke_viii(index,a1,a2,a3) {
  var sp = stackSave();
  try {
    dynCall_viii(index,a1,a2,a3);
  } catch(e) {
    stackRestore(sp);
    if (!(e instanceof EmscriptenEH)) throw e;
    _setThrew(1, 0);
  }
}

function invoke_vii(index,a1,a2) {
  var sp = stackSave();
  try {
    dynCall_vii(index,a1,a2);
  } catch(e) {
    stackRestore(sp);
    if (!(e instanceof EmscriptenEH)) throw e;
    _setThrew(1, 0);
  }
}

function invoke_iiii(index,a1,a2,a3) {
  var sp = stackSave();
  try {
    return dynCall_iiii(index,a1,a2,a3);
  } catch(e) {
    stackRestore(sp);
    if (!(e instanceof EmscriptenEH)) throw e;
    _setThrew(1, 0);
  }
}

function invoke_vi(index,a1) {
  var sp = stackSave();
  try {
    dynCall_vi(index,a1);
  } catch(e) {
    stackRestore(sp);
    if (!(e instanceof EmscriptenEH)) throw e;
    _setThrew(1, 0);
  }
}

function invoke_ii(index,a1) {
  var sp = stackSave();
  try {
    return dynCall_ii(index,a1);
  } catch(e) {
    stackRestore(sp);
    if (!(e instanceof EmscriptenEH)) throw e;
    _setThrew(1, 0);
  }
}

function invoke_i(index) {
  var sp = stackSave();
  try {
    return dynCall_i(index);
  } catch(e) {
    stackRestore(sp);
    if (!(e instanceof EmscriptenEH)) throw e;
    _setThrew(1, 0);
  }
}

function invoke_iiiii(index,a1,a2,a3,a4) {
  var sp = stackSave();
  try {
    return dynCall_iiiii(index,a1,a2,a3,a4);
  } catch(e) {
    stackRestore(sp);
    if (!(e instanceof EmscriptenEH)) throw e;
    _setThrew(1, 0);
  }
}

function invoke_v(index) {
  var sp = stackSave();
  try {
    dynCall_v(index);
  } catch(e) {
    stackRestore(sp);
    if (!(e instanceof EmscriptenEH)) throw e;
    _setThrew(1, 0);
  }
}

function invoke_iiiiiiiii(index,a1,a2,a3,a4,a5,a6,a7,a8) {
  var sp = stackSave();
  try {
    return dynCall_iiiiiiiii(index,a1,a2,a3,a4,a5,a6,a7,a8);
  } catch(e) {
    stackRestore(sp);
    if (!(e instanceof EmscriptenEH)) throw e;
    _setThrew(1, 0);
  }
}

function invoke_viiii(index,a1,a2,a3,a4) {
  var sp = stackSave();
  try {
    dynCall_viiii(index,a1,a2,a3,a4);
  } catch(e) {
    stackRestore(sp);
    if (!(e instanceof EmscriptenEH)) throw e;
    _setThrew(1, 0);
  }
}

function invoke_viiiii(index,a1,a2,a3,a4,a5) {
  var sp = stackSave();
  try {
    dynCall_viiiii(index,a1,a2,a3,a4,a5);
  } catch(e) {
    stackRestore(sp);
    if (!(e instanceof EmscriptenEH)) throw e;
    _setThrew(1, 0);
  }
}

function invoke_iiiiii(index,a1,a2,a3,a4,a5) {
  var sp = stackSave();
  try {
    return dynCall_iiiiii(index,a1,a2,a3,a4,a5);
  } catch(e) {
    stackRestore(sp);
    if (!(e instanceof EmscriptenEH)) throw e;
    _setThrew(1, 0);
  }
}

function invoke_iiiiiii(index,a1,a2,a3,a4,a5,a6) {
  var sp = stackSave();
  try {
    return dynCall_iiiiiii(index,a1,a2,a3,a4,a5,a6);
  } catch(e) {
    stackRestore(sp);
    if (!(e instanceof EmscriptenEH)) throw e;
    _setThrew(1, 0);
  }
}

function invoke_iiiiiiiiii(index,a1,a2,a3,a4,a5,a6,a7,a8,a9) {
  var sp = stackSave();
  try {
    return dynCall_iiiiiiiiii(index,a1,a2,a3,a4,a5,a6,a7,a8,a9);
  } catch(e) {
    stackRestore(sp);
    if (!(e instanceof EmscriptenEH)) throw e;
    _setThrew(1, 0);
  }
}

function invoke_viiiiii(index,a1,a2,a3,a4,a5,a6) {
  var sp = stackSave();
  try {
    dynCall_viiiiii(index,a1,a2,a3,a4,a5,a6);
  } catch(e) {
    stackRestore(sp);
    if (!(e instanceof EmscriptenEH)) throw e;
    _setThrew(1, 0);
  }
}

function invoke_viiiiiii(index,a1,a2,a3,a4,a5,a6,a7) {
  var sp = stackSave();
  try {
    dynCall_viiiiiii(index,a1,a2,a3,a4,a5,a6,a7);
  } catch(e) {
    stackRestore(sp);
    if (!(e instanceof EmscriptenEH)) throw e;
    _setThrew(1, 0);
  }
}

function invoke_iif(index,a1,a2) {
  var sp = stackSave();
  try {
    return dynCall_iif(index,a1,a2);
  } catch(e) {
    stackRestore(sp);
    if (!(e instanceof EmscriptenEH)) throw e;
    _setThrew(1, 0);
  }
}

function invoke_viff(index,a1,a2,a3) {
  var sp = stackSave();
  try {
    dynCall_viff(index,a1,a2,a3);
  } catch(e) {
    stackRestore(sp);
    if (!(e instanceof EmscriptenEH)) throw e;
    _setThrew(1, 0);
  }
}

function invoke_ji(index,a1) {
  var sp = stackSave();
  try {
    return dynCall_ji(index,a1);
  } catch(e) {
    stackRestore(sp);
    if (!(e instanceof EmscriptenEH)) throw e;
    _setThrew(1, 0);
    return 0n;
  }
}

function invoke_jiji(index,a1,a2,a3) {
  var sp = stackSave();
  try {
    return dynCall_jiji(index,a1,a2,a3);
  } catch(e) {
    stackRestore(sp);
    if (!(e instanceof EmscriptenEH)) throw e;
    _setThrew(1, 0);
    return 0n;
  }
}

function invoke_viiifi(index,a1,a2,a3,a4,a5) {
  var sp = stackSave();
  try {
    dynCall_viiifi(index,a1,a2,a3,a4,a5);
  } catch(e) {
    stackRestore(sp);
    if (!(e instanceof EmscriptenEH)) throw e;
    _setThrew(1, 0);
  }
}

function invoke_viiidi(index,a1,a2,a3,a4,a5) {
  var sp = stackSave();
  try {
    dynCall_viiidi(index,a1,a2,a3,a4,a5);
  } catch(e) {
    stackRestore(sp);
    if (!(e instanceof EmscriptenEH)) throw e;
    _setThrew(1, 0);
  }
}

function invoke_viijii(index,a1,a2,a3,a4,a5) {
  var sp = stackSave();
  try {
    dynCall_viijii(index,a1,a2,a3,a4,a5);
  } catch(e) {
    stackRestore(sp);
    if (!(e instanceof EmscriptenEH)) throw e;
    _setThrew(1, 0);
  }
}

function invoke_iiiiid(index,a1,a2,a3,a4,a5) {
  var sp = stackSave();
  try {
    return dynCall_iiiiid(index,a1,a2,a3,a4,a5);
  } catch(e) {
    stackRestore(sp);
    if (!(e instanceof EmscriptenEH)) throw e;
    _setThrew(1, 0);
  }
}

function invoke_iiiiiiii(index,a1,a2,a3,a4,a5,a6,a7) {
  var sp = stackSave();
  try {
    return dynCall_iiiiiiii(index,a1,a2,a3,a4,a5,a6,a7);
  } catch(e) {
    stackRestore(sp);
    if (!(e instanceof EmscriptenEH)) throw e;
    _setThrew(1, 0);
  }
}

function invoke_iiiiiiiiiiiii(index,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12) {
  var sp = stackSave();
  try {
    return dynCall_iiiiiiiiiiiii(index,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12);
  } catch(e) {
    stackRestore(sp);
    if (!(e instanceof EmscriptenEH)) throw e;
    _setThrew(1, 0);
  }
}

function invoke_fiii(index,a1,a2,a3) {
  var sp = stackSave();
  try {
    return dynCall_fiii(index,a1,a2,a3);
  } catch(e) {
    stackRestore(sp);
    if (!(e instanceof EmscriptenEH)) throw e;
    _setThrew(1, 0);
  }
}

function invoke_diii(index,a1,a2,a3) {
  var sp = stackSave();
  try {
    return dynCall_diii(index,a1,a2,a3);
  } catch(e) {
    stackRestore(sp);
    if (!(e instanceof EmscriptenEH)) throw e;
    _setThrew(1, 0);
  }
}

function invoke_iiiiiiiiiiii(index,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11) {
  var sp = stackSave();
  try {
    return dynCall_iiiiiiiiiiii(index,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11);
  } catch(e) {
    stackRestore(sp);
    if (!(e instanceof EmscriptenEH)) throw e;
    _setThrew(1, 0);
  }
}

function invoke_viiiiiiiiii(index,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10) {
  var sp = stackSave();
  try {
    dynCall_viiiiiiiiii(index,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10);
  } catch(e) {
    stackRestore(sp);
    if (!(e instanceof EmscriptenEH)) throw e;
    _setThrew(1, 0);
  }
}

function invoke_viiiiiiiiiiiiiii(index,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15) {
  var sp = stackSave();
  try {
    dynCall_viiiiiiiiiiiiiii(index,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15);
  } catch(e) {
    stackRestore(sp);
    if (!(e instanceof EmscriptenEH)) throw e;
    _setThrew(1, 0);
  }
}


// include: postamble.js
// === Auto-generated postamble setup entry stuff ===

var calledRun;

function callMain(args = []) {
  assert(runDependencies == 0, 'cannot call main when async dependencies remain! (listen on Module["onRuntimeInitialized"])');
  assert(typeof onPreRuns === 'undefined' || onPreRuns.length == 0, 'cannot call main when preRun functions remain to be called');

  var entryFunction = _main;

  args.unshift(thisProgram);

  var argc = args.length;
  var argv = stackAlloc((argc + 1) * 4);
  var argv_ptr = argv;
  for (var arg of args) {
    HEAPU32[((argv_ptr)>>2)] = stringToUTF8OnStack(arg);
    argv_ptr += 4;
  }
  HEAPU32[((argv_ptr)>>2)] = 0;

  try {

    var ret = entryFunction(argc, argv);

    // if we're not running an evented main loop, it's time to exit
    exitJS(ret, /* implicit = */ true);
    return ret;
  } catch (e) {
    return handleException(e);
  }
}

function stackCheckInit() {
  // This is normally called automatically during __wasm_call_ctors but need to
  // get these values before even running any of the ctors so we call it redundantly
  // here.
  _emscripten_stack_init();
  // TODO(sbc): Move writeStackCookie to native to to avoid this.
  writeStackCookie();
}

async function run(args = programArgs) {
  assert(!calledRun);
  calledRun = true;

  stackCheckInit();

  preRun();

  if (runDependencies) {
    await resolveRunDependencies();
  }

  var setStatus = Module['setStatus'];
  if (setStatus) {
    setStatus('Running...');
    // Yield to the event loop to allow the browser to paint "Running..."
    await new Promise((resolve) => setTimeout(resolve, 1));
    // Then we want to clear the status text, but only after the rest of this function runs.
    setTimeout(setStatus, 1, '');
  }

  if (ABORT) return;

  initRuntime();

  // No ATMAINS hooks

  Module['onRuntimeInitialized']?.();
  consumedModuleProp('onRuntimeInitialized');

  var noInitialRun = Module['noInitialRun'] || false;
  if (!noInitialRun) callMain(args);

  postRun();
}

function checkUnflushedContent() {
  // Compiler settings do not allow exiting the runtime, so flushing
  // the streams is not possible. but in ASSERTIONS mode we check
  // if there was something to flush, and if so tell the user they
  // should request that the runtime be exitable.
  // Normally we would not even include flush() at all, but in ASSERTIONS
  // builds we do so just for this check, and here we see if there is any
  // content to flush, that is, we check if there would have been
  // something a non-ASSERTIONS build would have not seen.
  // How we flush the streams depends on whether we are in SYSCALLS_REQUIRE_FILESYSTEM=0
  // mode (which has its own special function for this; otherwise, all
  // the code is inside libc)
  var oldOut = out;
  var oldErr = err;
  var has = false;
  out = err = (x) => {
    has = true;
  }
  try { // it doesn't matter if it fails
    _fflush(0);
    // also flush in the JS FS layer
    for (var name of ['stdout', 'stderr']) {
      var info = FS.analyzePath('/dev/' + name);
      if (!info) return;
      var stream = info.object;
      var rdev = stream.rdev;
      var tty = TTY.ttys[rdev];
      if (tty?.output?.length) {
        has = true;
      }
    }
  } catch(e) {}
  out = oldOut;
  err = oldErr;
  if (has) {
    warnOnce('stdio streams had content in them that was not flushed. you should set EXIT_RUNTIME to 1 (see the Emscripten FAQ), or make sure to emit a newline when you printf etc.');
  }
}

var wasmExports;

// With async instantation wasmExports is assigned asynchronously when the
// instance is received.
createWasm().then(() => run());

// end include: postamble.js

