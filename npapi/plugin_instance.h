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

#pragma once

#include "stdafx.h"

extern const char kPluginName[];
extern const char kPluginDescription[];

// Represents a plugin instance. One instance is created per <object> tag. This
// class handles the basic operations of the plugins and holds an instance of
// a ScriptableObject which wraps the application logic apd provides inteface to
// JavaScript in the borwser.
class PluginInstance {
 public:
  explicit PluginInstance(NPP instance);
  virtual ~PluginInstance();

  // Initializes a plugin instance. Called from NPP_New.
  virtual NPError Initialize(NPMIMEType pluginType,
                             uint16_t mode,
                             int16_t argc, char* argn[], char* argv[],
                             NPSavedData* saved);

  // Called before the plugin instance is destroyed.
  virtual void ShutDown();

  // Sets the plugin window. Called from NPP_SetWindow.
  virtual NPError SetWindow(const NPWindow& window);

  // Creates a new stream. Called from NPP_NewStream. Returns the error code for
  // the operation.
  virtual NPError NewStream(NPMIMEType type,
                            NPStream* stream,
                            NPBool seekable,
                            uint16_t* stype);

  // Destroys a stream. Called from NPP_DestroyStream. Returns the error code
  // for the operation.
  virtual NPError DestroyStream(NPStream* stream, NPReason reason);

  // Called from NPP_WriteReady.
  virtual int32_t WriteReady(NPStream* stream);

  // Called from NPP_Write.
  virtual int32_t Write(NPStream* stream,
                        int32_t offset, int32_t len, void* buffer);

  // Called from NPP_StreamAsFile.
  virtual void StreamAsFile(NPStream* stream, const char* fname);

  // Called from NPP_Print.
  virtual void Print(NPPrint* platformPrint);

  // Called from NPP_HandleEvent.
  virtual int16_t HandleEvent(NPEvent* event);

  // Called from NPP_URLNotify.
  virtual void URLNotify(const char* URL, NPReason reason, void* notifyData);

  // Called from NPP_GetValue. Returns the error code for the operation.
  virtual NPError GetValue(NPPVariable variable, void *value);

  // Called from NPP_SetValue. Returns the error code for the operation.
  virtual NPError SetValue(NPNVariable variable, void *value);

  // Called from GetValue when called for |NPPVpluginScriptableNPObject|.
  virtual NPObject* GetScriptableObject();

  // Called from GetValue when called for |NPPVpluginWindowBool|.
  virtual NPBool IsWindowed();

 private:
  NPP instance_;
  NPObject* npObject_;
};
