var messageHandler = require('./message_handler');

Pebble.addEventListener('ready', function(event) {
    console.log('PebbleKit JS ready!');
});

Pebble.addEventListener('appmessage', function(event) {
    messageHandler.handleAppMessage(event);
});

Pebble.addEventListener('showConfiguration', function(event) {
    console.log('Showing configuration page');
    // TODO: Replace with your hosted URL (e.g., GitHub Pages)
    // For local testing, you can use: https://your-username.github.io/pebble-sbb/config.html
    var url = 'https://albertgstoehl.github.io/pebble-sbb/config.html';
    Pebble.openURL(url);
});
