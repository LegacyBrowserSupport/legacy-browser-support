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

#include "npapi/npn_implementation.h"

#include "npapi/npapi_impl.h"

NPObject* NPN_CreateObject(NPP instance, NPClass* npClass) {
  return gBrowserFunctions->createobject(instance, npClass);
}

NPObject *NPN_RetainObject(NPObject *npobj) {
  return gBrowserFunctions->retainobject(npobj);
}

void NPN_ReleaseObject(NPObject *npobj) {
  gBrowserFunctions->releaseobject(npobj);
}

bool NPN_IdentifierIsString(NPIdentifier id) {
  return gBrowserFunctions->identifierisstring(id);
}

NPUTF8 *NPN_UTF8FromIdentifier(NPIdentifier id) {
  return gBrowserFunctions->utf8fromidentifier(id);
}

int32_t NPN_IntFromIdentifier(NPIdentifier id) {
  return gBrowserFunctions->intfromidentifier(id);
}

NPIdentifier NPN_GetStringIdentifier(const NPUTF8 *name) {
  return gBrowserFunctions->getstringidentifier(name);
}

NPIdentifier NPN_GetIntIdentifier(int32_t intid) {
  return gBrowserFunctions->getintidentifier(intid);
}

void NPN_ReleaseVariantValue(NPVariant *variant) {
  gBrowserFunctions->releasevariantvalue(variant);
}

bool NPN_Invoke(NPP instance, NPObject *object, NPIdentifier methodName,
                const NPVariant *args, uint32_t argCount, NPVariant *result) {
  return gBrowserFunctions->invoke(instance, object,
                                   methodName, args, argCount, result);
}

bool NPN_GetProperty(NPP instance, NPObject *object,
                     NPIdentifier propertyName, NPVariant *result) {
  return gBrowserFunctions->getproperty(instance, object, propertyName, result);
}

bool NPN_Enumerate(NPP instance, NPObject *object, NPIdentifier **identifier,
                   uint32_t *count) {
  return gBrowserFunctions->enumerate(instance, object, identifier, count);
}

void* NPN_MemAlloc(uint32_t size) {
  return gBrowserFunctions->memalloc(size);
}

void NPN_MemFree(void* ptr) {
  gBrowserFunctions->memfree(ptr);
}

NPError NPN_GetValue(NPP instance, NPNVariable variable, void *value) {
  return gBrowserFunctions->getvalue(instance, variable, value);
}
