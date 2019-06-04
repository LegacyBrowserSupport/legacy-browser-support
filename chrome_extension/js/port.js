// Copyright 2013 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Defines the Port object.
 */


/**
 * Constructor for the Port object. This object encapsulates the communication
 * over the native messaging API to the host application.
 * @param {function} connect_callback The callback to be called when the
 * connection to the host has been established.
 * @param {function} disconnect_callback The callback to be called if the
 * connection to the host has permanently failed to be established.
 * @constructor
 */
Port = function(connect_callback, disconnect_callback) {
  this.MAX_CONNECT_ATTEMPTS = 3;
  this.port = null;
  this.callbacks = [];
  this.next_id = 1;
  this.connect_attempts_left = this.MAX_CONNECT_ATTEMPTS;
  this.last_connect_time = 0;
  this.connect_callback = connect_callback;
  this.disconnect_callback = disconnect_callback;

  this.initialize();
};

/**
 * Called by the constructor to initialize the port and open the connection to
 * the native app.
 */
Port.prototype.initialize = function() {
  var self = this;
  self.port = chrome.runtime.connectNative('org.mozilla.browserswitcher');
  if (chrome.runtime.lastError) {
    console.error(chrome.runtime.lastError.message);
    self.port = null;
    self.disconnect_callback();
    return;
  }
  self.port.onMessage.addListener(self.onMessageReceived.bind(self));
  self.port.onDisconnect.addListener(function() {
    console.error('Lost connection to the native host: ' +
                  chrome.runtime.lastError.message);
    self.port = null;
    if (--self.connect_attempts_left > 0) {
      // Retry up to three times to re-establish connection with some delay.
      // Meanwhile requests will fail but retrying them should succeed.
      setTimeout(self.initialize.bind(self), 200);
      return;
    }
    self.disconnect_callback();
  });
  try {
    // Try to send one message to verify that the port is usable.
    // This is unfortunately needed because the connectNative function might not
    // fail in some cases where the native app is actually unreachable.
    self.logError('Ignore me.', self.connect_callback);
  } catch (err) {
    self.port = null;
  }
};

/**
 * Callback that is handling messages received from the native host. If the
 * message id is 0 this means the response is not interesting for us and will be
 * discarded, otherwise the callback associated with this id will be called and
 * the message will be passed.
 * @param {Object} message The object passed from the native host. It will
 * contain two mandatory fields: id : the sequential number of the request or 0,
 * success : true if the request succeeded or false otherwise. If success is
 * false there will be an error field with a text description of the error.
 */
Port.prototype.onMessageReceived = function(message) {
  if (!message.success)
    console.error('Command Nr.' + message.id + ' failed:' + message.error);
  if (message.id !== 0) {
    this.callbacks[message.id](message);
    delete this.callbacks[message.id];
  }
};

/**
 * Checks if the port is still open and calls the callback right away
 * with an error response if it isn't.
 * @param {function} callback to be called if the port is not open anymore.
 * @return {boolean} true if the port is still open and false otherwise.
 */
Port.prototype.checkPortState = function(callback) {
  if (!this.port) {
    if (callback)
      callback({success: false, error: 'No native connection!'});
    return false;
  }
  return true;
};

/**
 * Adds a callback to the callbacks registry.
 * @param {function} callback The callback to be added.
 * @return {integer} The id of the callback added.
 */
Port.prototype.registerCallback = function(callback) {
  var id = 0;
  if (callback) {
    id = this.next_id;
    this.callbacks[id] = callback;
    this.next_id++;
  }
  return id;
};

/**
 * Calls the "logError" method of the native host.
 * @param {string} error string to be printed in the native host log.
 * @param {function} callback to be called when the call has finished.
 */
Port.prototype.logError = function(error, callback) {
  if (!this.checkPortState(callback))
    return;
  this.port.postMessage({'id': this.registerCallback(callback),
                         'command': 'logError',
                         'error': error });
};

/**
 * Calls the "invokeAlternativeBrowser" method of the native host.
 * @param {string} url to be opened in the alternative browser.
 * @param {function} callback to be called when the call has finished.
 */
Port.prototype.invokeAlternativeBrowser = function(url, callback) {
  if (!this.checkPortState(callback))
    return;
  this.port.postMessage({'id': this.registerCallback(callback),
                         'command': 'invokeAlternativeBrowser',
                         'url': url });
};

/**
 * Calls the "setProperties" method of the native host.
 * @param {Array.<{name: string, value:string}>} properties array to be set.
 * This array must consist of dictionaries with two elements name and value.
 * For example:
 * [{name: 'alternative_browser_path', value: '${ie}'}, {name: 'a', value:'b'}].
 * @param {function} callback to be called when the call has finished.
 */
Port.prototype.setProperties = function(properties, callback) {
  if (!this.checkPortState(callback))
    return;
  this.port.postMessage({id: this.registerCallback(callback),
                         command: 'setProperties',
                         properties: properties});
};

/**
 * Calls the "saveSettings" method of the native host.
 * @param {function} callback to be called when the call has finished.
 */
Port.prototype.saveSettings = function(callback) {
  if (!this.checkPortState(callback))
    return;
  this.port.postMessage({id: this.registerCallback(callback),
                         command: 'saveSettings'});
};

/**
 * Calls the "downloadIESiteList" method of the native host.
 * @param {function} callback to be called when the call has finished.
 */
Port.prototype.downloadIESiteList = function(callback) {
  if (!this.checkPortState(callback))
    return;
  this.port.postMessage({id: this.registerCallback(callback),
                         command: 'downloadIESiteList'});
};

/**
 * Calls the "getIESiteList" method of the native host.
 * @param {function} callback to be called when the call has finished.
 */
Port.prototype.getIESiteList = function(callback) {
  if (!this.checkPortState(callback))
    return;
  this.port.postMessage({id: this.registerCallback(callback),
                         command: 'getIESiteList'});
};
