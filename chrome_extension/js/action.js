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
 * @fileoverview Declarative web requests don't support callback and it is good
 * so because this guarantees that any request that does not need to be
 * filtered is going to be done as fast as possible. Therefore we need to
 * emulate the callback for the requests we are interested in through redirects
 * and this is what this script does.
 */


/**
 * Shows an error message.
 * @param {string} error The error message to show.
 */
function showError(error) {
  // Notify the user we have a problem.
  document.getElementById('throbber').classList.add('fade');
  document.title = chrome.i18n.getMessage('error_title');
  document.getElementById('redirect-title').innerText =
      chrome.i18n.getMessage('error_title');
  document.getElementById('redirect-message').innerHTML = error;
}

/**
 * Callback that is called when the native component has completed the request
 * to start the alternative browser.
 * @param {Object} msg The message from the native component.
 */
function invokeFinished(msg) {
  if (msg.success) {
    // Check if this is the last tab in this window or not and close it if it is
    // not the last one or point it to the newtab page otherwise.
    chrome.windows.get(chrome.windows.WINDOW_ID_CURRENT, { populate: true },
        function(window) {
          chrome.tabs.getCurrent(function(tab) {
            var extension = chrome.extension.getBackgroundPage().extension;
            if (!extension.keep_last_chrome_tab || window.tabs.length > 1)
              chrome.tabs.remove(tab.id);
            else
              chrome.tabs.update(tab.id, {url: 'chrome://newtab'});
          });
        });
  } else {
    console.error(msg.error);
    showError(chrome.i18n.getMessage('browser_error'));
  }
}

var url = window.location.hash.substring(1);

// Before even try to invoke the plugin check if we are properly invoked.
var anchor = document.createElement('a');
anchor.href = url;
if (anchor.protocol !== 'http:' && anchor.protocol !== 'https:' &&
    anchor.protocol !== 'file:') {
  showError(chrome.i18n.getMessage('wrong_protocol_error', [encodeURI(url)]));
} else {
  // Show some message while the redirection happens.
  document.title = chrome.i18n.getMessage('redirect_title');
  document.getElementById('redirect-title').innerText =
      chrome.i18n.getMessage('redirect_title');
  document.getElementById('redirect-message').innerHTML =
      chrome.i18n.getMessage('redirect_message', [encodeURI(anchor.hostname)]);
  document.getElementById('learn-more-message').innerHTML =
      chrome.i18n.getMessage('learn_more_message');

  var extension = chrome.extension.getBackgroundPage().extension;
  if (extension.urlNeedsRedirect(url))
    extension.invokeAlternativeBrowser(url, invokeFinished);
  else
    showError(chrome.i18n.getMessage('browser_error'));
}
