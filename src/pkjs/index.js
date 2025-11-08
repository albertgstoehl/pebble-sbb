var messageHandler = require('./message_handler');
var configFavorites = [];
var configFavoritesExpected = 0;

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
        return;
    }

    // Otherwise handle normally
    messageHandler.handleAppMessage(event);
});

Pebble.addEventListener('showConfiguration', function(event) {
    console.log('Showing configuration page');

    // Request current favorites from watch
    Pebble.sendAppMessage({
        'REQUEST_FAVORITES': 1
    });

    // Build URL with existing favorites as query param
    var url = 'https://albertgstoehl.github.io/pebble-sbb/config.html';

    // Add favorites to URL once received (after small delay)
    setTimeout(function() {
        if (configFavorites.length > 0) {
            url += '?favorites=' + encodeURIComponent(JSON.stringify(configFavorites));
        }
        console.log('Opening config URL:', url);
        Pebble.openURL(url);
    }, 500);
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
