const statusLabel = document.getElementById('status');
let lastMessageTime = null;
let statusInterval = null;
let ws = null;
let frameCount = 0;
let fps = 0;
let fpsInterval = null;

function formatDuration(ms) {
    const tenths = Math.floor((ms % 1000) / 100);
    const sec = Math.floor(ms / 1000);
    if (sec < 60) return `${sec}.${tenths}s`;
    const min = Math.floor(sec / 60);
    const s = sec % 60;
    if (min < 60) return `${min}:${s.toString().padStart(2, '0')}`;
    const hr = Math.floor(min / 60);
    const m = min % 60;
    if (hr < 24) return `${hr}:${m.toString().padStart(2, '0')}:${s.toString().padStart(2, '0')}`;
    const days = Math.floor(hr / 24);
    const h = hr % 24;
    return `${days} days, ${h.toString().padStart(2, '0')}:${m.toString().padStart(2, '0')}:${s.toString().padStart(2, '0')}`;
}

function updateStatusLabel() {
    if (lastMessageTime) {
        const ago = Date.now() - lastMessageTime;
        statusLabel.textContent = `Connected, time since last state: ${formatDuration(ago)} (${fps} FPS)`;
    } else {
        statusLabel.textContent = 'Connected, waiting for panel state...';
    }
}

function startFpsCounter() {
    if (fpsInterval) clearInterval(fpsInterval);
    fpsInterval = setInterval(() => {
        fps = frameCount;
        frameCount = 0;
    }, 1000);
}

function openWebSocket(onMessageHandler) {
    fetch('/config.json')
        .then(r => r.json())
        .then(cfg => {
            // Extract the last non-empty directory part from the path
            const pathParts = window.location.pathname.split('/').filter(Boolean);
            let panelKey = pathParts[pathParts.length - 1];
            // If the last part is a file (contains a dot), use the previous part
            if (panelKey.includes('.')) {
                panelKey = pathParts[pathParts.length - 2];
            }
            const port = cfg.proxyPorts[panelKey];
            const wsUrl = `ws://${location.hostname}:${port}`;
            ws = new WebSocket(wsUrl);

            ws.onopen = () => {
                statusLabel.textContent = 'Connected';
                statusInterval = setInterval(updateStatusLabel, 1000); // update every 100ms for tenths
                startFpsCounter();
            };
            ws.onclose = () => {
                statusLabel.textContent = 'Disconnected';
                clearInterval(statusInterval);
                clearInterval(fpsInterval);
            };
            ws.onerror = (e) => {
                statusLabel.textContent = 'WebSocket error';
                clearInterval(statusInterval);
                clearInterval(fpsInterval);
            };
            ws.onmessage = (event) => {
                lastMessageTime = Date.now();
                frameCount++;
                if (onMessageHandler(event.data)) {
                    updateStatusLabel();
                }
            };
        });
}

