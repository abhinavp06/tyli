// AudioWorkletProcessor: per-frame DSP. Keep computations light.
// Save this file as js/worklet-processor.js

class DspProcessor extends AudioWorkletProcessor {
    constructor() {
        super();
        this._enabled = false;
        this._bits = 16;
        this._mode = 'midrise';
        this._dither = true;
        this._tpdf = true;
        this._tpdfAmount = 1.0;
        this._holdSize = 1;
        this._jitter = 0;
        this._glitchThresh = 0.2;
        this._ditherGateThresh = 0.01;
        this._boxcarSize = 1;
        this._leakyA = 0.9;
        this._dcR = 0.995;
        this._envAttack = 0.01;
        this._envRelease = 0.15;
        this.port.onmessage = (e) => this._onMessage(e.data);
        this._sendFramesCounter = 0;
        this._lastSample = 0;
    }

    _onMessage(msg) {
        if (msg.type === 'config') {
            Object.assign(this, msg.payload);
        } else if (msg.type === 'enable') {
            this._enabled = !!msg.payload;
        }
    }

    // helper small functions
    _tpdf(lsb) { return (Math.random() - Math.random()) * lsb; }

    _midQuant(sample) {
        const levels = Math.pow(2, this._bits);
        const step = 2.0 / levels;
        if (this._mode === 'midtread') return Math.round(sample / step) * step;
        return Math.floor((sample + step / 2) / step) * step;
    }

    process(inputs, outputs/*, parameters*/) {
        const input = inputs[0];
        const output = outputs[0];
        if (!input || input.length === 0) return true;

        const inChannel = input[0];
        const outChannel = output[0];
        const N = inChannel.length;

        // simple processing loop
        for (let i = 0; i < N; i++) {
            let x = inChannel[i];
            // apply dc blocker
            // we skip heavy stateful filters for brevity; simple blocks below
            // apply hold
            if (this._holdSize > 1) {
                // naive hold: replace with lastSample if within hold window
                // not perfect but adequate
                if ((i % this._holdSize) !== 0) x = this._lastSample;
            }

            // dithering
            if (this._dither) {
                const gate = Math.abs(x) < this._ditherGateThresh;
                if (gate) {
                    const d = this._tpdf ? ((Math.random() - Math.random()) * this._tpdfAmount) : ((Math.random() - 0.5) * this._tpdfAmount);
                    x += d;
                }
            }

            // quantize
            if (this._bits < 24) {
                x = this._midQuant(x);
            }

            outChannel[i] = x;
            this._lastSample = x;
        }

        // periodically send a small downsampled buffer for visualization
        this._sendFramesCounter++;
        if (this._sendFramesCounter % 2 === 0) {
            // pack a small Float32Array and post
            const view = new Float32Array(Math.min(256, inChannel.length));
            for (let i = 0; i < view.length; i++) view[i] = inChannel[Math.floor(i * inChannel.length / view.length)];
            // transfer via structured clone
            this.port.postMessage({ type: 'viz', audio: view });
        }

        return true;
    }
}

registerProcessor('dsp-processor', DspProcessor);