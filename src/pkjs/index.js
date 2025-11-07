const messageHandler = require('./message_handler');

Pebble.addEventListener('ready', () => {
  console.log('PebbleKit JS ready!');
});

Pebble.addEventListener('appmessage', (event) => {
  messageHandler.handleAppMessage(event);
});
