#pragma once
#include <Arduino.h>

// ==============================================================================
// ORBIT v2 — Self-Contained Wi-Fi Provisioning & Captive Portal Web Page
// Stored in PROGMEM with zero external CDN dependencies for reliable offline AP setup
// ==============================================================================

const char WIFI_SETUP_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no, viewport-fit=cover">
  <meta name="theme-color" content="#0a0e17">
  <meta name="apple-mobile-web-app-capable" content="yes">
  <meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
  <title>ORBIT OS — Wi-Fi Network Setup</title>
  <style>
    :root {
      --bg: #0a0e17;
      --card: rgba(16, 24, 38, 0.94);
      --card-hover: rgba(24, 36, 58, 0.96);
      --card-selected: rgba(0, 255, 255, 0.14);
      --border: rgba(0, 255, 255, 0.18);
      --border-active: #00ffff;
      --cyan: #00ffff;
      --purple: #b81fdf;
      --mint: #00ff9d;
      --amber: #ffaa00;
      --red: #ff3366;
      --text: #f0f6fc;
      --muted: #8b9bb4;
      --input-bg: rgba(9, 14, 23, 0.95);
    }
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      background: var(--bg);
      color: var(--text);
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
      min-height: 100vh;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: flex-start;
      padding: 20px 16px 40px;
      position: relative;
      overflow-x: hidden;
      -webkit-font-smoothing: antialiased;
    }
    .glow-sphere {
      position: absolute;
      border-radius: 50%;
      filter: blur(80px);
      pointer-events: none;
      z-index: 0;
    }
    .glow-cyan { width: 280px; height: 280px; background: rgba(0, 255, 255, 0.12); top: -60px; left: -60px; }
    .glow-purple { width: 300px; height: 300px; background: rgba(184, 31, 223, 0.12); bottom: -60px; right: -60px; }

    .portal-box {
      margin: auto 0;
      width: 100%;
      max-width: 440px;
      background: var(--card);
      border: 1px solid var(--border);
      border-radius: 16px;
      box-shadow: 0 16px 48px rgba(0, 0, 0, 0.65), 0 0 24px rgba(0, 255, 255, 0.08);
      backdrop-filter: blur(14px);
      -webkit-backdrop-filter: blur(14px);
      padding: 24px 20px;
      position: relative;
      z-index: 1;
    }

    /* Brand Header */
    .brand-header {
      display: flex;
      align-items: center;
      gap: 14px;
      margin-bottom: 20px;
      padding-bottom: 16px;
      border-bottom: 1px solid rgba(255, 255, 255, 0.08);
    }
    .orbit-logo {
      width: 44px;
      height: 44px;
      position: relative;
      display: flex;
      align-items: center;
      justify-content: center;
      flex-shrink: 0;
    }
    .ring {
      position: absolute;
      border-radius: 50%;
      border: 2px solid transparent;
    }
    .ring.outer {
      width: 40px;
      height: 40px;
      border-top-color: var(--cyan);
      border-bottom-color: var(--purple);
      animation: spin 6s linear infinite;
    }
    .ring.inner {
      width: 24px;
      height: 24px;
      border-left-color: var(--cyan);
      border-right-color: var(--mint);
      animation: spin-rev 3s linear infinite;
    }
    .core-dot {
      width: 8px;
      height: 8px;
      background: var(--cyan);
      border-radius: 50%;
      box-shadow: 0 0 10px var(--cyan);
    }
    @keyframes spin { 100% { transform: rotate(360deg); } }
    @keyframes spin-rev { 100% { transform: rotate(-360deg); } }

    .brand-info h1 {
      font-size: 1.3rem;
      font-weight: 700;
      letter-spacing: 0.5px;
      display: flex;
      align-items: center;
      gap: 8px;
    }
    .brand-badge {
      font-size: 0.7rem;
      background: rgba(0, 255, 255, 0.15);
      color: var(--cyan);
      border: 1px solid rgba(0, 255, 255, 0.3);
      padding: 2px 7px;
      border-radius: 20px;
      text-transform: uppercase;
      font-weight: 600;
    }
    .brand-info p {
      color: var(--muted);
      font-size: 0.82rem;
      margin-top: 3px;
    }

    /* Hotspot Mode Banner */
    .ap-banner {
      background: rgba(0, 255, 255, 0.06);
      border: 1px solid rgba(0, 255, 255, 0.22);
      border-radius: 10px;
      padding: 10px 14px;
      margin-bottom: 18px;
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 10px;
    }
    .ap-banner-text {
      font-size: 0.78rem;
      color: var(--muted);
    }
    .ap-banner-text strong {
      color: var(--cyan);
      font-weight: 600;
    }
    .btn-dash-mini {
      background: rgba(0, 255, 255, 0.15);
      color: var(--cyan);
      border: 1px solid rgba(0, 255, 255, 0.35);
      border-radius: 6px;
      font-size: 0.75rem;
      font-weight: 600;
      padding: 5px 10px;
      text-decoration: none;
      white-space: nowrap;
      transition: background 0.15s;
    }
    .btn-dash-mini:hover { background: rgba(0, 255, 255, 0.25); }

    /* Section Header */
    .section-title {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 10px;
    }
    .section-title label {
      font-size: 0.8rem;
      text-transform: uppercase;
      letter-spacing: 0.7px;
      font-weight: 600;
      color: var(--muted);
      margin: 0;
    }
    .btn-rescan {
      background: transparent;
      border: none;
      color: var(--cyan);
      font-size: 0.8rem;
      font-weight: 600;
      display: flex;
      align-items: center;
      gap: 5px;
      cursor: pointer;
      padding: 4px 8px;
      border-radius: 6px;
      transition: background 0.2s, opacity 0.2s;
    }
    .btn-rescan:hover { background: rgba(0, 255, 255, 0.1); }
    .btn-rescan.spinning svg { animation: spin 1s linear infinite; }

    /* Select Dropdown */
    .select-wrap {
      position: relative;
      margin-bottom: 12px;
    }
    select {
      width: 100%;
      background: var(--input-bg);
      border: 1px solid var(--border);
      border-radius: 10px;
      padding: 12px 36px 12px 14px;
      color: #fff;
      font-size: 15px;
      min-height: 48px;
      outline: none;
      appearance: none;
      -webkit-appearance: none;
      cursor: pointer;
      transition: border-color 0.2s, box-shadow 0.2s;
    }
    select:focus {
      border-color: var(--cyan);
      box-shadow: 0 0 12px rgba(0, 255, 255, 0.25);
    }
    .select-arrow {
      position: absolute;
      right: 14px;
      top: 50%;
      transform: translateY(-50%);
      pointer-events: none;
      color: var(--cyan);
    }
    select option {
      background: #0f1624;
      color: #fff;
      padding: 10px;
    }

    /* Network Cards List */
    .network-list {
      max-height: 180px;
      overflow-y: auto;
      border: 1px solid var(--border);
      border-radius: 10px;
      background: var(--input-bg);
      margin-bottom: 18px;
    }
    .network-list::-webkit-scrollbar { width: 5px; }
    .network-list::-webkit-scrollbar-thumb { background: rgba(0, 255, 255, 0.25); border-radius: 4px; }

    .net-item {
      display: flex;
      align-items: center;
      justify-content: space-between;
      padding: 11px 14px;
      border-bottom: 1px solid rgba(255, 255, 255, 0.05);
      cursor: pointer;
      transition: background 0.15s, border-color 0.15s;
    }
    .net-item:last-child { border-bottom: none; }
    .net-item:hover { background: var(--card-hover); }
    .net-item.selected {
      background: var(--card-selected);
      border-left: 3px solid var(--cyan);
    }
    .net-name {
      font-size: 0.92rem;
      font-weight: 500;
      display: flex;
      align-items: center;
      gap: 8px;
      flex: 1;
      min-width: 0;
    }
    .net-name span {
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
    }
    .net-meta {
      display: flex;
      align-items: center;
      gap: 8px;
      flex-shrink: 0;
      margin-left: 10px;
    }
    .net-rssi {
      font-size: 0.75rem;
      color: var(--muted);
      font-family: monospace;
    }
    .badge-open {
      font-size: 0.68rem;
      padding: 1px 6px;
      border-radius: 4px;
      background: rgba(0, 255, 157, 0.15);
      color: var(--mint);
      border: 1px solid rgba(0, 255, 157, 0.3);
      text-transform: uppercase;
      font-weight: 600;
    }
    .empty-state {
      padding: 24px;
      text-align: center;
      color: var(--muted);
      font-size: 0.85rem;
      line-height: 1.5;
    }
    .spinner {
      margin: 0 auto;
      width: 24px;
      height: 24px;
      border: 2px solid var(--border);
      border-top-color: var(--cyan);
      border-radius: 50%;
      animation: spin 0.8s linear infinite;
    }

    /* Form Fields */
    .form-group {
      margin-bottom: 14px;
    }
    .form-group label {
      display: block;
      font-size: 0.78rem;
      font-weight: 600;
      color: var(--muted);
      margin-bottom: 6px;
      text-transform: uppercase;
      letter-spacing: 0.5px;
    }
    .input-wrap {
      position: relative;
      display: flex;
      align-items: center;
    }
    input[type="text"], input[type="password"] {
      width: 100%;
      background: var(--input-bg);
      border: 1px solid var(--border);
      border-radius: 10px;
      padding: 12px 14px;
      color: #fff;
      font-size: 16px;
      min-height: 48px;
      touch-action: manipulation;
      outline: none;
      transition: border-color 0.2s, box-shadow 0.2s;
    }
    input[type="text"]:focus, input[type="password"]:focus {
      border-color: var(--cyan);
      box-shadow: 0 0 10px rgba(0, 255, 255, 0.25);
    }
    #pass { padding-right: 46px; }
    .btn-toggle-pw {
      position: absolute;
      right: 12px;
      background: transparent;
      border: none;
      color: var(--muted);
      cursor: pointer;
      display: flex;
      padding: 6px;
      transition: color 0.2s;
    }
    .btn-toggle-pw:hover { color: var(--cyan); }
    .input-hint {
      font-size: 0.75rem;
      color: var(--muted);
      margin-top: 4px;
    }

    /* Submit Button */
    .btn-connect {
      width: 100%;
      background: linear-gradient(135deg, #00d2ff, #00ffff);
      color: #050a12;
      border: none;
      border-radius: 10px;
      padding: 14px;
      font-size: 1rem;
      font-weight: 700;
      letter-spacing: 0.5px;
      cursor: pointer;
      margin-top: 10px;
      box-shadow: 0 4px 18px rgba(0, 255, 255, 0.35);
      transition: transform 0.15s, box-shadow 0.15s, opacity 0.15s;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 8px;
    }
    .btn-connect:hover {
      transform: translateY(-1px);
      box-shadow: 0 6px 24px rgba(0, 255, 255, 0.5);
    }
    .btn-connect:active { transform: translateY(1px); }
    .btn-connect:disabled {
      opacity: 0.5;
      cursor: not-allowed;
      transform: none;
      box-shadow: none;
    }

    /* Status Card */
    #status-card {
      display: none;
      margin-top: 18px;
      padding: 16px;
      border-radius: 12px;
      border: 1px solid var(--border);
      background: rgba(9, 14, 23, 0.96);
      animation: fadeIn 0.3s ease;
    }
    @keyframes fadeIn { from { opacity: 0; transform: translateY(6px); } to { opacity: 1; transform: translateY(0); } }

    .status-header {
      display: flex;
      align-items: center;
      gap: 10px;
      font-weight: 700;
      font-size: 1rem;
      margin-bottom: 8px;
    }
    .status-spinner {
      width: 18px;
      height: 18px;
      border: 2px solid rgba(0, 255, 255, 0.3);
      border-top-color: var(--cyan);
      border-radius: 50%;
      animation: spin 0.8s linear infinite;
    }
    .status-detail {
      font-size: 0.86rem;
      color: var(--muted);
      line-height: 1.5;
    }
    .btn-action-group {
      display: flex;
      flex-direction: column;
      gap: 8px;
      margin-top: 14px;
    }
    .btn-dashboard-link {
      display: block;
      text-align: center;
      background: var(--mint);
      color: #050a12;
      text-decoration: none;
      font-weight: 700;
      font-size: 0.9rem;
      padding: 11px 16px;
      border-radius: 8px;
      box-shadow: 0 4px 14px rgba(0, 255, 157, 0.35);
      transition: transform 0.15s;
    }
    .btn-dashboard-link:hover { transform: translateY(-1px); }
    .btn-dashboard-link.secondary {
      background: rgba(0, 255, 255, 0.15);
      color: var(--cyan);
      border: 1px solid rgba(0, 255, 255, 0.35);
      box-shadow: none;
    }
    .btn-retry {
      display: inline-block;
      margin-top: 12px;
      background: rgba(255, 51, 102, 0.15);
      color: var(--red);
      border: 1px solid rgba(255, 51, 102, 0.4);
      border-radius: 8px;
      padding: 9px 16px;
      font-size: 0.88rem;
      font-weight: 600;
      cursor: pointer;
      width: 100%;
    }

    /* Footer */
    .portal-footer {
      margin-top: 20px;
      text-align: center;
      font-size: 0.75rem;
      color: var(--muted);
    }
    .portal-footer span { color: var(--cyan); }
  </style>
</head>
<body>
  <div class="glow-sphere glow-cyan"></div>
  <div class="glow-sphere glow-purple"></div>

  <div class="portal-box">
    <!-- Brand Header -->
    <div class="brand-header">
      <div class="orbit-logo">
        <div class="ring outer"></div>
        <div class="ring inner"></div>
        <div class="core-dot"></div>
      </div>
      <div class="brand-info">
        <h1>ORBIT <span class="brand-badge">Wi-Fi Setup</span></h1>
        <p>Connect ORBIT to your local wireless network</p>
      </div>
    </div>

    <!-- Hotspot Status Banner -->
    <div class="ap-banner">
      <div class="ap-banner-text">
        Hotspot: <strong>ORBIT-Setup</strong> &bull; IP: <strong>192.168.4.1</strong>
      </div>
      <a href="/dash" class="btn-dash-mini">Dashboard &rarr;</a>
    </div>

    <!-- Available Wi-Fi Selection Section -->
    <div class="section-title">
      <label for="wifi-select">Select Wi-Fi Network</label>
      <button class="btn-rescan" id="btn-rescan" title="Scan again for nearby networks">
        <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><path d="M21.5 2v6h-6M2.5 22v-6h6M2 11.5a10 10 0 0 1 18.8-4.3M22 12.5a10 10 0 0 1-18.8 4.3"/></svg>
        <span>Rescan</span>
      </button>
    </div>

    <!-- Dropdown Selector -->
    <div class="select-wrap">
      %WIFI_DROPDOWN%
      <div class="select-arrow">
        <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><polyline points="6 9 12 15 18 9"></polyline></svg>
      </div>
    </div>

    <!-- Scanned Cards List (Alternative 1-Tap Picker) -->
    <div class="network-list" id="network-list">
      %NETWORKS_LIST%
    </div>

    <!-- Connection Form -->
    <form id="wifi-form">
      <div class="form-group">
        <label for="ssid">Selected Network (SSID)</label>
        <div class="input-wrap">
          <input type="text" id="ssid" name="ssid" placeholder="Choose above or type SSID" required autocomplete="off">
        </div>
      </div>

      <div class="form-group">
        <label for="pass">Network Password</label>
        <div class="input-wrap">
          <input type="password" id="pass" name="pass" placeholder="Enter Wi-Fi password (leave blank if open)">
          <button type="button" class="btn-toggle-pw" id="btn-toggle-pw" title="Show / hide password">
            <svg id="eye-icon" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path><circle cx="12" cy="12" r="3"></circle></svg>
          </button>
        </div>
        <div class="input-hint" id="pass-hint">Leave blank if network has no password</div>
      </div>

      <button type="submit" class="btn-connect" id="btn-submit">
        <span>Connect to Wi-Fi</span>
        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><line x1="5" y1="12" x2="19" y2="12"></line><polyline points="12 5 19 12 12 19"></polyline></svg>
      </button>
    </form>

    <!-- Status Feedback Card -->
    <div id="status-card">
      <div class="status-header" id="status-header">
        <div class="status-spinner" id="status-spinner"></div>
        <span id="status-title">Connecting...</span>
      </div>
      <div class="status-detail" id="status-detail">
        Sending credentials to ORBIT and negotiating connection...
      </div>
    </div>

    <!-- Footer -->
    <div class="portal-footer">
      Device: <span>ORBIT OS</span> &bull; Hotspot: <span>ORBIT-Setup</span>
    </div>
  </div>

  <script>
    const netSelect = document.getElementById('wifi-select');
    const netList = document.getElementById('network-list');
    const ssidInput = document.getElementById('ssid');
    const passInput = document.getElementById('pass');
    const passHint = document.getElementById('pass-hint');
    const btnRescan = document.getElementById('btn-rescan');
    const btnTogglePw = document.getElementById('btn-toggle-pw');
    const wifiForm = document.getElementById('wifi-form');
    const btnSubmit = document.getElementById('btn-submit');
    const statusCard = document.getElementById('status-card');
    const statusHeader = document.getElementById('status-header');
    const statusSpinner = document.getElementById('status-spinner');
    const statusTitle = document.getElementById('status-title');
    const statusDetail = document.getElementById('status-detail');

    let cachedNetworks = [];
    let autoRetryCount = 0;

    // Toggle password visibility
    btnTogglePw.addEventListener('click', () => {
      const isPw = passInput.type === 'password';
      passInput.type = isPw ? 'text' : 'password';
      btnTogglePw.style.color = isPw ? 'var(--cyan)' : 'var(--muted)';
    });

    // Helper: SVG Signal Icon based on RSSI
    function getSignalSvg(rssi) {
      const bars = rssi > -60 ? 4 : (rssi > -70 ? 3 : (rssi > -80 ? 2 : 1));
      const color = rssi > -65 ? 'var(--mint)' : (rssi > -78 ? 'var(--cyan)' : 'var(--amber)');
      return `
        <svg width="15" height="15" viewBox="0 0 24 24" fill="${color}">
          <rect x="2" y="16" width="3" height="6" rx="1" opacity="${bars >= 1 ? '1' : '0.25'}"/>
          <rect x="7" y="12" width="3" height="10" rx="1" opacity="${bars >= 2 ? '1' : '0.25'}"/>
          <rect x="12" y="8" width="3" height="14" rx="1" opacity="${bars >= 3 ? '1' : '0.25'}"/>
          <rect x="17" y="4" width="3" height="18" rx="1" opacity="${bars >= 4 ? '1' : '0.25'}"/>
        </svg>
      `;
    }

    // Helper: SVG Padlock
    function getLockSvg(secure) {
      if (!secure) return '<span class="badge-open">OPEN</span>';
      return `
        <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="var(--muted)" stroke-width="2.2">
          <rect x="3" y="11" width="18" height="11" rx="2" ry="2"></rect>
          <path d="M7 11V7a5 5 0 0 1 10 0v4"></path>
        </svg>
      `;
    }

    function escapeHtml(str) {
      return str.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');
    }

    // Select network action (globally accessible from inline HTML onclick)
    window.selectNetwork = function(ssid, secure) {
      const netSelect = document.getElementById('wifi-select');
      ssidInput.value = ssid;
      if (netSelect) netSelect.value = ssid;

      // Update card visual highlight
      document.querySelectorAll('.net-item').forEach(el => {
        if (el.getAttribute('data-ssid') === ssid) {
          el.classList.add('selected');
        } else {
          el.classList.remove('selected');
        }
      });

      // Update password field hint
      if (secure) {
        passHint.textContent = 'Secured network (WPA/WPA2/WPA3)';
        passHint.style.color = 'var(--muted)';
        passInput.placeholder = 'Enter Wi-Fi password';
        passInput.focus();
      } else {
        passHint.textContent = 'Open network (no password required)';
        passHint.style.color = 'var(--mint)';
        passInput.value = '';
        passInput.placeholder = 'Open network — leave blank';
      }
    };

    function attachSelectListener() {
      const netSelect = document.getElementById('wifi-select');
      if (!netSelect) return;
      netSelect.onchange = () => {
        const val = netSelect.value;
        if (!val) {
          ssidInput.value = '';
          document.querySelectorAll('.net-item').forEach(el => el.classList.remove('selected'));
          return;
        }
        if (val === '__custom__') {
          ssidInput.value = '';
          ssidInput.placeholder = 'Enter hidden SSID manually';
          ssidInput.focus();
          passHint.textContent = 'Enter network password if secured';
          passHint.style.color = 'var(--muted)';
          document.querySelectorAll('.net-item').forEach(el => el.classList.remove('selected'));
          return;
        }
        const opt = netSelect.options[netSelect.selectedIndex];
        const isSecure = opt ? (opt.getAttribute('data-secure') !== 'false') : true;
        window.selectNetwork(val, isSecure);
      };
    }

    // Fetch and render scanned Wi-Fi networks
    async function fetchNetworks() {
      btnRescan.classList.add('spinning');
      try {
        const res = await fetch('/api/wifi/scan');
        const data = await res.json();

        if (data.scanning) {
          setTimeout(fetchNetworks, 800);
          return;
        }

        if ((!data.networks || data.networks.length === 0) && autoRetryCount < 6) {
          autoRetryCount++;
          setTimeout(fetchNetworks, 1000);
          return;
        }

        btnRescan.classList.remove('spinning');
        cachedNetworks = data.networks || [];

        renderNetworkOptions(cachedNetworks);
      } catch (err) {
        btnRescan.classList.remove('spinning');
        netSelect.innerHTML = `
          <option value="">-- Choose from available Wi-Fi networks --</option>
          <option value="__custom__">+ Enter network manually (Hidden SSID)...</option>
        `;
        netList.innerHTML = '<div class="empty-state">Unable to load nearby networks.<br>Click <strong>Rescan</strong> or enter your SSID manually below.</div>';
      }
    }

    function renderNetworkOptions(networks) {
      // 1. Populate Dropdown <select>
      netSelect.innerHTML = '<option value="">-- Choose an available Wi-Fi network (' + networks.length + ' found) --</option>';
      networks.forEach(net => {
        const opt = document.createElement('option');
        opt.value = net.ssid;
        const lockStr = net.secure ? ' [🔒]' : ' [Open]';
        opt.textContent = `${net.ssid} (${net.rssi} dBm)${lockStr}`;
        netSelect.appendChild(opt);
      });
      const customOpt = document.createElement('option');
      customOpt.value = '__custom__';
      customOpt.textContent = '+ Enter network manually (Hidden SSID)...';
      netSelect.appendChild(customOpt);

      // Preserve current selection in dropdown if already set
      if (ssidInput.value) {
        netSelect.value = ssidInput.value;
      }

      attachSelectListener();

      // 2. Populate Interactive Cards List
      if (networks.length === 0) {
        netList.innerHTML = `
          <div class="empty-state">
            No wireless networks found.<br>Click <strong>Rescan</strong> or enter your SSID manually below.
          </div>
        `;
        return;
      }

      netList.innerHTML = '';
      networks.forEach(net => {
        const item = document.createElement('div');
        item.className = 'net-item';
        item.setAttribute('data-ssid', net.ssid);
        if (ssidInput.value === net.ssid) item.classList.add('selected');

        item.innerHTML = `
          <div class="net-name">
            ${getLockSvg(net.secure)}
            <span>${escapeHtml(net.ssid)}</span>
          </div>
          <div class="net-meta">
            <span class="net-rssi">${net.rssi} dBm</span>
            ${getSignalSvg(net.rssi)}
          </div>
        `;
        item.addEventListener('click', () => {
          selectNetwork(net.ssid, net.secure);
        });
        netList.appendChild(item);
      });
    }

    // Manual Rescan Button
    btnRescan.addEventListener('click', () => {
      autoRetryCount = 0;
      btnRescan.classList.add('spinning');
      netSelect.innerHTML = '<option value="">-- Scanning for available networks... --</option>';
      netList.innerHTML = `
        <div class="empty-state">
          <div class="spinner"></div>
          <div style="margin-top: 10px;">Scanning for nearby wireless networks...</div>
        </div>
      `;
      fetch('/api/wifi/scan', { method: 'POST' })
        .then(() => setTimeout(fetchNetworks, 1200))
        .catch(() => fetchNetworks());
    });

    // Form Submission: Connect to chosen Wi-Fi
    wifiForm.addEventListener('submit', async (e) => {
      e.preventDefault();
      const ssid = ssidInput.value.trim();
      const pass = passInput.value;

      if (!ssid) {
        alert('Please select or enter a Wi-Fi SSID.');
        return;
      }

      btnSubmit.disabled = true;
      statusCard.style.display = 'block';
      statusHeader.style.color = 'var(--cyan)';
      statusSpinner.style.display = 'block';
      statusTitle.textContent = 'Connecting to ' + ssid + '...';
      statusDetail.innerHTML = 'Sending credentials to ORBIT. Please wait while connection is established with your Wi-Fi router...';

      try {
        const formData = new URLSearchParams();
        formData.append('ssid', ssid);
        formData.append('pass', pass);

        await fetch('/api/wifi/connect', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: formData
        });

        // Poll connection status
        let attempts = 0;
        const maxAttempts = 22; // 22 x 800ms ~ 17 seconds
        const pollInterval = setInterval(async () => {
          attempts++;
          try {
            const statRes = await fetch('/api/wifi/status');
            const stat = await statRes.json();

            if (stat.connected || stat.connect_status === 'connected') {
              clearInterval(pollInterval);
              statusSpinner.style.display = 'none';
              statusHeader.style.color = 'var(--mint)';
              statusTitle.innerHTML = '&#x2705; Connected Successfully!';
              statusDetail.innerHTML = `
                ORBIT is now connected to <strong>${escapeHtml(stat.ssid || ssid)}</strong>.<br>
                Assigned Local IP: <strong>${stat.ip}</strong><br>
                <div class="btn-action-group">
                  <a class="btn-dashboard-link" href="http://${stat.ip}">Open ORBIT Dashboard (${stat.ip}) &rarr;</a>
                  <a class="btn-dashboard-link secondary" href="http://orbit.local">Open via http://orbit.local &rarr;</a>
                </div>
                <div style="margin-top: 12px; font-size: 0.78rem; color: var(--muted); text-align: center;">
                  Setup complete! You can now switch your phone back to <strong>${escapeHtml(stat.ssid || ssid)}</strong>.
                </div>
              `;
              btnSubmit.disabled = false;
              return;
            } else if (stat.connect_status === 'failed' || stat.connect_status === 'not_found' || stat.connect_status === 'timeout') {
              clearInterval(pollInterval);
              showConnectionError(ssid, stat.connect_status);
              return;
            }
          } catch (e) {
            // Transient fetch error during Wi-Fi state change is ignored
          }

          if (attempts >= maxAttempts) {
            clearInterval(pollInterval);
            showConnectionError(ssid, 'timeout');
          }
        }, 800);

      } catch (err) {
        showConnectionError(ssid, 'submission');
      }
    });

    function showConnectionError(ssid, reason) {
      statusSpinner.style.display = 'none';
      statusHeader.style.color = 'var(--red)';
      statusTitle.innerHTML = '&#x26A0;&#xFE0F; Connection Failed';

      let msg = `Could not connect to <strong>${escapeHtml(ssid)}</strong>.`;
      if (reason === 'failed') {
        msg += ' Authentication failed. Please verify that your Wi-Fi password is correct.';
      } else if (reason === 'not_found') {
        msg += ' The network SSID could not be reached. Ensure the router is powered on and within range.';
      } else if (reason === 'timeout') {
        msg += ' Connection timed out. Please check that the password is correct and try again.';
      } else {
        msg += ' Failed to transmit credentials. Ensure your device is still connected to the ORBIT-Setup hotspot.';
      }

      statusDetail.innerHTML = `
        ${msg}
        <button type="button" class="btn-retry" onclick="resetFormForRetry()">Try Again</button>
      `;
      btnSubmit.disabled = false;
    }

    window.resetFormForRetry = function() {
      statusCard.style.display = 'none';
      passInput.focus();
      passInput.select();
    };

    // Initial load: bind listeners to server-rendered dropdown
    attachSelectListener();
    // Only perform client-side fetch if server-rendered list was empty
    if (document.querySelectorAll('.net-item').length === 0) {
      fetchNetworks();
    }
  </script>
</body>
</html>
)rawliteral";
