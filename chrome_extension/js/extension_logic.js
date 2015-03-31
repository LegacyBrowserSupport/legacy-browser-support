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
 * @fileoverview Defines the ExtensionLogic object.
 */


/**
 * Encapsulates the logic of the extension.
 * @param {boolean} use_declarative_api Whether the old school webRequest API
 * should be used or the new and still borked declarativeWebRequest API.
 * @constructor
 */
ExtensionLogic = function(use_declarative_api) {
  this.use_declarative_api = use_declarative_api;
  this.url_list_info = [];
  this.url_greylist_info = [];
  this.url_list_loaded = false;
  this.keep_last_chrome_tab = true;
  this.port = null;

  // We have to be able to remove this listener so keep the bound function.
  this.request_listener = this.requestFilter.bind(this);

  /**
   * Types of url patterns known to the extension.
   * @enum {number}
   */
  this.UrlPatternType = {
    // A prefix pattern is of the form protocol://host[/path].
    PREFIX: 0,
    // A host pattern is of the form [subdomain.]domain[.tld].
    HOST: 1,
    // Negated prefix of the form !<PREFIX> is accepted if it doesn't match.
    NEGATED_PREFIX: 2,
    // Negated host is of the form !<HOST> and is accepted if it doesn't match.
    NEGATED_HOST: 3,
    // Is of the form * and matches any url.
    WILDCARD: 4
  };

  this.Policies = {
    URL_LIST: 'url_list',
    URL_GREYLIST: 'url_greylist',
    ALTERNATIVE_BROWSER_PATH: 'alternative_browser_path',
    ALTERNATIVE_BROWSER_ARGUMENTS: 'alternative_browser_arguments',
    CHROME_PATH: 'chrome_path',
    CHROME_ARGUMENTS: 'chrome_arguments',
    KEEP_LAST_CHROME_TAB: 'keep_last_chrome_tab'
  };

  this.initialize();
};

/**
 * Returns true when the policy has finished loading and false otherwise.
 * @return {boolean} True when policy has loaded.
 */
ExtensionLogic.prototype.hasFinishedLoading = function() {
  return this.url_list_loaded;
};

/**
 * Finishes the initialization process of the class once the Port has connected
 * or it has failed but the NPAPI plugin has been successfully instantiated.
 */
ExtensionLogic.prototype.finalizeInitialization = function() {
  if (!this.use_declarative_api)
    this.updateConfiguration('', 'managed');

  // Populate initial rules on installation or update.
  chrome.runtime.onInstalled.addListener(
      this.updateConfiguration.bind(this, '', 'managed'));
  // Observe future policy changes.
  chrome.storage.onChanged.addListener(this.updateConfiguration.bind(this));
};

/**
 * Callback that is called when the native messaging port has been initialized.
 * Only after this is true we can load the policy and start parsing urls.
 * It might be called later on again if the connection needed to be
 * re-established in which case initialization is skipped.
 */
ExtensionLogic.prototype.portConnected = function() {
  if (!this.hasFinishedLoading())
    this.finalizeInitialization();
};

/**
 * Callback that is called if the native messaging port loses its connection.
 * In this case the only thing we can do is stop redirecting since we will not
 * be able to invoke the alternative browser.
 */
ExtensionLogic.prototype.portDisconnected = function() {
  this.url_list_info = [];
  this.url_list_loaded = true;
  console.error('Native port not reachable!');
};

/**
 * Checks that a given url should be opened in the alternative browser based
 * on the given |url_list|. Returns those item of the |url_list| which forced
 * the redirection, or null if no need to redirect.
 * @param {string} url The url to be checked.
 * @return {string} reason The pattern in |url_list| which force to redirect.
 */
ExtensionLogic.prototype.urlListForceRedirection = function(url) {
  var anchor = document.createElement('a');
  anchor.href = url;

  var reason = null;
  this.url_list_info.some(function(item) {
    switch (item.type) {
      case this.UrlPatternType.WILDCARD:
        reason = item.url;
        break;
      case this.UrlPatternType.HOST:
        if (anchor.hostname.indexOf(item.url) !== -1) {
          reason = item.url;
          return true;
        }
        break;
      case this.UrlPatternType.NEGATED_HOST:
        if (anchor.hostname.indexOf(item.url) !== -1) {
          reason = null;
          return true;
        }
        break;
      case this.UrlPatternType.PREFIX:
        if (url.indexOf(item.url) === 0) {
          reason = item.url;
          return true;
        }
        break;
      case this.UrlPatternType.NEGATED_PREFIX:
        if (url.indexOf(item.url) === 0) {
          reason = null;
          return true;
        }
        break;
    }
    return false;
  }, this);
  return reason;
};

/**
 * Checks that a given url should continue loading based on the given
 * |url_greylist|. Returns those item of the |url_greylist| which forced
 * to stay in this browser, or null if no such items exist.
 * @param {string} url The url to be checked.
 * @return {string} reason The pattern in the greylist which force to stay.
 */
ExtensionLogic.prototype.urlGreyListForceStaying = function(url) {
  var anchor = document.createElement('a');
  anchor.href = url;

  var reason = null;
  this.url_greylist_info.some(function(item) {
    switch (item.type) {
      case this.UrlPatternType.HOST:
        if (anchor.hostname.indexOf(item.url) !== -1) {
          reason = item.url;
          return true;
        }
        break;
      case this.UrlPatternType.PREFIX:
        if (url.indexOf(item.url) === 0) {
          reason = item.url;
          return true;
        }
        break;
      case this.UrlPatternType.WILDCARD:
        reason = item.url;
        return true;
      case this.UrlPatternType.NEGATED_PREFIX:
      case this.UrlPatternType.NEGATED_HOST:
    }
    return false;
  }, this);
  return reason;
};

/**
 * Verifies if a given url has to be opened in the alternative browser. Returns
 * true if the url should be redirected and false if loading should continue in
 * Chrome.
 * @param {string} url The url to be checked.
 * @return {boolean} True iff the url should be opened in the alt. browser.
 */
ExtensionLogic.prototype.urlNeedsRedirect = function(url) {
  var anchor = document.createElement('a');
  anchor.href = url;

  if (anchor.protocol !== 'http:' && anchor.protocol !== 'https:' &&
      anchor.protocol !== 'file:') {
    return false;
  }

  var reason_to_go = this.urlListForceRedirection(url);
  var reason_to_stay = this.urlGreyListForceStaying(url);
  // Always prefer the more specific entry of the two lists.
  if (reason_to_stay !== null) {
     if (reason_to_go === '*')
       return true;
     if (reason_to_go !== null && reason_to_go.length < reason_to_stay.length)
       return false;
  }
  return (reason_to_go !== null);
};

/**
 * Processes a list of url patterns (|urls|) and create a list
 * of pairs (|urls_info|) as the pair of the pure url and its type. This
 * is done to speed up searching for matches when deciding for redirection.
 * @param {Array.<string>} urls The input url pattern list.
 * @return {Array.<{url: string, type: ExtensionLogic.UrlPatternType}>}
 * urls_info The output url pattern type list.
 */
ExtensionLogic.prototype.processUrlList = function(urls) {
  var type;
  var urls_info = [];
  urls.forEach(function(item) {
    if (item === '*') {
      type = this.UrlPatternType.WILDCARD;
    } else {
      if (item.indexOf('/') === -1) {
        type = this.UrlPatternType.HOST;
      } else {
        type = this.UrlPatternType.PREFIX;
      }
      if (item.indexOf('!') === 0) {
        type = (type == this.UrlPatternType.HOST) ?
            this.UrlPatternType.NEGATED_HOST :
            this.UrlPatternType.NEGATED_PREFIX;
        item = item.slice(1);
      }
    }
    urls_info.push({url: item, type: type});
  }, this);
  return urls_info;
};

/**
 * Normalizes URLs to be properly capitalized. Hosts are always lower case
 * whereas paths should preserve capitalization.
 * @param {string} item The item to add to the "url_list".
 * @return {string} href The normalized url.
 */
ExtensionLogic.prototype.normalizeUrl = function(item) {
  if (item.indexOf('/') === -1) {
    // Host entries should be pushed all lower case.
    return item.toLowerCase();
  } else {
    // Use an anchor to get the proper capitalization.
    var anchor = document.createElement('a');
    if (item.indexOf('!') === 0) {
      anchor.href = item.substring(1);
      return '!' + anchor.href;
    } else {
      anchor.href = item;
      return anchor.href;
    }
  }
};

/**
 * Creates a matcher for the declarativeWebRequest API from an entry in the URL
 * list.
 * We apply a simple rule here to decide what the admin wants. If the string
 * contains at least one forward slash it is assumed to be a proper url prefix
 * otherwise it is assumed to be host name piece.
 * @param {string} item The item to base the rule on.
 * @return {Object} matcher The matcher object based on the input url pattern.
 */
ExtensionLogic.prototype.createRequestMatcher = function(item) {
  var matcher = null;
  if (item.indexOf('/') === -1) {
    matcher = new chrome.declarativeWebRequest.RequestMatcher({
      url: { hostContains: item },
      resourceType: ['main_frame']
    });
  } else {
    matcher = new chrome.declarativeWebRequest.RequestMatcher({
      url: { urlPrefix: item },
      resourceType: ['main_frame']
    });
  }
  return matcher;
};

/**
 * Creates the set of rules for the declarative WebRequest api using the list of
 * urls specified in the managed storage under the "url_list" key. Then it sets
 * up the browser redirection parameters and saves the settings in the plugin.
 * If any item is missing it will be reset to the default by setting it to an
 * empty string or Array.
 * @param {Object} items Dictionary with items retrieved from the storage.
 */
ExtensionLogic.prototype.createRules = function(items) {
  if (chrome.runtime.lastError) {
    // Avoid wiping the current policy if there was an error reading the new
    // values. The cache should stay active until the situation is resolved.
    port.logError('background.js:createRules() : ' +
      chrome.runtime.lastError.message);
    return;
  }

  var self = this;

  var positive_condition_list = [];
  var negative_condition_list = [];
  var greylist_condition_list = [];
  var url_list_has_wildcard = false;
  var url_greylist_has_wildcard = false;
  var url_list = [];
  var url_greylist = [];
  var properties = [];

  // The function needs to verify if the passed url_list and url_greylist are
  // indeed Array-typed because if the policy lacks the proper schema in place
  // they will be served down as dictionary.
  if (items[this.Policies.URL_GREYLIST] &&
      items[this.Policies.URL_GREYLIST] instanceof Array) {
    if (items[this.Policies.URL_GREYLIST].indexOf('*') !== -1) {
      url_greylist_has_wildcard = true;
      url_greylist.push('*');
    } else {
      // We only process the rest of the greylist if wildcard was not set.
      items[this.Policies.URL_GREYLIST].forEach(function(item) {
        item = self.normalizeUrl(item);

        if (item.indexOf('!') !== 0) {
          if (self.use_declarative_api) {
            var matcher = self.createRequestMatcher(item);
            if (matcher) {
              greylist_condition_list.push(matcher);
            }
          }
          url_greylist.push(item);
        }
      });
    }
  }
  properties.push({name: this.Policies.URL_GREYLIST, value: url_greylist});

  if (items[this.Policies.URL_LIST] &&
      items[this.Policies.URL_LIST] instanceof Array) {
    items[this.Policies.URL_LIST].forEach(function(item) {
      item = self.normalizeUrl(item);
      if (self.use_declarative_api) {
        // If the url is prefixed with a '!' then it should be considered
        // negated and a single '*' means all urls.
        if (item === '*') {
          url_list_has_wildcard = true;
        } else if (item.indexOf('!') === 0) {
          var matcher = self.createRequestMatcher(item.substring(1));
          if (matcher)
            negative_condition_list.push(matcher);
        } else {
          var matcher = self.createRequestMatcher(item);
          if (matcher)
            positive_condition_list.push(matcher);
        }
      }
      if (item !== '*' || !url_greylist_has_wildcard) {
        url_list.push(item);
      }
    });
  }
  properties.push({name: 'urls_to_redirect', value: url_list});


  if (self.use_declarative_api) {
    var rules = [];
    // Create the rules based on the lists. The negative list has highest prio-
    // rity, then the positive list followed by the greylist. Finally the
    // catch-all list if required.
    if (negative_condition_list.length) {
      var negative_rule = {
        conditions: negative_condition_list,
        priority: 300,
        actions: [new chrome.declarativeWebRequest.IgnoreRules({
            lowerPriorityThan: 300
          })]
      };
      rules.push(negative_rule);
    }
    if (positive_condition_list.length) {
      var positive_rule = {
        conditions: positive_condition_list,
        priority: 200,
        actions: [new chrome.declarativeWebRequest.RedirectByRegEx({
          from: '(.*)',
          to: 'chrome-extension://' + chrome.i18n.getMessage('@@extension_id') +
              '/action.html#$1'
          }),
          new chrome.declarativeWebRequest.IgnoreRules({
            lowerPriorityThan: 200
          })]
      };
      rules.push(positive_rule);
    }
    if (greylist_condition_list.length) {
      var greylist_rule = {
        conditions: greylist_condition_list,
        priority: 150,
        actions: [new chrome.declarativeWebRequest.IgnoreRules({
            lowerPriorityThan: 150
          })]
      };
      rules.push(greylist_rule);
    }
    // When both wildcards are set then greylist takes preference.
    if (!url_greylist_has_wildcard && url_list_has_wildcard) {
      var catch_all_rule = {
        conditions: [ new chrome.declarativeWebRequest.RequestMatcher({
            url: {schemes: ["http", "https", "file"] },
            resourceType: ['main_frame'],
          })],
        priority: 100,
        actions: [new chrome.declarativeWebRequest.RedirectByRegEx({
          from: '(.*)',
          to: 'chrome-extension://' + chrome.i18n.getMessage('@@extension_id') +
              '/action.html#$1'
          })]
      };
      rules.push(catch_all_rule);
    }
    // Clear old and apply new rules if any.
    chrome.declarativeWebRequest.onRequest.removeRules(undefined, function () {
      if (rules.length)
        chrome.declarativeWebRequest.onRequest.addRules(rules);
    });
  }

  if (items[this.Policies.ALTERNATIVE_BROWSER_PATH]) {
    properties.push({name: 'alternative_browser',
                     value: items[this.Policies.ALTERNATIVE_BROWSER_PATH]});
    properties.alternative_browser =
        items[this.Policies.ALTERNATIVE_BROWSER_PATH];
  } else {
    properties.push({name: 'alternative_browser', value: ''});
  }

  if (items[this.Policies.ALTERNATIVE_BROWSER_ARGUMENTS]) {
    properties.push({
      name: this.Policies.ALTERNATIVE_BROWSER_ARGUMENTS,
      value: items[this.Policies.ALTERNATIVE_BROWSER_ARGUMENTS]
    });
  } else {
    properties.push({name: this.Policies.ALTERNATIVE_BROWSER_ARGUMENTS,
                     value: ''});
  }

  if (items[this.Policies.CHROME_PATH])
    properties.push({name: 'chrome_browser',
                     value: items[this.Policies.CHROME_PATH]});
  else
    properties.push({name: 'chrome_browser', value: ''});

  if (items[this.Policies.CHROME_ARGUMENTS]) {
    properties.push({name: this.Policies.CHROME_ARGUMENTS,
                     value: items[this.Policies.CHROME_ARGUMENTS]});
  } else {
    properties.push({name: this.Policies.CHROME_ARGUMENTS, value: ''});
  }

  if (items[this.Policies.KEEP_LAST_CHROME_TAB] !== undefined) {
    self.keep_last_chrome_tab = items[this.Policies.KEEP_LAST_CHROME_TAB];
  }

  self.port.setProperties(properties, function(msg) {
    if (msg.success)
      self.port.saveSettings();
  });

  if (!self.use_declarative_api) {
    // Sorting the list ensures higher priority for the negative entries.
    url_list.sort();
    self.url_list_info = self.processUrlList(url_list);
    self.url_greylist_info = self.processUrlList(url_greylist);
    self.url_list_loaded = true;
    setTimeout(function() {
      // Make sure also the already loaded tabs are scrutinized in this case.
      // Give it a second to make sure everything is really initialized.
      chrome.tabs.query({}, function(tabs) {
        tabs.forEach(function(tab) {
          if (self.urlNeedsRedirect(tab.url))
            chrome.tabs.reload(tab.id, {bypassCache: true});
        });
      });
    }, 1000);
  }
};

/**
 * Callback for the chrome.storage.onChanged event. Updates the configuration.
 * @param {Object} change Set of changes (unused parameter)
 * @param {string} area Storage area that changed.
 */
ExtensionLogic.prototype.updateConfiguration = function(change, area) {
  // This extension is intended to be controlled through policy only. Therefore
  // only the 'managed' area is monitored for changes.
  if (area === 'managed') {
    chrome.storage.managed.get([this.Policies.URL_LIST,
                                this.Policies.URL_GREYLIST,
                                this.Policies.ALTERNATIVE_BROWSER_PATH,
                                this.Policies.ALTERNATIVE_BROWSER_ARGUMENTS,
                                this.Policies.CHROME_PATH,
                                this.Policies.CHROME_ARGUMENTS,
                                this.Policies.KEEP_LAST_CHROME_TAB],
                               this.createRules.bind(this));
  }
};

/**
 * Callback for the chrome.webRequest.onBeforeRequest event. If the
 * declarativeWebRequest API is not available (as in Chrome 27) the extension
 * will use the old and somewhat slower blocking webRequest API instead.
 * @param {Object} details Parameters of the request. Most importantly the url.
 * @return {Object} The action to be performed for this request.
 */
ExtensionLogic.prototype.requestFilter = function(details) {
  // Only ever act on urls we actually care about.
  var anchor = document.createElement('a');
  anchor.href = details.url;
  if (anchor.protocol !== 'http:' && anchor.protocol !== 'https:' &&
      anchor.protocol !== 'file:') {
    return;
  }
  // If not yet loaded go to an intermediate page that will reload until cache
  // is loaded. This is a little bit better than doing nothing since it will
  // prevent the browser from rendering something of the actual page and spare
  // real network request. However this might not catch all cases therefore the
  // tab enumeration on after setting this flag is done.
  if (!this.url_list_loaded) {
    return {redirectUrl: 'chrome-extension://' +
      chrome.i18n.getMessage('@@extension_id') +
      '/loading.html#' + details.url};
  } else if (this.urlNeedsRedirect(details.url)) {
    return {redirectUrl: 'chrome-extension://' +
      chrome.i18n.getMessage('@@extension_id') +
      '/action.html#' + details.url};
  }
};

/**
 * Runs the alternative browser with the provided url.
 * @param {string} url The url to load.
 * @param {Function} callback to call when finished.
 */
ExtensionLogic.prototype.invokeAlternativeBrowser = function(url, callback) {
  this.port.invokeAlternativeBrowser(url, callback);
};

/**
 * Initializes the ExtensionLogic object and installs the listeners.
 */
ExtensionLogic.prototype.initialize = function() {
  // This extension only works for windows.
  if (/Win/.test(window.navigator.platform)) {
    // A temporary measure until the bug infesting the declarativeWebRequest
    // API is fixed. Meanwhile we use the good ol' webRequest API and suffer.
    if (!this.use_declarative_api) {
      chrome.webRequest.onBeforeRequest.addListener(
          this.request_listener,
          {urls: ['http://*/*', 'https://*/*'], types: ['main_frame']},
          ['blocking']);
    }

    this.port = new Port(this.portConnected.bind(this),
                         this.portDisconnected.bind(this));
  }
};
