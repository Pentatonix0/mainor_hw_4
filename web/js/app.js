function fetchData(callback) {
    fetch('/data')
        .then((r) => r.json())
        .then((d) => callback(d));
}

// ---------- CURRENT VALUE ----------
function startCurrent() {
    const el = document.getElementById('value');

    setInterval(() => {
        fetchData((data) => {
            el.innerText = data.current + ' cm';
        });
    }, 1000);
}

// ---------- GRAPH ----------
function startGraph() {
    const canvas = document.getElementById('graph');
    const ctx = canvas.getContext('2d');

    setInterval(() => {
        fetchData((data) => drawGraph(ctx, data.values));
    }, 1000);
}

function drawGraph(ctx, values) {
    ctx.clearRect(0, 0, 400, 200);

    const max = Math.max(...values);
    const min = Math.min(...values);
    const range = Math.max(max - min, 1);

    ctx.beginPath();
    ctx.moveTo(0, 200);

    for (let i = 0; i < values.length; i++) {
        const x = i * (400 / (values.length - 1));
        const y = 200 - ((values[i] - min) / range) * 180 - 10;
        ctx.lineTo(x, y);
    }

    ctx.strokeStyle = '#3498db';
    ctx.lineWidth = 2;
    ctx.stroke();
}
