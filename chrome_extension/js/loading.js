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
 * @fileoverview If using the old webRequest API we need to have some way to
 * wait for the storage to load. This we achieve by having this page wait for
 * |url_list_loaded| flag to be set in the background page and re-load the url
 * to force it to go through the actual set of filters.
 */

var url = window.location.hash.substring(1);

async function waitForStorageToLoad() {
  let response = await browser.runtime.sendMessage({msg: "hasFinishedLoading", url});
  if (response.hasFinishedLoading) {
    window.location.href = url;
    return;
  }
  setTimeout(waitForStorageToLoad, 100);
}

// Only ever act on urls we actually care about.
var anchor = document.createElement('a');
anchor.href = url;
if (anchor.protocol === 'http:' || anchor.protocol === 'https:' ||
    anchor.protocol === 'file:') {
  window.addEventListener('load', waitForStorageToLoad);
}
