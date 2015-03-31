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

#include "npapi/npp_implementation.h"

#include "core/logging.h"
#include "npapi/plugin_instance.h"

// Initializes a new plugin instance.
NPError NPP_New(NPMIMEType pluginType,
                NPP instance,
                uint16_t mode,
                int16_t argc, char* argn[], char* argv[],
                NPSavedData* saved) {
  if (!instance)
    return NPERR_INVALID_INSTANCE_ERROR;

  // set up our instance data.
  PluginInstance* instanceObject = new PluginInstance(instance);
  if (!instanceObject)
    return NPERR_OUT_OF_MEMORY_ERROR;
  instance->pdata = instanceObject;

  return instanceObject->Initialize(pluginType, mode, argc, argn, argv, saved);
}

// Destroys a plugin instance.
NPError NPP_Destroy(NPP instance, NPSavedData** save) {
  if (!instance)
    return NPERR_INVALID_INSTANCE_ERROR;

  PluginInstance* instanceObject =
      reinterpret_cast<PluginInstance*>(instance->pdata);

  instanceObject->ShutDown();
  delete instanceObject;

  return NPERR_NO_ERROR;
}

// Sets the window associated to a plugin instance.
NPError NPP_SetWindow(NPP instance, NPWindow* window) {
  if (!instance)
    return NPERR_INVALID_INSTANCE_ERROR;

  PluginInstance* instanceObject =
      reinterpret_cast<PluginInstance*>(instance->pdata);

  instanceObject->SetWindow(*window);

  return NPERR_NO_ERROR;
}

// Stream operations.
NPError NPP_NewStream(NPP instance,
                      NPMIMEType type,
                      NPStream* stream,
                      NPBool seekable,
                      uint16_t* stype) {
  if (!instance)
    return NPERR_INVALID_INSTANCE_ERROR;

  PluginInstance* instanceObject =
      reinterpret_cast<PluginInstance*>(instance->pdata);

  return instanceObject->NewStream(type, stream, seekable, stype);
}

NPError NPP_DestroyStream(NPP instance, NPStream* stream, NPReason reason) {
  if (!instance)
    return NPERR_INVALID_INSTANCE_ERROR;

  PluginInstance* instanceObject =
      reinterpret_cast<PluginInstance*>(instance->pdata);

  return instanceObject->DestroyStream(stream, reason);
}

int32_t NPP_WriteReady(NPP instance, NPStream* stream) {
  if (!instance)
    return NPERR_INVALID_INSTANCE_ERROR;

  PluginInstance* instanceObject =
      reinterpret_cast<PluginInstance*>(instance->pdata);

  return instanceObject->WriteReady(stream);
}

int32_t NPP_Write(NPP instance, NPStream* stream,
                  int32_t offset, int32_t len, void* buffer) {
  if (!instance)
    return NPERR_INVALID_INSTANCE_ERROR;

  PluginInstance* instanceObject =
      reinterpret_cast<PluginInstance*>(instance->pdata);

  return instanceObject->Write(stream, offset, len, buffer);
}

void NPP_StreamAsFile(NPP instance, NPStream* stream, const char* fname) {
  if (!instance)
    return;

  PluginInstance* instanceObject =
      reinterpret_cast<PluginInstance*>(instance->pdata);

  instanceObject->StreamAsFile(stream, fname);
}

void NPP_Print(NPP instance, NPPrint* platformPrint) {
  if (!instance)
    return;

  PluginInstance* instanceObject =
      reinterpret_cast<PluginInstance*>(instance->pdata);

  instanceObject->Print(platformPrint);
}

int16_t NPP_HandleEvent(NPP instance, void* event) {
  if (!instance)
    return 1;

  PluginInstance* instanceObject =
      reinterpret_cast<PluginInstance*>(instance->pdata);

  return instanceObject->HandleEvent(reinterpret_cast<NPEvent*>(event));
}

void NPP_URLNotify(NPP instance, const char* url,
                   NPReason reason, void* notifyData) {
  if (!instance)
    return;

  PluginInstance* instanceObject =
      reinterpret_cast<PluginInstance*>(instance->pdata);

  instanceObject->URLNotify(url, reason, notifyData);
}

NPError NPP_GetValue(NPP instance, NPPVariable variable, void *value) {
  if (!instance)
    return NPERR_INVALID_INSTANCE_ERROR;

  PluginInstance* instanceObject =
      reinterpret_cast<PluginInstance*>(instance->pdata);

  return instanceObject->GetValue(variable, value);
}

NPError NPP_SetValue(NPP instance, NPNVariable variable, void *value) {
  if (!instance)
    return NPERR_INVALID_INSTANCE_ERROR;

  PluginInstance* instanceObject =
      reinterpret_cast<PluginInstance*>(instance->pdata);

  return instanceObject->SetValue(variable, value);
}
