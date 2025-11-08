var messageHandler = require('./message_handler');
var configFavorites = [];
var configFavoritesExpected = 0;
var configPagePending = false;

Pebble.addEventListener('ready', function(event) {
    console.log('PebbleKit JS ready!');
});

Pebble.addEventListener('appmessage', function(event) {
    var message = event.payload;

    // Check if this is favorite data for configuration page
    if (message.NUM_FAVORITES !== undefined) {
        console.log('Expecting ' + message.NUM_FAVORITES + ' favorites from watch');
        configFavoritesExpected = message.NUM_FAVORITES;
        configFavorites = [];

        // If expecting 0 favorites and config page pending, open immediately
        if (configFavoritesExpected === 0 && configPagePending) {
            openConfigPage();
        }
        return;
    }

    if (message.FAVORITE_DESTINATION_ID !== undefined &&
        message.FAVORITE_DESTINATION_NAME !== undefined &&
        message.FAVORITE_DESTINATION_LABEL !== undefined) {

        configFavorites.push({
            id: message.FAVORITE_DESTINATION_ID,
            name: message.FAVORITE_DESTINATION_NAME,
            label: message.FAVORITE_DESTINATION_LABEL
        });
        console.log('Received favorite: ' + message.FAVORITE_DESTINATION_LABEL);

        // If we have all favorites and config page pending, open it
        if (configFavorites.length === configFavoritesExpected && configPagePending) {
            openConfigPage();
        }
        return;
    }

    // Otherwise handle normally
    messageHandler.handleAppMessage(event);
});

function openConfigPage() {
    configPagePending = false;

    var url = 'https://albertgstoehl.github.io/pebble-sbb/config.html';
    if (configFavorites.length > 0) {
        url += '?favorites=' + encodeURIComponent(JSON.stringify(configFavorites));
        console.log('Opening config URL with ' + configFavorites.length + ' favorites');
    } else {
        console.log('Opening config URL (no favorites)');
    }
    Pebble.openURL(url);
}

Pebble.addEventListener('showConfiguration', function(event) {
    console.log('Showing configuration page');

    // Reset state
    configFavorites = [];
    configFavoritesExpected = 0;
    configPagePending = true;

    // Request current favorites from watch
    console.log('Sending REQUEST_FAVORITES to watch');
    Pebble.sendAppMessage({
        'REQUEST_FAVORITES': 1
    }, function() {
        console.log('REQUEST_FAVORITES sent successfully');
    }, function(e) {
        console.error('Failed to send REQUEST_FAVORITES:', e);
        // Open anyway after failure
        setTimeout(function() {
            if (configPagePending) {
                openConfigPage();
            }
        }, 500);
    });

    // Fallback: open after 2 seconds if favorites don't arrive
    setTimeout(function() {
        if (configPagePending) {
            console.log('Timeout waiting for favorites, opening anyway');
            openConfigPage();
        }
    }, 2000);
});

Pebble.addEventListener('webviewclosed', function(event) {
    console.log('Configuration page closed');

    if (!event.response) {
        console.log('No response from config page');
        return;
    }

    try {
        var configData = JSON.parse(decodeURIComponent(event.response));
        console.log('Received config data:', JSON.stringify(configData));

        // Send favorites to watch with delays between messages
        if (configData.favorites && configData.favorites.length > 0) {
            console.log('Sending ' + configData.favorites.length + ' favorites to watch');

            // Send number of favorites first
            Pebble.sendAppMessage({
                'NUM_FAVORITES': configData.favorites.length
            }, function() {
                console.log('Sent NUM_FAVORITES successfully');
            }, function(e) {
                console.error('Failed to send NUM_FAVORITES:', e);
            });

            // Send each favorite with delay
            configData.favorites.forEach(function(fav, index) {
                setTimeout(function() {
                    console.log('Sending favorite ' + (index + 1) + ':', fav.label);
                    Pebble.sendAppMessage({
                        'FAVORITE_DESTINATION_ID': fav.id,
                        'FAVORITE_DESTINATION_NAME': fav.name,
                        'FAVORITE_DESTINATION_LABEL': fav.label
                    }, function() {
                        console.log('Successfully sent: ' + fav.label);
                    }, function(e) {
                        console.error('Failed to send ' + fav.label + ':', e);
                    });
                }, (index + 1) * 100); // 100ms delay between each
            });
        } else {
            console.log('No favorites to send');
        }
    } catch (error) {
        console.error('Error parsing config data:', error);
    }
});
