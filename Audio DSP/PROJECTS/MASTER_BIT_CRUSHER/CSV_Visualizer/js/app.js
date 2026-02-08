// Main application glue
import * as dsp from './dsp_modules.js';

const fileInput = document.getElementById('fileInput');
const playBtn = document.getElementById('playBtn');
const stopBtn = document.getElementById('stopBtn');
const exportWavBtn = document.getElementById('exportWavBtn');
const exportCsvBtn = document.getElementById('exportCsvBtn');

// controls
const bitsEl = document.getElementById('bits');
const bitsLabel = document.getElementById('bitsLabel');
const quantMode = document.getElementById('quantMode');
const ditherEnabled = document.getElementById('ditherEnabled');
const tpdfEnabled = document.getElementById('tpdfEnabled');
const tpdfAmount = document.getElementById('tpdfAmount');
const holdSizeEl = document.getElementById('holdSize');
const holdLabel = document.getElementById('holdLabel');
const glitchThresh = document.getElementById('glitchThresh');
const glitchLabel = document.getElementById('glitchLabel');
const enableAll = document.getElementById('enableAll');
const tablePreview = document.getElementById('tablePreview');
const bitsSlider = bitsEl;

const waveformCanvas = document.getElementById('waveform');
const processedCanvas = document.getElementById('processed');
const spectCanvas = document.getElementById('spectrogram');
const enobVal = document.getElementById('enobVal');
const nullRms = document.getElementById('nullRms');

let audioCtx = null;
let workletNode = null;
let source = null;
let buffer = null;
let vizBuf = new Float32Array(256);

bitsEl.addEventListener('input', () => bitsLabel.textContent = bitsEl.value);
holdSizeEl.addEventListener('input', () => holdLabel.textContent = holdSizeEl.value);
glitchThresh.addEventListener('input', () => glitchLabel.textContent = Number(glitchThresh.value).toFixed(3));

fileInput.addEventListener('change', async (ev) => {
    const f = ev.target.files[0];
    if (!f) return;
    playBtn.disabled = true;
    stopBtn.disabled = true;
    exportWavBtn.disabled = true;
    exportCsvBtn.disabled = true;

    if (f.name.toLowerCase().endsWith('.csv')) {
        // parse CSV of samples
        const text = await f.text();
        const lines = text.trim().split(/\r?\n/);
        // assume first line may be header
        let start = 0; let headers = null;
        const firstRow = lines[0].split(',').map(s => s.trim());
        if (firstRow.some(v => isNaN(Number(v)))) { headers = firstRow; start = 1; }
        const samples = lines.slice(start).map(l => l.split(',').map(Number));
        // if 1 column: mono
        if (samples.length === 0) { alert('CSV had no samples'); return; }
        if (samples[0].length === 1) {
            const arr = new Float32Array(samples.length);
            for (let i = 0; i < samples.length; i++) arr[i] = samples[i][0];
            await setupFromFloat32(arr, 44100);
        } else {
            // pick first column as x/time and second column as amplitude
            const arr = new Float32Array(samples.length);
            for (let i = 0; i < samples.length; i++) arr[i] = samples[i][1];
            await setupFromFloat32(arr, 44100);
        }
        buildCsvPreview(samples, headers);
    } else {
        // audio file
        const arrayBuffer = await f.arrayBuffer();
        if (!audioCtx) audioCtx = new AudioContext();
        const decoded = await audioCtx.decodeAudioData(arrayBuffer);
        buffer = decoded;
        playBtn.disabled = false;
        exportWavBtn.disabled = false;
        exportCsvBtn.disabled = false;
        buildAudioPreview(decoded);
    }
});

async function setupFromFloat32(floats, sr) {
    // build AudioBuffer
    if (!audioCtx) audioCtx = new AudioContext();
    buffer = audioCtx.createBuffer(1, floats.length, sr);
    buffer.copyToChannel(floats, 0, 0);
    playBtn.disabled = false;
    exportWavBtn.disabled = false;
    exportCsvBtn.disabled = false;
}

function buildCsvPreview(rows, headers) {
    let html = '<table><thead><tr>';
    if (headers) { headers.forEach(h => html += `<th>${h}</th>`); } else { rows[0].forEach((_, i) => html += `<th>c${i}</th>`); }
    html += '</tr></thead><tbody>';
    const limit = Math.min(rows.length, 500);
    for (let i = 0; i < limit; i++) {
        html += '<tr>' + rows[i].map(c => `<td>${c}</td>`).join('') + '</tr>';
    }
    html += '</tbody></table>';
    tablePreview.innerHTML = html;
}

function buildAudioPreview(decoded) {
    const ch = decoded.numberOfChannels;
    const len = decoded.length;
    const sr = decoded.sampleRate;
    // show a tiny table or simple stats
    tablePreview.innerHTML = `<div>Channels: ${ch}, Length: ${len}, SR: ${sr}</div>`;
}

async function ensureWorklet() {
    if (!audioCtx) audioCtx = new AudioContext();
    if (!audioCtx.audioWorklet) { alert('AudioWorklet not supported in this browser'); return; }
    if (!workletNode) {
        await audioCtx.audioWorklet.addModule('js/worklet-processor.js');
        workletNode = new AudioWorkletNode(audioCtx, 'dsp-processor');
        workletNode.port.onmessage = (e) => {
            const d = e.data;
            if (d.type === 'viz') {
                // d.audio is a Float32Array-like
                vizBuf = Float32Array.from(d.audio);
                drawWaveform(vizBuf, waveformCanvas, '#00b37e');
            }
        };
    }
}

playBtn.addEventListener('click', async () => {
    if (!buffer) return;
    await ensureWorklet();
    if (source) source.stop();
    source = audioCtx.createBufferSource();
    source.buffer = buffer;
    source.connect(workletNode).connect(audioCtx.destination);
    // pass config
    postConfig();
    source.start();
    playBtn.disabled = true; stopBtn.disabled = false;
});

stopBtn.addEventListener('click', () => {
    if (source) try { source.stop(); } catch (e) { }
    source = null; playBtn.disabled = false; stopBtn.disabled = true;
});

function postConfig() {
    if (!workletNode) return;
    const cfg = {
        _bits: Number(bitsEl.value),
        _mode: quantMode.value,
        _dither: ditherEnabled.checked,
        _tpdf: tpdfEnabled.checked,
        _tpdfAmount: Number(tpdfAmount.value),
        _holdSize: Number(holdSizeEl.value),
        _jitter: Number(document.getElementById('jitter').value),
        _glitchThresh: Number(glitchThresh.value),
        _ditherGateThresh: Number(document.getElementById('ditherGateThresh').value),
        _boxcarSize: Number(document.getElementById('boxcarSize').value),
        _leakyA: Number(document.getElementById('leakyA').value),
        _dcR: Number(document.getElementById('dcR').value),
        _envAttack: Number(document.getElementById('envAttack').value),
        _envRelease: Number(document.getElementById('envRelease').value)
    };
    workletNode.port.postMessage({ type: 'config', payload: cfg });
    workletNode.port.postMessage({ type: 'enable', payload: enableAll.checked });
}

// redraw loop for processed canvas (we approximate by processing offline small block)
function drawWaveform(buf, canvasEl, stroke) {
    const ctx = canvasEl.getContext('2d');
    const w = canvasEl.width = canvasEl.clientWidth;
    const h = canvasEl.height = canvasEl.clientHeight;
    ctx.fillStyle = '#000'; ctx.fillRect(0, 0, w, h);
    ctx.lineWidth = 1; ctx.strokeStyle = stroke;
    ctx.beginPath();
    const N = buf.length;
    for (let i = 0; i < N; i++) {
        const x = (i / N) * w;
        const y = h / 2 - buf[i] * (h / 2);
        if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
    }
    ctx.stroke();
}

// export processed audio by rendering offline through the worklet or using JS processors
exportWavBtn.addEventListener('click', async () => {
    if (!buffer) return alert('No buffer');
    // naive: apply same processing chain offline in JS, then encode WAV
    const inData = buffer.getChannelData(0);
    let processed = new Float32Array(inData.length);
    const bits = Number(bitsEl.value);
    // simple offline processing applying quantization + dither
    for (let i = 0; i < inData.length; i++) {
        let x = inData[i];
        if (ditherEnabled.checked) {
            x += (tpdfEnabled.checked ? (Math.random() - Math.random()) * Number(tpdfAmount.value) : (Math.random() - 0.5) * Number(tpdfAmount.value));
        }
        // midrise/midtread
        const levels = Math.pow(2, bits);
        const step = 2.0 / levels;
        if (quantMode.value === 'midtread') x = Math.round(x / step) * step; else x = Math.floor((x + step / 2) / step) * step;
        processed[i] = x;
    }
    const wav = encodeWAV(processed, buffer.sampleRate);
    const blob = new Blob([wav], { type: 'audio/wav' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a'); a.href = url; a.download = 'processed.wav'; a.click();
});

exportCsvBtn.addEventListener('click', () => {
    if (!buffer) return alert('No buffer');
    const arr = buffer.getChannelData(0);
    let csv = 'sample\n';
    for (let i = 0; i < arr.length; i += Math.max(1, Math.floor(arr.length / 10000))) csv += arr[i].toFixed(8) + '\n';
    const blob = new Blob([csv], { type: 'text/csv' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a'); a.href = url; a.download = 'processed.csv'; a.click();
});

function encodeWAV(samples, sampleRate) {
    const bufferLen = 44 + samples.length * 2;
    const buffer = new ArrayBuffer(bufferLen);
    const view = new DataView(buffer);

    function writeString(view, offset, string) {
        for (let i = 0; i < string.length; i++) view.setUint8(offset + i, string.charCodeAt(i));
    }

    writeString(view, 0, 'RIFF');
    view.setUint32(4, 36 + samples.length * 2, true);
    writeString(view, 8, 'WAVE');
    writeString(view, 12, 'fmt ');
    view.setUint32(16, 16, true); // PCM chunk size
    view.setUint16(20, 1, true); // PCM
    view.setUint16(22, 1, true); // channels
    view.setUint32(24, sampleRate, true);
    view.setUint32(28, sampleRate * 2, true);
    view.setUint16(32, 2, true);
    view.setUint16(34, 16, true);
    writeString(view, 36, 'data');
    view.setUint32(40, samples.length * 2, true);

    // write samples
    let offset = 44;
    for (let i = 0; i < samples.length; i++) {
        let s = Math.max(-1, Math.min(1, samples[i]));
        s = s < 0 ? s * 0x8000 : s * 0x7FFF;
        view.setInt16(offset, s, true); offset += 2;
    }
    return view;
}

// periodically update config
setInterval(() => { if (workletNode) postConfig(); }, 250);

// initial draw empty
drawWaveform(new Float32Array(256), waveformCanvas, '#4fd9b2');