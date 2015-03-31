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

#include "stdafx.h"

#include "npapi/npapi_impl.h"

#include "core/logging.h"
#include "npapi/npp_implementation.h"

const char kPluginName[] = "Legacy Browser Support";
const char kPluginDescription[] = " Legacy Browser Support Helper Plugin";

// Keeps the pointer to the browser functions provided in NP_Initialize.
NPNetscapeFuncs* gBrowserFunctions = NULL;

extern "C" {

// Sets up the entry points of the plugin functions.
NPError WINAPI NP_GetEntryPoints(NPPluginFuncs* plugin_functions) {
  if (plugin_functions->size <
      offsetof(NPPluginFuncs, setvalue) + sizeof(NPP_SetValueProcPtr)) {
    return NPERR_INVALID_FUNCTABLE_ERROR;
  }

  plugin_functions->newp = NPP_New;
  plugin_functions->destroy = NPP_Destroy;
  plugin_functions->setwindow = NPP_SetWindow;
  plugin_functions->newstream = NPP_NewStream;
  plugin_functions->destroystream = NPP_DestroyStream;
  plugin_functions->asfile = NPP_StreamAsFile;
  plugin_functions->writeready = NPP_WriteReady;
  plugin_functions->write = NPP_Write;
  plugin_functions->print = NPP_Print;
  plugin_functions->event = NPP_HandleEvent;
  plugin_functions->urlnotify = NPP_URLNotify;
  plugin_functions->getvalue = NPP_GetValue;
  plugin_functions->setvalue = NPP_SetValue;

  return NPERR_NO_ERROR;
}

// Initializes the plugin instance and saves the provided pointer to the browser
// functions.
NPError WINAPI NP_Initialize(NPNetscapeFuncs* browser_functions) {
  gBrowserFunctions = browser_functions;

  return NPERR_NO_ERROR;
}

// Cleans up the plugin instance.
NPError WINAPI NP_Shutdown() {
  return NPERR_NO_ERROR;
}

}  // extern "C"
