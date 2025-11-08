var messageHandler = require('./message_handler');

Pebble.addEventListener('ready', function(event) {
    console.log('PebbleKit JS ready!');
});

Pebble.addEventListener('appmessage', function(event) {
    messageHandler.handleAppMessage(event);
});

Pebble.addEventListener('showConfiguration', function(event) {
    console.log('Showing configuration page');
    var url = 'https://albertgstoehl.github.io/pebble-sbb/config.html';
    Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function(event) {
    console.log('Configuration page closed');

    if (!event.response) {
        console.log('No response from config page');
        return;
    }

    try {
        var configData = JSON.parse(decodeURIComponent(event.response));
        console.log('Received config data:', configData);

        // Send favorites to watch
        if (configData.favorites && configData.favorites.length > 0) {
            // Send number of favorites first
            Pebble.sendAppMessage({
                'NUM_FAVORITES': configData.favorites.length
            });

            // Send each favorite
            configData.favorites.forEach(function(fav) {
                Pebble.sendAppMessage({
                    'FAVORITE_DESTINATION_ID': fav.id,
                    'FAVORITE_DESTINATION_NAME': fav.name,
                    'FAVORITE_DESTINATION_LABEL': fav.label
                });
            });

            console.log('Sent ' + configData.favorites.length + ' favorites to watch');
        }
    } catch (error) {
        console.error('Error parsing config data:', error);
    }
});
