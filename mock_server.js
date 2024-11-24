const express = require('express');
const path = require('path');
const app = express();
const port = 4000;

// Enable JSON body parsing
app.use(express.json());

// Sample plant data that mimics your ESP32 setup
const plants = [
    {
        name: "Prickly Pear",
        ozPerWatering: 3.0,
        intervalMinutes: 20160,
        wateringHistory: [
            { timestamp: Date.now()/1000 - 86400, amount: 3.0 },
            { timestamp: Date.now()/1000 - 172800, amount: 3.0 },
            { timestamp: Date.now()/1000 - 259200, amount: 3.0 },
            { timestamp: Date.now()/1000 - 345600, amount: 3.0 },
            { timestamp: Date.now()/1000 - 432000, amount: 3.0 }
        ]
    },
    {
        name: "Rosemary",
        ozPerWatering: 4.0,
        intervalMinutes: 10080,
        wateringHistory: [
            { timestamp: Date.now()/1000 - 43200, amount: 4.0 },
            { timestamp: Date.now()/1000 - 129600, amount: 4.0 },
            { timestamp: Date.now()/1000 - 216000, amount: 4.0 },
            { timestamp: Date.now()/1000 - 302400, amount: 4.0 },
            { timestamp: Date.now()/1000 - 388800, amount: 4.0 }
        ]
    },
    {
        name: "Fittonia",
        ozPerWatering: 2.5,
        intervalMinutes: 1440,
        wateringHistory: [
            { timestamp: Date.now()/1000 - 3600, amount: 2.5 },
            { timestamp: Date.now()/1000 - 90000, amount: 2.5 },
            { timestamp: Date.now()/1000 - 176400, amount: 2.5 },
            { timestamp: Date.now()/1000 - 262800, amount: 2.5 },
            { timestamp: Date.now()/1000 - 349200, amount: 2.5 }
        ]
    }
];

// Serve static files from the data directory
app.use(express.static(path.join(__dirname, 'data')));

// Enable CORS for all routes
app.use((req, res, next) => {
    res.header('Access-Control-Allow-Origin', '*');
    res.header('Access-Control-Allow-Methods', 'GET, POST, PUT, DELETE, OPTIONS');
    res.header('Access-Control-Allow-Headers', 'Content-Type');
    if (req.method === 'OPTIONS') {
        return res.sendStatus(200);
    }
    next();
});

// Serve homepage at root
app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, 'data', 'homepage.html'));
});

// API endpoint for plant data
app.get('/api/plants', (req, res) => {
    res.json(plants);
});

// Update watering amount
app.put('/api/plants/:index/amount', (req, res) => {
    const index = parseInt(req.params.index);
    const { ozPerWatering } = req.body;
    
    if (isNaN(index) || index < 0 || index >= plants.length) {
        return res.status(400).json({ error: 'Invalid plant index' });
    }
    
    if (typeof ozPerWatering !== 'number' || ozPerWatering <= 0) {
        return res.status(400).json({ error: 'Invalid watering amount' });
    }
    
    plants[index].ozPerWatering = ozPerWatering;
    res.json({ success: true });
});

// Update watering interval
app.put('/api/plants/:index/interval', (req, res) => {
    const index = parseInt(req.params.index);
    const { intervalDays } = req.body;
    
    if (isNaN(index) || index < 0 || index >= plants.length) {
        return res.status(400).json({ error: 'Invalid plant index' });
    }
    
    if (typeof intervalDays !== 'number' || intervalDays <= 0) {
        return res.status(400).json({ error: 'Invalid interval' });
    }
    
    // Convert days to minutes
    plants[index].intervalMinutes = intervalDays * 24 * 60;
    res.json({ success: true });
});

// Trigger immediate watering
app.post('/api/plants/:index/water-now', (req, res) => {
    const index = parseInt(req.params.index);
    
    if (isNaN(index) || index < 0 || index >= plants.length) {
        return res.status(400).json({ error: 'Invalid plant index' });
    }
    
    // Add watering event to history
    plants[index].wateringHistory.unshift({
        timestamp: Date.now()/1000,
        amount: plants[index].ozPerWatering
    });
    
    // Keep only last 5 watering events
    plants[index].wateringHistory = plants[index].wateringHistory.slice(0, 5);
    
    res.json({ success: true });
});

app.put('/api/plants/:index/name', (req, res) => {
    const index = parseInt(req.params.index);
    const { name } = req.body;
    
    if (isNaN(index) || index < 0 || index >= plants.length) {
        return res.status(400).json({ error: 'Invalid plant index' });
    }
    
    if (typeof name !== 'string' || !name.trim()) {
        return res.status(400).json({ error: 'Invalid name' });
    }
    
    plants[index].name = name.trim();
    res.json({ success: true });
});

app.listen(port, () => {
    console.log(`Mock ESP32 server running at http://localhost:${port}`);
});

