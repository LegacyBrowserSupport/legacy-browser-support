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

// Implements portion of the NPAPI NPN API that is used by our plugin.
NPObject* NPN_CreateObject(NPP instance, NPClass* npClass);
NPObject *NPN_RetainObject(NPObject *npobj);
void NPN_ReleaseObject(NPObject *npobj);

bool NPN_IdentifierIsString(NPIdentifier id);
NPUTF8 *NPN_UTF8FromIdentifier(NPIdentifier id);
int32_t NPN_IntFromIdentifier(NPIdentifier id);
NPIdentifier NPN_GetStringIdentifier(const NPUTF8 *name);
NPIdentifier NPN_GetIntIdentifier(int32_t intid);

void NPN_ReleaseVariantValue(NPVariant *variant);

bool NPN_Invoke(NPP instance, NPObject *object, NPIdentifier methodName,
                const NPVariant *args, uint32_t argCount, NPVariant *result);
bool NPN_GetProperty(NPP instance, NPObject *npObject,
                     NPIdentifier propertyName, NPVariant *result);
bool NPN_Enumerate(NPP instance, NPObject *object, NPIdentifier **identifier,
                   uint32_t *count);

void* NPN_MemAlloc(uint32_t size);
void NPN_MemFree(void* ptr);
NPError NPN_GetValue(NPP instance, NPNVariable variable, void *value);
