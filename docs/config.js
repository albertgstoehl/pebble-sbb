(function() {
    var favorites = [];
    var selectedStation = null;
    var searchTimeout = null;

    // Initialize on page load
    window.addEventListener('load', function() {
        // Request current favorites from watch
        if (window.Pebble) {
            Pebble.addEventListener('ready', function() {
                console.log('Config page ready');
                // Watch will send favorites if any exist
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

    function handleSave() {
        if (!window.Pebble) {
            // Testing mode - show what would be saved
            var message = 'Testing Mode - Would save ' + favorites.length + ' favorites:\n\n';
            favorites.forEach(function(fav) {
                message += '• ' + fav.label + ': ' + fav.name + '\n';
            });
            message += '\nTo actually save, open this page from the Pebble app Settings button.';
            alert(message);
            return;
        }

        // Send number of favorites first
        Pebble.sendAppMessage({
            'NUM_FAVORITES': favorites.length
        });

        // Send each favorite
        favorites.forEach(function(fav) {
            Pebble.sendAppMessage({
                'FAVORITE_DESTINATION_ID': fav.id,
                'FAVORITE_DESTINATION_NAME': fav.name,
                'FAVORITE_DESTINATION_LABEL': fav.label
            });
        });

        alert('Settings saved to watch!');

        // Close config page
        setTimeout(function() {
            window.location.href = 'pebblejs://close';
        }, 1000);
    }
})();
