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

#include "npapi/plugin_instance.h"

#include "core/logging.h"
#include "npapi/npn_implementation.h"
#include "npapi/scriptable_object.h"

extern const char kPluginName[];
extern const char kPluginDescription[];

// Saves the persitant data for a plugin instance.
PluginInstance::PluginInstance(NPP instance)
    : instance_(instance),
      npObject_(NULL) {
}

PluginInstance::~PluginInstance() {
}

  // Initializes a plugin instance. Called from NPP_New.
NPError PluginInstance::Initialize(NPMIMEType pluginType,
                                   uint16_t mode,
                                   int16_t argc, char* argn[], char* argv[],
                                   NPSavedData* saved) {
  return NPERR_NO_ERROR;
}

// Called before the plugin instance is destroyed.
void PluginInstance::ShutDown() {
}

// Sets the plugin window. Called from NPP_SetWindow.
NPError PluginInstance::SetWindow(const NPWindow& window) {
  return NPERR_NO_ERROR;
}

// Creates a new stream. Called from NPP_NewStream. Returns the error code for
// the operation.
NPError PluginInstance::NewStream(NPMIMEType type,
                                  NPStream* stream,
                                  NPBool seekable,
                                  uint16_t* stype) {
  return NPERR_GENERIC_ERROR;
}

// Destroys a stream. Called from NPP_DestroyStream. Returns the error code
// for the operation.
NPError PluginInstance::DestroyStream(NPStream* stream, NPReason reason) {
  return NPERR_GENERIC_ERROR;
}

// Called from NPP_WriteReady.
int32_t PluginInstance::WriteReady(NPStream* stream) {
  return 0;
}

// Called from NPP_Write.
int32_t PluginInstance::Write(NPStream* stream,
                      int32_t offset, int32_t len, void* buffer) {
  return 0;
}

// Called from NPP_StreamAsFile.
void PluginInstance::StreamAsFile(NPStream* stream, const char* fname) {
}

// Called from NPP_Print.
void PluginInstance::Print(NPPrint* platformPrint) {
}

// Called from NPP_HandleEvent.
int16_t PluginInstance::HandleEvent(NPEvent* event) {
  return 1;
}

// Called from NPP_URLNotify.
void PluginInstance::URLNotify(const char* url, NPReason reason,
                               void* notifyData) {
}

// Called from NPP_GetValue. Returns the error code for the operation.
NPError PluginInstance::GetValue(NPPVariable variable, void *value) {
  NPError result = NPERR_NO_ERROR;
  switch (variable) {
    case NPPVpluginNameString:
      *(reinterpret_cast<char**>(value)) = const_cast<char*>(kPluginName);
      break;
    case NPPVpluginDescriptionString:
      *(reinterpret_cast<char**>(value)) =
          const_cast<char*>(kPluginDescription);
      break;
    case NPPVpluginScriptableNPObject: {
      NPObject* object = GetScriptableObject();
      if (!object)
        return NPERR_GENERIC_ERROR;
      *(reinterpret_cast<NPObject**>(value)) = object;
      break;
    }
    case NPPVpluginWindowBool:
      *(reinterpret_cast<NPBool*>(value)) = IsWindowed();
      break;
    default:
      result = NPERR_GENERIC_ERROR;
  }
  return result;
}

// Called from NPP_SetValue. Returns the error code for the operation.
NPError PluginInstance::SetValue(NPNVariable variable, void *value) {
  return NPERR_GENERIC_ERROR;
}

// Called from GetValue when called for |NPPVpluginScriptableNPObject|.
NPObject* PluginInstance::GetScriptableObject() {
  if (npObject_) {
    return npObject_;
  } else {
    // Creates a new ScriptableObject and returns a pointer to it.
    npObject_ = NPN_CreateObject(instance_, &ScriptableObject::npClass_);
  }

  NPN_RetainObject(npObject_);

  return npObject_;
}

// Called from GetValue when called for |NPPVpluginWindowBool|.
NPBool PluginInstance::IsWindowed() {
  return false;
}

