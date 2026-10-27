#pragma once
#include <Arduino.h>

// ==============================================================================
// ORBIT v2 — Self-Contained Web OTA & Firmware Flashing Studio
// Served at: http://orbit.local/update
// ==============================================================================

const char OTA_UPDATE_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ORBIT // OTA Firmware & File Upload Studio</title>
  <style>
    :root {
      --bg: #090e17;
      --card: rgba(16, 24, 38, 0.95);
      --border: rgba(0, 255, 255, 0.25);
      --border-active: rgba(0, 255, 255, 0.8);
      --cyan: #00ffff;
      --mint: #2fe89f;
      --amber: #fea000;
      --red: #ff3366;
      --text: #e6edf3;
      --muted: #8b949e;
      --font: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, monospace;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      background: var(--bg);
      color: var(--text);
      font-family: var(--font);
      min-height: 100vh;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      padding: 24px;
      position: relative;
      overflow-x: hidden;
    }
    .glow-cyan {
      position: absolute;
      width: 400px;
      height: 400px;
      background: radial-gradient(circle, rgba(0,255,255,0.08) 0%, transparent 70%);
      top: -100px;
      left: -100px;
      pointer-events: none;
    }
    .glow-purple {
      position: absolute;
      width: 400px;
      height: 400px;
      background: radial-gradient(circle, rgba(184,31,223,0.08) 0%, transparent 70%);
      bottom: -100px;
      right: -100px;
      pointer-events: none;
    }
    .container {
      width: 100%;
      max-width: 580px;
      background: var(--card);
      border: 1px solid var(--border);
      border-radius: 12px;
      padding: 32px;
      box-shadow: 0 16px 40px rgba(0,0,0,0.6), 0 0 20px rgba(0,255,255,0.1);
      position: relative;
      z-index: 1;
    }
    .header {
      display: flex;
      align-items: center;
      gap: 12px;
      margin-bottom: 24px;
      border-bottom: 1px solid rgba(255,255,255,0.08);
      padding-bottom: 16px;
    }
    .logo-badge {
      background: rgba(0, 255, 255, 0.12);
      border: 1px solid var(--cyan);
      color: var(--cyan);
      font-weight: 800;
      font-size: 0.85rem;
      padding: 6px 12px;
      border-radius: 6px;
      letter-spacing: 1px;
    }
    h2 {
      font-size: 1.3rem;
      font-weight: 700;
      color: #fff;
      letter-spacing: 0.5px;
    }
    .subtitle {
      font-size: 0.85rem;
      color: var(--muted);
      margin-top: 2px;
    }
    .target-selector {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 12px;
      margin-bottom: 20px;
    }
    .target-btn {
      background: rgba(0,0,0,0.3);
      border: 1px solid var(--border);
      color: var(--text);
      padding: 12px;
      border-radius: 8px;
      cursor: pointer;
      font-weight: 600;
      font-size: 0.88rem;
      text-align: center;
      transition: all 0.2s ease;
    }
    .target-btn.active {
      background: rgba(0, 255, 255, 0.15);
      border-color: var(--cyan);
      color: var(--cyan);
      box-shadow: 0 0 12px rgba(0, 255, 255, 0.2);
    }
    .dropzone {
      border: 2px dashed var(--border);
      border-radius: 10px;
      padding: 36px 20px;
      text-align: center;
      background: rgba(0,0,0,0.25);
      cursor: pointer;
      transition: all 0.2s ease;
      margin-bottom: 24px;
    }
    .dropzone:hover, .dropzone.dragover {
      border-color: var(--cyan);
      background: rgba(0, 255, 255, 0.05);
    }
    .dropzone svg {
      stroke: var(--cyan);
      margin-bottom: 12px;
    }
    .drop-title {
      font-size: 1rem;
      font-weight: 600;
      color: #fff;
      margin-bottom: 4px;
    }
    .drop-desc {
      font-size: 0.8rem;
      color: var(--muted);
    }
    .file-input { display: none; }
    .file-info {
      display: none;
      background: rgba(0, 255, 255, 0.08);
      border: 1px solid rgba(0, 255, 255, 0.3);
      padding: 12px 16px;
      border-radius: 8px;
      margin-bottom: 20px;
      font-size: 0.88rem;
      display: flex;
      justify-content: space-between;
      align-items: center;
    }
    .progress-box {
      display: none;
      margin-bottom: 24px;
    }
    .progress-bar-bg {
      background: rgba(255,255,255,0.08);
      height: 10px;
      border-radius: 5px;
      overflow: hidden;
      margin-bottom: 8px;
    }
    .progress-bar-fill {
      background: linear-gradient(90deg, var(--cyan), var(--mint));
      height: 100%;
      width: 0%;
      transition: width 0.15s ease;
      box-shadow: 0 0 10px var(--cyan);
    }
    .progress-labels {
      display: flex;
      justify-content: space-between;
      font-size: 0.8rem;
      color: var(--muted);
      font-family: monospace;
    }
    .btn-upload {
      width: 100%;
      padding: 14px;
      background: var(--cyan);
      border: none;
      border-radius: 8px;
      color: #000;
      font-weight: 700;
      font-size: 0.95rem;
      cursor: pointer;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 8px;
      transition: all 0.2s ease;
    }
    .btn-upload:hover {
      background: #33ffff;
      box-shadow: 0 0 18px rgba(0, 255, 255, 0.5);
    }
    .btn-upload:disabled {
      background: rgba(255,255,255,0.15);
      color: var(--muted);
      cursor: not-allowed;
      box-shadow: none;
    }
    .status-msg {
      margin-top: 16px;
      padding: 12px;
      border-radius: 8px;
      font-size: 0.88rem;
      display: none;
      text-align: center;
    }
    .status-msg.success { background: rgba(47, 232, 159, 0.15); color: var(--mint); border: 1px solid var(--mint); }
    .status-msg.error { background: rgba(255, 51, 102, 0.15); color: var(--red); border: 1px solid var(--red); }
    .footer-links {
      margin-top: 24px;
      text-align: center;
      font-size: 0.8rem;
    }
    .footer-links a {
      color: var(--cyan);
      text-decoration: none;
      margin: 0 8px;
    }
    .footer-links a:hover { text-decoration: underline; }
  </style>
</head>
<body>
  <div class="glow-cyan"></div>
  <div class="glow-purple"></div>

  <div class="container">
    <div class="header">
      <div class="logo-badge">ORBIT OTA</div>
      <div>
        <h2>Firmware & File Studio</h2>
        <div class="subtitle">Compile and upload binaries over Wi-Fi</div>
      </div>
    </div>

    <!-- Target Partition -->
    <div class="target-selector">
      <button class="target-btn active" id="btn-target-flash" type="button" onclick="selectTarget('flash')">
        Firmware (.bin)
      </button>
      <button class="target-btn" id="btn-target-fs" type="button" onclick="selectTarget('fs')">
        Filesystem (.bin / web)
      </button>
    </div>

    <!-- Drag & Drop Zone -->
    <div class="dropzone" id="dropzone" onclick="document.getElementById('file-input').click()">
      <svg width="40" height="40" viewBox="0 0 24 24" fill="none" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round">
        <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"/>
        <polyline points="17 8 12 3 7 8"/>
        <line x1="12" y1="3" x2="12" y2="15"/>
      </svg>
      <div class="drop-title" id="drop-label">Choose or Drop Compiled Binary</div>
      <div class="drop-desc" id="drop-sub">Accepts orbit.ino.bin or firmware.bin</div>
    </div>
    <input type="file" id="file-input" class="file-input" accept=".bin" onchange="handleFileSelect(this.files)">

    <div class="file-info" id="file-info" style="display:none;">
      <span id="file-name" style="font-weight:600; color:var(--cyan); word-break:break-all;"></span>
      <span id="file-size" style="color:var(--muted); font-family:monospace; min-width:80px; text-align:right;"></span>
    </div>

    <!-- Progress Display -->
    <div class="progress-box" id="progress-box">
      <div class="progress-bar-bg">
        <div class="progress-bar-fill" id="progress-fill"></div>
      </div>
      <div class="progress-labels">
        <span id="progress-status">Uploading...</span>
        <span id="progress-pct">0%</span>
      </div>
    </div>

    <button class="btn-upload" id="btn-upload" type="button" disabled onclick="startUpload()">
      <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M12 2v10M17 7l-5-5-5 5M2 17v3a2 2 0 0 0 2 2h16a2 2 0 0 0 2-2v-3"/></svg>
      <span>Flash Over Wi-Fi</span>
    </button>

    <div class="status-msg" id="status-msg"></div>

    <div class="footer-links">
      <a href="/">Dashboard</a> &bull;
      <a href="/wifi">Wi-Fi Setup</a> &bull;
      <a href="/api/system/info" target="_blank">Diagnostics JSON</a>
    </div>
  </div>

  <script>
    let selectedFile = null;
    let uploadTarget = 'flash'; // 'flash' or 'fs'

    function selectTarget(target) {
      uploadTarget = target;
      document.getElementById('btn-target-flash').classList.toggle('active', target === 'flash');
      document.getElementById('btn-target-fs').classList.toggle('active', target === 'fs');
      document.getElementById('drop-sub').textContent = target === 'flash' 
        ? 'Uploads compiled firmware to active APP partition' 
        : 'Uploads LittleFS / SPIFFS filesystem binary';
    }

    // Drag and Drop
    const dz = document.getElementById('dropzone');
    ['dragenter', 'dragover'].forEach(name => {
      dz.addEventListener(name, (e) => { e.preventDefault(); dz.classList.add('dragover'); });
    });
    ['dragleave', 'drop'].forEach(name => {
      dz.addEventListener(name, (e) => { e.preventDefault(); dz.classList.remove('dragover'); });
    });
    dz.addEventListener('drop', (e) => {
      if (e.dataTransfer.files.length) handleFileSelect(e.dataTransfer.files);
    });

    function handleFileSelect(files) {
      if (!files || !files.length) return;
      selectedFile = files[0];
      document.getElementById('file-name').textContent = selectedFile.name;
      document.getElementById('file-size').textContent = (selectedFile.size / 1024).toFixed(1) + ' KB';
      document.getElementById('file-info').style.display = 'flex';
      document.getElementById('btn-upload').disabled = false;
      hideStatus();
    }

    function showStatus(msg, isSuccess) {
      const el = document.getElementById('status-msg');
      el.textContent = msg;
      el.className = 'status-msg ' + (isSuccess ? 'success' : 'error');
      el.style.display = 'block';
    }

    function hideStatus() {
      document.getElementById('status-msg').style.display = 'none';
    }

    function startUpload() {
      if (!selectedFile) return;

      const uploadBtn = document.getElementById('btn-upload');
      uploadBtn.disabled = true;
      document.getElementById('progress-box').style.display = 'block';
      hideStatus();

      const formData = new FormData();
      // If filesystem target, prefix filename to cue U_SPIFFS
      const filename = (uploadTarget === 'fs') ? 'littlefs_' + selectedFile.name : selectedFile.name;
      formData.append('file', selectedFile, filename);

      const xhr = new XMLHttpRequest();
      xhr.open('POST', '/update?target=' + uploadTarget, true);

      xhr.upload.onprogress = (e) => {
        if (e.lengthComputable) {
          const pct = Math.round((e.loaded / e.total) * 100);
          document.getElementById('progress-fill').style.width = pct + '%';
          document.getElementById('progress-pct').textContent = pct + '%';
          const loadedKb = (e.loaded / 1024).toFixed(0);
          const totalKb = (e.total / 1024).toFixed(0);
          document.getElementById('progress-status').textContent = `Uploading: ${loadedKb} / ${totalKb} KB`;
        }
      };

      xhr.onload = () => {
        if (xhr.status === 200) {
          document.getElementById('progress-fill').style.width = '100%';
          document.getElementById('progress-pct').textContent = '100%';
          document.getElementById('progress-status').textContent = 'Flash Complete!';
          showStatus('OTA Flashing Successful! Device is rebooting into new firmware...', true);
          let count = 8;
          const timer = setInterval(() => {
            count--;
            if (count > 0) {
              document.getElementById('progress-status').textContent = `Rebooting in ${count}s...`;
            } else {
              clearInterval(timer);
              window.location.href = '/';
            }
          }, 1000);
        } else {
          showStatus('OTA Flash Error: ' + xhr.responseText, false);
          uploadBtn.disabled = false;
        }
      };

      xhr.onerror = () => {
        showStatus('Network connection lost during upload. Check Wi-Fi and retry.', false);
        uploadBtn.disabled = false;
      };

      xhr.send(formData);
    }
  </script>
</body>
</html>
)rawliteral";

