// ============================================================================
// src/audio.js — Procedural Web Audio API SFX & Ambient Soundscape
// ============================================================================

export class SoundEngine {
  constructor() {
    this.ctx = null;
    this.muted = false;
    this.windGain = null;
    this.initialized = false;
  }

  unlock() {
    if (this.initialized) {
      if (this.ctx && this.ctx.state === 'suspended') this.ctx.resume();
      return;
    }
    try {
      const AC = window.AudioContext || window.webkitAudioContext;
      if (!AC) return;
      this.ctx = new AC();
      this.initialized = true;
      this._startAmbientWind();
    } catch {
      // Ignore audio failures in headless browsers
    }
  }

  setMuted(flag) {
    this.muted = flag;
    if (this.windGain && this.ctx) {
      this.windGain.gain.setTargetAtTime(flag ? 0 : 0.035, this.ctx.currentTime, 0.15);
    }
  }

  _startAmbientWind() {
    if (!this.ctx) return;
    const bufferSize = this.ctx.sampleRate * 2;
    const buffer = this.ctx.createBuffer(1, bufferSize, this.ctx.sampleRate);
    const data = buffer.getChannelData(0);
    let last = 0;
    for (let i = 0; i < bufferSize; i++) {
      const white = Math.random() * 2 - 1;
      last = (last + 0.03 * white) / 1.03;
      data[i] = last * 3.0;
    }
    const src = this.ctx.createBufferSource();
    src.buffer = buffer;
    src.loop = true;

    const filter = this.ctx.createBiquadFilter();
    filter.type = 'bandpass';
    filter.frequency.value = 340;
    filter.Q.value = 1.6;

    this.windGain = this.ctx.createGain();
    this.windGain.gain.value = this.muted ? 0 : 0.035;

    src.connect(filter);
    filter.connect(this.windGain);
    this.windGain.connect(this.ctx.destination);
    src.start();
  }

  _noiseBurst(duration, freq, q, gainPeak, type = 'bandpass') {
    if (!this.ctx || this.muted) return;
    const now = this.ctx.currentTime;
    const len = Math.floor(this.ctx.sampleRate * duration);
    const buf = this.ctx.createBuffer(1, len, this.ctx.sampleRate);
    const d = buf.getChannelData(0);
    for (let i = 0; i < len; i++) d[i] = Math.random() * 2 - 1;
    const src = this.ctx.createBufferSource();
    src.buffer = buf;
    const filt = this.ctx.createBiquadFilter();
    filt.type = type;
    filt.frequency.setValueAtTime(freq, now);
    filt.frequency.exponentialRampToValueAtTime(Math.max(60, freq * 0.35), now + duration);
    filt.Q.value = q;
    const g = this.ctx.createGain();
    g.gain.setValueAtTime(0.001, now);
    g.gain.linearRampToValueAtTime(gainPeak, now + duration * 0.14);
    g.gain.exponentialRampToValueAtTime(0.001, now + duration);
    src.connect(filt);
    filt.connect(g);
    g.connect(this.ctx.destination);
    src.start(now);
  }

  _tone(freqStart, freqEnd, duration, type = 'sine', gainPeak = 0.12) {
    if (!this.ctx || this.muted) return;
    const now = this.ctx.currentTime;
    const osc = this.ctx.createOscillator();
    osc.type = type;
    osc.frequency.setValueAtTime(freqStart, now);
    osc.frequency.exponentialRampToValueAtTime(Math.max(30, freqEnd), now + duration);
    const g = this.ctx.createGain();
    g.gain.setValueAtTime(0.001, now);
    g.gain.linearRampToValueAtTime(gainPeak, now + 0.02);
    g.gain.exponentialRampToValueAtTime(0.001, now + duration);
    osc.connect(g);
    g.connect(this.ctx.destination);
    osc.start(now);
    osc.stop(now + duration + 0.02);
  }

  playSwordSwing(combo = 0) {
    this._noiseBurst(0.18, 820 + combo * 160, 2.2, 0.14);
    this._tone(260 + combo * 45, 110, 0.14, 'triangle', 0.06);
  }

  playHit(heavy = false) {
    this._noiseBurst(heavy ? 0.22 : 0.12, heavy ? 280 : 480, 1.2, heavy ? 0.24 : 0.16, 'lowpass');
    this._tone(heavy ? 130 : 190, 48, heavy ? 0.22 : 0.13, 'sawtooth', heavy ? 0.14 : 0.09);
  }

  playBlock() {
    this._tone(980, 520, 0.18, 'triangle', 0.14);
    this._tone(1480, 1120, 0.22, 'sine', 0.08);
  }

  playRoll() {
    this._noiseBurst(0.22, 420, 1.5, 0.11, 'lowpass');
  }

  playDragonFlap() {
    this._noiseBurst(0.32, 180, 1.1, 0.16, 'lowpass');
  }

  playFireBreath() {
    this._noiseBurst(0.35, 520, 0.9, 0.18, 'bandpass');
    this._tone(110, 72, 0.3, 'sawtooth', 0.06);
  }

  playGold() {
    this._tone(880, 1320, 0.14, 'sine', 0.1);
    setTimeout(() => this._tone(1320, 1760, 0.18, 'sine', 0.08), 70);
  }

  playQuest() {
    const notes = [440, 554.37, 659.25, 880];
    notes.forEach((n, i) => {
      setTimeout(() => this._tone(n, n * 1.01, 0.28, 'triangle', 0.1), i * 85);
    });
  }

  playBossTelegraph() {
    this._tone(115, 78, 0.55, 'sawtooth', 0.16);
    this._tone(172, 118, 0.5, 'triangle', 0.1);
  }

  playPlayerHurt() {
    this._tone(160, 65, 0.22, 'sawtooth', 0.16);
    this._noiseBurst(0.16, 300, 1.0, 0.18, 'lowpass');
  }
}
