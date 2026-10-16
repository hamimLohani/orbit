// ==============================================================================
// ORBIT v2 — Companion Web App Frontend Controller
// ==============================================================================

class OrbitWebController {
  constructor() {
    this.ws = null;
    this.connected = false;
    this.reconnectTimer = null;
    this.fftData = new Array(16).fill(0);
    this.fftPeaks = new Array(16).fill(0);
    this.activeApp = 'home';
    this.isRecording = false;

    this.init();
  }

  init() {
    this.setupTabs();
    this.setupWebSocket();
    this.setupSpectrumCanvas();
    this.setupAppLauncher();
    this.setupRgbControls();
    this.setupVoiceMemos();
    this.setupWallpaperStudio();
    this.setupToastForm();
    this.setupPingButton();
  }

  // Tab Switching
  setupTabs() {
    const tabs = document.querySelectorAll('.nav-tab');
    tabs.forEach(tab => {
      tab.addEventListener('click', () => {
        tabs.forEach(t => t.classList.remove('active'));
        document.querySelectorAll('.tab-content').forEach(c => c.classList.remove('active'));

        tab.classList.add('active');
        const target = tab.dataset.tab;
        const panel = document.getElementById(`tab-${target}`);
        if (panel) panel.classList.add('active');
      });
    });
  }

  // WebSocket Connection
  setupWebSocket() {
    const host = window.location.host || 'orbit.local';
    const wsUrl = `ws://${host}/ws`;

    try {
      this.ws = new WebSocket(wsUrl);

      this.ws.onopen = () => {
        this.connected = true;
        this.updateConnectionStatus(true);
        if (this.reconnectTimer) clearInterval(this.reconnectTimer);
      };

      this.ws.onmessage = (event) => {
        try {
          const msg = JSON.parse(event.data);
          if (msg.type === 'telemetry') {
            this.handleTelemetry(msg.d);
          }
        } catch (e) {
          console.error('WS Parse Error:', e);
        }
      };

      this.ws.onclose = () => {
        this.connected = false;
        this.updateConnectionStatus(false);
        this.scheduleReconnect();
      };

      this.ws.onerror = () => {
        this.connected = false;
        this.updateConnectionStatus(false);
      };
    } catch (err) {
      console.warn('WS Init failed, fallback to offline demo mode:', err);
      this.updateConnectionStatus(false);
      this.startDemoMode();
    }
  }

  scheduleReconnect() {
    if (!this.reconnectTimer) {
      this.reconnectTimer = setInterval(() => {
        console.log('Attempting WebSocket reconnect...');
        this.setupWebSocket();
      }, 3000);
    }
  }

  updateConnectionStatus(connected) {
    const dot = document.getElementById('ws-dot');
    const text = document.getElementById('ws-status-text');
    if (connected) {
      dot.className = 'status-dot connected';
      text.textContent = 'Device Connected';
    } else {
      dot.className = 'status-dot disconnected';
      text.textContent = 'Disconnected (orbit.local)';
    }
  }

  send(data) {
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      this.ws.send(JSON.stringify(data));
    } else {
      console.log('[Offline Send]:', data);
    }
  }

  // Handle Telemetry JSON
  handleTelemetry(data) {
    if (!data) return;

    if (data.temp !== undefined) document.getElementById('val-temp').textContent = `${data.temp}°C`;
    if (data.heat_idx !== undefined) document.getElementById('val-heat-index').textContent = `Heat Index: ${data.heat_idx}°C`;
    if (data.hum !== undefined) document.getElementById('val-hum').textContent = `${data.hum}%`;
    if (data.mic_db !== undefined) {
      document.getElementById('val-mic').textContent = `${data.mic_db} dB`;
      const stateEl = document.getElementById('val-noise-state');
      stateEl.textContent = data.mic_db > 65 ? 'Status: High Noise!' : 'Status: Normal';
      stateEl.style.color = data.mic_db > 65 ? 'var(--color-neon-red)' : 'var(--color-text-dark)';
    }

    if (data.active_app) {
      this.activeApp = data.active_app;
      document.getElementById('val-active-app').textContent = data.active_app;
      this.highlightActiveAppCard(data.active_app);
    }

    if (data.time) document.getElementById('val-time').textContent = `Time: ${data.time}`;
    if (data.ip) document.getElementById('val-ip').textContent = data.ip;
    if (data.wifi_rssi !== undefined) document.getElementById('val-rssi').textContent = `${data.wifi_rssi} dBm`;
    if (data.heap !== undefined) document.getElementById('val-heap').textContent = `${Math.round(data.heap / 1024)} KB`;

    if (data.uptime !== undefined) {
      const h = Math.floor(data.uptime / 3600);
      const m = Math.floor((data.uptime % 3600) / 60);
      const s = data.uptime % 60;
      document.getElementById('val-uptime').textContent = `${h}h ${m}m ${s}s`;
    }

    if (data.moon_phase !== undefined) {
      document.getElementById('val-moon').textContent = `${Math.round(data.moon_phase * 100)}% Illum`;
    }

    if (data.fft && Array.isArray(data.fft)) {
      this.fftData = data.fft;
    }

    if (data.recording !== undefined) {
      this.isRecording = data.recording;
      this.updateRecordingUI(data.recording);
    }
  }

  // Spectrum Canvas Rendering (25-30 FPS)
  setupSpectrumCanvas() {
    const canvas = document.getElementById('spectrum-canvas');
    if (!canvas) return;
    const ctx = canvas.getContext('2d');

    const render = () => {
      ctx.clearRect(0, 0, canvas.width, canvas.height);

      const numBars = 16;
      const barSpacing = 12;
      const totalSpacing = (numBars - 1) * barSpacing;
      const barWidth = (canvas.width - 40 - totalSpacing) / numBars;
      const maxH = canvas.height - 35;
      const baseY = canvas.height - 20;

      for (let i = 0; i < numBars; i++) {
        const val = this.fftData[i] || 0;
        const barH = (val / 255) * maxH;
        const x = 20 + i * (barWidth + barSpacing);

        // Update peak hold
        if (val > (this.fftPeaks[i] || 0)) {
          this.fftPeaks[i] = val;
        } else {
          this.fftPeaks[i] = Math.max(0, (this.fftPeaks[i] || 0) - 2);
        }

        const peakH = ((this.fftPeaks[i] || 0) / 255) * maxH;

        // Gradient
        const grad = ctx.createLinearGradient(0, baseY, 0, baseY - maxH);
        grad.addColorStop(0, '#00f0ff');
        grad.addColorStop(0.6, '#b81fdf');
        grad.addColorStop(1, '#ff3366');

        ctx.fillStyle = grad;
        ctx.beginPath();
        ctx.roundRect(x, baseY - barH, barWidth, barH, [4, 4, 0, 0]);
        ctx.fill();

        // Peak line
        if (peakH > 2) {
          ctx.fillStyle = '#ffffff';
          ctx.fillRect(x, baseY - peakH - 2, barWidth, 3);
        }
      }

      requestAnimationFrame(render);
    };

    requestAnimationFrame(render);
  }

  // App Launcher
  setupAppLauncher() {
    const container = document.getElementById('apps-container');
    if (!container) return;

    container.addEventListener('click', (e) => {
      const card = e.target.closest('.app-card');
      if (card && card.dataset.app) {
        const appId = card.dataset.app;
        this.send({ cmd: 'switch_app', app: appId });
        this.highlightActiveAppCard(appId);
      }
    });
  }

  highlightActiveAppCard(appId) {
    document.querySelectorAll('.app-card').forEach(card => {
      if (card.dataset.app === appId) {
        card.classList.add('active');
      } else {
        card.classList.remove('active');
      }
    });
  }

  // RGB Lighting Controls
  setupRgbControls() {
    // Mode Buttons
    const modeBtns = document.querySelectorAll('.mode-btn');
    modeBtns.forEach(btn => {
      btn.addEventListener('click', () => {
        modeBtns.forEach(b => b.classList.remove('active'));
        btn.classList.add('active');
        const mode = parseInt(btn.dataset.mode);
        this.send({ cmd: 'set_rgb_mode', mode: mode });
      });
    });

    // Color Input & Swatches
    const colorInput = document.getElementById('rgb-color-input');
    if (colorInput) {
      colorInput.addEventListener('input', (e) => {
        this.send({ cmd: 'set_rgb_color', color: e.target.value });
      });
    }

    document.querySelectorAll('.swatch').forEach(swatch => {
      swatch.addEventListener('click', () => {
        const color = swatch.dataset.color;
        if (colorInput) colorInput.value = color;
        this.send({ cmd: 'set_rgb_color', color: color });
      });
    });

    // Display Backlight Slider
    const brightSlider = document.getElementById('display-bright-slider');
    const brightVal = document.getElementById('display-bright-val');
    if (brightSlider) {
      brightSlider.addEventListener('input', (e) => {
        const val = e.target.value;
        if (brightVal) brightVal.textContent = `${val}%`;
        this.send({ cmd: 'set_brightness', val: parseInt(val) });
      });
    }
  }

  // Voice Memos
  setupVoiceMemos() {
    const recBtn = document.getElementById('btn-toggle-rec');
    if (recBtn) {
      recBtn.addEventListener('click', () => {
        const newAction = this.isRecording ? 'stop' : 'start';
        this.send({ cmd: 'record_memo', action: newAction });
      });
    }

    const transcribeDemoBtn = document.getElementById('btn-transcribe-demo');
    if (transcribeDemoBtn) {
      transcribeDemoBtn.addEventListener('click', () => {
        const transcriptEl = document.getElementById('ai-transcript');
        transcriptEl.value = "Analyzing audio via Whisper AI...\n\n" +
          "TRANSCRIPT: 'Remind team to calibrate the INMP441 I2S microphone and test the ST7735 display double buffer before firmware release.'\n\n" +
          "ACTION ITEMS:\n" +
          "1. Calibrate microphone noise threshold in settings (65dB).\n" +
          "2. Validate SPI bus mutex with SD card asset loading.";
      });
    }

    // Load memos from REST
    this.fetchMemos();
  }

  updateRecordingUI(recording) {
    const recBtn = document.getElementById('btn-toggle-rec');
    const recText = document.getElementById('rec-btn-text');
    if (!recBtn) return;

    if (recording) {
      recBtn.classList.add('recording');
      recText.textContent = 'Stop Recording';
    } else {
      recBtn.classList.remove('recording');
      recText.textContent = 'Record Voice Memo';
      this.fetchMemos();
    }
  }

  fetchMemos() {
    fetch('/api/recordings')
      .then(res => res.json())
      .then(files => {
        const container = document.getElementById('memos-container');
        if (!container) return;

        if (!files || files.length === 0) {
          container.innerHTML = '<div class="empty-state"><p>No voice memos recorded yet. Press the record button or hold the device ACTION button!</p></div>';
          return;
        }

        container.innerHTML = files.map(file => `
          <div class="memo-item">
            <div>
              <strong>${file}</strong>
              <div style="font-size:0.75rem;color:var(--color-text-muted)">SD Card WAV (16kHz PCM)</div>
            </div>
            <audio controls preload="none" src="/api/download?file=${file}" style="height:32px;"></audio>
          </div>
        `).join('');
      })
      .catch(() => {
        // Mock fallback if offline
        const container = document.getElementById('memos-container');
        if (container) {
          container.innerHTML = '<div class="empty-state"><p>Demo Mode: Hold device Quick-Action button to record audio to SD card.</p></div>';
        }
      });
  }

  // Wallpaper Studio
  setupWallpaperStudio() {
    const dropzone = document.getElementById('image-dropzone');
    const fileInput = document.getElementById('image-file-input');
    const canvas = document.getElementById('preview-canvas');
    const sendBtn = document.getElementById('btn-send-wallpaper');

    if (!dropzone || !fileInput || !canvas) return;

    dropzone.addEventListener('click', () => fileInput.click());

    dropzone.addEventListener('dragover', (e) => {
      e.preventDefault();
      dropzone.style.borderColor = 'var(--color-neon-cyan)';
    });

    dropzone.addEventListener('dragleave', () => {
      dropzone.style.borderColor = 'var(--color-border)';
    });

    dropzone.addEventListener('drop', (e) => {
      e.preventDefault();
      dropzone.style.borderColor = 'var(--color-border)';
      if (e.dataTransfer.files.length) {
        this.processImage(e.dataTransfer.files[0], canvas, sendBtn);
      }
    });

    fileInput.addEventListener('change', (e) => {
      if (e.target.files.length) {
        this.processImage(e.target.files[0], canvas, sendBtn);
      }
    });
  }

  processImage(file, canvas, sendBtn) {
    const reader = new FileReader();
    reader.onload = (e) => {
      const img = new Image();
      img.onload = () => {
        const ctx = canvas.getContext('2d');
        // Cover-fit 128x160
        ctx.fillStyle = '#000';
        ctx.fillRect(0, 0, 128, 160);

        const imgRatio = img.width / img.height;
        const targetRatio = 128 / 160;

        let drawW, drawH, offsetX = 0, offsetY = 0;
        if (imgRatio > targetRatio) {
          drawH = 160;
          drawW = 160 * imgRatio;
          offsetX = (128 - drawW) / 2;
        } else {
          drawW = 128;
          drawH = 128 / imgRatio;
          offsetY = (160 - drawH) / 2;
        }

        ctx.drawImage(img, offsetX, offsetY, drawW, drawH);
        sendBtn.disabled = false;
      };
      img.src = e.target.result;
    };
    reader.readAsDataURL(file);
  }

  // Push Toast Form
  setupToastForm() {
    const form = document.getElementById('toast-form');
    if (!form) return;

    form.addEventListener('submit', (e) => {
      e.preventDefault();
      const title = document.getElementById('toast-title').value;
      const body = document.getElementById('toast-body').value;
      const duration = parseInt(document.getElementById('toast-duration').value) || 3000;
      const color = document.getElementById('toast-color').value;

      this.send({
        cmd: 'toast',
        title: title,
        body: body,
        duration_ms: duration,
        rgb_color: color
      });

      alert('Toast notification transmitted to ORBIT display!');
    });
  }

  // Ping Find Device
  setupPingButton() {
    const btn = document.getElementById('btn-ping');
    if (btn) {
      btn.addEventListener('click', () => {
        this.send({ cmd: 'find_device' });
      });
    }
  }

  // Offline Simulation Demo
  startDemoMode() {
    setInterval(() => {
      // Simulate live random spectrum fluctuations
      for (let i = 0; i < 16; i++) {
        this.fftData[i] = Math.floor(Math.random() * 200 * (1 - i / 20));
      }
    }, 50);
  }
}

// Instantiate on DOM load
window.addEventListener('DOMContentLoaded', () => {
  window.orbitApp = new OrbitWebController();
});
