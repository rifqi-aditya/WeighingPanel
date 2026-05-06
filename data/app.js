let ws;
let weightDecimals = 2;
let uiFrozen = false;
let isCommandMode = false;
const gateway = `ws://${window.location.hostname}/ws`;

function initWebSocket() {
    ws = new WebSocket(gateway);
    ws.onopen = () => updateWsStatus(true);
    ws.onclose = () => {
        updateWsStatus(false);
        setTimeout(initWebSocket, 2000);
    };
    ws.onmessage = onMessage;
}

function updateWsStatus(connected) {
    const badge = document.getElementById('ws-status-badge');
    const statusText = badge?.querySelector('.status-text');
    const overlay = document.getElementById('weight-overlay');

    if (connected) {
        if (statusText) statusText.innerText = 'WEB CONNECTED';
        if (badge) badge.className = 'ws-status ws-connected';
        if (overlay) overlay.style.display = 'none';
    } else {
        if (statusText) statusText.innerText = 'WEB DISCONNECTED';
        if (badge) badge.className = 'ws-status ws-disconnected';
        if (overlay) overlay.style.display = 'flex';
    }
}

// FUNGSI SENTRAL UNTUK UPDATE UI COMMAND MODE
// Menghindari pengulangan teks di banyak tempat
function updateCommandUI(state) {
    if (!isCommandMode) return;

    const btn = document.querySelector('.btn-print');
    if (!btn) return;

    switch (state) {
        case 0: // IDLE_TARE
            btn.innerHTML = "Ambil Tare";
            btn.disabled = false;
            btn.style.background = "#3b82f6";
            uiFrozen = false; // Otomatis unfreeze jika sistem kembali ke awal
            break;
        case 1: // WAIT_TARE
            btn.innerHTML = "⏳ Mengambil Tare...";
            btn.disabled = true;
            break;
        case 2: // IDLE_GROSS
            btn.innerHTML = "Ambil Gross";
            btn.disabled = false;
            btn.style.background = "#22c55e";
            break;
        case 3: // WAIT_GROSS
            btn.innerHTML = "⏳ Mengambil Gross...";
            btn.disabled = true;
            break;
        case 4: // DONE / PRINTING
            btn.innerHTML = "Mulai Ulang Penimbangan";
            btn.disabled = false;
            btn.style.background = "#f59e0b"; // Warna Orange
            uiFrozen = true; // Bekukan data di layar agar bisa dibaca
            break;
    }
}

function onMessage(event) {
    const data = JSON.parse(event.data);
    
    if (data.type === 'weight') {
        if (!uiFrozen) {
            document.getElementById('weight-display').innerText = data.val.toFixed(weightDecimals);
        }
    } else if (data.type === 'command_update') {
        // Sinkronisasi: Jika operator lapangan mulai timbang baru (State 0), cairkan UI Web
        if (data.state === 0 && uiFrozen) uiFrozen = false;

        if (!uiFrozen) {
            document.getElementById('weight-display').innerText = data.netto.toFixed(weightDecimals);
            document.getElementById('val-tare').innerText = data.tare.toFixed(weightDecimals);
            document.getElementById('val-gross').innerText = data.gross.toFixed(weightDecimals);
        }
        updateCommandUI(data.state);
    } else if (data.type === 'stats') {
        updateStats(data);
    }
}

function updateStats(data) {
    const rssiEl = document.getElementById('stat-rssi');
    if (rssiEl) {
        if (data.rssi === 0) rssiEl.innerText = "AP Mode";
        else rssiEl.innerText = data.rssi + " dBm";
    }

    const setEl = (id, val) => {
        const el = document.getElementById(id);
        if (el) el.innerText = val;
    };

    setEl('stat-heap', `${Math.round((data.heapSize - data.heap) / 1024)}KB / ${Math.round(data.heapSize / 1024)}KB`);
    setEl('stat-uptime', formatUptime(data.uptime));
    setEl('stat-temp', data.temp.toFixed(1) + " °C");
    setEl('stat-reset', data.reset);
    setEl('stat-freq', data.freq + " MHz");
    setEl('rtc-time-display', data.rtcTime || "RTC Error");
    setEl('stat-ip', data.ip || "N/A");

    const indEl = document.getElementById('stat-indicator');
    if (indEl) {
        indEl.innerText = data.indicator ? "ONLINE" : "OFFLINE";
        indEl.style.color = data.indicator ? "#22c55e" : "#ef4444";
    }
}

function formatUptime(seconds) {
    const h = Math.floor(seconds / 3600);
    const m = Math.floor((seconds % 3600) / 60);
    const s = seconds % 60;
    return `${h}h ${m}m ${s}s`;
}

function showToast(message, type = 'success') {
    const container = document.getElementById('toast-container');
    if (!container) return;

    const toast = document.createElement('div');
    toast.className = `toast ${type}`;
    toast.innerHTML = `
        <span class="toast-icon">${type === 'success' ? '✅' : '❌'}</span>
        <span class="toast-msg">${message}</span>
    `;

    container.appendChild(toast);

    // Auto remove
    setTimeout(() => {
        toast.classList.add('fade-out');
        setTimeout(() => toast.remove(), 300);
    }, 3000);
}

function showTab(tabId) {
    document.querySelectorAll('.tab-content').forEach(t => t.classList.remove('active'));
    document.querySelectorAll('.nav-btn').forEach(b => b.classList.remove('active'));
    document.getElementById(tabId)?.classList.add('active');
    document.querySelector(`.nav-btn[data-tab="${tabId}"]`)?.classList.add('active');

    if (tabId === 'settings') loadSettings();
    if (tabId === 'history') loadHistory();
}

function toggleTheme() {
    const theme = document.body.getAttribute('data-theme') === 'dark' ? 'light' : 'dark';
    document.body.setAttribute('data-theme', theme);
    localStorage.setItem('theme', theme);
}

function loadSettings() {
    fetch('/api/config').then(r => r.json()).then(data => {
        isCommandMode = data.isCommandMode;
        const ptNameInput = document.getElementById('input-pt-name');
        if (ptNameInput) ptNameInput.value = data.companyName;
        
        const ptHeader = document.getElementById('header-pt-name');
        if (ptHeader) ptHeader.innerText = data.companyName;
        
        weightDecimals = data.weightDecimals || 2;
        const decInput = document.getElementById('input-decimals');
        if (decInput) decInput.value = weightDecimals;
        
        // Atur visibilitas dashboard berdasarkan mode
        const commandGrid = document.getElementById('command-values-grid');
        if (commandGrid) commandGrid.style.display = isCommandMode ? 'grid' : 'none';
        
        const weightLabel = document.querySelector('.weight-label');
        if (weightLabel) weightLabel.innerText = isCommandMode ? "Berat Bersih (Netto)" : "Berat Bersih (Continuous)";
        
        // Panggil fungsi sentral untuk inisialisasi tombol
        updateCommandUI(0);

        // Isi form settings lainnya (Handle angka 0 agar tidak dianggap kosong)
        const setVal = (id, val) => { 
            const el = document.getElementById(id); 
            if (el) el.value = (val !== undefined && val !== null) ? val : ''; 
        };
        setVal('input-wifi-ssid', data.wifiSsid);
        setVal('input-static-ip', data.staticIp);
        setVal('input-static-gw', data.gateway);
        setVal('input-static-sn', data.subnet);
        setVal('input-printer-type', data.printerType);
        setVal('input-paper-size', data.paperSize);
        
        const staticCheck = document.getElementById('input-static-enable');
        if (staticCheck) {
            staticCheck.checked = data.useStaticIp;
            toggleStaticFields();
        }
    });
}

function saveSettings() {
    const config = {
        companyName: document.getElementById('input-pt-name').value,
        weightDecimals: parseInt(document.getElementById('input-decimals').value),
        printerType: parseInt(document.getElementById('input-printer-type').value),
        paperSize: parseInt(document.getElementById('input-paper-size').value)
    };

    const status = document.getElementById('save-status');
    if (status) {
        status.innerText = "⏳ Menyimpan...";
        status.style.color = "var(--text-secondary)";
    }

    fetch('/api/config', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(config)
    })
    .then(r => r.json())
    .then(data => {
        if (data.status === 'saved') {
            showToast("Pengaturan Tiket Berhasil Disimpan!");
            // Update Header PT Name secara live
            document.getElementById('header-pt-name').innerText = config.companyName;
        }
    })
    .catch(err => {
        showToast("Gagal menyimpan pengaturan!", "error");
    });
}

function saveWiFi() {
    const ssid = document.getElementById('input-wifi-ssid').value;
    const pass = document.getElementById('input-wifi-pass').value;
    
    if (!ssid) {
        showToast("SSID WiFi tidak boleh kosong!", "error");
        return;
    }

    const config = {
        wifiSsid: ssid,
        wifiPassword: pass,
        useStaticIp: document.getElementById('input-static-enable').checked,
        staticIp: document.getElementById('input-static-ip').value,
        gateway: document.getElementById('input-static-gw').value,
        subnet: document.getElementById('input-static-sn').value
    };

    if (!confirm("Sistem akan RESTART untuk menerapkan pengaturan WiFi baru. Lanjutkan?")) return;

    fetch('/api/config', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(config)
    })
    .then(r => r.json())
    .then(data => {
        showToast("Konfigurasi WiFi Terkirim! Perangkat akan restart.", "success");
        setTimeout(() => location.reload(), 2000);
    })
    .catch(err => showToast("Gagal mengirim konfigurasi WiFi", "error"));
}

function toggleStaticFields() {
    const check = document.getElementById('input-static-enable');
    const fields = document.getElementById('static-ip-fields');
    if (check && fields) fields.style.display = check.checked ? 'block' : 'none';
}

function loadHistory() {
    const tableBody = document.getElementById('history-table-body');
    if (!tableBody) return;
    
    tableBody.innerHTML = '<tr><td colspan="2" style="text-align: center; padding: 1rem;">Loading...</td></tr>';

    fetch('/api/history').then(r => r.text()).then(csv => {
        const lines = csv.trim().split('\n');
        if (lines.length <= 1) {
            tableBody.innerHTML = '<tr><td colspan="2" style="text-align: center; padding: 1rem;">No data yet.</td></tr>';
            return;
        }
        let html = '';
        const startIdx = Math.max(1, lines.length - 50);
        for (let i = lines.length - 1; i >= startIdx; i--) {
            const cols = lines[i].split(',');
            if (cols.length >= 2) {
                html += `<tr style="border-bottom: 1px solid var(--border-color);">
                            <td style="padding: 0.75rem;">${cols[0]}</td>
                            <td style="padding: 0.75rem; font-weight: bold; color: var(--accent-color);">${cols[1]} kg</td>
                        </tr>`;
            }
        }
        tableBody.innerHTML = html;
    });
}

function clearHistory() {
    if (!confirm("Hapus semua catatan timbangan? Data tidak bisa dikembalikan.")) return;

    fetch('/api/history', { method: 'DELETE' })
        .then(r => r.json())
        .then(data => {
            if (data.status === 'cleared') {
                showToast("Log riwayat telah dibersihkan!");
                loadHistory(); // Refresh tabel
            }
        })
        .catch(err => showToast("Gagal menghapus riwayat!", "error"));
}

function printTicket() {
    // Jika UI sedang membeku (setelah cetak), klik tombol ini akan mereset tampilan saja
    if (uiFrozen) {
        uiFrozen = false;
        document.getElementById('weight-display').innerText = "0.00";
        document.getElementById('val-tare').innerText = "0.00";
        document.getElementById('val-gross').innerText = "0.00";
        updateCommandUI(0);
        return;
    }

    if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send("print");
        showToast("Perintah Cetak Terkirim!", "success");
        // Beri feedback visual sederhana jika bukan mode command
        if (!isCommandMode) {
            const btn = document.querySelector('.btn-print');
            const oldText = btn.innerHTML;
            btn.innerHTML = "⏳ Mencetak...";
            setTimeout(() => { btn.innerHTML = oldText; }, 2000);
        }
    } else {
        showToast("Koneksi WebSocket terputus!", "error");
    }
}

function syncTime() {
    const now = new Date();
    const unixTime = Math.floor(now.getTime() / 1000);
    
    fetch(`/api/set_time?unix=${unixTime}`)
        .then(r => r.text())
        .then(() => {
            showToast("Waktu RTC berhasil disinkronkan!");
        })
        .catch(err => showToast("Gagal sinkronisasi waktu!", "error"));
}

window.onload = () => {
    initWebSocket();
    loadSettings();
    const theme = localStorage.getItem('theme');
    if (theme) document.body.setAttribute('data-theme', theme);
};