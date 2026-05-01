let ws;
        let weightDecimals = 2;
        const gateway = `ws://${window.location.hostname}/ws`;

        function initWebSocket() {
            console.log('Trying to open a WebSocket connection...');
            ws = new WebSocket(gateway);
            ws.onopen = onOpen;
            ws.onclose = onClose;
            ws.onmessage = onMessage;
        }

        function onOpen(event) {
            console.log('Connection opened');
            updateWsStatus(true);
        }

        function onClose(event) {
            console.log('Connection closed');
            updateWsStatus(false);
            setTimeout(initWebSocket, 2000);
        }

        function updateWsStatus(connected) {
            const badge = document.getElementById('ws-status-badge');
            const statusText = badge.querySelector('.status-text');
            const overlay = document.getElementById('weight-overlay');
            
            if (connected) {
                statusText.innerText = 'WEB CONNECTED';
                badge.className = 'ws-status ws-connected';
                overlay.style.display = 'none';
            } else {
                statusText.innerText = 'WEB DISCONNECTED';
                badge.className = 'ws-status ws-disconnected';
                overlay.style.display = 'flex';
            }
        }

        function onMessage(event) {
            const data = JSON.parse(event.data);
            if (data.type === 'weight') {
                document.getElementById('weight-display').innerText = data.val.toFixed(weightDecimals);
            } else if (data.type === 'stats') {
                const rssiVal = data.rssi;
                const rssiEl = document.getElementById('stat-rssi');
                if (rssiVal === 0) {
                    rssiEl.innerText = "AP Mode";
                    rssiEl.style.color = "var(--text-primary)";
                } else {
                    rssiEl.innerText = rssiVal + " dBm";
                    // Color coding for RSSI
                    if (rssiVal > -60) rssiEl.style.color = "#22c55e"; // Green
                    else if (rssiVal > -75) rssiEl.style.color = "#f59e0b"; // Orange
                    else rssiEl.style.color = "#ef4444"; // Red
                }
                
                const usedHeap = Math.round((data.heapSize - data.heap) / 1024);
                const totalHeap = Math.round(data.heapSize / 1024);
                document.getElementById('stat-heap').innerText = `${usedHeap}KB / ${totalHeap}KB`;
                
                document.getElementById('stat-uptime').innerText = formatUptime(data.uptime);
                
                const tempVal = data.temp;
                const tempEl = document.getElementById('stat-temp');
                tempEl.innerText = tempVal.toFixed(1) + " °C";
                
                // Enhanced color coding for temperature
                if (tempVal > 75) {
                    tempEl.style.color = "#ef4444"; // Red
                    tempEl.style.fontWeight = "bold";
                } else if (tempVal > 60) {
                    tempEl.style.color = "#f59e0b"; // Orange
                } else {
                    tempEl.style.color = "#22c55e"; // Green
                }

                document.getElementById('stat-reset').innerText = data.reset;
                document.getElementById('stat-freq').innerText = data.freq + " MHz";
                if (data.rtcTime) {
                    document.getElementById('rtc-time-display').innerText = data.rtcTime;
                }
                if (data.ip) {
                    document.getElementById('stat-ip').innerText = data.ip;
                }

                const indEl = document.getElementById('stat-indicator');
                if (data.indicator) {
                    indEl.innerText = "ONLINE";
                    indEl.style.color = "#22c55e";
                } else {
                    indEl.innerText = "OFFLINE";
                    indEl.style.color = "#ef4444";
                }
            }
        }

        function formatUptime(seconds) {
            const h = Math.floor(seconds / 3600);
            const m = Math.floor((seconds % 3600) / 60);
            const s = seconds % 60;
            return `${h}h ${m}m ${s}s`;
        }

        function showTab(tabId) {
            document.querySelectorAll('.tab-content').forEach(t => t.classList.remove('active'));
            document.querySelectorAll('.nav-btn').forEach(b => b.classList.remove('active'));
            
            const activeTab = document.getElementById(tabId);
            if (activeTab) {
                activeTab.classList.add('active');
                const activeBtn = document.querySelector(`.nav-btn[data-tab="${tabId}"]`);
                if (activeBtn) activeBtn.classList.add('active');
            }
            
            if (tabId === 'settings') loadSettings();
            if (tabId === 'history') loadHistory();
        }

        function toggleTheme() {
            const body = document.body;
            const currentTheme = body.getAttribute('data-theme');
            const newTheme = currentTheme === 'dark' ? 'light' : 'dark';
            body.setAttribute('data-theme', newTheme);
            localStorage.setItem('theme', newTheme);
        }

        function loadSettings() {
            fetch('/api/config')
                .then(r => r.json())
                .then(data => {
                    document.getElementById('input-pt-name').value = data.companyName;
                    document.getElementById('header-pt-name').innerText = data.companyName;
                    if (data.weightDecimals !== undefined) {
                        weightDecimals = data.weightDecimals;
                        document.getElementById('input-decimals').value = data.weightDecimals;
                    }
                    if (data.printerType !== undefined) {
                        document.getElementById('input-printer-type').value = data.printerType;
                    }
                    if (data.paperSize !== undefined) {
                        document.getElementById('input-paper-size').value = data.paperSize;
                    }
                    document.getElementById('input-wifi-ssid').value = data.wifiSsid || '';
                    document.getElementById('input-static-enable').checked = data.useStaticIp;
                    document.getElementById('input-static-ip').value = data.staticIp;
                    document.getElementById('input-static-gw').value = data.gateway;
                    document.getElementById('input-static-sn').value = data.subnet;
                    toggleStaticFields();
                });
        }

        function toggleStaticFields() {
            const isEnabled = document.getElementById('input-static-enable').checked;
            document.getElementById('static-ip-fields').style.display = isEnabled ? 'block' : 'none';
        }
        
        document.getElementById('input-static-enable')?.addEventListener('change', toggleStaticFields);

        function loadHistory() {
            const tableBody = document.getElementById('history-table-body');
            tableBody.innerHTML = '<tr><td colspan="2" style="text-align: center; padding: 1rem;">Loading...</td></tr>';
            
            fetch('/api/history')
                .then(r => r.text())
                .then(csv => {
                    const lines = csv.trim().split('\n');
                    if (lines.length <= 1) {
                        tableBody.innerHTML = '<tr><td colspan="2" style="text-align: center; padding: 1rem;">No data yet.</td></tr>';
                        return;
                    }
                    
                    let html = '';
                    // Skip header, take last 50 entries
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
            if (!confirm("Are you sure you want to clear all weighing logs?")) return;
            fetch('/api/history', { method: 'DELETE' })
                .then(() => loadHistory());
        }

        function saveWiFi() {
            const ssid = document.getElementById('input-wifi-ssid').value;
            const pass = document.getElementById('input-wifi-pass').value;
            const status = document.getElementById('save-status');
            const btn = event.target;
            
            if (!ssid) {
                alert("SSID cannot be empty!");
                return;
            }

            if (!confirm("Update WiFi settings and restart the panel?")) return;

            btn.disabled = true;
            btn.innerText = "⏳ Saving...";
            status.innerText = "Mengirim konfigurasi...";
            status.style.color = "var(--text-secondary)";

            fetch('/api/config', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ 
                    wifiSsid: ssid, 
                    wifiPassword: pass,
                    useStaticIp: document.getElementById('input-static-enable').checked,
                    staticIp: document.getElementById('input-static-ip').value,
                    gateway: document.getElementById('input-static-gw').value,
                    subnet: document.getElementById('input-static-sn').value
                })
            })
            .then(r => {
                if (!r.ok) throw new Error("HTTP Error " + r.status);
                return r.json();
            })
            .then(data => {
                if (data.status === 'saved') {
                    status.innerText = "✅ Tersimpan! Menunggu Restart...";
                    status.style.color = "#22c55e";
                    alert("WiFi Settings saved! The panel will now restart. Please reconnect to the new WiFi network in a few moments.");
                    setTimeout(() => { window.location.reload(); }, 5000);
                } else {
                    throw new Error("Save failed: " + data.status);
                }
            })
            .catch(err => {
                status.innerText = "❌ Gagal: " + err.message;
                status.style.color = "#ef4444";
                btn.disabled = false;
                btn.innerText = "Update WiFi & Restart";
                alert("Error saving WiFi: " + err.message);
            });
        }

        function saveSettings() {
            const ptName = document.getElementById('input-pt-name').value;
            const decimals = parseInt(document.getElementById('input-decimals').value);
            const pType = parseInt(document.getElementById('input-printer-type').value);
            const pSize = parseInt(document.getElementById('input-paper-size').value);
            const status = document.getElementById('save-status');
            status.innerText = "Menyimpan...";
            status.style.color = "var(--text-secondary)";

            fetch('/api/config', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ 
                    companyName: ptName, 
                    weightDecimals: decimals, 
                    printerType: pType,
                    paperSize: pSize
                })
            })
                .then(r => r.json())
                .then(data => {
                    if (data.status === 'saved') {
                        status.innerText = "✅ Berhasil disimpan!";
                        status.style.color = "#22c55e";
                        document.getElementById('header-pt-name').innerText = ptName;
                        weightDecimals = decimals;
                    }
                })
                .catch(() => {
                    status.innerText = "❌ Gagal menyimpan!";
                    status.style.color = "#ef4444";
                });
        }

        function syncTime() {
            const unixTime = Math.floor(Date.now() / 1000);
            fetch('/api/set_time?unix=' + unixTime)
                .then(r => r.text())
                .then(data => alert('Waktu RTC disinkronkan ke WIB!'));
        }

        function printTicket() {
            if (ws && ws.readyState === WebSocket.OPEN) {
                ws.send("print");
                // Animasi sederhana
                const btn = document.querySelector('.btn-print');
                const originalText = btn.innerText;
                btn.innerText = "⏳ Mencetak...";
                btn.disabled = true;
                setTimeout(() => {
                    btn.innerText = originalText;
                    btn.disabled = false;
                }, 3000);
            } else {
                alert('WebSocket belum terkoneksi!');
            }
        }

        // Initialize
        window.onload = () => {
            initWebSocket();
            loadSettings();
            const savedTheme = localStorage.getItem('theme');
            if (savedTheme) document.body.setAttribute('data-theme', savedTheme);
        };