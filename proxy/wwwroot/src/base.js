var statusLabel;

/**
 * Creates a WebSocket with a bounded incoming‐message queue.
 *
 * @param {string} url            – WebSocket URL
 * @param {(data: any) => void} handleMessage
 *                                – Your message‐processor callback
 * @param {object} [opts]
 * @param {number} [opts.maxQueue=100]
 *                                – Maximum queued messages
 * @returns {WebSocket}
 */
function openWebSocket(handleMessage, opts = {}) {
    const maxQueue = opts.maxQueue ?? 5;
    const msgQueue = [];
    let processing = false;
    let lastMessageTime = null;
    let statusInterval = null;
    let ws = null;
    let frameCount = 0;
    let fps = 0;
    let fpsInterval = null;

    // Format milliseconds into a human-readable duration
    function formatDuration(ms) {
        const sec = Math.floor(ms / 1000);
        const min = Math.floor(sec / 60);
        const s = sec % 60;
        if (min < 60)
            return `${min}:${s.toString().padStart(2, '0')}`;
        const hr = Math.floor(min / 60);
        const m = min % 60;
        if (hr < 24)
            return `${hr}:${m.toString().padStart(2, '0')}:${s.toString().padStart(2, '0')}`;
        const days = Math.floor(hr / 24);
        const h = hr % 24;

        return `${days} days, ${h.toString().padStart(2, '0')}:${m.toString().padStart(2, '0')}:${s.toString().padStart(2, '0')}`;
    }

    // Update the status label with the last message time and FPS
    function updateStatusLabel() {
        if (lastMessageTime) {
            const ago = Date.now() - lastMessageTime;
            statusLabel && (statusLabel.textContent = `Connected, time since last state: ${formatDuration(ago)} (${fps} FPS)`);
        } else
            statusLabel && (statusLabel.textContent = 'Connected, waiting for panel state...');
    }

    // Start the FPS counter
    function startFpsCounter() {
        fpsInterval && clearInterval(fpsInterval);

        fpsInterval = setInterval(() => {
            fps = frameCount;
            frameCount = 0;
        }, 1000);
    }

    // Enqueue + trigger processing
    function enqueue(msg) {
        if (msgQueue.length >= maxQueue) {
            // drop the oldest
            console.warn("WS message queue full, dropping oldest message");
            msgQueue.shift();
        }
        msgQueue.push(msg);
        processQueue();
    }

    // Drain the queue
    function processQueue() {
        if (processing)
            return;

        processing = true;

        while (msgQueue.length > 0) {
            frameCount++;

            const msg = msgQueue.shift();
            try {
                if (handleMessage(msg))
                    updateStatusLabel();
            } catch (e) {
                console.error("Error in handleMessage:", e);
            }

        }

        processing = false;
    }

    statusLabel = document.getElementById('status');

    // Create the WebSocket, using the port from the config provided by the proxy
    fetch('/config.json')
        .then(r => r.json())
        .then(cfg => {
            // Extract the last non-empty directory part from the path
            const pathParts = window.location.pathname.split('/').filter(Boolean);
            let panelKey = pathParts[pathParts.length - 1];

            // If the last part is a file (contains a dot), use the previous part
            if (panelKey.includes('.'))
                panelKey = pathParts[pathParts.length - 2];

            const port = cfg.proxyPorts[panelKey];
            const wsUrl = `ws://${location.hostname}:${port}`;
            ws = new WebSocket(wsUrl);

            ws.onopen = () => {
                statusLabel && (statusLabel.textContent = 'Connected');
                statusInterval = setInterval(updateStatusLabel, 1000); // update every 100ms for tenths
                startFpsCounter();
            };

            ws.onclose = () => {
                statusLabel && (statusLabel.textContent = 'Disconnected');
                clearInterval(statusInterval);
                clearInterval(fpsInterval);
            };

            ws.onerror = (e) => {
                statusLabel && (statusLabel.textContent = 'WebSocket error');
                clearInterval(statusInterval);
                clearInterval(fpsInterval);
            };

            ws.onmessage = (event) => {
                lastMessageTime = Date.now();
                enqueue(event.data);
            };
        });
}

