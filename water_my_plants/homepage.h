#ifndef HOMEPAGE_H
#define HOMEPAGE_H

const char HOMEPAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <title>Smart Plant Watering System</title>
    <link href="https://cdnjs.cloudflare.com/ajax/libs/tailwindcss/2.2.19/tailwind.min.css" rel="stylesheet">
    <style>
        /* iOS specific styles */
        input[type="number"], input[type="text"] {
            -webkit-appearance: none;
            margin: 0;
            -moz-appearance: textfield;
            border-radius: 8px;
            padding: 0.5rem;
        }
        
        /* Prevent zoom on input focus for iOS */
        input, select, textarea {
            font-size: 16px;
        }
        
        /* Better tap targets for iOS */
        button {
            padding: 12px !important;
            touch-action: manipulation;
        }
        
        /* Disable pull-to-refresh */
        body {
            overscroll-behavior-y: contain;
        }
        
        /* Smooth scrolling for iOS */
        .scroll-container {
            -webkit-overflow-scrolling: touch;
        }

        /* Modal styles */
        .modal {
            position: fixed;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background-color: rgba(0, 0, 0, 0.5);
            display: flex;
            justify-content: center;
            align-items: center;
            z-index: 50;
        }

        .modal-content {
            background-color: white;
            padding: 1.5rem;
            border-radius: 1rem;
            width: 90%;
            max-width: 400px;
            margin: 1rem;
            box-shadow: 0 20px 25px -5px rgb(0 0 0 / 0.1), 0 8px 10px -6px rgb(0 0 0 / 0.1);
        }

        /* Prevent background scrolling when modal is open */
        .modal-open {
            overflow: hidden;
            position: fixed;
            width: 100%;
            height: 100%;
        }

        .edit-icon {
            cursor: pointer;
            opacity: 0.6;
            transition: opacity 0.2s;
        }
        
        .edit-icon:hover {
            opacity: 1;
        }

        @keyframes pulse {
            0% {
                transform: scale(1);
                opacity: 1;
                box-shadow: 0 0 0 0 rgba(34, 197, 94, 0.4);
            }
            50% {
                transform: scale(1.3);
                opacity: 0.7;
                box-shadow: 0 0 0 6px rgba(34, 197, 94, 0);
            }
            100% {
                transform: scale(1);
                opacity: 1;
                box-shadow: 0 0 0 0 rgba(34, 197, 94, 0);
            }
        }

        .status-pulse {
            animation: pulse 1.5s cubic-bezier(0.4, 0, 0.6, 1) infinite;
        }
    </style>
</head>
<body class="bg-gradient-to-br from-green-50 to-blue-50 min-h-screen p-4">
    <div class="max-w-7xl mx-auto">
        <header class="text-center mb-8">
            <h1 class="text-3xl font-bold text-gray-800 mb-2">Smart Plant Watering System</h1>
            <p class="text-gray-600 text-sm">Keeping your plants happy and healthy</p>
            <div class="mt-4 inline-flex items-center px-4 py-2 bg-green-100 rounded-full">
                <div id="status-indicator" class="w-3 h-3 rounded-full bg-green-500 mr-2 status-pulse"></div>
                <span id="update-status" class="text-sm text-green-700">Last updated: Just now</span>
            </div>
        </header>

        <div id="plants-container" class="grid grid-cols-1 gap-6 scroll-container">
            <div class="text-center p-8">
                <div class="inline-block p-6 bg-blue-50 rounded-lg">
                    <p class="text-blue-600 font-medium">Loading plant data...</p>
                </div>
            </div>
        </div>
    </div>

    <!-- Water confirmation modal -->
    <div id="confirmModal" class="modal" style="display: none;">
        <div class="modal-content">
            <h3 class="text-lg font-semibold text-gray-900 mb-4">Confirm Watering</h3>
            <p class="text-gray-600 mb-6">Are you sure you want to water this plant now?</p>
            <div class="flex justify-end space-x-3">
                <button onclick="hideConfirmModal()" class="px-4 py-2 text-gray-700 bg-gray-100 rounded-lg hover:bg-gray-200">
                    Cancel
                </button>
                <button id="confirmWaterButton" class="px-4 py-2 text-white bg-blue-500 rounded-lg hover:bg-blue-600">
                    Water Now
                </button>
            </div>
        </div>
    </div>

    <!-- Name edit modal -->
    <div id="nameEditModal" class="modal" style="display: none;">
        <div class="modal-content">
            <h3 class="text-lg font-semibold text-gray-900 mb-4">Rename Plant</h3>
            <div class="mb-6">
                <label for="newPlantName" class="block text-sm font-medium text-gray-700 mb-2">New Name:</label>
                <input 
                    type="text" 
                    id="newPlantName"
                    class="w-full px-3 py-2 border border-gray-300 rounded-lg shadow-sm focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-blue-500"
                    placeholder="Enter new name"
                >
            </div>
            <div class="flex justify-end space-x-3">
                <button onclick="hideNameEditModal()" class="px-4 py-2 text-gray-700 bg-gray-100 rounded-lg hover:bg-gray-200">
                    Cancel
                </button>
                <button id="confirmNameButton" class="px-4 py-2 text-white bg-blue-500 rounded-lg hover:bg-blue-600">
                    Save
                </button>
            </div>
        </div>
    </div>

    <script>
        const API_ENDPOINT = '/api/plants';
        
        function formatDate(timestamp) {
            if (!timestamp) return 'No record';
            try {
                return new Date(timestamp * 1000).toLocaleString();
            } catch (e) {
                console.error('Error formatting date:', e);
                return 'Invalid date';
            }
        }

        function formatInterval(minutes) {
            if (!minutes || isNaN(minutes)) return 'Unknown';
            return `${(minutes / 1440).toFixed(1)} days`;
        }

        // Store the pending actions
        let pendingWaterAction = null;
        let pendingNameEdit = null;

        // Water confirmation modal functions
        function showConfirmModal(index) {
            pendingWaterAction = index;
            document.getElementById('confirmModal').style.display = 'flex';
            document.body.classList.add('modal-open');
            
            document.getElementById('confirmWaterButton').onclick = () => {
                if (pendingWaterAction !== null) {
                    waterNow(pendingWaterAction);
                }
                hideConfirmModal();
            };
        }

        function hideConfirmModal() {
            document.getElementById('confirmModal').style.display = 'none';
            document.body.classList.remove('modal-open');
            pendingWaterAction = null;
        }

        // Name edit modal functions
        function showNameEditModal(index, currentName) {
            pendingNameEdit = index;
            document.getElementById('nameEditModal').style.display = 'flex';
            document.body.classList.add('modal-open');
            
            const input = document.getElementById('newPlantName');
            input.value = currentName;
            input.select();
            
            document.getElementById('confirmNameButton').onclick = () => {
                const newName = input.value.trim();
                if (newName) {
                    updatePlantName(pendingNameEdit, newName);
                }
                hideNameEditModal();
            };

            input.onkeyup = (e) => {
                if (e.key === 'Enter') {
                    document.getElementById('confirmNameButton').click();
                } else if (e.key === 'Escape') {
                    hideNameEditModal();
                }
            };
        }

        function hideNameEditModal() {
            document.getElementById('nameEditModal').style.display = 'none';
            document.body.classList.remove('modal-open');
            pendingNameEdit = null;
        }

        // Close modals when clicking outside
        document.getElementById('confirmModal').addEventListener('click', (e) => {
            if (e.target.id === 'confirmModal') {
                hideConfirmModal();
            }
        });

        document.getElementById('nameEditModal').addEventListener('click', (e) => {
            if (e.target.id === 'nameEditModal') {
                hideNameEditModal();
            }
        });

        // Wrapper functions for backward compatibility
        async function updateWateringAmount(index, amount) {
            return updatePlantAmount(index, amount);
        }

        async function updateWateringInterval(index, days) {
            return updatePlantInterval(index, days);
        }

        // Update dashboard to use fetchPlants
        async function updateDashboard() {
            return fetchPlants();
        }

        // Rename fetchPlants to be more descriptive
        async function fetchPlants() {
            try {
                const response = await fetch(API_ENDPOINT);
                if (!response.ok) {
                    throw new Error(`HTTP error! status: ${response.status}`);
                }
                const plants = await response.json();
                
                if (!Array.isArray(plants)) {
                    throw new Error('Invalid data format received from API');
                }

                const container = document.getElementById('plants-container');
                if (!container) {
                    throw new Error('Plants container not found');
                }

                const plantCards = plants.map((plant, index) => {
                    try {
                        return createPlantCard(plant, index);
                    } catch (e) {
                        console.error('Error creating plant card:', e);
                        return '';
                    }
                }).join('');

                container.innerHTML = plantCards || `
                    <div class="col-span-3 text-center p-8">
                        <div class="inline-block p-6 bg-yellow-50 rounded-lg">
                            <p class="text-yellow-600 font-medium">No plants found</p>
                        </div>
                    </div>
                `;

                updateTimestamp();
                const indicator = document.getElementById('status-indicator');
                if (indicator) {
                    indicator.classList.remove('bg-red-500');
                    indicator.classList.add('bg-green-500');
                }
            } catch (error) {
                console.error('Error updating dashboard:', error);
                const container = document.getElementById('plants-container');
                const indicator = document.getElementById('status-indicator');

                if (indicator) {
                    indicator.classList.remove('bg-green-500');
                    indicator.classList.add('bg-red-500');
                }

                if (container) {
                    container.innerHTML = `
                        <div class="col-span-3 text-center p-8">
                            <div class="inline-block p-6 bg-red-50 rounded-lg">
                                <p class="text-red-600 font-medium">Error loading plant data</p>
                                <p class="text-red-500 text-sm mt-2">Please check your connection and try again</p>
                            </div>
                        </div>
                    `;
                }
            }
        }

        // Initial load and setup refresh
        fetchPlants();
        setInterval(fetchPlants, 60000);

        async function updatePlantAmount(index, amount) {
            try {
                const response = await fetch(`${API_ENDPOINT}/amount`, {
                    method: 'PUT',
                    headers: {
                        'Content-Type': 'application/json'
                    },
                    body: JSON.stringify({
                        plantIndex: index,
                        ozPerWatering: amount
                    })
                });
                
                if (!response.ok) throw new Error('Failed to update amount');
                await fetchPlants(); // Refresh the display
            } catch (error) {
                console.error('Error updating amount:', error);
                alert('Failed to update amount. Please try again.');
            }
        }

        async function updatePlantInterval(index, days) {
            try {
                const response = await fetch(`${API_ENDPOINT}/interval`, {
                    method: 'PUT',
                    headers: {
                        'Content-Type': 'application/json'
                    },
                    body: JSON.stringify({
                        plantIndex: index,
                        intervalDays: days
                    })
                });
                
                if (!response.ok) throw new Error('Failed to update interval');
                await fetchPlants(); // Refresh the display
            } catch (error) {
                console.error('Error updating interval:', error);
                alert('Failed to update interval. Please try again.');
            }
        }

        async function updatePlantName(index, name) {
            try {
                const response = await fetch(`${API_ENDPOINT}/name`, {
                    method: 'PUT',
                    headers: {
                        'Content-Type': 'application/json'
                    },
                    body: JSON.stringify({
                        plantIndex: index,
                        name: name
                    })
                });
                
                if (!response.ok) throw new Error('Failed to update name');
                await fetchPlants(); // Refresh the display
            } catch (error) {
                console.error('Error updating name:', error);
                alert('Failed to update name. Please try again.');
            }
        }

        async function waterNow(index) {
            try {
                const response = await fetch(`${API_ENDPOINT}/water-now`, {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json'
                    },
                    body: JSON.stringify({
                        plantIndex: index
                    })
                });
                
                if (!response.ok) throw new Error('Failed to water plant');
                await fetchPlants(); // Refresh the display
            } catch (error) {
                console.error('Error watering plant:', error);
                alert('Failed to water plant. Please try again.');
            }
        }

        function createPlantCard(plant, index) {
            if (!plant || !plant.name) {
                console.error('Invalid plant data:', plant);
                return '';
            }

            return `
                <div class="bg-white rounded-xl shadow-lg overflow-hidden">
                    <div class="p-4 sm:p-6">
                        <div class="flex justify-between items-start mb-4">
                            <div class="flex items-center space-x-2">
                                <h2 class="text-xl sm:text-2xl font-semibold text-gray-800 mb-1">${plant.name}</h2>
                                <button 
                                    onclick="showNameEditModal(${index}, '${plant.name.replace(/'/g, "\\'")}')"
                                    class="edit-icon p-1 text-gray-400 hover:text-gray-600"
                                >
                                    ✎
                                </button>
                            </div>
                            <div class="flex flex-col items-end">
                                <span class="text-gray-500 text-sm mb-1">Pump ${index + 1}</span>
                            </div>
                        </div>

                        <div class="space-y-4 bg-gray-50 p-4 rounded-lg mb-4">
                            <div class="flex flex-col space-y-2">
                                <label class="text-gray-600 text-sm">Water Amount (oz):</label>
                                <div class="flex space-x-2">
                                    <input 
                                        type="number" 
                                        step="0.1" 
                                        value="${plant.ozPerWatering}"
                                        class="flex-1 border rounded-lg px-3 py-2 shadow-sm"
                                        onchange="updateWateringAmount(${index}, parseFloat(this.value))"
                                    >
                                </div>
                            </div>
                            <div class="flex flex-col space-y-2">
                                <label class="text-gray-600 text-sm">Watering Interval (days):</label>
                                <div class="flex space-x-2">
                                    <input 
                                        type="number" 
                                        step="0.5" 
                                        value="${(plant.intervalMinutes / 1440).toFixed(1)}"
                                        class="flex-1 border rounded-lg px-3 py-2 shadow-sm"
                                        onchange="updateWateringInterval(${index}, parseFloat(this.value))"
                                    >
                                </div>
                            </div>
                            <button 
                                onclick="showConfirmModal(${index})"
                                class="w-full bg-blue-500 hover:bg-blue-600 active:bg-blue-700 text-white font-medium py-3 px-4 rounded-lg transition-colors"
                            >
                                Water Now
                            </button>
                        </div>

                        <div class="border-t pt-4">
                            <h3 class="text-sm font-semibold text-gray-700 mb-3">Recent Watering History</h3>
                            <div class="space-y-3">
                                ${(plant.wateringHistory || []).map((event, i) => `
                                    <div class="flex justify-between items-center text-sm ${i === 0 ? 'text-gray-800 font-medium' : 'text-gray-500'}">
                                        <span>${formatDate(event.timestamp)}</span>
                                        <span class="font-mono">${event.amount.toFixed(1)} oz</span>
                                    </div>
                                `).join('')}
                            </div>
                        </div>
                    </div>
                </div>
            `;
        }

        function updateTimestamp() {
            const status = document.getElementById('update-status');
            if (status) {
                status.textContent = `Last updated: ${new Date().toLocaleTimeString()}`;
            }
        }
    </script>
</body>
</html>
)rawliteral";

#endif