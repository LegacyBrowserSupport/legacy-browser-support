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

#include "core/browser_switcher_core.h"

// Implements the glue needed to create a scriptable NPObject and provides empty
// implementations for all functions that are not required for this application.
// Contains an instance of the BrowserSwitcherCore class which contains the
// actual application logic.
class ScriptableObject : public NPObject {
 public:
  static NPObject* Allocate(NPP npp, NPClass *aClass);
  static void Deallocate(NPObject *npobj);
  static void Invalidate(NPObject *npobj);
  static bool HasMethod(NPObject *npobj, NPIdentifier name);
  static bool Invoke(NPObject *npobj,
                     NPIdentifier name,
                     const NPVariant *args, uint32_t argCount,
                     NPVariant *result);
  static bool InvokeDefault(NPObject *npobj,
                            const NPVariant *args,
                            uint32_t argCount, NPVariant *result);
  static bool HasProperty(NPObject * npobj, NPIdentifier name);
  static bool GetProperty(NPObject *npobj,
                          NPIdentifier name, NPVariant *result);
  static bool SetProperty(NPObject *npobj,
                          NPIdentifier name, const NPVariant *value);
  static bool RemoveProperty(NPObject *npobj, NPIdentifier name);
  static bool Enumerate(NPObject *npobj,
                        NPIdentifier **identifier, uint32_t *count);
  static bool Construct(NPObject *npobj,
                        const NPVariant *args, uint32_t argCount,
                        NPVariant *result);

  static NPClass npClass_;

 private:
  explicit ScriptableObject(NPP instance);
  virtual ~ScriptableObject();

  virtual void Invalidate();
  virtual bool HasMethod(NPIdentifier name);

  virtual bool Invoke(NPIdentifier name,
                      const NPVariant *args, uint32_t argCount,
                      NPVariant *result);
  virtual bool InvokeDefault(const NPVariant *args,
                             uint32_t argCount, NPVariant *result);
  virtual bool HasProperty(NPIdentifier name);
  virtual bool GetProperty(NPIdentifier name, NPVariant *result);
  virtual bool SetProperty(NPIdentifier name, const NPVariant *value);
  virtual bool RemoveProperty(NPIdentifier name);
  virtual bool Enumerate(NPIdentifier **identifier, uint32_t *count);
  virtual bool Construct(const NPVariant *args, uint32_t argCount,
                         NPVariant *result);

  std::wstring VariantToWString(const NPVariant& variant);
  std::wstring IdentifierToWString(const NPIdentifier& identifier);
  void WStringToVariant(const std::wstring string, NPVariant* variant);

  BrowserSwitcherCore browser_switcher_;
  NPP instance_;
};
