(function() {
    var favorites = [];
    var selectedStation = null;
    var searchTimeout = null;
    var pebbleReady = false;

    // Check for Pebble environment
    function checkPebbleEnvironment() {
        console.log('Checking Pebble environment...');
        console.log('window.Pebble exists:', !!window.Pebble);
        console.log('User agent:', navigator.userAgent);

        if (window.Pebble) {
            console.log('Pebble object found!');
            pebbleReady = true;
        }
    }

    // Initialize on page load
    window.addEventListener('load', function() {
        checkPebbleEnvironment();

        // Request current favorites from watch
        if (window.Pebble) {
            Pebble.addEventListener('ready', function() {
                console.log('Pebble ready event fired');
                pebbleReady = true;
            });
        }

        // Set up event listeners
        document.getElementById('stationSearch').addEventListener('input', handleStationSearch);
        document.getElementById('labelInput').addEventListener('input', handleLabelInput);
        document.getElementById('addButton').addEventListener('click', handleAddFavorite);
        document.getElementById('saveButton').addEventListener('click', handleSave);

        // Hide autocomplete when clicking outside
        document.addEventListener('click', function(e) {
            if (!e.target.matches('#stationSearch')) {
                document.getElementById('autocompleteResults').style.display = 'none';
            }
        });

        renderFavorites();
        updateMaxWarning();
    });

    function handleStationSearch(e) {
        var query = e.target.value.trim();

        if (query.length < 2) {
            document.getElementById('autocompleteResults').style.display = 'none';
            selectedStation = null;
            updateAddButton();
            return;
        }

        // Debounce search
        clearTimeout(searchTimeout);
        searchTimeout = setTimeout(function() {
            searchStation(query);
        }, 300);
    }

    function searchStation(query) {
        var url = 'https://transport.opendata.ch/v1/locations?query=' + encodeURIComponent(query) + '&type=station';

        fetch(url)
            .then(function(response) {
                return response.json();
            })
            .then(function(data) {
                displayAutocompleteResults(data.stations || []);
            })
            .catch(function(error) {
                console.error('Search error:', error);
            });
    }

    function displayAutocompleteResults(stations) {
        var resultsDiv = document.getElementById('autocompleteResults');
        resultsDiv.innerHTML = '';

        if (stations.length === 0) {
            resultsDiv.style.display = 'none';
            return;
        }

        stations.slice(0, 10).forEach(function(station) {
            var item = document.createElement('div');
            item.className = 'autocomplete-item';
            item.textContent = station.name;
            item.dataset.id = station.id;
            item.dataset.name = station.name;

            item.addEventListener('click', function() {
                selectedStation = {
                    id: this.dataset.id,
                    name: this.dataset.name
                };
                document.getElementById('stationSearch').value = this.dataset.name;
                resultsDiv.style.display = 'none';
                updateAddButton();
            });

            resultsDiv.appendChild(item);
        });

        resultsDiv.style.display = 'block';
    }

    function handleLabelInput() {
        updateAddButton();
    }

    function updateAddButton() {
        var label = document.getElementById('labelInput').value.trim();
        var button = document.getElementById('addButton');
        button.disabled = !(selectedStation && label.length > 0 && favorites.length < 10);
    }

    function handleAddFavorite() {
        var label = document.getElementById('labelInput').value.trim();

        if (!selectedStation || !label || favorites.length >= 10) {
            return;
        }

        favorites.push({
            id: selectedStation.id,
            name: selectedStation.name,
            label: label
        });

        // Clear inputs
        document.getElementById('stationSearch').value = '';
        document.getElementById('labelInput').value = '';
        selectedStation = null;

        renderFavorites();
        updateMaxWarning();
        updateAddButton();
    }

    function handleDeleteFavorite(index) {
        favorites.splice(index, 1);
        renderFavorites();
        updateMaxWarning();
        updateAddButton();
    }

    function renderFavorites() {
        var list = document.getElementById('favoriteList');

        if (favorites.length === 0) {
            list.innerHTML = '<div class="empty-state">No favorites yet. Add your first destination below.</div>';
            return;
        }

        list.innerHTML = '';
        favorites.forEach(function(fav, index) {
            var li = document.createElement('li');
            li.className = 'favorite-item';

            var info = document.createElement('div');
            info.className = 'favorite-info';

            var labelDiv = document.createElement('div');
            labelDiv.className = 'favorite-label';
            labelDiv.textContent = fav.label;

            var stationDiv = document.createElement('div');
            stationDiv.className = 'favorite-station';
            stationDiv.textContent = fav.name;

            info.appendChild(labelDiv);
            info.appendChild(stationDiv);

            var deleteBtn = document.createElement('button');
            deleteBtn.className = 'delete-btn';
            deleteBtn.textContent = 'Delete';
            deleteBtn.addEventListener('click', function() {
                handleDeleteFavorite(index);
            });

            li.appendChild(info);
            li.appendChild(deleteBtn);
            list.appendChild(li);
        });
    }

    function updateMaxWarning() {
        var warning = document.getElementById('maxWarning');
        warning.style.display = favorites.length >= 10 ? 'block' : 'none';
    }

    function getQueryParam(variable, defaultValue) {
        var query = location.search.substring(1);
        var vars = query.split('&');
        for (var i = 0; i < vars.length; i++) {
            var pair = vars[i].split('=');
            if (pair[0] === variable) {
                return decodeURIComponent(pair[1]);
            }
        }
        return defaultValue || false;
    }

    function handleSave() {
        console.log('Save clicked');

        // Get return_to URL from query parameters (Pebble passes this)
        var return_to = getQueryParam('return_to', 'pebblejs://close#');
        console.log('return_to:', return_to);

        // Encode favorites as JSON and pass back via URL
        var configData = {
            favorites: favorites
        };

        var url = return_to + encodeURIComponent(JSON.stringify(configData));
        console.log('Closing with URL:', url);

        // Close and return data
        document.location = url;
    }
})();
