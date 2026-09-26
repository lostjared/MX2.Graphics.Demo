// Browser integration test: Node.js 22+ and a running demo/browser with remote debugging.
// SHADER_TEST_PORT=9223 node tests/shader-loading.cjs [--fallback]
// --fallback forces KHR_parallel_shader_compile off to exercise demand-only loading.
const assert = require('node:assert/strict');
(async () => {
    const fallback = process.argv.includes('--fallback');
    const targets =
        await (await fetch('http://127.0.0.1:' + (process.env.SHADER_TEST_PORT || 9223) + '/json'))
            .json();
    const runtimeErrors = [];
    let recordRuntimeErrors = false;
    const w = new WebSocket(targets.find(t => t.type === 'page').webSocketDebuggerUrl);
    let id = 0;
    const pending = new Map();
    w.onmessage = e => {
        const m = JSON.parse(e.data);
        if (m.method === 'Runtime.exceptionThrown' && recordRuntimeErrors)
            runtimeErrors.push(m.params.exceptionDetails.exception?.description ||
                               m.params.exceptionDetails.text);
        if (m.id) {
            pending.get(m.id)?.(m.result);
            pending.delete(m.id)
        }
    };
    await new Promise(r => w.onopen = r);
    const call = (method, params = {}) => new Promise(r => {
        pending.set(++id, r);
        w.send(JSON.stringify({id, method, params}))
    });
    const ev = async expression => {
        const r =
            await call('Runtime.evaluate', {expression, returnByValue : true, awaitPromise : true});
        if (r.exceptionDetails)
            throw Error(r.exceptionDetails.exception?.description || r.exceptionDetails.text);
        return r.result?.value;
    };
    const sleep = ms => new Promise(r => setTimeout(r, ms));
    const wait = async (expression, timeout = 12000) => {
        const end = Date.now() + timeout;
        while (Date.now() < end) {
            if (await ev(expression))
                return;
            await sleep(50)
        }
        throw Error('Timeout: ' + expression)
    };
    await call('Page.enable');
    await call('Runtime.enable');
    await call('Emulation.setDeviceMetricsOverride',
               {width : 640, height : 480, deviceScaleFactor : 1, mobile : false});
    function auditGL(fallback) {
        window.compileAudit = {links : 0, max : 0, active : 0, early : [], polls : 0, fallback};
        const proto = WebGL2RenderingContext.prototype;
        const metadata = new WeakMap();
        const extension = proto.getExtension;
        proto.getExtension = function(name) {
            if (fallback && name === 'KHR_parallel_shader_compile')
                return null;
            return extension.call(this, name);
        };
        const link = proto.linkProgram;
        proto.linkProgram = function(program) {
            const info = {
                startup : !window.shaderLibraryProgress?.total,
                polled : false,
                polls : 0,
                active : true
            };
            compileAudit.links++;
            metadata.set(program, info);
            if (!info.startup) {
                compileAudit.active++;
                compileAudit.max = Math.max(compileAudit.max, compileAudit.active);
            }
            return link.call(this, program);
        };
        const parameter = proto.getProgramParameter;
        proto.getProgramParameter = function(program, pname) {
            const info = metadata.get(program);
            if (info && !info.startup && pname === 0x91B1) {
                compileAudit.polls++;
                // Keep each program pending for several ticks even if the driver finishes quickly.
                if (++info.polls < 6)
                    return false;
                const result = parameter.call(this, program, pname);
                if (result)
                    info.polled = true;
                return result;
            }
            if (info && !info.startup && pname === this.LINK_STATUS) {
                if (!fallback && !info.polled)
                    compileAudit.early.push('LINK_STATUS');
                if (info.active) {
                    info.active = false;
                    compileAudit.active--;
                }
            }
            return parameter.call(this, program, pname);
        };
        for (const method
                 of ['getUniformLocation', 'getActiveUniform', 'useProgram', 'getProgramInfoLog']) {
            const original = proto[method];
            proto[method] = function(program, ...args) {
                const info = metadata.get(program);
                if (info && !info.startup && !fallback && !info.polled)
                    compileAudit.early.push(method);
                return original.call(this, program, ...args);
            };
        }
        const deletion = proto.deleteProgram;
        proto.deleteProgram = function(program) {
            const info = metadata.get(program);
            if (info && !info.startup && info.active) {
                info.active = false;
                compileAudit.active--;
            }
            return deletion.call(this, program);
        };
        const shaderParameter = proto.getShaderParameter;
        proto.getShaderParameter = function(shader, pname) {
            if (window.shaderLibraryProgress?.total && pname === this.COMPILE_STATUS)
                compileAudit.early.push('COMPILE_STATUS');
            return shaderParameter.call(this, shader, pname);
        };
    }
    await call('Page.addScriptToEvaluateOnNewDocument',
               {source : '(' + auditGL.toString() + ')(' + fallback + ')'});
    await call('Page.reload', {ignoreCache : true});
    await sleep(500);
    await wait(
        "typeof Module !== 'undefined' && !!Module.getShaderLoadState && window.shaderLibraryProgress.ready >= 4",
        30000);
    await wait(
        "document.getElementById('shader').options.length===Module.getShaderCount() && document.getElementById('mobile-shader').options.length===Module.getShaderCount()");
    assert.equal(await ev("document.getElementById('shader').options[0].textContent"),
                 '[0] Bubble');
    const catalogCount = await ev('Module.getShaderCount()');
    await ev(
        'window.originalCatalogEntries=window.getOrderedShaderEntries;window.getOrderedShaderEntries=()=>[];window.refreshShaderDropdownList();window.getOrderedShaderEntries=window.originalCatalogEntries;');
    assert.equal(await ev("document.getElementById('shader').options.length"), catalogCount);
    recordRuntimeErrors = true;
    console.log(
        'startup',
        await ev(
            'JSON.stringify({progress:shaderLibraryProgress,audit:compileAudit,index:Module.getIndex(),defaultName:Module.getShaderNameAt(Module.getShaderCount()-1)})'));
    assert.equal(await ev('shaderLibraryProgress.parallel'), !fallback);
    await ev(
        'window.realConsumeIdle=window.consumeShaderIdleTime;window.consumeShaderIdleTime=()=>false;');
    await wait('shaderLibraryProgress.pending===0 && shaderLibraryProgress.queued===0');
    const idleCount = await ev('shaderLibraryProgress.ready');
    await sleep(1200);
    assert.equal(await ev('shaderLibraryProgress.ready'), idleCount);
    const prior = await ev('Module.getIndex()');
    await ev(
        'Module.setShaderIndex(1000);Module.setShaderIndex(1001);Module.setShaderIndex(900);Module.resize();');
    assert.equal(await ev('Module.getIndex()'), prior);
    assert.equal(await ev('Module.getPendingShaderIndex()'), 900);
    await wait('Module.getIndex()===900');
    assert.equal(await ev('Module.getShaderLoadState(900)'), 'READY');
    await ev('Module.addShaderPass(950);Module.addShaderPass(960);Module.enableMultipass(true);');
    await wait(
        "Module.getShaderLoadState(950)==='READY' && Module.getShaderLoadState(960)==='READY'");
    console.log(
        'selections/multipass',
        await ev(
            'JSON.stringify({progress:shaderLibraryProgress,audit:compileAudit,index:Module.getIndex()})'));
    const good =
        '#version 300 es\nprecision highp float;out vec4 color;void main(){color=vec4(0.2,0.4,0.8,1.0);}';
    const queued = await ev(`Module.compileCustomShader(${JSON.stringify(good)})`);
    assert.ok(queued.startsWith('QUEUED'));
    assert.equal(await ev('Module.getIndex()'), 900);
    await wait(
        "Module.getIndex()===Module.getShaderCount()-1 && Module.getShaderLoadState(Module.getIndex())==='READY'");
    const custom = await ev('Module.getIndex()');
    assert.ok(
        (await ev("document.getElementById('shader-log-textarea').value")).startsWith('SUCCESS'));
    const failures = await ev('shaderFailureEntries.length');
    await ev(`Module.compileCustomShader(${
        JSON.stringify(
            '#version 300 es\nprecision highp float;out vec4 color;void main(){color=broken_identifier;}')})`);
    await wait(`shaderFailureEntries.length===${failures + 1}`);
    assert.equal(await ev('Module.getIndex()'), custom);
    assert.equal(await ev(`Module.getShaderLoadState(${custom})`), 'READY');
    assert.ok(
        (await ev("document.getElementById('shader-log-textarea').value")).startsWith('ERROR'));
    await ev(`Module.compileCustomShader(${JSON.stringify(good)});Module.compileCustomShader(${
        JSON.stringify(good.replace('0.2', '0.7'))});`);
    await wait(
        "document.getElementById('shader-log-textarea').value.startsWith('SUCCESS') && shaderLibraryProgress.pending===0 && shaderLibraryProgress.queued===0");
    await ev(
        'Module.enableMultipass(false);Module.loadModel("3dplus.mxmod.z");Module.setShader3DMode(true);');
    await sleep(200);
    await ev('Module.setShader3DMode(false);');
    // A playing video must prevent idle catalog compilation, while explicit demand still works.
    await ev(
        `(async()=>{window.testCanvas=document.createElement('canvas');testCanvas.width=32;testCanvas.height=32;window.testPaint=setInterval(()=>testCanvas.getContext('2d').fillRect(0,0,32,32),30);const v=document.getElementById('camera-video');v.muted=true;v.srcObject=testCanvas.captureStream(30);await v.play();window.consumeShaderIdleTime=window.realConsumeIdle;window.lastShaderInteraction=0;window.shaderIdlePermitUntil=performance.now()+10000;})()`);
    const during = await ev('shaderLibraryProgress.ready');
    await sleep(1100);
    assert.equal(await ev('shaderLibraryProgress.ready'), during);
    await ev('Module.setShaderIndex(980)');
    await wait('Module.getIndex()===980');
    await ev(
        "document.getElementById('camera-video').pause();clearInterval(testPaint);window.consumeShaderIdleTime=()=>false;");
    if (!fallback) {
        // Render the cheap pass-through so an idle deadline can genuinely have headroom.
        await ev(
            'Module.setShaderIndex(Module.getShaderCount()-2);window.lastShaderInteraction=0;window.consumeShaderIdleTime=window.realConsumeIdle;');
        const beforeIdle = await ev('shaderLibraryProgress.ready');
        await wait('shaderLibraryProgress.ready > ' + beforeIdle);
        await ev('window.consumeShaderIdleTime=()=>false;');
        await wait('shaderLibraryProgress.pending===0 && shaderLibraryProgress.queued===0');
    }
    await ev('window.finalizeShaderFailureLog()');
    assert.ok((await ev("Module.FS.readFile('/shader-failures.log',{encoding:'utf8'})"))
                  .includes('broken_identifier'));
    assert.ok(await ev('compileAudit.max<=4'));
    assert.deepEqual(JSON.parse(await ev('JSON.stringify(compileAudit.early)')), []);
    assert.equal(await ev("document.getElementById('canvas').getContext('webgl2').isContextLost()"),
                 false);
    console.log(
        'PASS', fallback ? 'fallback' : 'parallel',
        await ev(
            'JSON.stringify({progress:shaderLibraryProgress,audit:compileAudit,status:document.getElementById("shader-loading-status").textContent,failures:shaderFailureEntries.length})'));
    assert.deepEqual(runtimeErrors, []);
    w.close();
})().catch(e => {
    console.error(e);
    process.exit(1)
});
