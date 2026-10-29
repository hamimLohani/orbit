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
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=5.0, viewport-fit=cover">
  <meta name="theme-color" content="#0a0e17">
  <meta name="apple-mobile-web-app-capable" content="yes">
  <meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
  <title>ORBIT OS — Wi-Fi Network Setup</title>
  <style>
    :root {
      --bg: #0a0e17;
      --card: rgba(16, 24, 38, 0.92);
      --card-hover: rgba(23, 34, 54, 0.95);
      --border: rgba(0, 255, 255, 0.18);
      --border-active: rgba(0, 255, 255, 0.6);
      --cyan: #00ffff;
      --purple: #b81fdf;
      --mint: #00ff9d;
      --red: #ff3366;
      --text: #f0f6fc;
      --muted: #8b9bb4;
      --input-bg: rgba(9, 14, 23, 0.9);
    }
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      background: var(--bg);
      color: var(--text);
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Oxygen, Ubuntu, sans-serif;
      min-height: 100vh;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      padding: 16px;
      position: relative;
      overflow-x: hidden;
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
      width: 100%;
      max-width: 440px;
      background: var(--card);
      border: 1px solid var(--border);
      border-radius: 16px;
      box-shadow: 0 12px 40px rgba(0, 0, 0, 0.6), 0 0 24px rgba(0, 255, 255, 0.08);
      backdrop-filter: blur(12px);
      padding: 28px 24px;
      position: relative;
      z-index: 1;
    }

    /* Brand Header */
    .brand-header {
      display: flex;
      align-items: center;
      gap: 14px;
      margin-bottom: 24px;
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
      font-size: 1.35rem;
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
      font-size: 0.85rem;
      margin-top: 2px;
    }

    /* Section Header */
    .section-title {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 12px;
    }
    .section-title span {
      font-size: 0.85rem;
      text-transform: uppercase;
      letter-spacing: 0.8px;
      font-weight: 600;
      color: var(--muted);
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

    /* Network List */
    .network-list {
      max-height: 190px;
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
      padding: 10px 14px;
      border-bottom: 1px solid rgba(255, 255, 255, 0.05);
      cursor: pointer;
      transition: background 0.15s, border-color 0.15s;
    }
    .net-item:last-child { border-bottom: none; }
    .net-item:hover { background: var(--card-hover); }
    .net-item.selected {
      background: rgba(0, 255, 255, 0.12);
      border-left: 3px solid var(--cyan);
    }
    .net-name {
      font-size: 0.92rem;
      font-weight: 500;
      display: flex;
      align-items: center;
      gap: 8px;
      word-break: break-all;
    }
    .net-meta {
      display: flex;
      align-items: center;
      gap: 8px;
    }
    .net-rssi {
      font-size: 0.75rem;
      color: var(--muted);
      font-family: monospace;
    }
    .empty-state {
      padding: 24px;
      text-align: center;
      color: var(--muted);
      font-size: 0.85rem;
    }

    /* Form Fields */
    .form-group {
      margin-bottom: 14px;
    }
    label {
      display: block;
      font-size: 0.8rem;
      font-weight: 500;
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
      border-radius: 8px;
      padding: 12px 14px;
      color: #fff;
      font-size: 16px;
      min-height: 46px;
      touch-action: manipulation;
      outline: none;
      transition: border-color 0.2s, box-shadow 0.2s;
    }
    input[type="text"]:focus, input[type="password"]:focus {
      border-color: var(--cyan);
      box-shadow: 0 0 10px rgba(0, 255, 255, 0.25);
    }
    .input-wrap input { padding-right: 42px; }
    .btn-toggle-pw {
      position: absolute;
      right: 10px;
      background: transparent;
      border: none;
      color: var(--muted);
      cursor: pointer;
      display: flex;
      padding: 5px;
      transition: color 0.2s;
    }
    .btn-toggle-pw:hover { color: var(--cyan); }

    /* Submit Button */
    .btn-connect {
      width: 100%;
      background: linear-gradient(135deg, #00d2ff, #00ffff);
      color: #050a12;
      border: none;
      border-radius: 8px;
      padding: 13px;
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

    /* Status Modal / Card */
    #status-card {
      display: none;
      margin-top: 18px;
      padding: 16px;
      border-radius: 10px;
      border: 1px solid var(--border);
      background: rgba(9, 14, 23, 0.95);
      animation: fadeIn 0.3s ease;
    }
    @keyframes fadeIn { from { opacity: 0; transform: translateY(6px); } to { opacity: 1; transform: translateY(0); } }

    .status-header {
      display: flex;
      align-items: center;
      gap: 10px;
      font-weight: 600;
      font-size: 0.95rem;
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
      font-size: 0.85rem;
      color: var(--muted);
      line-height: 1.4;
    }
    .btn-dashboard-link {
      display: inline-block;
      margin-top: 12px;
      background: var(--mint);
      color: #050a12;
      text-decoration: none;
      font-weight: 700;
      font-size: 0.9rem;
      padding: 9px 16px;
      border-radius: 6px;
      box-shadow: 0 4px 14px rgba(0, 255, 157, 0.35);
    }

    /* Footer Info */
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

    <!-- Direct Companion Dashboard Launcher -->
    <div style="background: rgba(0, 255, 255, 0.08); border: 1px solid rgba(0, 255, 255, 0.25); border-radius: 12px; padding: 12px 14px; margin-bottom: 20px; display: flex; align-items: center; justify-content: space-between; gap: 10px;">
      <div>
        <div style="font-weight: 700; font-size: 0.88rem; color: #00ffff; display: flex; align-items: center; gap: 6px;">
          <span>⚡</span>
          <span>Companion Dashboard</span>
        </div>
        <div style="font-size: 0.76rem; color: #8b9bb4; margin-top: 2px;">Control AI, lights & sensors in Hotspot mode</div>
      </div>
      <a href="/dash" style="background: #00ffff; color: #0a0e17; font-weight: 700; font-size: 0.8rem; padding: 7px 14px; border-radius: 6px; text-decoration: none; white-space: nowrap; box-shadow: 0 2px 10px rgba(0,255,255,0.3); transition: transform 0.15s ease;">Open &rarr;</a>
    </div>

    <!-- Scanned Wi-Fi Section -->
    <div class="section-title">
      <span>Nearby Networks (2.4 GHz)</span>
      <button class="btn-rescan" id="btn-rescan" title="Scan again">
        <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><path d="M21.5 2v6h-6M2.5 22v-6h6M2 11.5a10 10 0 0 1 18.8-4.3M22 12.5a10 10 0 0 1-18.8 4.3"/></svg>
        <span>Rescan</span>
      </button>
    </div>

    <div class="network-list" id="network-list">
      <div class="empty-state" id="scan-status-msg">Scanning wireless networks...</div>
    </div>

    <!-- Connection Form -->
    <form id="wifi-form">
      <div class="form-group">
        <label for="ssid">Network Name (SSID)</label>
        <div class="input-wrap">
          <input type="text" id="ssid" name="ssid" placeholder="Choose above or type SSID" required autocomplete="off">
        </div>
      </div>

      <div class="form-group">
        <label for="pass">Password</label>
        <div class="input-wrap">
          <input type="password" id="pass" name="pass" placeholder="Network password (leave blank if open)">
          <button type="button" class="btn-toggle-pw" id="btn-toggle-pw" title="Show / hide password">
            <svg id="eye-icon" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path><circle cx="12" cy="12" r="3"></circle></svg>
          </button>
        </div>
      </div>

      <button type="submit" class="btn-connect" id="btn-submit">
        <span>Connect to Wi-Fi</span>
        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><line x1="5" y1="12" x2="19" y2="12"></line><polyline points="12 5 19 12 12 19"></polyline></svg>
      </button>
    </form>

    <!-- Status Card -->
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
      Device SoftAP: <span>ORBIT-Setup</span> &bull; IP: <span>192.168.4.1</span>
    </div>
  </div>

  <script>
    const netList = document.getElementById('network-list');
    const ssidInput = document.getElementById('ssid');
    const passInput = document.getElementById('pass');
    const btnRescan = document.getElementById('btn-rescan');
    const btnTogglePw = document.getElementById('btn-toggle-pw');
    const wifiForm = document.getElementById('wifi-form');
    const btnSubmit = document.getElementById('btn-submit');
    const statusCard = document.getElementById('status-card');
    const statusHeader = document.getElementById('status-header');
    const statusSpinner = document.getElementById('status-spinner');
    const statusTitle = document.getElementById('status-title');
    const statusDetail = document.getElementById('status-detail');

    // Password visibility toggle
    btnTogglePw.addEventListener('click', () => {
      const isPw = passInput.type === 'password';
      passInput.type = isPw ? 'text' : 'password';
      btnTogglePw.style.color = isPw ? 'var(--cyan)' : 'var(--muted)';
    });

    // Helper: SVG Signal Icon based on RSSI
    function getSignalSvg(rssi) {
      const bars = rssi > -60 ? 4 : (rssi > -70 ? 3 : (rssi > -80 ? 2 : 1));
      const color = rssi > -65 ? 'var(--mint)' : (rssi > -78 ? 'var(--cyan)' : 'var(--red)');
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
      if (!secure) return '';
      return `
        <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="var(--muted)" stroke-width="2.5">
          <rect x="3" y="11" width="18" height="11" rx="2" ry="2"></rect>
          <path d="M7 11V7a5 5 0 0 1 10 0v4"></path>
        </svg>
      `;
    }

    // Fetch and render scanned Wi-Fi networks
    let autoRetryCount = 0;
    async function fetchNetworks() {
      btnRescan.classList.add('spinning');
      try {
        const res = await fetch('/api/wifi/scan');
        const data = await res.json();

        if (data.scanning) {
          // Still scanning, retry after 800ms
          setTimeout(fetchNetworks, 800);
          return;
        }

        // If no networks returned yet, auto-retry a few times (device scan may be completing)
        if ((!data.networks || data.networks.length === 0) && autoRetryCount < 4) {
          autoRetryCount++;
          setTimeout(fetchNetworks, 1200);
          return;
        }

        btnRescan.classList.remove('spinning');
        netList.innerHTML = '';

        if (!data.networks || data.networks.length === 0) {
          netList.innerHTML = `
            <div class="empty-state">
              No 2.4 GHz networks discovered yet.<br>Click <strong>Rescan</strong> or enter your SSID manually below.
              <div style="margin-top: 10px; font-size: 0.75rem; color: #8b9bb4; line-height: 1.4;">
                💡 <em>Note: If connecting to a mobile phone hotspot, ensure <strong>2.4 GHz Band</strong> or <strong>"Maximize Compatibility"</strong> is turned ON in your phone's Hotspot settings.</em>
              </div>
            </div>
          `;
          return;
        }

        data.networks.forEach(net => {
          const item = document.createElement('div');
          item.className = 'net-item';
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
            document.querySelectorAll('.net-item').forEach(el => el.classList.remove('selected'));
            item.classList.add('selected');
            ssidInput.value = net.ssid;
            passInput.focus();
          });
          netList.appendChild(item);
        });
      } catch (err) {
        btnRescan.classList.remove('spinning');
        netList.innerHTML = '<div class="empty-state">Unable to load scan results. Enter SSID manually below.</div>';
      }
    }

    function escapeHtml(str) {
      return str.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');
    }

    // Manual Rescan Button
    btnRescan.addEventListener('click', () => {
      autoRetryCount = 0;
      btnRescan.classList.add('spinning');
      netList.innerHTML = '<div class="empty-state"><div class="spinner" style="margin: 0 auto 12px; width: 24px; height: 24px; border: 2px solid var(--border); border-top-color: var(--cyan); border-radius: 50%; animation: spin 1s linear infinite;"></div>Scanning wireless networks...</div>';
      fetch('/api/wifi/scan', { method: 'POST' }).then(() => {
        setTimeout(fetchNetworks, 1200);
      }).catch(() => fetchNetworks());
    });

    // Form Submission: Connect
    wifiForm.addEventListener('submit', async (e) => {
      e.preventDefault();
      const ssid = ssidInput.value.trim();
      const pass = passInput.value;

      if (!ssid) return;

      btnSubmit.disabled = true;
      statusCard.style.display = 'block';
      statusHeader.style.color = 'var(--cyan)';
      statusSpinner.style.display = 'block';
      statusTitle.textContent = 'Connecting to ' + ssid + '...';
      statusDetail.innerHTML = 'Sending credentials to ORBIT. Please wait while the connection is established...';

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
        const maxAttempts = 16;
        const pollInterval = setInterval(async () => {
          attempts++;
          try {
            const statRes = await fetch('/api/wifi/status');
            const stat = await statRes.json();

            if (stat.connected) {
              clearInterval(pollInterval);
              statusSpinner.style.display = 'none';
              statusHeader.style.color = 'var(--mint)';
              statusTitle.innerHTML = '&#x2705; Connected Successfully!';
              statusDetail.innerHTML = `
                ORBIT is now connected to <strong>${escapeHtml(stat.ssid || ssid)}</strong>.<br>
                Assigned Local IP: <strong>${stat.ip}</strong><br><br>
                You can now connect your device to your home Wi-Fi and open:<br>
                <a class="btn-dashboard-link" href="http://${stat.ip}">Open ORBIT Dashboard (${stat.ip})</a>
                &nbsp;or&nbsp;
                <a class="btn-dashboard-link" href="http://orbit.local">http://orbit.local</a>
              `;
              btnSubmit.disabled = false;
              return;
            }
          } catch (e) {}

          if (attempts >= maxAttempts) {
            clearInterval(pollInterval);
            statusSpinner.style.display = 'none';
            statusHeader.style.color = 'var(--red)';
            statusTitle.innerHTML = '&#x26A0;&#xFE0F; Connection Failed';
            statusDetail.innerHTML = 'Could not connect to <strong>' + escapeHtml(ssid) + '</strong>. Please verify that the password is correct, ensure your router has 2.4GHz enabled, and try again.';
            btnSubmit.disabled = false;
          }
        }, 1000);

      } catch (err) {
        statusSpinner.style.display = 'none';
        statusHeader.style.color = 'var(--red)';
        statusTitle.textContent = 'Submission Failed';
        statusDetail.textContent = 'Could not send credentials. Please ensure your device is still connected to the ORBIT-Setup Wi-Fi network.';
        btnSubmit.disabled = false;
      }
    });

    // Initial load
    fetchNetworks();
  </script>
</body>
</html>
)rawliteral";
