// Reusable DSP helper functions (also used by the worklet)

export function midQuantize(sample, bits, mode) {
    // bits: number of bits (1..24). mode: 'midrise' or 'midtread'
    if (!isFinite(sample)) return 0;
    const levels = Math.pow(2, bits);
    // assume input in -1..1 range
    const step = 2.0 / levels; // full-scale range / levels
    if (mode === 'midtread') {
        // has a level at zero -> round to nearest level
        const q = Math.round(sample / step) * step;
        return q;
    } else {
        // midrise: zero is between steps. shift by half-step
        const q = (Math.floor((sample + step / 2) / step)) * step;
        return q;
    }
}

export function tpdfDither(lsb = 1.0) {
    // produce triangular pdf in range [-lsb, +lsb]
    const u1 = Math.random();
    const u2 = Math.random();
    return (u1 - u2) * lsb;
}

export function holdDownsample(buffer, holdSize, jitter = 0) {
    if (holdSize <= 1 && jitter === 0) return buffer;
    const out = new Float32Array(buffer.length);
    let idx = 0;
    while (idx < buffer.length) {
        const jitterOffset = Math.floor((Math.random() * 2 - 1) * jitter * holdSize);
        const size = Math.max(1, holdSize + jitterOffset);
        const val = buffer[idx];
        for (let k = 0; k < size && idx < buffer.length; k++) out[idx++] = val;
    }
    return out;
}

export function glitchedetect(buffer, thresh) {
    const glitches = [];
    for (let i = 1; i < buffer.length; i++) {
        if (Math.abs(buffer[i] - buffer[i - 1]) > thresh) glitches.push(i);
    }
    return glitches;
}

export function boxcarFilter(buffer, N) {
    if (N <= 1) return buffer;
    const out = new Float32Array(buffer.length);
    let sum = 0;
    for (let i = 0; i < buffer.length; i++) {
        sum += buffer[i];
        if (i >= N) sum -= buffer[i - N];
        out[i] = sum / Math.min(N, i + 1);
    }
    return out;
}

export function leakyIntegrator(buffer, alpha) {
    const out = new Float32Array(buffer.length);
    let y = 0;
    for (let i = 0; i < buffer.length; i++) {
        y = alpha * y + (1 - alpha) * buffer[i];
        out[i] = y;
    }
    return out;
}

export function dcBlocker(buffer, R = 0.995) {
    const out = new Float32Array(buffer.length);
    let y1 = 0, x1 = 0;
    for (let i = 0; i < buffer.length; i++) {
        const x = buffer[i];
        const y = x - x1 + R * y1;
        out[i] = y;
        x1 = x; y1 = y;
    }
    return out;
}

export function envelopeFollower(buffer, attack = 0.01, release = 0.2) {
    const out = new Float32Array(buffer.length);
    let env = 0;
    for (let i = 0; i < buffer.length; i++) {
        const x = Math.abs(buffer[i]);
        if (x > env) env += (x - env) * (1 - Math.exp(-1 / Math.max(1, attack * 44100)));
        else env += (x - env) * (1 - Math.exp(-1 / Math.max(1, release * 44100)));
        out[i] = env;
    }
    return out;
}

// simple RMS
export function rms(buffer) {
    let s = 0; for (let i = 0; i < buffer.length; i++) { s += buffer[i] * buffer[i]; }
    return Math.sqrt(s / buffer.length || 0);
}

// ENOB (estimate) from SINAD: ENOB = (SINAD - 1.76)/6.02
export function enobFromSinad(sinadDb) {
    return (sinadDb - 1.76) / 6.02;
}

// naive sinad estimator vs a pure sine: user must feed known-tone reference
export function estimateSinad(signal, sampleRate, fTone) {
    // very simple: compute FFT and measure signal bin vs rest
    // here we keep a simple placeholder returning -Infinity if not computable
    return -Infinity;
}