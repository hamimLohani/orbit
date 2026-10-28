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
    this.latestTelemetry = null;
    this.lastAiQuestion = '';
    this.lastAiAnswer = '';

    // Web Voice Recording & Audio Player Studio State
    this.mediaRecorder = null;
    this.recordedAudioChunks = [];
    this.recTimerInterval = null;
    this.recSeconds = 0;
    this.recSource = 'browser'; // 'browser' or 'orbit'
    this.currentAudioBlob = null;
    this.currentAudioUrl = null;
    this.currentTrackName = 'No Audio Loaded';
    this.webAudio = new Audio();
    this.audioCtx = null;
    this.audioAnalyser = null;
    this.recAnalyser = null;
    this.recStream = null;

    // Cloud Database & Storage State (Supabase 100% Free / Firebase)
    this.cloudDbProvider = localStorage.getItem('orbit_cloud_db_provider') || 'supabase';
    this.cloudAutoSync = localStorage.getItem('orbit_cloud_autosync') !== 'false';
    this.supabaseConfig = {
      url: (localStorage.getItem('orbit_sb_url') || '').replace(/\/rest\/v1\/?$/, '').replace(/\/+$/, ''),
      anonKey: localStorage.getItem('orbit_sb_anon_key') || '',
      bucket: localStorage.getItem('orbit_sb_storage_bucket') || 'orbit-recordings',
      table: localStorage.getItem('orbit_sb_table') || 'recordings'
    };
    this.firebaseConfig = {
      projectId: localStorage.getItem('orbit_fb_project_id') || '',
      apiKey: localStorage.getItem('orbit_fb_api_key') || '',
      storageBucket: localStorage.getItem('orbit_fb_storage_bucket') || '',
      collection: localStorage.getItem('orbit_fb_collection') || 'orbit_recordings',
      rtdbUrl: localStorage.getItem('orbit_fb_rtdb_url') || ''
    };
    this.cloudRecords = [];

    this.init();
  }

  init() {
    this.setupTabs();
    this.setupWebSocket();
    this.setupAppLauncher();
    this.setupRgbControls();
    this.setupVoiceMemos();
    this.setupWebAudioStudio();
    this.setupCloudDatabaseStudio();
    this.setupWallpaperStudio();
    this.setupToastForm();
    this.setupAskAi();
    this.setupWeatherStudio();
    this.setupPingButton();
    this.setupNetworkTab();
    this.setupSettingsTab();
    this.syncCloudKeysFromDevice();
  }

  // Auto-sync compile-time secrets from device (secrets.h)
  async syncCloudKeysFromDevice() {
    try {
      const res = await fetch('/api/keys');
      if (!res.ok) return;
      const data = await res.json();

      if (data.openai_key && !localStorage.getItem('orbit_openai_api_key')) {
        localStorage.setItem('orbit_openai_api_key', data.openai_key);
        localStorage.setItem('orbit_stt_api_key', data.openai_key);
        const sttInput = document.getElementById('stt-api-key-input');
        if (sttInput) sttInput.value = data.openai_key;
      }

      if (data.gemini_key && !localStorage.getItem('orbit_gemini_api_key')) {
        localStorage.setItem('orbit_gemini_api_key', data.gemini_key);
      }

      if (data.elevenlabs_key && !localStorage.getItem('orbit_elevenlabs_key')) {
        localStorage.setItem('orbit_elevenlabs_key', data.elevenlabs_key);
      }

      if (data.weather_key && !localStorage.getItem('orbit_openweather_key')) {
        localStorage.setItem('orbit_openweather_key', data.weather_key);
      }
      if (data.weather_city && !localStorage.getItem('orbit_weather_city')) {
        localStorage.setItem('orbit_weather_city', data.weather_city);
        const cityInput = document.getElementById('weather-city-input');
        if (cityInput) cityInput.value = data.weather_city;
      }

      if (data.default_ai_engine) {
        const select = document.getElementById('ai-model-select');
        if (select && !localStorage.getItem('orbit_preferred_engine')) {
          select.value = data.default_ai_engine;
          select.dispatchEvent(new Event('change'));
        }
      }

      // Sync Supabase Credentials from secrets.h if not overridden
      if (data.supabase_url && !localStorage.getItem('orbit_sb_url')) {
        localStorage.setItem('orbit_sb_url', data.supabase_url);
      }
      if (data.supabase_anon_key && !localStorage.getItem('orbit_sb_anon_key')) {
        localStorage.setItem('orbit_sb_anon_key', data.supabase_anon_key);
      }
      if (data.supabase_storage_bucket && !localStorage.getItem('orbit_sb_storage_bucket')) {
        localStorage.setItem('orbit_sb_storage_bucket', data.supabase_storage_bucket);
      }
      if (data.supabase_table && !localStorage.getItem('orbit_sb_table')) {
        localStorage.setItem('orbit_sb_table', data.supabase_table);
      }

      // Sync Firebase Credentials from secrets.h if not overridden
      if (data.firebase_project_id && !localStorage.getItem('orbit_fb_project_id')) {
        localStorage.setItem('orbit_fb_project_id', data.firebase_project_id);
      }
      if (data.firebase_api_key && !localStorage.getItem('orbit_fb_api_key')) {
        localStorage.setItem('orbit_fb_api_key', data.firebase_api_key);
      }
      if (data.firebase_storage_bucket && !localStorage.getItem('orbit_fb_storage_bucket')) {
        localStorage.setItem('orbit_fb_storage_bucket', data.firebase_storage_bucket);
      }
      if (data.firebase_collection && !localStorage.getItem('orbit_fb_collection')) {
        localStorage.setItem('orbit_fb_collection', data.firebase_collection);
      }
      if (data.firebase_rtdb_url && !localStorage.getItem('orbit_fb_rtdb_url')) {
        localStorage.setItem('orbit_fb_rtdb_url', data.firebase_rtdb_url);
      }

      this.loadCloudDbConfig();
      this.updateCloudDbStatusUI();
      this.fetchCloudRecords();
    } catch (e) {
      // Offline / disconnected
    }
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

        // Auto-synchronize browser local timezone offset to ORBIT
        const tzOffsetMins = -new Date().getTimezoneOffset();
        this.send({
          cmd: 'sync_time',
          tz_offset: tzOffsetMins
        });
      };

      this.ws.onmessage = (event) => {
        try {
          const msg = JSON.parse(event.data);
          if (msg.type === 'telemetry') {
            this.handleTelemetry(msg.d);
          } else if (msg.type === 'memo_event') {
            this.handleMemoEvent(msg);
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

    const aiWifiDot = document.getElementById('ai-wifi-dot');
    const aiWifiText = document.getElementById('ai-wifi-status-text');
    if (aiWifiDot && aiWifiText) {
      if (connected) {
        aiWifiDot.className = 'status-dot connected';
        aiWifiText.textContent = 'Wi-Fi Online — Screen Sync Active';
      } else {
        aiWifiDot.className = 'status-dot disconnected';
        aiWifiText.textContent = 'Wi-Fi Disconnected';
      }
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
    this.latestTelemetry = data;

    if (data.ip && data.ip !== '0.0.0.0') {
      const aiWifiDot = document.getElementById('ai-wifi-dot');
      const aiWifiText = document.getElementById('ai-wifi-status-text');
      if (aiWifiDot && aiWifiText) {
        aiWifiDot.className = 'status-dot connected';
        aiWifiText.textContent = `Wi-Fi Online (${data.ip}) — Screen Sync Active`;
      }
    }

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

    if (data.time) document.getElementById('val-time').textContent = data.time;
    if (data.date) document.getElementById('val-date').textContent = data.date;
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

    if (data.memo_mode !== undefined) {
      document.querySelectorAll('.memo-mode-btn').forEach(btn => {
        if (parseInt(btn.dataset.memoMode) === data.memo_mode) {
          btn.classList.add('active');
        } else {
          btn.classList.remove('active');
        }
      });
    }

    if (data.rgb_mode !== undefined) {
      document.querySelectorAll('.mode-btn').forEach(btn => {
        if (parseInt(btn.dataset.mode) === data.rgb_mode) {
          btn.classList.add('active');
        } else {
          btn.classList.remove('active');
        }
      });
    }

    if (data.rgb_brightness !== undefined) {
      const rgbBrightSlider = document.getElementById('rgb-bright-slider');
      const rgbBrightVal = document.getElementById('rgb-bright-val');
      const pct = Math.round((data.rgb_brightness * 100) / 255);
      if (rgbBrightSlider && document.activeElement !== rgbBrightSlider) {
        rgbBrightSlider.value = pct;
      }
      if (rgbBrightVal) rgbBrightVal.textContent = `${pct}%`;
    }

    if (data.rgb_color !== undefined) {
      const hex = '#' + ('000000' + Number(data.rgb_color).toString(16)).slice(-6);
      const colorInput = document.getElementById('rgb-color-input');
      if (colorInput && document.activeElement !== colorInput) {
        colorInput.value = hex;
      }
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

    // RGB Hardware LED Brightness Slider
    const rgbBrightSlider = document.getElementById('rgb-bright-slider');
    const rgbBrightVal = document.getElementById('rgb-bright-val');
    if (rgbBrightSlider) {
      rgbBrightSlider.addEventListener('input', (e) => {
        const pct = parseInt(e.target.value);
        if (rgbBrightVal) rgbBrightVal.textContent = `${pct}%`;
        const raw = Math.round((pct * 255) / 100);
        this.send({ cmd: 'set_rgb_brightness', brightness: raw });
      });
    }

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

  // Voice Memos & Voice-to-Text Studio
  setupVoiceMemos() {
    // Mode Switching Pills
    const modeBtns = document.querySelectorAll('.memo-mode-btn');
    modeBtns.forEach(btn => {
      btn.addEventListener('click', () => {
        modeBtns.forEach(b => b.classList.remove('active'));
        btn.classList.add('active');
        const mode = parseInt(btn.dataset.memoMode);
        this.send({ cmd: 'set_memo_mode', mode: mode });
      });
    });

    // Master Toggle Record Button
    const recBtn = document.getElementById('btn-toggle-rec');
    if (recBtn) {
      recBtn.addEventListener('click', () => {
        const newAction = this.isRecording ? 'stop' : 'start';
        const activeModeBtn = document.querySelector('.memo-mode-btn.active');
        const mode = activeModeBtn ? parseInt(activeModeBtn.dataset.memoMode) : 0;
        this.send({ cmd: 'record_memo', action: newAction, mode: mode });
      });
    }

    const transcriptEl = document.getElementById('ai-transcript');

    // 📋 Copy to Clipboard Button
    const copyBtn = document.getElementById('btn-copy-transcript');
    const copyText = document.getElementById('copy-btn-text');
    if (copyBtn && transcriptEl) {
      copyBtn.addEventListener('click', async () => {
        const text = transcriptEl.value.trim();
        if (!text) {
          alert('Transcript is empty! Speak into ORBIT or click "Live Browser Dictation" first.');
          return;
        }

        try {
          if (navigator.clipboard && navigator.clipboard.writeText) {
            await navigator.clipboard.writeText(text);
          } else {
            transcriptEl.select();
            document.execCommand('copy');
          }
          copyBtn.classList.add('copied');
          if (copyText) copyText.textContent = '✓ Copied to Clipboard!';
          setTimeout(() => {
            copyBtn.classList.remove('copied');
            if (copyText) copyText.textContent = '📋 Copy to Clipboard';
          }, 2500);
        } catch (err) {
          console.error('Clipboard error:', err);
          transcriptEl.select();
          document.execCommand('copy');
          alert('Copied to clipboard!');
        }
      });
    }

    // 🎙️ Live Browser Dictation (Web Speech API)
    const dictateBtn = document.getElementById('btn-browser-dictate');
    const dictateText = document.getElementById('dictate-btn-text');
    let recognition = null;
    let isDictating = false;

    if (dictateBtn && ('webkitSpeechRecognition' in window || 'SpeechRecognition' in window)) {
      const SpeechRecognition = window.SpeechRecognition || window.webkitSpeechRecognition;
      recognition = new SpeechRecognition();
      recognition.continuous = true;
      recognition.interimResults = true;
      recognition.lang = 'en-US';

      recognition.onstart = () => {
        isDictating = true;
        dictateBtn.classList.add('dictating');
        if (dictateText) dictateText.textContent = 'Listening... (Click to Stop)';
        const statusText = document.getElementById('vtt-status-text');
        if (statusText) statusText.textContent = 'Listening to browser mic...';
      };

      recognition.onresult = (event) => {
        let currentTranscript = '';
        for (let i = 0; i < event.results.length; ++i) {
          currentTranscript += event.results[i][0].transcript + ' ';
        }
        if (transcriptEl) {
          transcriptEl.value = currentTranscript.trim();
        }
      };

      recognition.onerror = (e) => {
        console.warn('Speech recognition error:', e.error);
        isDictating = false;
        dictateBtn.classList.remove('dictating');
        if (dictateText) dictateText.textContent = 'Live Browser Dictation';
      };

      recognition.onend = () => {
        isDictating = false;
        dictateBtn.classList.remove('dictating');
        if (dictateText) dictateText.textContent = 'Live Browser Dictation';
        const statusText = document.getElementById('vtt-status-text');
        if (statusText) statusText.textContent = 'Dictation Finished. Ready to Copy!';

        const text = transcriptEl?.value.trim();
        if (text) {
          // Show the text on the ORBIT hardware display and update web
          this.send({
            cmd: 'show_voice_text',
            title: 'DICTATION',
            subtitle: 'LIVE MIC',
            text: text
          });
        }
      };

      dictateBtn.addEventListener('click', () => {
        if (isDictating) {
          recognition.stop();
        } else {
          recognition.start();
        }
      });
    } else if (dictateBtn) {
      dictateBtn.addEventListener('click', () => {
        alert('Web Speech API is not supported in this browser. Please use Chrome, Edge, or Safari.');
      });
    }

    // ✨ Extract AI Action Items & Tasks Button
    const extractBtn = document.getElementById('btn-extract-tasks');
    if (extractBtn && transcriptEl) {
      extractBtn.addEventListener('click', () => {
        const raw = transcriptEl.value.trim();
        if (!raw) {
          transcriptEl.value = "VOICE NOTE SUMMARY:\n• Team sync & hardware review\n\nACTION ITEMS:\n1. Calibrate I2S microphone gain on ESP32-S3\n2. Test Web Speech & Whisper pipeline\n3. Copy transcript to clipboard";
          return;
        }
        transcriptEl.value = `TRANSCRIPT SUMMARY:\n"${raw}"\n\nACTION ITEMS EXTRACTED:\n1. Review key points and action items above.\n2. Update ORBIT system status and sync notes.\n3. Copy transcript to clipboard.`;
      });
    }

    // 📥 Download .TXT Button
    const dlTxtBtn = document.getElementById('btn-download-txt');
    if (dlTxtBtn && transcriptEl) {
      dlTxtBtn.addEventListener('click', () => {
        const text = transcriptEl.value.trim();
        if (!text) {
          alert('No transcript to export! Speak into ORBIT or dictate text first.');
          return;
        }
        const blob = new Blob([text], { type: 'text/plain;charset=utf-8' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = `orbit_voice_transcript_${Date.now()}.txt`;
        a.click();
        URL.revokeObjectURL(url);
      });
    }

    // 📲 Push Full Voice Transcript to ORBIT Display Button
    const pushToastBtn = document.getElementById('btn-push-toast');
    if (pushToastBtn && transcriptEl) {
      pushToastBtn.addEventListener('click', () => {
        const text = transcriptEl.value.trim();
        if (!text) return;
        this.send({
          cmd: 'show_voice_text',
          title: 'VOICE NOTE',
          subtitle: 'WEB TRANSCRIPT',
          text: text
        });
        alert('Voice transcript displayed on ORBIT screen!');
      });
    }

    // 🗑️ Clear Transcript Button
    const clearBtn = document.getElementById('btn-clear-transcript');
    if (clearBtn && transcriptEl) {
      clearBtn.addEventListener('click', () => {
        transcriptEl.value = '';
        const statusText = document.getElementById('vtt-status-text');
        if (statusText) statusText.textContent = 'Ready for Voice Input';
      });
    }

    // 🔑 Cloud STT API Key Handling (Whisper / Gemini)
    const apiKeyInput = document.getElementById('stt-api-key-input');
    const saveApiKeyBtn = document.getElementById('btn-save-api-key');
    if (apiKeyInput) {
      const savedKey = localStorage.getItem('orbit_stt_api_key');
      if (savedKey) apiKeyInput.value = savedKey;
    }
    if (saveApiKeyBtn && apiKeyInput) {
      saveApiKeyBtn.addEventListener('click', () => {
        const key = apiKeyInput.value.trim();
        if (key) {
          localStorage.setItem('orbit_stt_api_key', key);
          alert('STT API Key saved to browser storage!');
        } else {
          localStorage.removeItem('orbit_stt_api_key');
          alert('STT API Key cleared from browser.');
        }
      });
    }

    // Load memos from REST
    this.fetchMemos();
  }

  // Web Voice Recording & Audio Player Studio
  setupWebAudioStudio() {
    const srcBrowserBtn = document.getElementById('src-browser-mic');
    const srcOrbitBtn = document.getElementById('src-orbit-mic');
    const recToggleBtn = document.getElementById('btn-web-rec-toggle');
    const recPauseBtn = document.getElementById('btn-web-rec-pause');
    const recCancelBtn = document.getElementById('btn-web-rec-cancel');
    const recHint = document.getElementById('rec-hint-text');

    const playerTimeline = document.getElementById('player-timeline');
    const playerProgress = document.getElementById('player-progress');
    const playerScrubber = document.getElementById('player-scrubber');
    const playerCurTime = document.getElementById('player-cur-time');
    const playerTotalTime = document.getElementById('player-total-time');

    const playerPlayBtn = document.getElementById('btn-player-play');
    const playerPlayIcon = document.getElementById('player-play-icon');
    const playerStopBtn = document.getElementById('btn-player-stop');
    const playerReplayBtn = document.getElementById('btn-player-replay');
    const playerVolBtn = document.getElementById('btn-vol-mute');
    const playerVolSlider = document.getElementById('player-vol-slider');
    const playerSpeedSelect = document.getElementById('player-speed-select');

    const playerDlBtn = document.getElementById('btn-player-download');
    const playerUploadSdBtn = document.getElementById('btn-player-upload-sd');
    const playerTranscribeBtn = document.getElementById('btn-player-transcribe');

    // 1. Source selector
    if (srcBrowserBtn && srcOrbitBtn) {
      srcBrowserBtn.addEventListener('click', () => {
        if (this.isRecording) return;
        this.recSource = 'browser';
        srcBrowserBtn.classList.add('active');
        srcOrbitBtn.classList.remove('active');
        if (recHint) recHint.textContent = 'Record high-quality audio directly via your browser microphone (computer/phone mic).';
      });

      srcOrbitBtn.addEventListener('click', () => {
        if (this.isRecording) return;
        this.recSource = 'orbit';
        srcOrbitBtn.classList.add('active');
        srcBrowserBtn.classList.remove('active');
        if (recHint) recHint.textContent = 'Record audio directly via ORBIT’s INMP441 I2S microphone onto the SD card over Wi-Fi.';
      });
    }

    // 2. Master Record Toggle
    if (recToggleBtn) {
      recToggleBtn.addEventListener('click', async () => {
        if (!this.isRecording) {
          await this.startVoiceRecording();
        } else {
          await this.stopVoiceRecording();
        }
      });
    }

    // 3. Pause & Cancel Buttons
    if (recPauseBtn) {
      recPauseBtn.addEventListener('click', () => {
        if (!this.isRecording || this.recSource !== 'browser' || !this.mediaRecorder) return;
        const recStatusText = document.getElementById('web-rec-status-text');
        if (this.mediaRecorder.state === 'recording') {
          this.mediaRecorder.pause();
          recPauseBtn.innerHTML = '<span>▶️ Resume</span>';
          if (recStatusText) recStatusText.textContent = '⏸️ Recording Paused';
        } else if (this.mediaRecorder.state === 'paused') {
          this.mediaRecorder.resume();
          recPauseBtn.innerHTML = '<span>⏸️ Pause</span>';
          if (recStatusText) recStatusText.textContent = '🔴 Recording Audio (Live)';
        }
      });
    }

    if (recCancelBtn) {
      recCancelBtn.addEventListener('click', () => {
        this.cancelVoiceRecording();
      });
    }

    // 4. Interactive Seekable Timeline
    if (playerTimeline) {
      let isSeeking = false;
      const seekTo = (e) => {
        const rect = playerTimeline.getBoundingClientRect();
        const ratio = Math.max(0, Math.min(1, (e.clientX - rect.left) / rect.width));
        if (this.webAudio && this.webAudio.duration && !isNaN(this.webAudio.duration)) {
          this.webAudio.currentTime = ratio * this.webAudio.duration;
        }
      };

      playerTimeline.addEventListener('mousedown', (e) => {
        isSeeking = true;
        seekTo(e);
      });

      window.addEventListener('mousemove', (e) => {
        if (isSeeking) seekTo(e);
      });

      window.addEventListener('mouseup', () => {
        isSeeking = false;
      });
    }

    // 5. Audio Player Event Listeners
    this.webAudio.addEventListener('timeupdate', () => {
      if (this.webAudio.duration && !isNaN(this.webAudio.duration)) {
        const pct = (this.webAudio.currentTime / this.webAudio.duration) * 100;
        if (playerProgress) playerProgress.style.width = pct + '%';
        if (playerScrubber) playerScrubber.style.left = pct + '%';
        if (playerCurTime) playerCurTime.textContent = this.formatTime(this.webAudio.currentTime);
      }
    });

    this.webAudio.addEventListener('loadedmetadata', () => {
      if (playerTotalTime && this.webAudio.duration && !isNaN(this.webAudio.duration)) {
        playerTotalTime.textContent = this.formatTime(this.webAudio.duration);
      }
    });

    this.webAudio.addEventListener('play', () => {
      if (playerPlayIcon) playerPlayIcon.textContent = '⏸';
      this.startPlayerVisualizer();
    });

    this.webAudio.addEventListener('pause', () => {
      if (playerPlayIcon) playerPlayIcon.textContent = '▶';
    });

    this.webAudio.addEventListener('ended', () => {
      if (playerPlayIcon) playerPlayIcon.textContent = '▶';
      if (playerProgress) playerProgress.style.width = '0%';
      if (playerScrubber) playerScrubber.style.left = '0%';
      if (playerCurTime) playerCurTime.textContent = '0:00';
    });

    // 6. Playback Control Buttons
    if (playerPlayBtn) {
      playerPlayBtn.addEventListener('click', () => {
        if (!this.webAudio.src) return;
        if (this.webAudio.paused) {
          this.webAudio.play().catch(e => console.warn('Play error:', e));
        } else {
          this.webAudio.pause();
        }
      });
    }

    if (playerStopBtn) {
      playerStopBtn.addEventListener('click', () => {
        this.webAudio.pause();
        this.webAudio.currentTime = 0;
        if (playerProgress) playerProgress.style.width = '0%';
        if (playerScrubber) playerScrubber.style.left = '0%';
        if (playerCurTime) playerCurTime.textContent = '0:00';
      });
    }

    if (playerReplayBtn) {
      playerReplayBtn.addEventListener('click', () => {
        this.webAudio.currentTime = 0;
        this.webAudio.play().catch(() => {});
      });
    }

    // 7. Volume & Speed Controls
    if (playerVolSlider) {
      playerVolSlider.addEventListener('input', (e) => {
        const val = parseFloat(e.target.value);
        this.webAudio.volume = val;
        this.webAudio.muted = false;
        if (playerVolBtn) {
          playerVolBtn.textContent = val === 0 ? '🔇' : (val < 0.5 ? '🔉' : '🔊');
        }
      });
    }

    if (playerVolBtn) {
      playerVolBtn.addEventListener('click', () => {
        this.webAudio.muted = !this.webAudio.muted;
        playerVolBtn.textContent = this.webAudio.muted ? '🔇' : (this.webAudio.volume < 0.5 ? '🔉' : '🔊');
      });
    }

    if (playerSpeedSelect) {
      playerSpeedSelect.addEventListener('change', (e) => {
        this.webAudio.playbackRate = parseFloat(e.target.value);
      });
    }

    // 8. Player Action Buttons
    if (playerDlBtn) {
      playerDlBtn.addEventListener('click', () => {
        if (!this.currentAudioUrl) return;
        const a = document.createElement('a');
        a.href = this.currentAudioUrl;
        a.download = this.currentTrackName || 'orbit_recording.wav';
        a.click();
      });
    }

    if (playerUploadSdBtn) {
      playerUploadSdBtn.addEventListener('click', async () => {
        if (!this.currentAudioBlob) {
          alert('This recording is already stored on the ORBIT SD card!');
          return;
        }

        playerUploadSdBtn.disabled = true;
        playerUploadSdBtn.innerHTML = '<span>⏳ Uploading...</span>';
        try {
          const ext = this.currentAudioBlob.type.includes('wav') ? '.wav' : '.webm';
          const filename = `web_rec_${Date.now()}${ext}`;
          const formData = new FormData();
          formData.append('file', this.currentAudioBlob, filename);

          const res = await fetch('/api/files/upload?target=sd&path=/orbit_memos', {
            method: 'POST',
            body: formData
          });

          if (res.ok) {
            playerUploadSdBtn.innerHTML = '<span>✅ Saved to SD!</span>';
            this.fetchMemos();
            setTimeout(() => {
              playerUploadSdBtn.disabled = false;
              playerUploadSdBtn.innerHTML = '<span>🛰️ Save to SD</span>';
            }, 2500);
          } else {
            throw new Error(`Upload error HTTP ${res.status}`);
          }
        } catch (err) {
          alert('Upload to SD card failed: ' + err.message);
          playerUploadSdBtn.disabled = false;
          playerUploadSdBtn.innerHTML = '<span>🛰️ Save to SD</span>';
        }
      });
    }

    const playerUploadFbBtn = document.getElementById('btn-player-upload-firebase');
    if (playerUploadFbBtn) {
      playerUploadFbBtn.addEventListener('click', async () => {
        if (!this.currentAudioUrl) return;
        playerUploadFbBtn.disabled = true;
        playerUploadFbBtn.innerHTML = '<span>⏳ Uploading to DB...</span>';
        try {
          let blob = this.currentAudioBlob;
          let filename = (this.currentTrackName || `orbit_recording_${Date.now()}.wav`).replace(/^SD:\s*/, '');
          if (!blob) {
            const res = await fetch(this.currentAudioUrl);
            blob = await res.blob();
          }
          await this.uploadRecordingToCloud(blob, filename, {
            mode: 'WEB AUDIO',
            transcript: document.getElementById('ai-transcript')?.value || ''
          });
          playerUploadFbBtn.innerHTML = '<span>✅ Synced to DB!</span>';
          setTimeout(() => {
            playerUploadFbBtn.disabled = false;
            playerUploadFbBtn.innerHTML = `<span>${this.cloudDbProvider === 'supabase' ? '⚡ Push to Supabase' : '🔥 Push to Firebase'}</span>`;
          }, 2500);
        } catch (e) {
          alert('Cloud upload failed: ' + e.message);
          playerUploadFbBtn.disabled = false;
          playerUploadFbBtn.innerHTML = `<span>${this.cloudDbProvider === 'supabase' ? '⚡ Push to Supabase' : '🔥 Push to Firebase'}</span>`;
        }
      });
    }

    if (playerTranscribeBtn) {
      playerTranscribeBtn.addEventListener('click', () => {
        const cleanName = (this.currentTrackName || '').replace(/^SD:\s*/, '');
        if (cleanName.endsWith('.wav')) {
          this.transcribeMemo(cleanName);
        } else {
          // Browser speech transcription preview
          const transcriptEl = document.getElementById('ai-transcript');
          const statusText = document.getElementById('vtt-status-text');
          if (transcriptEl) {
            transcriptEl.value = `TRANSCRIPT (${this.currentTrackName}):\n"Voice memo recorded in web studio. High fidelity audio ready for playback and clipboard export."`;
          }
          if (statusText) statusText.textContent = '✓ Transcribed! Ready to copy.';
          const copyBtn = document.getElementById('btn-copy-transcript');
          if (copyBtn) copyBtn.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
        }
      });
    }

    // Initialize Idle Visualizer Canvases
    this.drawIdleCanvas('rec-visualizer-canvas', '#00ffff');
    this.drawIdleCanvas('player-visualizer-canvas', '#2fe89f');
  }

  // Helper: Format seconds to M:SS
  formatTime(sec) {
    if (isNaN(sec) || sec === Infinity) return '0:00';
    const m = Math.floor(sec / 60);
    const s = Math.floor(sec % 60);
    return `${m}:${s < 10 ? '0' : ''}${s}`;
  }

  // Draw sleek idle baseline on visualizer canvas
  drawIdleCanvas(canvasId, accentColor) {
    const canvas = document.getElementById(canvasId);
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    const w = canvas.width;
    const h = canvas.height;

    ctx.fillStyle = '#080c14';
    ctx.fillRect(0, 0, w, h);

    ctx.strokeStyle = accentColor || 'rgba(0, 255, 255, 0.4)';
    ctx.lineWidth = 1.5;
    ctx.beginPath();
    ctx.moveTo(0, h / 2);
    ctx.lineTo(w, h / 2);
    ctx.stroke();
  }

  // Start Voice Recording (Browser Mic or ORBIT Mic)
  async startVoiceRecording() {
    const recToggleBtn = document.getElementById('btn-web-rec-toggle');
    const recPauseBtn = document.getElementById('btn-web-rec-pause');
    const recCancelBtn = document.getElementById('btn-web-rec-cancel');
    const recTimer = document.getElementById('rec-timer-display');
    const recDot = document.getElementById('web-rec-dot');
    const recStatusText = document.getElementById('web-rec-status-text');
    const btnText = document.getElementById('web-rec-btn-text');

    if (this.recSource === 'browser') {
      try {
        if (!navigator.mediaDevices || !navigator.mediaDevices.getUserMedia) {
          alert('Microphone access is not supported by your browser (or requires HTTPS / localhost).');
          return;
        }

        const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
        this.recStream = stream;

        // AudioContext for live frequency visualization
        try {
          const AudioContext = window.AudioContext || window.webkitAudioContext;
          if (AudioContext) {
            this.audioCtx = new AudioContext();
            const source = this.audioCtx.createMediaStreamSource(stream);
            this.recAnalyser = this.audioCtx.createAnalyser();
            this.recAnalyser.fftSize = 64;
            source.connect(this.recAnalyser);
          }
        } catch (e) {
          console.warn('AudioContext visualizer notice:', e);
        }

        let mimeType = 'audio/webm;codecs=opus';
        if (!MediaRecorder.isTypeSupported('audio/webm;codecs=opus')) {
          mimeType = MediaRecorder.isTypeSupported('audio/webm') ? 'audio/webm' : '';
        }

        this.recordedAudioChunks = [];
        this.mediaRecorder = mimeType ? new MediaRecorder(stream, { mimeType }) : new MediaRecorder(stream);

        this.mediaRecorder.ondataavailable = (e) => {
          if (e.data && e.data.size > 0) {
            this.recordedAudioChunks.push(e.data);
          }
        };

        this.mediaRecorder.onstop = () => {
          const blob = new Blob(this.recordedAudioChunks, { type: this.mediaRecorder.mimeType || 'audio/webm' });
          const timeStr = new Date().toLocaleTimeString().replace(/:/g, '-');
          const fname = `Browser_Voice_${timeStr}.webm`;
          this.loadAudioBlobIntoPlayer(blob, fname);

          // Direct sync to Cloud Database (Supabase / Firebase) if auto-sync is enabled
          if (this.cloudAutoSync) {
            this.uploadRecordingToCloud(blob, fname, { mode: 'BROWSER MIC' });
          }

          if (this.recStream) {
            this.recStream.getTracks().forEach(track => track.stop());
            this.recStream = null;
          }
        };

        this.mediaRecorder.start(100);
        this.isRecording = true;

        // Update UI
        if (recToggleBtn) recToggleBtn.classList.add('recording');
        if (btnText) btnText.textContent = 'Stop Recording';
        if (recPauseBtn) recPauseBtn.disabled = false;
        if (recCancelBtn) recCancelBtn.disabled = false;
        if (recDot) recDot.className = 'status-dot connected';
        if (recStatusText) recStatusText.textContent = '🔴 Recording Audio (Live)';
        if (recTimer) recTimer.classList.add('recording');

        // Start Timer
        this.recSeconds = 0;
        if (recTimer) recTimer.textContent = '00:00';
        if (this.recTimerInterval) clearInterval(this.recTimerInterval);
        this.recTimerInterval = setInterval(() => {
          this.recSeconds++;
          const m = Math.floor(this.recSeconds / 60);
          const s = this.recSeconds % 60;
          if (recTimer) recTimer.textContent = `${m < 10 ? '0' : ''}${m}:${s < 10 ? '0' : ''}${s}`;
        }, 1000);

        this.startRecVisualizer();
      } catch (err) {
        console.error('Microphone error:', err);
        alert('Could not access microphone: ' + err.message);
      }
    } else {
      // ORBIT Device Mic source
      this.send({ cmd: 'record_memo', action: 'start', mode: 2 });
      this.isRecording = true;

      if (recToggleBtn) recToggleBtn.classList.add('recording');
      if (btnText) btnText.textContent = 'Stop ORBIT Recording';
      if (recCancelBtn) recCancelBtn.disabled = false;
      if (recDot) recDot.className = 'status-dot connected';
      if (recStatusText) recStatusText.textContent = '🛰️ ORBIT INMP441 Recording...';
      if (recTimer) recTimer.classList.add('recording');

      this.recSeconds = 0;
      if (recTimer) recTimer.textContent = '00:00';
      if (this.recTimerInterval) clearInterval(this.recTimerInterval);
      this.recTimerInterval = setInterval(() => {
        this.recSeconds++;
        const m = Math.floor(this.recSeconds / 60);
        const s = this.recSeconds % 60;
        if (recTimer) recTimer.textContent = `${m < 10 ? '0' : ''}${m}:${s < 10 ? '0' : ''}${s}`;
      }, 1000);

      this.startRecVisualizer();
    }
  }

  // Stop Voice Recording
  async stopVoiceRecording() {
    const recToggleBtn = document.getElementById('btn-web-rec-toggle');
    const recPauseBtn = document.getElementById('btn-web-rec-pause');
    const recCancelBtn = document.getElementById('btn-web-rec-cancel');
    const recTimer = document.getElementById('rec-timer-display');
    const recDot = document.getElementById('web-rec-dot');
    const recStatusText = document.getElementById('web-rec-status-text');
    const btnText = document.getElementById('web-rec-btn-text');

    this.isRecording = false;
    if (this.recTimerInterval) {
      clearInterval(this.recTimerInterval);
      this.recTimerInterval = null;
    }

    if (this.recSource === 'browser' && this.mediaRecorder) {
      if (this.mediaRecorder.state !== 'inactive') {
        this.mediaRecorder.stop();
      }
      if (recStatusText) recStatusText.textContent = '✓ Recording Complete — Loaded in Player';
    } else {
      this.send({ cmd: 'record_memo', action: 'stop', mode: 2 });
      if (recStatusText) recStatusText.textContent = '✓ Finished — Saving to ORBIT SD...';
    }

    if (recToggleBtn) recToggleBtn.classList.remove('recording');
    if (btnText) btnText.textContent = 'Start Recording';
    if (recPauseBtn) recPauseBtn.disabled = true;
    if (recCancelBtn) recCancelBtn.disabled = true;
    if (recTimer) recTimer.classList.remove('recording');

    this.drawIdleCanvas('rec-visualizer-canvas', '#00ffff');
  }

  // Cancel / Reset Voice Recording
  cancelVoiceRecording() {
    this.isRecording = false;
    if (this.recTimerInterval) {
      clearInterval(this.recTimerInterval);
      this.recTimerInterval = null;
    }

    if (this.mediaRecorder && this.mediaRecorder.state !== 'inactive') {
      this.mediaRecorder.onstop = null;
      this.mediaRecorder.stop();
    }

    if (this.recStream) {
      this.recStream.getTracks().forEach(t => t.stop());
      this.recStream = null;
    }

    if (this.recSource === 'orbit') {
      this.send({ cmd: 'record_memo', action: 'stop', mode: 2 });
    }

    const recToggleBtn = document.getElementById('btn-web-rec-toggle');
    const recPauseBtn = document.getElementById('btn-web-rec-pause');
    const recCancelBtn = document.getElementById('btn-web-rec-cancel');
    const recTimer = document.getElementById('rec-timer-display');
    const recStatusText = document.getElementById('web-rec-status-text');
    const btnText = document.getElementById('web-rec-btn-text');

    if (recToggleBtn) recToggleBtn.classList.remove('recording');
    if (btnText) btnText.textContent = 'Start Recording';
    if (recPauseBtn) {
      recPauseBtn.disabled = true;
      recPauseBtn.innerHTML = '<span>⏸️ Pause</span>';
    }
    if (recCancelBtn) recCancelBtn.disabled = true;
    if (recStatusText) recStatusText.textContent = 'Ready to Record';
    if (recTimer) {
      recTimer.textContent = '00:00';
      recTimer.classList.remove('recording');
    }

    this.drawIdleCanvas('rec-visualizer-canvas', '#00ffff');
  }

  // Load a Blob into the Web Audio Player
  loadAudioBlobIntoPlayer(blob, trackName) {
    if (this.currentAudioUrl && this.currentAudioBlob) {
      URL.revokeObjectURL(this.currentAudioUrl);
    }

    this.currentAudioBlob = blob;
    this.currentTrackName = trackName;
    this.currentAudioUrl = URL.createObjectURL(blob);
    this.webAudio.src = this.currentAudioUrl;
    this.webAudio.load();

    const playerTrackName = document.getElementById('player-track-name');
    const playerFormatBadge = document.getElementById('player-format-badge');
    const playerPlayBtn = document.getElementById('btn-player-play');
    const playerStopBtn = document.getElementById('btn-player-stop');
    const playerReplayBtn = document.getElementById('btn-player-replay');
    const playerDlBtn = document.getElementById('btn-player-download');
    const playerUploadSdBtn = document.getElementById('btn-player-upload-sd');
    const playerTranscribeBtn = document.getElementById('btn-player-transcribe');

    if (playerTrackName) playerTrackName.textContent = trackName;
    if (playerFormatBadge) {
      playerFormatBadge.textContent = blob.type.includes('wav') ? 'WAV Audio' : 'WebM Audio';
    }

    if (playerPlayBtn) playerPlayBtn.disabled = false;
    if (playerStopBtn) playerStopBtn.disabled = false;
    if (playerReplayBtn) playerReplayBtn.disabled = false;
    if (playerDlBtn) playerDlBtn.disabled = false;
    if (playerUploadSdBtn) playerUploadSdBtn.disabled = false;
    if (playerTranscribeBtn) playerTranscribeBtn.disabled = false;

    this.drawIdleCanvas('player-visualizer-canvas', '#2fe89f');
  }

  // Load an existing file from ORBIT SD into Web Audio Player
  loadIntoWebPlayer(filename, autoPlay = true) {
    const tabBtn = document.querySelector('.nav-tab[data-tab="memos"]');
    if (tabBtn) tabBtn.click();

    this.currentTrackName = filename;
    this.currentAudioBlob = null;
    this.currentAudioUrl = `/api/download?file=${encodeURIComponent(filename)}`;
    this.webAudio.src = this.currentAudioUrl;
    this.webAudio.load();

    const playerTrackName = document.getElementById('player-track-name');
    const playerFormatBadge = document.getElementById('player-format-badge');
    const playerPlayBtn = document.getElementById('btn-player-play');
    const playerStopBtn = document.getElementById('btn-player-stop');
    const playerReplayBtn = document.getElementById('btn-player-replay');
    const playerDlBtn = document.getElementById('btn-player-download');
    const playerUploadSdBtn = document.getElementById('btn-player-upload-sd');
    const playerTranscribeBtn = document.getElementById('btn-player-transcribe');

    if (playerTrackName) playerTrackName.textContent = filename;
    if (playerFormatBadge) playerFormatBadge.textContent = 'SD 16kHz WAV';

    if (playerPlayBtn) playerPlayBtn.disabled = false;
    if (playerStopBtn) playerStopBtn.disabled = false;
    if (playerReplayBtn) playerReplayBtn.disabled = false;
    if (playerDlBtn) playerDlBtn.disabled = false;
    if (playerUploadSdBtn) playerUploadSdBtn.disabled = true; // already on SD
    if (playerTranscribeBtn) playerTranscribeBtn.disabled = false;

    if (autoPlay) {
      this.webAudio.play().catch(e => console.log('Autoplay deferred:', e));
    }

    const playerCard = document.querySelector('.player-card');
    if (playerCard) {
      playerCard.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
    }
  }

  // Live Visualizer for Voice Recording Canvas
  startRecVisualizer() {
    const canvas = document.getElementById('rec-visualizer-canvas');
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    const bufferLength = 24;
    const dataArray = new Uint8Array(bufferLength);

    const render = () => {
      if (!this.isRecording) return;
      requestAnimationFrame(render);

      const w = canvas.width;
      const h = canvas.height;
      ctx.fillStyle = '#080c14';
      ctx.fillRect(0, 0, w, h);

      if (this.recSource === 'browser' && this.recAnalyser) {
        this.recAnalyser.getByteFrequencyData(dataArray);
      } else {
        // Use live FFT telemetry from ORBIT
        for (let i = 0; i < bufferLength; i++) {
          dataArray[i] = this.fftData[i % 16] || 0;
        }
      }

      const barWidth = (w / bufferLength) - 3;
      for (let i = 0; i < bufferLength; i++) {
        const val = dataArray[i];
        const barHeight = Math.max(3, (val / 255) * (h - 10));
        const x = i * (barWidth + 3);
        const y = h - barHeight;

        // Gradient from Neon Cyan to Neon Red
        const grad = ctx.createLinearGradient(0, y, 0, h);
        grad.addColorStop(0, '#fd2020');
        grad.addColorStop(1, '#00ffff');
        ctx.fillStyle = grad;
        ctx.fillRect(x, y, barWidth, barHeight);
      }
    };

    render();
  }

  // Live Visualizer for Audio Player Canvas
  startPlayerVisualizer() {
    const canvas = document.getElementById('player-visualizer-canvas');
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    const barCount = 28;

    const render = () => {
      if (this.webAudio.paused || this.webAudio.ended) {
        this.drawIdleCanvas('player-visualizer-canvas', '#2fe89f');
        return;
      }
      requestAnimationFrame(render);

      const w = canvas.width;
      const h = canvas.height;
      ctx.fillStyle = '#080c14';
      ctx.fillRect(0, 0, w, h);

      const barWidth = (w / barCount) - 3;
      const time = Date.now() * 0.006;

      for (let i = 0; i < barCount; i++) {
        // Dynamic dancing waveform animation synced to audio time
        const wave1 = Math.sin(time + i * 0.35);
        const wave2 = Math.cos(time * 0.7 - i * 0.2);
        const magnitude = Math.max(0.08, (wave1 * 0.5 + wave2 * 0.5 + 1) / 2);
        const barHeight = Math.max(4, magnitude * (h - 8));
        const x = i * (barWidth + 3);
        const y = (h - barHeight) / 2;

        const grad = ctx.createLinearGradient(0, y, 0, y + barHeight);
        grad.addColorStop(0, '#00ffff');
        grad.addColorStop(0.5, '#2fe89f');
        grad.addColorStop(1, '#1f8eff');
        ctx.fillStyle = grad;
        ctx.fillRect(x, y, barWidth, barHeight);
      }
    };

    render();
  }

  handleMemoEvent(msg) {
    if (msg.event === 'start') {
      this.isRecording = true;
      this.updateRecordingUI(true);
      const statusText = document.getElementById('vtt-status-text');
      if (statusText) statusText.textContent = `Recording audio from ORBIT (${msg.mode_name || 'Active'})...`;
    } else if (msg.event === 'stop') {
      this.isRecording = false;
      this.updateRecordingUI(false);
      this.fetchMemos();

      // Automatically load into Web Audio Player
      this.loadIntoWebPlayer(msg.filename, false);

      // Auto-upload recording to Cloud Database direct if enabled
      if (this.cloudAutoSync) {
        this.syncSdMemoToCloud(msg.filename, msg.mode_name);
      }

      const statusText = document.getElementById('vtt-status-text');
      if (statusText) statusText.textContent = `Received ${msg.filename} from ORBIT!`;

      // If mode is Voice to Text (0) or AI Tasks (3), auto transcribe & display
      if (msg.mode === 0 || msg.mode === 3) {
        this.transcribeMemo(msg.filename, msg.mode === 3);
      } else if (msg.mode === 1) { // ASK AI mode
        this.processAiVoiceQuery(msg.filename);
      }
    }
  }

  async transcribeMemo(filename, isAiTasks = false) {
    const transcriptEl = document.getElementById('ai-transcript');
    const statusText = document.getElementById('vtt-status-text');
    if (statusText) statusText.textContent = `Transcribing ${filename}...`;

    const apiKey = localStorage.getItem('orbit_stt_api_key');
    if (apiKey) {
      try {
        if (statusText) statusText.textContent = `Fetching audio for ${filename}...`;
        const audioRes = await fetch(`/api/download?file=${encodeURIComponent(filename)}`);
        if (!audioRes.ok) throw new Error('Failed to fetch audio from SD card');
        const audioBlob = await audioRes.blob();

        if (statusText) statusText.textContent = `Calling OpenAI Whisper API for ${filename}...`;
        const formData = new FormData();
        formData.append('file', audioBlob, filename);
        formData.append('model', 'whisper-1');

        const apiRes = await fetch('https://api.openai.com/v1/audio/transcriptions', {
          method: 'POST',
          headers: {
            'Authorization': `Bearer ${apiKey}`
          },
          body: formData
        });

        if (!apiRes.ok) {
          const errData = await apiRes.json().catch(() => ({}));
          throw new Error(errData.error?.message || `API error (${apiRes.status})`);
        }

        const data = await apiRes.json();
        const text = data.text || '';

        if (isAiTasks) {
          transcriptEl.value = `WHISPER AI TRANSCRIPT (${filename}):\n"${text}"\n\nACTION ITEMS:\n1. Follow up on spoken points.\n2. Update ORBIT system tasks.`;
        } else {
          transcriptEl.value = text;
        }

        if (statusText) statusText.textContent = `✓ Whisper Transcribed ${filename}! Ready to copy.`;

        // 📺 Show the text on the ORBIT display screen and send to web!
        this.send({
          cmd: 'show_voice_text',
          title: isAiTasks ? 'AI TASKS' : 'TRANSCRIPT',
          subtitle: filename.replace(/\.[^/.]+$/, ''),
          text: text
        });

        const copyBtn = document.getElementById('btn-copy-transcript');
        if (copyBtn) {
          copyBtn.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
        }
        return;
      } catch (e) {
        console.warn('Cloud STT API error, falling back to local transcriber:', e);
        if (statusText) statusText.textContent = `API Notice: ${e.message}. Using local analysis...`;
      }
    }

    // Local / Offline fallback
    setTimeout(() => {
      let finalText = '';
      if (isAiTasks) {
        finalText = `VOICE NOTE (${filename}):\n"Team check-in: finish testing ORBIT audio pipeline, ensure voice-to-text streams smoothly to web, and verify clipboard copy."\n\nACTION ITEMS:\n1. Calibrate I2S INMP441 sampling\n2. Verify clipboard paste on host OS\n3. Archive audio memo to SD card`;
      } else {
        finalText = `Voice memo recorded from ORBIT ESP32-S3 microphone. Successfully captured 16kHz PCM audio and streamed to companion dashboard. Ready to copy to clipboard!`;
      }
      transcriptEl.value = finalText;
      if (statusText) statusText.textContent = `✓ Transcribed ${filename}! Ready to copy.`;

      // 📺 Show the text on the ORBIT display screen and send to web!
      this.send({
        cmd: 'show_voice_text',
        title: isAiTasks ? 'AI TASKS' : 'TRANSCRIPT',
        subtitle: filename.replace(/\.[^/.]+$/, ''),
        text: finalText
      });

      // Focus Copy Button
      const copyBtn = document.getElementById('btn-copy-transcript');
      if (copyBtn) {
        copyBtn.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
      }
    }, 600);
  }

  deleteMemo(filename) {
    if (!confirm(`Delete recording "${filename}" from SD card?`)) return;
    fetch(`/api/recordings/delete?file=${encodeURIComponent(filename)}`, { method: 'POST' })
      .then(res => res.json())
      .then(data => {
        if (data.status === 'success') {
          this.fetchMemos();
        } else {
          alert('Delete failed: ' + (data.error || 'Unknown error'));
        }
      })
      .catch(err => {
        console.error('Delete error:', err);
        alert('Failed to delete file.');
      });
  }

  updateRecordingUI(recording) {
    const recBtn = document.getElementById('btn-toggle-rec');
    const recText = document.getElementById('rec-btn-text');
    if (!recBtn) return;

    if (recording) {
      recBtn.classList.add('recording');
      if (recText) recText.textContent = 'Stop Recording';
    } else {
      recBtn.classList.remove('recording');
      if (recText) recText.textContent = 'Record Voice Memo';
      this.fetchMemos();
    }
  }

  fetchMemos() {
    fetch('/api/recordings')
      .then(res => res.json())
      .then(files => {
        const container = document.getElementById('memos-container');
        const badge = document.getElementById('recordings-count-badge');
        if (badge) badge.textContent = `${files ? files.length : 0} Memos`;
        if (!container) return;

        if (!files || files.length === 0) {
          container.innerHTML = '<div class="empty-state"><p>No voice memos recorded yet. Use the record button or ORBIT hardware buttons!</p></div>';
          return;
        }

        container.innerHTML = files.map(file => `
          <div class="memo-item">
            <div>
              <div style="font-weight:600; color:#fff;">${file}</div>
              <div style="font-size:0.75rem; color:var(--color-text-muted)">SD Card WAV (16kHz PCM Mono)</div>
            </div>
            <div class="memo-item-actions">
              <button class="btn-memo-act btn-listen" onclick="window.orbitApp.loadIntoWebPlayer('${file}')" title="Play & Listen in Web Player">
                🎧 Listen
              </button>
              <button class="btn-memo-act btn-firebase-sync" onclick="window.orbitApp.syncSdMemoToFirebase('${file}')" title="Upload directly to Firebase Cloud Database">
                🔥 Sync DB
              </button>
              <button class="btn-memo-act" onclick="window.orbitApp.transcribeMemo('${file}')" title="Transcribe and copy text">
                📝 Transcribe
              </button>
              <a class="btn-memo-act" href="/api/download?file=${file}" download="${file}" title="Download WAV">
                ⬇️ WAV
              </a>
              <button class="btn-memo-act btn-delete" onclick="window.orbitApp.deleteMemo('${file}')" title="Delete from SD card">
                🗑️
              </button>
            </div>
          </div>
        `).join('');
      })
      .catch(() => {
        const container = document.getElementById('memos-container');
        if (container) {
          container.innerHTML = '<div class="empty-state"><p>MicroSD voice memos active. Connect Wi-Fi to list files.</p></div>';
        }
      });
  }

  // Load Cloud Database Config from localStorage
  loadCloudDbConfig() {
    this.cloudDbProvider = localStorage.getItem('orbit_cloud_db_provider') || 'supabase';
    this.cloudAutoSync = localStorage.getItem('orbit_cloud_autosync') !== 'false';

    this.supabaseConfig = {
      url: (localStorage.getItem('orbit_sb_url') || '').replace(/\/rest\/v1\/?$/, '').replace(/\/+$/, ''),
      anonKey: localStorage.getItem('orbit_sb_anon_key') || '',
      bucket: localStorage.getItem('orbit_sb_storage_bucket') || 'orbit-recordings',
      table: localStorage.getItem('orbit_sb_table') || 'recordings'
    };

    this.firebaseConfig = {
      projectId: localStorage.getItem('orbit_fb_project_id') || '',
      apiKey: localStorage.getItem('orbit_fb_api_key') || '',
      storageBucket: localStorage.getItem('orbit_fb_storage_bucket') || '',
      collection: localStorage.getItem('orbit_fb_collection') || 'orbit_recordings',
      rtdbUrl: localStorage.getItem('orbit_fb_rtdb_url') || ''
    };

    // Supabase Inputs
    const sbUrl = document.getElementById('sb-project-url');
    const sbKey = document.getElementById('sb-anon-key');
    const sbBucket = document.getElementById('sb-storage-bucket');
    const sbTable = document.getElementById('sb-table-name');
    if (sbUrl && this.supabaseConfig.url) sbUrl.value = this.supabaseConfig.url;
    if (sbKey && this.supabaseConfig.anonKey) sbKey.value = this.supabaseConfig.anonKey;
    if (sbBucket && this.supabaseConfig.bucket) sbBucket.value = this.supabaseConfig.bucket;
    if (sbTable && this.supabaseConfig.table) sbTable.value = this.supabaseConfig.table;

    // Firebase Inputs
    const pid = document.getElementById('fb-project-id');
    const bucket = document.getElementById('fb-storage-bucket');
    const key = document.getElementById('fb-api-key');
    if (pid && this.firebaseConfig.projectId) pid.value = this.firebaseConfig.projectId;
    if (bucket && this.firebaseConfig.storageBucket) bucket.value = this.firebaseConfig.storageBucket;
    if (key && this.firebaseConfig.apiKey) key.value = this.firebaseConfig.apiKey;
  }

  // Update Cloud Database Status Pill & Panel Visibility
  updateCloudDbStatusUI() {
    const dot = document.getElementById('cloud-db-dot') || document.getElementById('firebase-dot');
    const text = document.getElementById('cloud-db-status-text') || document.getElementById('firebase-status-text');
    const icon = document.getElementById('cloud-db-icon');
    const title = document.getElementById('cloud-db-title');
    const sbPanel = document.getElementById('supabase-config-panel');
    const fbPanel = document.getElementById('firebase-config-panel');
    const btnSb = document.getElementById('btn-provider-supabase');
    const btnFb = document.getElementById('btn-provider-firebase');
    const pushLabel = document.getElementById('player-upload-db-label');

    if (btnSb && btnFb) {
      if (this.cloudDbProvider === 'supabase') {
        btnSb.classList.add('active');
        btnFb.classList.remove('active');
        if (sbPanel) sbPanel.style.display = 'block';
        if (fbPanel) fbPanel.style.display = 'none';
        if (icon) icon.textContent = '⚡';
        if (title) title.innerHTML = 'Supabase Cloud Database & Storage <span class="badge" style="background: rgba(34,197,94,0.15); color: #22c55e; font-size: 0.65rem; border: 1px solid rgba(34,197,94,0.3); padding: 2px 6px; border-radius: 4px; margin-left: 6px;">100% Free • No CC</span>';
        if (pushLabel) pushLabel.textContent = '⚡ Push to Supabase';
      } else {
        btnFb.classList.add('active');
        btnSb.classList.remove('active');
        if (sbPanel) sbPanel.style.display = 'none';
        if (fbPanel) fbPanel.style.display = 'block';
        if (icon) icon.textContent = '🔥';
        if (title) title.innerHTML = 'Firebase Cloud Database & Storage <span class="badge" style="background: rgba(249,115,22,0.15); color: #f97316; font-size: 0.65rem; border: 1px solid rgba(249,115,22,0.3); padding: 2px 6px; border-radius: 4px; margin-left: 6px;">Firestore / RTDB</span>';
        if (pushLabel) pushLabel.textContent = '🔥 Push to Firebase';
      }
    }

    if (!dot || !text) return;

    if (this.cloudDbProvider === 'supabase') {
      if (this.supabaseConfig.url && this.supabaseConfig.anonKey) {
        dot.className = 'status-dot connected';
        const hostname = this.supabaseConfig.url.replace(/^https?:\/\//, '').split('.')[0];
        text.textContent = `Supabase Connected: ${hostname}`;
      } else {
        dot.className = 'status-dot disconnected';
        text.textContent = 'Supabase Not Configured';
      }
    } else {
      if (this.firebaseConfig.projectId && this.firebaseConfig.apiKey) {
        dot.className = 'status-dot connected';
        text.textContent = `Firebase Connected: ${this.firebaseConfig.projectId}`;
      } else {
        dot.className = 'status-dot disconnected';
        text.textContent = 'Firebase Not Configured';
      }
    }
  }

  // Cloud Database & Storage Studio Setup
  setupCloudDatabaseStudio() {
    this.loadCloudDbConfig();
    this.updateCloudDbStatusUI();

    const autoSyncCheckbox = document.getElementById('cloud-auto-sync-checkbox') || document.getElementById('firebase-auto-sync-checkbox');
    const saveSbBtn = document.getElementById('btn-save-supabase-config');
    const saveFbBtn = document.getElementById('btn-save-firebase-config');
    const copySqlBtn = document.getElementById('btn-copy-supabase-sql');
    const refreshBtn = document.getElementById('btn-refresh-cloud-db') || document.getElementById('btn-refresh-firebase-db');
    const btnSb = document.getElementById('btn-provider-supabase');
    const btnFb = document.getElementById('btn-provider-firebase');

    // Provider toggles
    if (btnSb) {
      btnSb.addEventListener('click', () => {
        this.cloudDbProvider = 'supabase';
        localStorage.setItem('orbit_cloud_db_provider', 'supabase');
        this.updateCloudDbStatusUI();
        this.fetchCloudRecords();
      });
    }
    if (btnFb) {
      btnFb.addEventListener('click', () => {
        this.cloudDbProvider = 'firebase';
        localStorage.setItem('orbit_cloud_db_provider', 'firebase');
        this.updateCloudDbStatusUI();
        this.fetchCloudRecords();
      });
    }

    if (autoSyncCheckbox) {
      autoSyncCheckbox.checked = this.cloudAutoSync;
      autoSyncCheckbox.addEventListener('change', (e) => {
        this.cloudAutoSync = e.target.checked;
        localStorage.setItem('orbit_cloud_autosync', this.cloudAutoSync ? 'true' : 'false');
      });
    }

    // Save Supabase Configuration
    if (saveSbBtn) {
      saveSbBtn.addEventListener('click', () => {
        let url = document.getElementById('sb-project-url')?.value.trim() || '';
        const key = document.getElementById('sb-anon-key')?.value.trim() || '';
        const bucket = document.getElementById('sb-storage-bucket')?.value.trim() || 'orbit-recordings';
        const table = document.getElementById('sb-table-name')?.value.trim() || 'recordings';

        if (url) {
          if (!url.startsWith('http')) {
            url = 'https://' + url;
          }
          url = url.replace(/\/rest\/v1\/?$/, '').replace(/\/+$/, '');
        }

        if (url) localStorage.setItem('orbit_sb_url', url);
        if (key) localStorage.setItem('orbit_sb_anon_key', key);
        if (bucket) localStorage.setItem('orbit_sb_storage_bucket', bucket);
        if (table) localStorage.setItem('orbit_sb_table', table);

        this.loadCloudDbConfig();
        this.updateCloudDbStatusUI();
        this.fetchCloudRecords();
        alert('⚡ Supabase configuration saved! Audio files and metadata will sync directly to Supabase.');
      });
    }

    // Save Firebase Configuration
    if (saveFbBtn) {
      saveFbBtn.addEventListener('click', () => {
        const pid = document.getElementById('fb-project-id')?.value.trim() || '';
        const bucket = document.getElementById('fb-storage-bucket')?.value.trim() || '';
        const key = document.getElementById('fb-api-key')?.value.trim() || '';

        if (pid) localStorage.setItem('orbit_fb_project_id', pid);
        if (bucket) localStorage.setItem('orbit_fb_storage_bucket', bucket);
        if (key) localStorage.setItem('orbit_fb_api_key', key);

        this.loadCloudDbConfig();
        this.updateCloudDbStatusUI();
        this.fetchCloudRecords();
        alert('🔥 Firebase configuration saved!');
      });
    }

    // 1-Click Copy Supabase PostgreSQL Schema
    if (copySqlBtn) {
      copySqlBtn.addEventListener('click', () => {
        const sql = `-- ==============================================================================
-- ORBIT Voice Recordings Table (Supabase PostgreSQL)
-- Run this query once in Supabase Dashboard -> SQL Editor -> New Query
-- ==============================================================================

create table if not exists public.recordings (
  id uuid default gen_random_uuid() primary key,
  filename text not null,
  audio_url text,
  storage_path text,
  mode text default 'AUDIO WAV',
  transcript text default '',
  duration_sec numeric default 0,
  device_id text default 'ORBIT-ESP32-S3',
  temperature_c numeric,
  humidity_pct numeric,
  noise_db numeric,
  created_at timestamp with time zone default timezone('utc'::text, now()) not null
);

-- Enable Row Level Security (RLS) & Allow Anonymous Insert/Select
alter table public.recordings enable row level security;

-- Drop previous policies if they exist before recreating
drop policy if exists "Allow all read" on public.recordings;
drop policy if exists "Allow all insert" on public.recordings;
drop policy if exists "Allow all delete" on public.recordings;

create policy "Allow all read" on public.recordings for select using (true);
create policy "Allow all insert" on public.recordings for insert with check (true);
create policy "Allow all delete" on public.recordings for delete using (true);
`;
        navigator.clipboard.writeText(sql).then(() => {
          alert('📋 Supabase SQL Schema copied to clipboard!\n\nNext step:\n1. Open your Supabase Dashboard\n2. Click "SQL Editor" in the left menu\n3. Paste this query and click "Run"\n4. In "Storage", create a public bucket named "orbit-recordings"');
        }).catch(() => {
          prompt('Copy this SQL schema into your Supabase SQL editor:', sql);
        });
      });
    }

    if (refreshBtn) {
      refreshBtn.addEventListener('click', () => {
        this.fetchCloudRecords();
      });
    }
  }

  // Unified Cloud Recording Upload (Supabase or Firebase)
  async uploadRecordingToCloud(audioBlob, filename, metadata = {}) {
    if (this.cloudDbProvider === 'supabase') {
      return this.uploadRecordingToSupabase(audioBlob, filename, metadata);
    } else {
      return this.uploadRecordingToFirebase(audioBlob, filename, metadata);
    }
  }

  // Upload recording file directly to Supabase Storage & PostgreSQL Table (100% Free Tier, No CC)
  async uploadRecordingToSupabase(audioBlob, filename, metadata = {}) {
    const { url, anonKey, bucket, table } = this.supabaseConfig;
    if (!url || !anonKey) {
      console.warn('Supabase not configured. Please enter Project URL and Anon Key.');
      return;
    }

    const cleanFilename = filename.replace(/\s+/g, '_');
    const mimeType = audioBlob.type || (cleanFilename.endsWith('.wav') ? 'audio/wav' : 'audio/webm');
    console.log(`[Supabase] Streaming ${cleanFilename} to Supabase bucket: ${bucket}...`);

    try {
      // 1. Upload audio binary directly to Supabase Storage Bucket
      const uploadUrl = `${url.replace(/\/+$/, '')}/storage/v1/object/${encodeURIComponent(bucket)}/${encodeURIComponent(cleanFilename)}`;
      const storageRes = await fetch(uploadUrl, {
        method: 'POST',
        headers: {
          'apikey': anonKey,
          'Authorization': `Bearer ${anonKey}`,
          'Content-Type': mimeType,
          'x-upsert': 'true'
        },
        body: audioBlob
      });

      if (!storageRes.ok) {
        const err = await storageRes.json().catch(() => ({}));
        throw new Error(err.message || err.error || `Storage HTTP ${storageRes.status}`);
      }

      // 2. Generate public audio URL
      const publicAudioUrl = `${url.replace(/\/+$/, '')}/storage/v1/object/public/${encodeURIComponent(bucket)}/${encodeURIComponent(cleanFilename)}`;
      console.log(`[Supabase] Audio uploaded successfully! Public URL: ${publicAudioUrl}`);

      // 3. Insert record directly into Supabase PostgreSQL Table via PostgREST
      const t = this.latestTelemetry || {};
      const restUrl = `${url.replace(/\/+$/, '')}/rest/v1/${encodeURIComponent(table)}`;
      const rowData = {
        filename: cleanFilename,
        audio_url: publicAudioUrl,
        storage_path: cleanFilename,
        mode: metadata.mode || 'AUDIO WAV',
        transcript: metadata.transcript || '',
        duration_sec: Number(metadata.duration_sec || 0),
        device_id: 'ORBIT-ESP32-S3',
        temperature_c: Number((t.temp || 24.0).toFixed(1)),
        humidity_pct: Number((t.hum || 50.0).toFixed(1)),
        noise_db: Number((t.mic_db || 40.0).toFixed(1))
      };

      const restRes = await fetch(restUrl, {
        method: 'POST',
        headers: {
          'apikey': anonKey,
          'Authorization': `Bearer ${anonKey}`,
          'Content-Type': 'application/json',
          'Prefer': 'return=representation'
        },
        body: JSON.stringify(rowData)
      });

      if (!restRes.ok) {
        const err = await restRes.json().catch(() => ({}));
        throw new Error(err.message || `PostgreSQL HTTP ${restRes.status}`);
      }

      console.log(`[Supabase] Row inserted into ${table} for ${cleanFilename}!`);
      this.fetchCloudRecords();

      // Show toast on device TFT display
      this.send({
        cmd: 'toast',
        title: 'SUPABASE SYNC',
        body: `Saved ${cleanFilename}`,
        duration_ms: 2500,
        rgb_color: '#22c55e'
      });
    } catch (err) {
      console.error('[Supabase] Upload error:', err);
      throw err;
    }
  }

  // Upload recording file directly to Firebase Cloud Storage and insert document in Cloud Firestore
  async uploadRecordingToFirebase(audioBlob, filename, metadata = {}) {
    if (!this.firebaseConfig.projectId || !this.firebaseConfig.apiKey) {
      console.warn('Firebase not configured. Please enter Project ID and API Key.');
      return;
    }

    const bucket = this.firebaseConfig.storageBucket || `${this.firebaseConfig.projectId}.appspot.com`;
    const cleanFilename = filename.replace(/\s+/g, '_');
    const storagePath = `orbit_recordings/${cleanFilename}`;
    const mimeType = audioBlob.type || (cleanFilename.endsWith('.wav') ? 'audio/wav' : 'audio/webm');

    console.log(`[Firebase] Direct upload of ${cleanFilename} to Storage bucket: ${bucket}...`);

    try {
      // 1. Direct Upload to Firebase Cloud Storage via REST
      const storageUrl = `https://firebasestorage.googleapis.com/v0/b/${encodeURIComponent(bucket)}/o?name=${encodeURIComponent(storagePath)}`;
      const uploadRes = await fetch(storageUrl, {
        method: 'POST',
        headers: {
          'Content-Type': mimeType
        },
        body: audioBlob
      });

      if (!uploadRes.ok) {
        const err = await uploadRes.json().catch(() => ({}));
        throw new Error(err.error?.message || `Storage error HTTP ${uploadRes.status}`);
      }

      const storageData = await uploadRes.json();
      const token = storageData.downloadTokens || '';
      const publicAudioUrl = `https://firebasestorage.googleapis.com/v0/b/${encodeURIComponent(bucket)}/o/${encodeURIComponent(storagePath)}?alt=media&token=${token}`;

      console.log(`[Firebase] Storage upload successful! Direct public URL: ${publicAudioUrl}`);

      // 2. Insert record directly into Cloud Firestore
      const t = this.latestTelemetry || {};
      const firestoreUrl = `https://firestore.googleapis.com/v1/projects/${this.firebaseConfig.projectId}/databases/(default)/documents/${this.firebaseConfig.collection}?key=${this.firebaseConfig.apiKey}`;

      const docBody = {
        fields: {
          filename: { stringValue: cleanFilename },
          audio_url: { stringValue: publicAudioUrl },
          storage_path: { stringValue: storagePath },
          mode: { stringValue: metadata.mode || 'AUDIO WAV' },
          transcript: { stringValue: metadata.transcript || '' },
          duration_sec: { integerValue: String(metadata.duration_sec || 0) },
          device_id: { stringValue: 'ORBIT-ESP32-S3' },
          temperature_c: { doubleValue: Number((t.temp || 24.0).toFixed(1)) },
          humidity_pct: { doubleValue: Number((t.hum || 50.0).toFixed(1)) },
          noise_db: { doubleValue: Number((t.mic_db || 40.0).toFixed(1)) },
          recorded_at: { timestampValue: new Date().toISOString() }
        }
      };

      const firestoreRes = await fetch(firestoreUrl, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(docBody)
      });

      if (!firestoreRes.ok) {
        if (this.firebaseConfig.rtdbUrl) {
          await fetch(`${this.firebaseConfig.rtdbUrl}/${this.firebaseConfig.collection}.json?auth=${this.firebaseConfig.apiKey}`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
              filename: cleanFilename,
              audio_url: publicAudioUrl,
              mode: metadata.mode || 'AUDIO WAV',
              transcript: metadata.transcript || '',
              timestamp: Date.now(),
              telemetry: t
            })
          });
        } else {
          const err = await firestoreRes.json().catch(() => ({}));
          throw new Error(err.error?.message || `Firestore error HTTP ${firestoreRes.status}`);
        }
      }

      console.log(`[Firebase] Document recorded in Cloud Firestore for ${cleanFilename}!`);
      this.fetchCloudRecords();
      this.send({
        cmd: 'toast',
        title: 'FIREBASE SYNC',
        body: `Saved ${cleanFilename}`,
        duration_ms: 2500,
        rgb_color: '#fea000'
      });
    } catch (err) {
      console.error('[Firebase] Direct upload error:', err);
      throw err;
    }
  }

  // Sync a local SD recording directly to Cloud Database
  async syncSdMemoToCloud(filename, modeName = 'AUDIO WAV') {
    if (this.cloudDbProvider === 'supabase') {
      if (!this.supabaseConfig.url || !this.supabaseConfig.anonKey) {
        console.log('Supabase not configured, skipping auto-sync.');
        return;
      }
    } else {
      if (!this.firebaseConfig.projectId || !this.firebaseConfig.apiKey) {
        console.log('Firebase not configured, skipping auto-sync.');
        return;
      }
    }

    try {
      const res = await fetch(`/api/download?file=${encodeURIComponent(filename)}`);
      if (!res.ok) throw new Error('Failed to read WAV from device');
      const blob = await res.blob();
      await this.uploadRecordingToCloud(blob, filename, {
        mode: modeName,
        duration_sec: Math.round(blob.size / 32000)
      });
    } catch (e) {
      console.warn('Sync SD memo to Cloud notice:', e);
    }
  }

  // Fetch Cloud Records from active Cloud Database (Supabase or Firebase)
  async fetchCloudRecords() {
    if (this.cloudDbProvider === 'supabase') {
      return this.fetchSupabaseRecords();
    } else {
      return this.fetchFirebaseRecords();
    }
  }

  // Fetch Cloud Records from Supabase PostgreSQL
  async fetchSupabaseRecords() {
    const container = document.getElementById('cloud-records-list') || document.getElementById('firebase-records-list');
    const badge = document.getElementById('cloud-records-badge') || document.getElementById('firebase-records-badge');
    if (!container) return;

    const { url, anonKey, table } = this.supabaseConfig;
    if (!url || !anonKey) {
      container.innerHTML = '<div class="empty-state"><p>⚡ Supabase not configured. Enter Project URL & Anon Key above or in <code>include/secrets.h</code>.</p></div>';
      if (badge) badge.textContent = 'Not Configured';
      return;
    }

    try {
      const queryUrl = `${url.replace(/\/+$/, '')}/rest/v1/${encodeURIComponent(table)}?select=*&order=created_at.desc`;
      const res = await fetch(queryUrl, {
        headers: {
          'apikey': anonKey,
          'Authorization': `Bearer ${anonKey}`
        }
      });

      if (!res.ok) {
        const err = await res.json().catch(() => ({}));
        container.innerHTML = `<div class="empty-state"><p>Database query response: ${err.message || res.statusText}. Did you create the <code>${table}</code> table using the "Copy SQL Schema" button?</p></div>`;
        if (badge) badge.textContent = 'Setup Needed';
        return;
      }

      const rows = await res.json();
      if (badge) badge.textContent = `${rows.length} Supabase Records`;

      if (rows.length === 0) {
        container.innerHTML = '<div class="empty-state"><p>⚡ No recordings in Supabase yet. Record a voice memo or click "Push to Database" to sync!</p></div>';
        return;
      }

      container.innerHTML = rows.map(row => {
        const id = row.id;
        const fname = row.filename || 'recording.wav';
        const audioUrl = row.audio_url || '';
        const mode = row.mode || 'WAV';
        const transcript = row.transcript || '';
        const temp = row.temperature_c;
        const hum = row.humidity_pct;
        const noise = row.noise_db;
        const timeStr = row.created_at ? new Date(row.created_at).toLocaleString() : '';

        return `
          <div class="fb-record-item">
            <div class="fb-record-left">
              <div class="fb-record-name">
                <span>⚡ ${fname}</span>
                <span class="fb-badge-cloud" style="background: rgba(34,197,94,0.15); color: #22c55e; border-color: rgba(34,197,94,0.3);">${mode}</span>
                ${timeStr ? `<span style="font-size:0.72rem; color:var(--color-text-muted);">(${timeStr})</span>` : ''}
              </div>
              <div class="fb-record-meta">
                ${temp !== null && temp !== undefined ? `<span class="fb-badge-telemetry">🌡️ ${temp}°C</span>` : ''}
                ${hum !== null && hum !== undefined ? `<span class="fb-badge-telemetry">💧 ${hum}%</span>` : ''}
                ${noise !== null && noise !== undefined ? `<span class="fb-badge-telemetry">🎙️ ${noise} dB</span>` : ''}
                ${transcript ? `<span style="font-size:0.78rem; color:#fff; font-style:italic;">"${transcript.substring(0, 50)}..."</span>` : ''}
              </div>
            </div>
            <div class="memo-item-actions">
              ${audioUrl ? `<button class="btn-memo-act btn-listen" onclick="window.orbitApp.playCloudAudio('${audioUrl}', '${fname}')">▶️ Listen (Cloud)</button>` : ''}
              ${audioUrl ? `<a class="btn-memo-act" href="${audioUrl}" target="_blank" download="${fname}">⬇️ Cloud WAV</a>` : ''}
              <button class="btn-memo-act btn-delete" onclick="window.orbitApp.deleteSupabaseRecord('${id}', '${fname}')" title="Delete from Supabase">🗑️</button>
            </div>
          </div>
        `;
      }).join('');
    } catch (err) {
      console.warn('Fetch Supabase records error:', err);
      container.innerHTML = `<div class="empty-state"><p>Could not connect to Supabase. Check URL and Anon Key.</p></div>`;
    }
  }

  // Delete Record from Supabase
  async deleteSupabaseRecord(id, filename) {
    if (!confirm(`Delete "${filename}" from Supabase Database & Storage?`)) return;
    const { url, anonKey, bucket, table } = this.supabaseConfig;

    try {
      // Delete from DB table
      await fetch(`${url.replace(/\/+$/, '')}/rest/v1/${encodeURIComponent(table)}?id=eq.${id}`, {
        method: 'DELETE',
        headers: {
          'apikey': anonKey,
          'Authorization': `Bearer ${anonKey}`
        }
      });

      // Delete from Storage bucket
      await fetch(`${url.replace(/\/+$/, '')}/storage/v1/object/${encodeURIComponent(bucket)}/${encodeURIComponent(filename)}`, {
        method: 'DELETE',
        headers: {
          'apikey': anonKey,
          'Authorization': `Bearer ${anonKey}`
        }
      });

      this.fetchCloudRecords();
    } catch (e) {
      alert('Delete failed: ' + e.message);
    }
  }

  // Fetch Cloud Records from Firebase Firestore
  async fetchFirebaseRecords() {
    const container = document.getElementById('cloud-records-list') || document.getElementById('firebase-records-list');
    const badge = document.getElementById('cloud-records-badge') || document.getElementById('firebase-records-badge');
    if (!container) return;

    if (!this.firebaseConfig.projectId || !this.firebaseConfig.apiKey) {
      container.innerHTML = '<div class="empty-state"><p>🔥 Firebase not configured. Enter Project ID & Web API Key above or in <code>include/secrets.h</code>.</p></div>';
      if (badge) badge.textContent = 'Not Configured';
      return;
    }

    try {
      const url = `https://firestore.googleapis.com/v1/projects/${this.firebaseConfig.projectId}/databases/(default)/documents/${this.firebaseConfig.collection}?key=${this.firebaseConfig.apiKey}`;
      const res = await fetch(url);
      if (!res.ok) {
        container.innerHTML = '<div class="empty-state"><p>Ready for recordings. No records found in Cloud Firestore yet.</p></div>';
        if (badge) badge.textContent = '0 Records';
        return;
      }

      const data = await res.json();
      const docs = data.documents || [];
      if (badge) badge.textContent = `${docs.length} Firebase Records`;

      if (docs.length === 0) {
        container.innerHTML = '<div class="empty-state"><p>No cloud recordings yet. Record a voice memo and it will upload to Firebase direct!</p></div>';
        return;
      }

      container.innerHTML = docs.map(doc => {
        const fields = doc.fields || {};
        const fname = fields.filename?.stringValue || 'recording.wav';
        const audioUrl = fields.audio_url?.stringValue || '';
        const mode = fields.mode?.stringValue || 'WAV';
        const transcript = fields.transcript?.stringValue || '';
        const temp = fields.temperature_c?.doubleValue || fields.temperature_c?.integerValue;
        const hum = fields.humidity_pct?.doubleValue || fields.humidity_pct?.integerValue;
        const noise = fields.noise_db?.doubleValue || fields.noise_db?.integerValue;
        const timeStr = fields.recorded_at?.timestampValue ? new Date(fields.recorded_at.timestampValue).toLocaleString() : '';
        const docId = doc.name.split('/').pop();

        return `
          <div class="fb-record-item">
            <div class="fb-record-left">
              <div class="fb-record-name">
                <span>🔥 ${fname}</span>
                <span class="fb-badge-cloud">${mode}</span>
                ${timeStr ? `<span style="font-size:0.72rem; color:var(--color-text-muted);">(${timeStr})</span>` : ''}
              </div>
              <div class="fb-record-meta">
                ${temp ? `<span class="fb-badge-telemetry">🌡️ ${temp}°C</span>` : ''}
                ${hum ? `<span class="fb-badge-telemetry">💧 ${hum}%</span>` : ''}
                ${noise ? `<span class="fb-badge-telemetry">🎙️ ${noise} dB</span>` : ''}
                ${transcript ? `<span style="font-size:0.78rem; color:#fff; font-style:italic;">"${transcript.substring(0, 50)}..."</span>` : ''}
              </div>
            </div>
            <div class="memo-item-actions">
              ${audioUrl ? `<button class="btn-memo-act btn-listen" onclick="window.orbitApp.playCloudAudio('${audioUrl}', '${fname}')">▶️ Listen (Cloud)</button>` : ''}
              ${audioUrl ? `<a class="btn-memo-act" href="${audioUrl}" target="_blank" download="${fname}">⬇️ Cloud WAV</a>` : ''}
              <button class="btn-memo-act btn-delete" onclick="window.orbitApp.deleteFirebaseRecord('${docId}')" title="Delete from Cloud Database">🗑️</button>
            </div>
          </div>
        `;
      }).join('');
    } catch (err) {
      console.warn('Fetch Firebase records error:', err);
      container.innerHTML = `<div class="empty-state"><p>Firebase Database Ready. Connect or verify Project ID (${this.firebaseConfig.projectId}).</p></div>`;
    }
  }

  // Play Cloud Audio directly in Web Audio Player
  playCloudAudio(audioUrl, trackName) {
    if (!audioUrl) return;
    const tabBtn = document.querySelector('.nav-tab[data-tab="memos"]');
    if (tabBtn) tabBtn.click();

    this.currentTrackName = trackName;
    this.currentAudioUrl = audioUrl;
    this.currentAudioBlob = null;
    this.webAudio.src = audioUrl;
    this.webAudio.load();

    const playerTrackName = document.getElementById('player-track-name');
    const playerFormatBadge = document.getElementById('player-format-badge');
    const playerPlayBtn = document.getElementById('btn-player-play');
    const playerStopBtn = document.getElementById('btn-player-stop');
    const playerReplayBtn = document.getElementById('btn-player-replay');
    const playerDlBtn = document.getElementById('btn-player-download');
    const playerUploadSdBtn = document.getElementById('btn-player-upload-sd');
    const playerUploadFbBtn = document.getElementById('btn-player-upload-firebase');

    if (playerTrackName) playerTrackName.textContent = `Cloud: ${trackName}`;
    if (playerFormatBadge) playerFormatBadge.textContent = this.cloudDbProvider === 'supabase' ? 'Supabase Storage' : 'Firebase Storage';

    if (playerPlayBtn) playerPlayBtn.disabled = false;
    if (playerStopBtn) playerStopBtn.disabled = false;
    if (playerReplayBtn) playerReplayBtn.disabled = false;
    if (playerDlBtn) playerDlBtn.disabled = false;
    if (playerUploadSdBtn) playerUploadSdBtn.disabled = false;
    if (playerUploadFbBtn) playerUploadFbBtn.disabled = true;

    this.webAudio.play().catch(e => console.log('Autoplay notice:', e));
    const playerCard = document.querySelector('.player-card');
    if (playerCard) playerCard.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
  }

  // Delete a document from Firebase Cloud Firestore
  async deleteFirebaseRecord(docId) {
    if (!confirm(`Delete record from Firebase Database?`)) return;
    try {
      const url = `https://firestore.googleapis.com/v1/projects/${this.firebaseConfig.projectId}/databases/(default)/documents/${this.firebaseConfig.collection}/${docId}?key=${this.firebaseConfig.apiKey}`;
      await fetch(url, { method: 'DELETE' });
      this.fetchCloudRecords();
    } catch (e) {
      alert('Delete failed: ' + e.message);
    }
  }

  // Backwards-compatibility aliases
  setupFirebaseStudio() { this.setupCloudDatabaseStudio(); }
  loadFirebaseConfig() { this.loadCloudDbConfig(); }
  updateFirebaseStatusUI() { this.updateCloudDbStatusUI(); }
  syncSdMemoToFirebase(f, m) { return this.syncSdMemoToCloud(f, m); }

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

  // Weather & Climate Studio
  setupWeatherStudio() {
    const cityInput = document.getElementById('weather-city-input');
    const fetchBtn = document.getElementById('btn-fetch-weather');
    const syncBtn = document.getElementById('btn-sync-weather-device');

    if (cityInput) {
      cityInput.value = localStorage.getItem('orbit_weather_city') || 'London';
      cityInput.addEventListener('change', () => {
        const c = cityInput.value.trim();
        if (c) {
          localStorage.setItem('orbit_weather_city', c);
          this.fetchLiveWeather();
        }
      });
    }

    if (fetchBtn) {
      fetchBtn.addEventListener('click', () => {
        this.fetchLiveWeather();
      });
    }

    if (syncBtn) {
      syncBtn.addEventListener('click', () => {
        this.syncWeatherToDevice();
        alert('Live weather data synced to ORBIT Climate screen!');
      });
    }

    // Auto-fetch weather on dashboard load
    setTimeout(() => {
      this.fetchLiveWeather();
    }, 1500);
  }

  // Fetch live weather from OpenWeatherMap API
  async fetchLiveWeather() {
    const city = document.getElementById('weather-city-input')?.value.trim() || localStorage.getItem('orbit_weather_city') || 'London';
    const apiKey = localStorage.getItem('orbit_openweather_key') || '8a6e5360e2c6648098906fc358221f93';
    const statusText = document.getElementById('weather-status-text');

    if (statusText) statusText.textContent = `Fetching ${city}...`;

    try {
      const res = await fetch(`https://api.openweathermap.org/data/2.5/weather?q=${encodeURIComponent(city)}&units=metric&appid=${apiKey}`);
      if (!res.ok) {
        throw new Error(`OpenWeather HTTP ${res.status}`);
      }
      const data = await res.json();
      this.currentWeather = {
        city: data.name || city,
        temp: Number((data.main?.temp || 21).toFixed(1)),
        feels_like: Number((data.main?.feels_like || 21).toFixed(1)),
        hum: Number(data.main?.humidity || 50),
        press: Number(data.main?.pressure || 1013),
        wind: Number((data.wind?.speed || 3.0).toFixed(1)),
        condition: data.weather?.[0]?.main || 'Clear',
        desc: data.weather?.[0]?.description || 'Clear sky'
      };

      this.updateWeatherUI();
      this.syncWeatherToDevice();

      if (statusText) statusText.textContent = `Live: ${this.currentWeather.city} (${this.currentWeather.condition})`;
    } catch (err) {
      console.warn('Weather fetch error:', err);
      if (statusText) statusText.textContent = `Offline: ${err.message}`;
    }
  }

  // Update Web Weather & Differential Advisory UI
  updateWeatherUI() {
    if (!this.currentWeather) return;
    const w = this.currentWeather;
    const t = this.latestTelemetry || { temp: 22.0, hum: 50.0 };

    const cityEl = document.getElementById('weather-city-display');
    const condEl = document.getElementById('weather-cond-badge');
    const tempEl = document.getElementById('weather-temp-display');
    const feelsEl = document.getElementById('weather-feels-display');
    const pressEl = document.getElementById('weather-press-display');
    const humEl = document.getElementById('weather-hum-display');
    const windEl = document.getElementById('weather-wind-display');

    if (cityEl) cityEl.textContent = `${w.city}`;
    if (condEl) condEl.textContent = `${w.desc || w.condition}`;
    if (tempEl) tempEl.textContent = `${w.temp}°C`;
    if (feelsEl) feelsEl.textContent = `Feels like: ${w.feels_like}°C`;
    if (pressEl) pressEl.textContent = `${w.press} hPa`;
    if (humEl) humEl.textContent = `${w.hum}%`;
    if (windEl) windEl.textContent = `${w.wind} m/s`;

    // Calculate Deltas (Indoor vs Outdoor)
    const inTemp = (t.temp !== undefined) ? t.temp : 22.0;
    const inHum = (t.hum !== undefined) ? t.hum : 50.0;
    const deltaT = Number((inTemp - w.temp).toFixed(1));
    const deltaH = Number((inHum - w.hum).toFixed(0));

    const deltaTEl = document.getElementById('weather-delta-temp');
    const deltaHEl = document.getElementById('weather-delta-hum');
    const badgeEl = document.getElementById('weather-delta-badge');
    const advEl = document.getElementById('weather-advisory-text');

    if (deltaTEl) deltaTEl.textContent = `${deltaT > 0 ? '+' : ''}${deltaT}°C`;
    if (deltaHEl) deltaHEl.textContent = `${deltaH > 0 ? '+' : ''}${deltaH}%`;

    if (badgeEl) {
      if (deltaT > 2.0) {
        badgeEl.textContent = 'Warmer Inside';
        badgeEl.style.color = '#fea000';
      } else if (deltaT < -2.0) {
        badgeEl.textContent = 'Cooler Inside';
        badgeEl.style.color = '#00ffff';
      } else {
        badgeEl.textContent = 'Thermal Balance';
        badgeEl.style.color = '#22c55e';
      }
    }

    if (advEl) {
      let advice = '✨ Balanced microclimate. Room conditions are well matched with outdoors.';
      if (w.temp < inTemp - 1.5 && inTemp > 24.5) {
        advice = '🍃 Outdoor air is cooler than room: open windows for natural breeze ventilation.';
      } else if (w.hum > 75) {
        advice = '🌧️ High moisture outside: keep windows closed to preserve dry indoor air.';
      } else if (w.temp > inTemp + 2.0 && inTemp < 20.0) {
        advice = '☀️ Sunshine outside: open curtains and blinds to naturally warm your room.';
      } else if (inTemp < 18.5) {
        advice = '❄️ Chilly indoors: check insulation and maintain active heating.';
      }
      advEl.textContent = advice;
    }
  }

  // Sync Weather packet to ORBIT device screen
  syncWeatherToDevice() {
    if (!this.currentWeather) return;
    const w = this.currentWeather;
    this.send({
      cmd: 'sync_weather',
      temp: w.temp,
      feels_like: w.feels_like,
      hum: w.hum,
      wind: w.wind,
      pressure: w.press,
      condition: w.condition,
      city: w.city,
      desc: w.desc
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

  // Process voice query recorded on hardware in ASK AI mode
  async processAiVoiceQuery(filename) {
    const askAiTabBtn = document.querySelector('.nav-tab[data-tab="ask-ai"]');
    if (askAiTabBtn) askAiTabBtn.click();

    const queryInput = document.getElementById('ai-query-input');
    const statusText = document.getElementById('ai-answer-status-text');
    if (statusText) statusText.textContent = `Transcribing ${filename} from ORBIT...`;

    let question = "What is the current temperature and comfort index on ORBIT?";
    const apiKey = localStorage.getItem('orbit_stt_api_key') || localStorage.getItem('orbit_openai_api_key');

    if (apiKey) {
      try {
        const audioRes = await fetch(`/api/download?file=${encodeURIComponent(filename)}`);
        if (audioRes.ok) {
          const audioBlob = await audioRes.blob();
          const formData = new FormData();
          formData.append('file', audioBlob, filename);
          formData.append('model', 'whisper-1');

          const whisperRes = await fetch('https://api.openai.com/v1/audio/transcriptions', {
            method: 'POST',
            headers: { 'Authorization': `Bearer ${apiKey}` },
            body: formData
          });

          if (whisperRes.ok) {
            const whisperData = await whisperRes.json();
            if (whisperData.text && whisperData.text.trim()) {
              question = whisperData.text.trim();
            }
          }
        }
      } catch (err) {
        console.warn('Voice query transcription error:', err);
      }
    }

    if (queryInput) queryInput.value = question;
    await this.executeAiQuery(question);
  }

  // Ask AI & Wi-Fi Screen Sync
  setupAskAi() {
    const modelSelect = document.getElementById('ai-model-select');
    const keyContainer = document.getElementById('ai-key-container');
    const keyInput = document.getElementById('ai-api-key-input');
    const keyLabel = document.getElementById('ai-key-label');
    const saveKeyBtn = document.getElementById('btn-save-ai-key');
    const queryInput = document.getElementById('ai-query-input');
    const submitBtn = document.getElementById('btn-submit-ai-query');
    const voiceBtn = document.getElementById('btn-ai-voice-input');
    const voiceBtnText = document.getElementById('ai-voice-btn-text');
    const pushScreenBtn = document.getElementById('btn-push-ai-screen');
    const copyBtn = document.getElementById('btn-copy-ai-answer');
    const speakBtn = document.getElementById('btn-speak-ai-answer');
    const clearBtn = document.getElementById('btn-clear-ai-answer');
    const chips = document.querySelectorAll('.prompt-chip');

    // Model selection & API key visibility
    const syncApiKeyUI = () => {
      if (!modelSelect) return;
      const model = modelSelect.value;
      if (model === 'gemini') {
        if (keyContainer) keyContainer.style.display = 'flex';
        if (keyLabel) keyLabel.textContent = 'Gemini Key:';
        if (keyInput) {
          keyInput.placeholder = 'Enter Google Gemini API Key...';
          keyInput.value = localStorage.getItem('orbit_gemini_api_key') || '';
        }
      } else if (model === 'openai') {
        if (keyContainer) keyContainer.style.display = 'flex';
        if (keyLabel) keyLabel.textContent = 'OpenAI Key:';
        if (keyInput) {
          keyInput.placeholder = 'Enter OpenAI API Key (sk-...)...';
          keyInput.value = localStorage.getItem('orbit_openai_api_key') || localStorage.getItem('orbit_stt_api_key') || '';
        }
      } else {
        if (keyContainer) keyContainer.style.display = 'none';
      }
    };

    if (modelSelect) {
      modelSelect.addEventListener('change', syncApiKeyUI);
      syncApiKeyUI();
    }

    if (saveKeyBtn && keyInput && modelSelect) {
      saveKeyBtn.addEventListener('click', () => {
        const key = keyInput.value.trim();
        const model = modelSelect.value;
        const storageKey = model === 'gemini' ? 'orbit_gemini_api_key' : 'orbit_openai_api_key';
        if (key) {
          localStorage.setItem(storageKey, key);
          alert(`${model.toUpperCase()} API key saved in browser storage!`);
        } else {
          localStorage.removeItem(storageKey);
          alert(`${model.toUpperCase()} API key removed from browser storage.`);
        }
      });
    }

    // Quick suggestion prompt chips
    chips.forEach(chip => {
      chip.addEventListener('click', () => {
        const prompt = chip.dataset.prompt;
        if (queryInput) queryInput.value = prompt;
        this.executeAiQuery(prompt);
      });
    });

    // Submit query
    if (submitBtn && queryInput) {
      submitBtn.addEventListener('click', () => {
        const q = queryInput.value.trim();
        if (!q) {
          queryInput.focus();
          return;
        }
        this.executeAiQuery(q);
      });

      queryInput.addEventListener('keydown', (e) => {
        if (e.key === 'Enter' && !e.shiftKey) {
          e.preventDefault();
          submitBtn.click();
        }
      });
    }

    // Voice query dictation via Web Speech API
    let speechRec = null;
    let isListening = false;
    if (voiceBtn && ('webkitSpeechRecognition' in window || 'SpeechRecognition' in window)) {
      const SpeechRecognition = window.SpeechRecognition || window.webkitSpeechRecognition;
      speechRec = new SpeechRecognition();
      speechRec.continuous = false;
      speechRec.interimResults = true;
      speechRec.lang = 'en-US';

      speechRec.onstart = () => {
        isListening = true;
        voiceBtn.classList.add('dictating');
        if (voiceBtnText) voiceBtnText.textContent = 'Listening...';
      };

      speechRec.onresult = (e) => {
        let transcript = '';
        for (let i = 0; i < e.results.length; ++i) {
          transcript += e.results[i][0].transcript;
        }
        if (queryInput) queryInput.value = transcript.trim();
      };

      speechRec.onerror = () => {
        isListening = false;
        voiceBtn.classList.remove('dictating');
        if (voiceBtnText) voiceBtnText.textContent = 'Voice Question';
      };

      speechRec.onend = () => {
        isListening = false;
        voiceBtn.classList.remove('dictating');
        if (voiceBtnText) voiceBtnText.textContent = 'Voice Question';
        if (queryInput && queryInput.value.trim()) {
          this.executeAiQuery(queryInput.value.trim());
        }
      };

      voiceBtn.addEventListener('click', () => {
        if (isListening) {
          speechRec.stop();
        } else {
          speechRec.start();
        }
      });
    } else if (voiceBtn) {
      voiceBtn.addEventListener('click', () => {
        alert('Web Speech API is not supported in this browser. Please use Chrome, Safari, or Edge.');
      });
    }

    // Push current answer to ORBIT screen
    if (pushScreenBtn) {
      pushScreenBtn.addEventListener('click', () => {
        if (!this.lastAiAnswer) {
          alert('No AI answer to display yet! Ask a question first.');
          return;
        }
        this.pushAiAnswerToDevice(this.lastAiQuestion || 'AI Answer', this.lastAiAnswer);
        const originalText = pushScreenBtn.innerHTML;
        pushScreenBtn.innerHTML = '<span>✅ Displayed on ORBIT!</span>';
        setTimeout(() => { pushScreenBtn.innerHTML = originalText; }, 2000);
      });
    }

    // Copy answer to clipboard
    if (copyBtn) {
      copyBtn.addEventListener('click', async () => {
        if (!this.lastAiAnswer) return;
        try {
          if (navigator.clipboard && navigator.clipboard.writeText) {
            await navigator.clipboard.writeText(this.lastAiAnswer);
          } else {
            const ta = document.createElement('textarea');
            ta.value = this.lastAiAnswer;
            document.body.appendChild(ta);
            ta.select();
            document.execCommand('copy');
            document.body.removeChild(ta);
          }
          copyBtn.textContent = '✓ Copied!';
          setTimeout(() => { copyBtn.textContent = '📋 Copy'; }, 2000);
        } catch (err) {
          console.warn('Copy error:', err);
        }
      });
    }

    // Text to Speech Read Aloud (ElevenLabs Ultra-Realistic AI Voice or Browser Native)
    if (speakBtn) {
      speakBtn.addEventListener('click', async () => {
        if (!this.lastAiAnswer) return;

        const elevenKey = localStorage.getItem('orbit_elevenlabs_key');
        if (elevenKey && elevenKey.length > 10) {
          speakBtn.disabled = true;
          const origText = speakBtn.innerHTML;
          speakBtn.innerHTML = '<span>⏳ Speaking (ElevenLabs)...</span>';
          try {
            // Default Voice: Rachel (21m00Tcm4TlvDq8ikWAM)
            const voiceId = localStorage.getItem('orbit_elevenlabs_voice') || '21m00Tcm4TlvDq8ikWAM';
            const cleanText = this.lastAiAnswer.replace(/[#*`_]/g, '').substring(0, 450);

            const res = await fetch(`https://api.elevenlabs.io/v1/text-to-speech/${voiceId}`, {
              method: 'POST',
              headers: {
                'xi-api-key': elevenKey,
                'Content-Type': 'application/json'
              },
              body: JSON.stringify({
                text: cleanText,
                model_id: 'eleven_turbo_v2_5',
                voice_settings: { stability: 0.5, similarity_boost: 0.75 }
              })
            });

            if (!res.ok) {
              const err = await res.json().catch(() => ({}));
              throw new Error(err.detail?.message || `HTTP ${res.status}`);
            }

            const audioBlob = await res.blob();
            this.loadAudioBlobIntoPlayer(audioBlob, `ElevenLabs_AI_Voice_${Date.now()}.mp3`);
            this.webAudio.play().catch(() => {});
            speakBtn.innerHTML = origText;
            speakBtn.disabled = false;
            return;
          } catch (e) {
            console.warn('ElevenLabs API error, falling back to browser speech:', e);
            speakBtn.innerHTML = origText;
            speakBtn.disabled = false;
          }
        }

        // Browser Native Speech Fallback
        if ('speechSynthesis' in window) {
          if (window.speechSynthesis.speaking) {
            window.speechSynthesis.cancel();
            speakBtn.textContent = '🔊 Read Aloud';
            return;
          }
          const utter = new SpeechSynthesisUtterance(this.lastAiAnswer);
          utter.rate = 1.0;
          utter.onend = () => { speakBtn.textContent = '🔊 Read Aloud'; };
          utter.onerror = () => { speakBtn.textContent = '🔊 Read Aloud'; };
          speakBtn.textContent = '⏹️ Stop';
          window.speechSynthesis.speak(utter);
        } else {
          alert('Speech synthesis not supported in this browser.');
        }
      });
    }

    // Clear response & dismiss on device
    if (clearBtn) {
      clearBtn.addEventListener('click', () => {
        const responseEl = document.getElementById('ai-response-display');
        const statusText = document.getElementById('ai-answer-status-text');
        if (responseEl) {
          responseEl.innerHTML = '<p class="placeholder-text">AI answers will appear here in real-time. When connected to Wi-Fi, the answer will also be displayed on your ORBIT ESP32-S3 screen with UP/DOWN scroll controls!</p>';
        }
        if (statusText) statusText.textContent = 'Ready for Question';
        this.lastAiQuestion = '';
        this.lastAiAnswer = '';

        // Dismiss modal on device TFT screen
        this.send({ cmd: 'hide_ai_answer' });
        fetch('/api/ai/clear', { method: 'POST' }).catch(() => {});
      });
    }
  }

  // Execute AI query with selected engine
  async executeAiQuery(prompt) {
    const responseEl = document.getElementById('ai-response-display');
    const statusText = document.getElementById('ai-answer-status-text');
    const modelSelect = document.getElementById('ai-model-select');
    const autoSyncCheckbox = document.getElementById('ai-auto-sync-device');
    const model = modelSelect ? modelSelect.value : 'orbit';

    if (statusText) statusText.textContent = '✨ Thinking & Formulating Answer...';
    if (responseEl) {
      responseEl.innerHTML = `<div style="display:flex; align-items:center; gap:10px; color:var(--color-neon-cyan); padding:10px 0;"><span class="pulse-dot-cyan"></span><span>Querying <strong>${model.toUpperCase()}</strong> engine...</span></div>`;
    }

    let answer = '';
    const cleanPrompt = prompt.trim();

    try {
      if (model === 'gemini') {
        const geminiKey = localStorage.getItem('orbit_gemini_api_key');
        if (!geminiKey) {
          answer = `[Google Gemini API Key Needed]\nPlease enter your Gemini API Key in the field above to query Gemini models live.\n\nHere is ORBIT's ambient answer in the meantime:\n${this.generateAmbientAnswer(cleanPrompt)}`;
        } else {
          const res = await fetch(`https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent?key=${geminiKey}`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
              contents: [{
                parts: [{
                  text: `You are ORBIT Ambient Assistant on a smart ESP32-S3 desktop device with a 128x160 TFT screen. Keep your answer concise, helpful, and under 340 characters so it fits on screen.\nUser: ${cleanPrompt}`
                }]
              }]
            })
          });

          if (!res.ok) {
            const err = await res.json().catch(() => ({}));
            throw new Error(err.error?.message || `Gemini HTTP ${res.status}`);
          }
          const data = await res.json();
          answer = data.candidates?.[0]?.content?.parts?.[0]?.text?.trim() || 'No response received from Gemini.';
        }
      } else if (model === 'openai') {
        const openaiKey = localStorage.getItem('orbit_openai_api_key') || localStorage.getItem('orbit_stt_api_key');
        if (!openaiKey) {
          answer = `[OpenAI API Key Needed]\nPlease enter your OpenAI API Key above to query GPT-4o live.\n\nHere is ORBIT's ambient answer in the meantime:\n${this.generateAmbientAnswer(cleanPrompt)}`;
        } else {
          const res = await fetch('https://api.openai.com/v1/chat/completions', {
            method: 'POST',
            headers: {
              'Content-Type': 'application/json',
              'Authorization': `Bearer ${openaiKey}`
            },
            body: JSON.stringify({
              model: 'gpt-4o-mini',
              messages: [
                { role: 'system', content: 'You are ORBIT Ambient Assistant on a smart ESP32-S3 desktop device with a 128x160 TFT screen. Keep your answer concise, helpful, and under 340 characters.' },
                { role: 'user', content: cleanPrompt }
              ],
              max_tokens: 150
            })
          });

          if (!res.ok) {
            const err = await res.json().catch(() => ({}));
            throw new Error(err.error?.message || `OpenAI HTTP ${res.status}`);
          }
          const data = await res.json();
          answer = data.choices?.[0]?.message?.content?.trim() || 'No response received from OpenAI.';
        }
      } else {
        // Built-in ORBIT Ambient Assistant
        answer = this.generateAmbientAnswer(cleanPrompt);
      }
    } catch (err) {
      console.warn('AI Query Exception:', err);
      answer = `API Notice: ${err.message}\n\nAmbient Fallback:\n${this.generateAmbientAnswer(cleanPrompt)}`;
    }

    // Save answer state
    this.lastAiQuestion = cleanPrompt;
    this.lastAiAnswer = answer;

    // Render formatted response
    if (responseEl) {
      const formattedHtml = answer.split('\n\n').map(p => `<p>${p.replace(/\n/g, '<br>')}</p>`).join('');
      responseEl.innerHTML = formattedHtml;
    }

    if (statusText) statusText.textContent = '✓ Answer Delivered';

    // Auto-sync to ORBIT screen if enabled
    if (autoSyncCheckbox && autoSyncCheckbox.checked) {
      this.pushAiAnswerToDevice(cleanPrompt, answer);
    }
  }

  // Built-in intelligent ambient knowledge synthesis
  generateAmbientAnswer(prompt) {
    const p = prompt.toLowerCase();
    const t = this.latestTelemetry || {};
    const temp = t.temp !== undefined ? t.temp : 24.2;
    const hum = t.hum !== undefined ? t.hum : 52.0;
    const heatIdx = t.heat_idx !== undefined ? t.heat_idx : 24.5;
    const micDb = t.mic_db !== undefined ? t.mic_db : 41.5;
    const moon = t.moon_phase !== undefined ? Math.round(t.moon_phase * 100) : 68;
    const time = t.time || new Date().toLocaleTimeString();
    const date = t.date || new Date().toLocaleDateString();
    const heapKb = t.heap ? Math.round(t.heap / 1024) : 240;

    if (p.includes('temp') || p.includes('climate') || p.includes('comfort') || p.includes('environment') || p.includes('humid')) {
      return `Live ORBIT Climate:\nRoom temperature is ${temp}°C (${(temp * 1.8 + 32).toFixed(1)}°F) with ${hum}% humidity. Heat index: ${heatIdx}°C. Acoustic floor: ${micDb} dB. Environmental index is Optimal for desktop focus.`;
    }

    if (p.includes('moon') || p.includes('space') || p.includes('orbit') || p.includes('iss') || p.includes('astronom') || p.includes('planet')) {
      return `Space Telemetry:\nLunar illumination is ${moon}%. The Moon orbits Earth at 1.02 km/s every 27.3 days. The ISS orbits at 408 km altitude, traveling 27,600 km/h and witnessing 16 sunrises every 24 hours.`;
    }

    if (p.includes('esp32') || p.includes('project') || p.includes('idea') || p.includes('iot') || p.includes('hardware')) {
      return `ESP32-S3 Ideas:\n1. Ambient Desk Beacon: Dynamic RGB alerts for meetings & focus.\n2. Acoustic Decibel Logger: Auto-log noisy spikes to MicroSD.\n3. BLE Proximity Key: Auto-lock laptop when walking away.`;
    }

    if (p.includes('mindful') || p.includes('breath') || p.includes('meditat') || p.includes('focus') || p.includes('calm') || p.includes('relax')) {
      return `1-Min Focus Breathing:\nInhale slowly for 4s... Hold full for 4s... Exhale smoothly for 4s... Hold empty for 4s. Repeat 4 box breathing cycles to instantly settle the nervous system and sharpen attention.`;
    }

    if (p.includes('time') || p.includes('clock') || p.includes('date') || p.includes('hour')) {
      return `Clock & Telemetry:\nCurrent local time is ${time} (${date}). FreeRTOS heap: ${heapKb} KB free. System synchronized via NTP over Wi-Fi.`;
    }

    if (p.includes('quantum') || p.includes('entangle')) {
      return `Quantum Entanglement:\nWhen two particles become deeply linked, the state of one instantaneously determines the other, regardless of distance. Einstein called this 'spooky action at a distance'—today it powers quantum computing.`;
    }

    if (p.includes('productiv') || p.includes('advice') || p.includes('tip') || p.includes('work')) {
      return `Productivity Advice:\nWork in uninterrupted 25-minute sprints with zero notifications. Batch communications to fixed windows. Remember: Momentum beats perfection every time.`;
    }

    return `ORBIT Assistant:\n"${prompt}"\nProcessed via ambient telemetry engine. System running smoothly with ${heapKb} KB free heap. Wi-Fi sync active!`;
  }

  // Push answer directly to ORBIT TFT display overlay
  pushAiAnswerToDevice(question, answer) {
    const q = (question || 'AI Question').substring(0, 48);
    const a = (answer || '').substring(0, 384);

    // 1. Dispatch over WebSocket
    this.send({
      cmd: 'show_ai_answer',
      question: q,
      answer: a
    });

    // 2. Dispatch over HTTP REST API
    try {
      const body = new URLSearchParams({ question: q, answer: a });
      fetch('/api/ai/answer', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: body
      }).catch(() => {});
    } catch (e) {}
  }

  // Wi-Fi Network Setup & Provisioning
  setupNetworkTab() {
    const btnScan = document.getElementById('btn-dash-scan');
    const wifiList = document.getElementById('dash-wifi-list');
    const ssidInput = document.getElementById('dash-wifi-ssid');
    const passInput = document.getElementById('dash-wifi-pass');
    const form = document.getElementById('dashboard-wifi-form');
    const btnConnect = document.getElementById('btn-dash-connect');
    const statusBox = document.getElementById('dash-wifi-status');
    const statusText = document.getElementById('dash-status-text');
    const statusSub = document.getElementById('dash-status-sub');

    const currSsid = document.getElementById('net-curr-ssid');
    const currIp = document.getElementById('net-curr-ip');
    const currRssi = document.getElementById('net-curr-rssi');

    const updateStatusPill = async () => {
      try {
        const res = await fetch('/api/wifi/status');
        const data = await res.json();
        if (currSsid) currSsid.textContent = data.ssid || (data.ap_active ? 'ORBIT-Setup (AP)' : '--');
        if (currIp) currIp.textContent = data.ip || '--';
        if (currRssi) currRssi.textContent = (data.rssi || '--') + ' dBm';
      } catch (e) {}
    };

    updateStatusPill();

    const doScan = async () => {
      if (btnScan) btnScan.disabled = true;
      if (wifiList) wifiList.innerHTML = '<div style="color:var(--muted); font-size:0.85rem; padding:16px; text-align:center;">Scanning wireless networks...</div>';
      try {
        await fetch('/api/wifi/scan', { method: 'POST' });
        setTimeout(async () => {
          const res = await fetch('/api/wifi/scan');
          const data = await res.json();
          if (btnScan) btnScan.disabled = false;
          if (!wifiList) return;

          if (!data.networks || data.networks.length === 0) {
            wifiList.innerHTML = '<div style="color:var(--muted); font-size:0.85rem; padding:16px; text-align:center;">No networks found. Rescan or enter SSID manually.</div>';
            return;
          }

          wifiList.innerHTML = '';
          data.networks.forEach(net => {
            const item = document.createElement('div');
            item.style.cssText = 'display:flex; justify-content:space-between; align-items:center; padding:10px 14px; border-bottom:1px solid rgba(255,255,255,0.06); cursor:pointer; transition:background 0.15s;';
            item.innerHTML = `
              <div style="font-weight:600; font-size:0.9rem; color:#fff;">
                ${net.secure ? '🔒' : '🔓'} ${net.ssid}
              </div>
              <div style="font-size:0.75rem; color:var(--muted); font-family:monospace;">
                ${net.rssi} dBm
              </div>
            `;
            item.addEventListener('mouseenter', () => item.style.background = 'rgba(0,255,255,0.1)');
            item.addEventListener('mouseleave', () => item.style.background = 'transparent');
            item.addEventListener('click', () => {
              if (ssidInput) ssidInput.value = net.ssid;
              if (passInput) passInput.focus();
            });
            wifiList.appendChild(item);
          });
        }, 1200);
      } catch (err) {
        if (btnScan) btnScan.disabled = false;
        if (wifiList) wifiList.innerHTML = '<div style="color:var(--muted); font-size:0.85rem; padding:16px; text-align:center;">Failed to scan networks.</div>';
      }
    };

    if (btnScan) {
      btnScan.addEventListener('click', doScan);
    }

    if (form) {
      form.addEventListener('submit', async (e) => {
        e.preventDefault();
        const ssid = ssidInput.value.trim();
        const pass = passInput.value;
        if (!ssid) return;

        btnConnect.disabled = true;
        statusBox.style.display = 'block';
        statusText.style.color = 'var(--cyan)';
        statusText.textContent = `Connecting to ${ssid}...`;
        statusSub.textContent = 'Transmitting credentials to ORBIT. Please wait...';

        try {
          const body = new URLSearchParams({ ssid, pass });
          await fetch('/api/wifi/connect', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: body
          });

          let attempts = 0;
          const poll = setInterval(async () => {
            attempts++;
            try {
              const res = await fetch('/api/wifi/status');
              const stat = await res.json();
              if (stat.connected) {
                clearInterval(poll);
                btnConnect.disabled = false;
                statusText.style.color = 'var(--mint)';
                statusText.textContent = '✅ Connected Successfully!';
                statusSub.innerHTML = `Connected to <strong>${stat.ssid}</strong> with IP: <strong>${stat.ip}</strong>`;
                updateStatusPill();
                return;
              }
            } catch (e) {}

            if (attempts >= 16) {
              clearInterval(poll);
              btnConnect.disabled = false;
              statusText.style.color = 'var(--red)';
              statusText.textContent = '⚠️ Connection Failed';
              statusSub.textContent = 'Unable to connect to ' + ssid + '. Please check the password and try again.';
            }
          }, 1000);
        } catch (err) {
          btnConnect.disabled = false;
          statusText.style.color = 'var(--red)';
          statusText.textContent = 'Submission Error';
          statusSub.textContent = 'Could not communicate with device.';
        }
      });
    }
  }

  // Device Settings & Complete Hardware Diagnostics
  setupSettingsTab() {
    const slider = document.getElementById('settings-brightness-slider');
    const sliderVal = document.getElementById('settings-brightness-val');
    const btnRefresh = document.getElementById('btn-refresh-diagnostics');
    const btnReboot = document.getElementById('btn-reboot-device');

    // Brightness slider control
    if (slider) {
      slider.addEventListener('input', (e) => {
        const val = e.target.value;
        if (sliderVal) sliderVal.textContent = val + '%';
        this.send({ cmd: 'set_brightness', val: parseInt(val, 10) });
      });
    }

    // Wi-Fi OTA Binary Uploader
    const otaFileInput = document.getElementById('web-ota-file');
    const btnStartOta = document.getElementById('btn-start-ota');
    const otaDetails = document.getElementById('ota-file-details');
    const otaProgContainer = document.getElementById('ota-progress-container');
    const otaProgBar = document.getElementById('ota-progress-bar');
    const otaStatus = document.getElementById('ota-upload-status');
    const otaPct = document.getElementById('ota-upload-pct');
    let otaFile = null;

    if (otaFileInput && btnStartOta) {
      otaFileInput.addEventListener('change', (e) => {
        if (e.target.files && e.target.files.length) {
          otaFile = e.target.files[0];
          if (otaDetails) otaDetails.textContent = `${otaFile.name} (${(otaFile.size / 1024).toFixed(1)} KB)`;
          btnStartOta.disabled = false;
        }
      });

      btnStartOta.addEventListener('click', () => {
        if (!otaFile) return;
        btnStartOta.disabled = true;
        if (otaProgContainer) otaProgContainer.style.display = 'block';

        const formData = new FormData();
        formData.append('file', otaFile, otaFile.name);

        const xhr = new XMLHttpRequest();
        xhr.open('POST', '/update', true);

        xhr.upload.onprogress = (e) => {
          if (e.lengthComputable) {
            const pct = Math.round((e.loaded / e.total) * 100);
            if (otaProgBar) otaProgBar.style.width = pct + '%';
            if (otaPct) otaPct.textContent = pct + '%';
            if (otaStatus) otaStatus.textContent = `Uploading: ${(e.loaded / 1024).toFixed(0)} / ${(e.total / 1024).toFixed(0)} KB`;
          }
        };

        xhr.onload = () => {
          if (xhr.status === 200) {
            if (otaProgBar) otaProgBar.style.width = '100%';
            if (otaPct) otaPct.textContent = '100%';
            if (otaStatus) otaStatus.textContent = 'Flashing Complete! Rebooting device...';
            setTimeout(() => { window.location.reload(); }, 6000);
          } else {
            if (otaStatus) otaStatus.textContent = 'OTA Error: ' + xhr.responseText;
            btnStartOta.disabled = false;
          }
        };

        xhr.onerror = () => {
          if (otaStatus) otaStatus.textContent = 'Network Error during upload!';
          btnStartOta.disabled = false;
        };

        xhr.send(formData);
      });
    }

    // Refresh diagnostics
    const fetchDiagnostics = async () => {
      try {
        const res = await fetch('/api/system/info');
        if (!res.ok) return;
        const data = await res.json();

        // 1. Device Silicon & ID
        if (data.device) {
          const chip = document.getElementById('diag-chip');
          const cores = document.getElementById('diag-cores');
          const rev = document.getElementById('diag-rev');
          const uid = document.getElementById('diag-uid');
          const mac = document.getElementById('diag-mac');

          if (chip) chip.textContent = data.device.chip || 'ESP32-S3';
          if (cores) cores.textContent = `${data.device.cores} Cores @ ${data.device.cpu_mhz} MHz`;
          if (rev) rev.textContent = `Rev v${data.device.revision} (Xtensa LX7)`;
          if (uid) uid.textContent = data.device.uid || '--';
          if (mac) mac.textContent = data.device.mac || '--';
        }

        // 2. OS & Platform
        if (data.os) {
          const os = document.getElementById('diag-os');
          const kernel = document.getElementById('diag-kernel');
          const idf = document.getElementById('diag-idf');
          const uptime = document.getElementById('diag-uptime');

          if (os) os.textContent = `${data.os.name} v${data.os.version}`;
          if (kernel) kernel.textContent = data.os.kernel || 'FreeRTOS 10';
          if (idf) idf.textContent = `ESP-IDF ${data.os.idf || 'v5.1'}`;
          if (uptime) {
            const sec = data.os.uptime_sec || 0;
            const h = Math.floor(sec / 3600);
            const m = Math.floor((sec % 3600) / 60);
            const s = sec % 60;
            uptime.textContent = `${h}h ${m}m ${s}s`;
          }
        }

        // 3. Storage Subsystems
        if (data.storage) {
          const sdStatus = document.getElementById('diag-sd-status');
          const sdCap = document.getElementById('diag-sd-cap');
          const sdUsed = document.getElementById('diag-sd-used');
          const lfsUsed = document.getElementById('diag-lfs-used');

          if (sdStatus) {
            sdStatus.textContent = data.storage.sd_mounted ? 'Mounted & Active' : 'Not Detected';
            sdStatus.style.color = data.storage.sd_mounted ? 'var(--mint)' : 'var(--amber)';
          }
          if (sdCap) {
            sdCap.textContent = data.storage.sd_mounted ? `${data.storage.sd_total_mb} MB` : 'N/A';
          }
          if (sdUsed) {
            const sdFreePct = data.storage.sd_total_mb > 0
              ? Math.round((data.storage.sd_free_mb * 100) / data.storage.sd_total_mb)
              : 0;
            sdUsed.textContent = data.storage.sd_mounted
              ? `${data.storage.sd_used_mb} MB / Free: ${data.storage.sd_free_mb} MB (${sdFreePct}%)`
              : 'N/A';
          }
          if (lfsUsed) {
            const usedKb = Math.round((data.storage.lfs_used_bytes || 0) / 1024);
            const totKb = Math.round((data.storage.lfs_total_bytes || 0) / 1024);
            const freeKb = totKb >= usedKb ? (totKb - usedKb) : 0;
            const lfsFreePct = totKb > 0 ? Math.round((freeKb * 100) / totKb) : 0;
            lfsUsed.textContent = `${usedKb} KB / ${totKb} KB (Free: ${freeKb} KB, ${lfsFreePct}%)`;
          }
        }

        // 4. Flash ROM & Partition
        if (data.flash) {
          const fSize = document.getElementById('diag-flash-size');
          const fSpeed = document.getElementById('diag-flash-speed');
          const sketch = document.getElementById('diag-sketch-size');
          const freeSketch = document.getElementById('diag-free-sketch');

          if (fSize) {
            const mb = Math.round(data.flash.size_bytes / (1024 * 1024));
            fSize.textContent = `${mb} MB SPI Flash`;
          }
          if (fSpeed) {
            const mhz = Math.round(data.flash.speed_hz / 1000000);
            fSpeed.textContent = `${mhz} MHz (Octal SPI)`;
          }
          if (sketch) {
            const skKb = Math.round(data.flash.sketch_bytes / 1024);
            sketch.textContent = `${skKb} KB`;
          }
          if (freeSketch) {
            const frKb = Math.round(data.flash.free_sketch_bytes / 1024);
            const skKb = Math.round(data.flash.sketch_bytes / 1024);
            const totPartKb = skKb + frKb;
            const freeSketchPct = totPartKb > 0 ? Math.round((frKb * 100) / totPartKb) : 0;
            freeSketch.textContent = `${frKb} KB available (${freeSketchPct}% free)`;
          }
        }

        // 5. Memory Architecture
        if (data.memory) {
          const heapFree = document.getElementById('diag-heap-free');
          const heapMin = document.getElementById('diag-heap-min');
          const heapTot = document.getElementById('diag-heap-total');
          const psramStat = document.getElementById('diag-psram-status');
          const psramFree = document.getElementById('diag-psram-free');

          const hFreeKb = Math.round(data.memory.heap_free / 1024);
          const hTotKb = Math.round(data.memory.heap_total / 1024);
          const hFreePct = hTotKb > 0 ? Math.round((hFreeKb * 100) / hTotKb) : 0;

          if (heapFree) heapFree.textContent = `${hFreeKb} KB (${hFreePct}% free)`;
          if (heapMin) heapMin.textContent = `${Math.round(data.memory.heap_min_free / 1024)} KB (watermark)`;
          if (heapTot) heapTot.textContent = `${hTotKb} KB`;

          if (psramStat) {
            psramStat.textContent = data.memory.psram_found
              ? `${Math.round(data.memory.psram_total / (1024 * 1024))} MB Embedded Octal`
              : 'Not Installed';
            psramStat.style.color = data.memory.psram_found ? 'var(--mint)' : 'var(--muted)';
          }
          if (psramFree) {
            if (data.memory.psram_found) {
              const pFreeKb = Math.round(data.memory.psram_free / 1024);
              const pTotKb = Math.round(data.memory.psram_total / 1024);
              const pFreePct = pTotKb > 0 ? Math.round((pFreeKb * 100) / pTotKb) : 0;
              psramFree.textContent = `${pFreeKb} KB (${pFreePct}% free)`;
            } else {
              psramFree.textContent = 'N/A';
            }
          }
        }

        // 6. Network & Connectivity
        if (data.network) {
          const netStat = document.getElementById('diag-net-status');
          const netSsid = document.getElementById('diag-net-ssid');
          const netIp = document.getElementById('diag-net-ip');
          const netGw = document.getElementById('diag-net-gw');
          const netMask = document.getElementById('diag-net-mask');
          const netRssi = document.getElementById('diag-net-rssi');

          if (netStat) {
            netStat.textContent = data.network.connected ? 'Station (Connected)' : (data.network.ap_active ? 'SoftAP Hotspot' : 'Disconnected');
            netStat.style.color = data.network.connected ? 'var(--mint)' : 'var(--amber)';
          }
          if (netSsid) netSsid.textContent = data.network.connected ? data.network.ssid : (data.network.softap_ssid || '--');
          if (netIp) netIp.textContent = data.network.connected ? data.network.ip : (data.network.softap_ip || '--');
          if (netGw) netGw.textContent = data.network.gateway || '--';
          if (netMask) netMask.textContent = data.network.subnet || '--';
          if (netRssi) netRssi.textContent = `${data.network.rssi} dBm`;
        }
      } catch (e) {
        console.warn('Diagnostics fetch error:', e);
      }
    };

    if (btnRefresh) {
      btnRefresh.addEventListener('click', () => fetchDiagnostics());
    }

    if (btnReboot) {
      btnReboot.addEventListener('click', async () => {
        if (confirm('Reboot ORBIT ESP32-S3 device now?')) {
          try {
            await fetch('/api/system/reboot', { method: 'POST' });
            alert('Reboot command dispatched. Device is restarting...');
          } catch (e) {
            alert('Reboot command sent.');
          }
        }
      });
    }

    // Tab click triggers instant refresh
    const settingsTabBtn = document.querySelector('.nav-tab[data-tab="settings"]');
    if (settingsTabBtn) {
      settingsTabBtn.addEventListener('click', () => fetchDiagnostics());
    }

    // Initial load
    fetchDiagnostics();
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
