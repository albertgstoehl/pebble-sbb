var messageHandler = require('./message_handler');

Pebble.addEventListener('ready', function(event) {
    console.log('PebbleKit JS ready!');
});

Pebble.addEventListener('appmessage', function(event) {
    messageHandler.handleAppMessage(event);
});
