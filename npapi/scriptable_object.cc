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

#include "npapi/scriptable_object.h"

#include "core/logging.h"

// Helper macro for printing the argument of a method that accepts NPIdentifer.
#define LOGNAME(function, name) LOG(INFO) << "ScriptableObject::" << function \
    << " : " << NPN_IdentifierIsString(name) << " name " \
    << (NPN_IdentifierIsString(name) ? IdentifierToWString(name) : L"") \
    << " id " \
    << (NPN_IdentifierIsString(name) ? 0 : NPN_IntFromIdentifier(name)) \
    << std::endl;

namespace {
// Names of the different methods and properties of this object.
const wchar_t kInvokeAlternativeBrowser[] = L"invokeAlternativeBrowser";
const wchar_t kSaveSettingsMethod[] = L"saveSettings";
const wchar_t kLogError[] = L"logError";
const wchar_t kAlternativeBrowserProperty[] = L"alternative_browser";
const wchar_t kAltBrowserParametersProperty[] =
    L"alternative_browser_arguments";
const wchar_t kChromeBrowserProperty[] = L"chrome_browser";
const wchar_t kChromeParametersProperty[] = L"chrome_arguments";
const wchar_t kUrlListProperty[] = L"urls_to_redirect";
const wchar_t kUrlGreyListProperty[] = L"url_greylist";
}  // namespace

// static
NPClass ScriptableObject::npClass_ = {
    NP_CLASS_STRUCT_VERSION,
    ScriptableObject::Allocate,
    ScriptableObject::Deallocate,
    ScriptableObject::Invalidate,
    ScriptableObject::HasMethod,
    ScriptableObject::Invoke,
    ScriptableObject::InvokeDefault,
    ScriptableObject::HasProperty,
    ScriptableObject::GetProperty,
    ScriptableObject::SetProperty,
    ScriptableObject::RemoveProperty,
    ScriptableObject::Enumerate,
    ScriptableObject::Construct
};

// static
NPObject* ScriptableObject::Allocate(NPP npp, NPClass *aClass) {
  NPObject *object = new ScriptableObject(npp);
  return object;
}

// static
void ScriptableObject::Deallocate(NPObject *npobj) {
  delete npobj;
}

// static
void ScriptableObject::Invalidate(NPObject *npobj) {
  (static_cast<ScriptableObject*>(npobj))->Invalidate();
}

// static
bool ScriptableObject::HasMethod(NPObject *npobj, NPIdentifier name) {
  return (static_cast<ScriptableObject*>(npobj))->HasMethod(name);
}

// static
bool ScriptableObject::Invoke(NPObject *npobj,
                              NPIdentifier name,
                              const NPVariant *args, uint32_t argCount,
                              NPVariant *result) {
  return (static_cast<ScriptableObject*>(npobj))->Invoke(name,
                                                         args, argCount,
                                                         result);
}

// static
bool ScriptableObject::InvokeDefault(NPObject *npobj,
                                     const NPVariant *args,
                                     uint32_t argCount, NPVariant *result) {
  return (static_cast<ScriptableObject*>(npobj))->InvokeDefault(args,
                                                                argCount,
                                                                result);
}

// static
bool ScriptableObject::HasProperty(NPObject * npobj, NPIdentifier name) {
  return (static_cast<ScriptableObject*>(npobj))->HasProperty(name);
}

// static
bool ScriptableObject::GetProperty(NPObject *npobj,
                                   NPIdentifier name, NPVariant *result) {
  return (static_cast<ScriptableObject*>(npobj))->GetProperty(name, result);
}

// static
bool ScriptableObject::SetProperty(NPObject *npobj,
                                   NPIdentifier name, const NPVariant *value) {
  return (static_cast<ScriptableObject*>(npobj))->SetProperty(name, value);
}

// static
bool ScriptableObject::RemoveProperty(NPObject *npobj, NPIdentifier name) {
  return (static_cast<ScriptableObject*>(npobj))->RemoveProperty(name);
}

// static
bool ScriptableObject::Enumerate(NPObject *npobj,
                                 NPIdentifier **identifier, uint32_t *count) {
  return (static_cast<ScriptableObject*>(npobj))->Enumerate(identifier,
                                                            count);
}

// static
bool ScriptableObject::Construct(NPObject *npobj,
                                 const NPVariant *args, uint32_t argCount,
                                 NPVariant *result) {
  return (static_cast<ScriptableObject*>(npobj))->Construct(args, argCount,
                                                            result);
}

ScriptableObject::ScriptableObject(NPP instance)
    : instance_(instance) {
}

ScriptableObject::~ScriptableObject() {
}

void ScriptableObject::Invalidate() {
}

bool ScriptableObject::HasMethod(NPIdentifier name) {
  LOGNAME("HasMethod", name);

  std::wstring property_name = IdentifierToWString(name);
  if (property_name.compare(kSaveSettingsMethod) == 0 ||
      property_name.compare(kInvokeAlternativeBrowser) == 0 ||
      property_name.compare(kLogError) == 0) {
    return true;
  }

  return false;
}

bool ScriptableObject::Invoke(NPIdentifier name,
                              const NPVariant *args, uint32_t argCount,
                              NPVariant *result) {
  LOGNAME("Invoke", name);

  std::wstring property_name = IdentifierToWString(name);
  if (property_name.compare(kSaveSettingsMethod) == 0) {
    bool success = browser_switcher_.SaveConfigFile();
    BOOLEAN_TO_NPVARIANT(success, *result);
    return true;
  }
  if (property_name.compare(kInvokeAlternativeBrowser) == 0) {
    if (argCount > 0 && NPVARIANT_IS_STRING(args[0])) {
      std::wstring url = VariantToWString(args[0]);

      bool success = false;
      if (browser_switcher_.ShouldOpenInAlternativeBrowser(url))
        success = browser_switcher_.InvokeAlternativeBrowser(url);
      BOOLEAN_TO_NPVARIANT(success, *result);
      return true;
    }
  }
  if (property_name.compare(kLogError) == 0) {
    if (argCount > 0 && NPVARIANT_IS_STRING(args[0])) {
      std::wstring error_msg = VariantToWString(args[0]);

      LOG(ERR) << "Runtime error from javascript : "
          << error_msg.c_str() << std::endl;
      BOOLEAN_TO_NPVARIANT(true, *result);
      return true;
    }
  }

  return false;
}

bool ScriptableObject::InvokeDefault(const NPVariant *args,
                                     uint32_t argCount, NPVariant *result) {
  return false;
}

bool ScriptableObject::HasProperty(NPIdentifier name) {
  LOGNAME("HasProperty", name);
  if (!NPN_IdentifierIsString(name))
    return false;

  std::wstring property_name = IdentifierToWString(name);
  if (property_name.compare(kAlternativeBrowserProperty) == 0 ||
      property_name.compare(kAltBrowserParametersProperty) == 0 ||
      property_name.compare(kChromeBrowserProperty) == 0 ||
      property_name.compare(kChromeParametersProperty) == 0 ||
      property_name.compare(kUrlListProperty) == 0 ||
      property_name.compare(kUrlGreyListProperty) == 0) {
    LOG(INFO) << "\tFound : " << property_name.c_str() << std::endl;
    return true;
  }

  return false;
}

bool ScriptableObject::GetProperty(NPIdentifier name, NPVariant *result) {
  LOGNAME("GetProperty", name);
  if (!NPN_IdentifierIsString(name))
    return false;

  std::wstring property_name = IdentifierToWString(name);
  if (property_name.compare(kAlternativeBrowserProperty) == 0) {
    std::wstring browser = browser_switcher_.GetAlternativeBrowserPath();
    LOG(INFO) << "\t" << property_name.c_str() << " : "
              << browser.c_str() << std::endl;
    WStringToVariant(browser, result);
    return true;
  }
  if (property_name.compare(kAltBrowserParametersProperty) == 0) {
    std::wstring parameters =
        browser_switcher_.GetAlternativeBrowserParameters();
    LOG(INFO) << "\t" << property_name.c_str() << " : "
              << parameters.c_str() << std::endl;
    WStringToVariant(parameters, result);
    return true;
  }
  if (property_name.compare(kChromeBrowserProperty) == 0) {
    std::wstring browser = browser_switcher_.GetChromePath();
    LOG(INFO) << "\t" << property_name.c_str() << " : "
              << browser.c_str() << std::endl;
    WStringToVariant(browser, result);
    return true;
  }
  if (property_name.compare(kChromeParametersProperty) == 0) {
    std::wstring parameters = browser_switcher_.GetChromeParameters();
    LOG(INFO) << "\t" << property_name.c_str() << " : "
              << parameters.c_str() << std::endl;
    WStringToVariant(parameters, result);
    return true;
  }
  if (property_name.compare(kUrlListProperty) == 0 ||
      property_name.compare(kUrlGreyListProperty) == 0) {
    NPObject* window = NULL;
    NPN_GetValue(instance_, NPNVWindowNPObject, &window);
    NPVariant array_variant;
    NPN_Invoke(instance_, window,
               NPN_GetStringIdentifier("Array"), NULL, 0, &array_variant);
    NPObject* array_object = NPVARIANT_TO_OBJECT(array_variant);

    NPIdentifier push_identifer = NPN_GetStringIdentifier("push");
    std::vector<std::wstring> urls;
    if (property_name.compare(kUrlListProperty))
      urls = browser_switcher_.GetUrlsToRedirect();
    else
      urls = browser_switcher_.GetUrlGreylist();
    for (std::vector<std::wstring>::const_iterator it = urls.begin();
         it != urls.end(); ++it) {
      NPVariant item;
      NPVariant push_result;
      WStringToVariant(*it, &item);
      NPN_Invoke(instance_, array_object,
                 push_identifer, &item, 1, &push_result);
      NPN_ReleaseVariantValue(&push_result);
    }
    OBJECT_TO_NPVARIANT(array_object, *result);
    return true;
  }


  return false;
}

bool ScriptableObject::SetProperty(NPIdentifier name, const NPVariant *value) {
  LOGNAME("SetProperty", name);
  if (!NPN_IdentifierIsString(name))
    return false;

  std::wstring property_name = IdentifierToWString(name);
  if (property_name.compare(kAlternativeBrowserProperty) == 0) {
    browser_switcher_.SetAlternativeBrowserPath(VariantToWString(*value));
    LOG(INFO) << "\t" << property_name.c_str() << " : "
              << VariantToWString(*value).c_str() << std::endl;
    return true;
  }
  if (property_name.compare(kAltBrowserParametersProperty) == 0) {
    browser_switcher_.SetAlternativeBrowserParameters(
        VariantToWString(*value));
    LOG(INFO) << "\t" << property_name.c_str() << " : "
              << VariantToWString(*value).c_str() << std::endl;
    return true;
  }
  if (property_name.compare(kChromeBrowserProperty) == 0) {
    browser_switcher_.SetChromePath(VariantToWString(*value));
    LOG(INFO) << "\t" << property_name.c_str() << " : "
              << VariantToWString(*value).c_str() << std::endl;
    return true;
  }
  if (property_name.compare(kChromeParametersProperty) == 0) {
    browser_switcher_.SetChromeParameters(VariantToWString(*value));
    LOG(INFO) << "\t" << property_name.c_str() << " : "
              << VariantToWString(*value).c_str() << std::endl;
    return true;
  }
  if (property_name.compare(kUrlListProperty) == 0 ||
      property_name.compare(kUrlGreyListProperty) == 0) {
    std::vector<std::wstring> urls;
    NPObject *object = NPVARIANT_TO_OBJECT(*value);

    NPVariant length;
    NPN_GetProperty(instance_, object,
                    NPN_GetStringIdentifier("length"), &length);
    unsigned int count;
    if (NPVARIANT_IS_DOUBLE(length))
      count = static_cast<int>(NPVARIANT_TO_DOUBLE(length));
    else
      count = NPVARIANT_TO_INT32(length);

    LOG(INFO) << "\tArray lenght : " << count << std::endl;
    for (unsigned int i = 0; i < count; ++i) {
      NPVariant value;
      NPN_GetProperty(instance_, object, NPN_GetIntIdentifier(i), &value);
      if (NPVARIANT_IS_STRING(value)) {
        urls.push_back(VariantToWString(value));
        LOG(INFO) << "\tElement [" << i << "] : "
                  << urls[urls.size()-1] << std::endl;
      }
    }

    if (property_name.compare(kUrlListProperty) == 0)
      browser_switcher_.SetUrlsToRedirect(urls);
    else
      browser_switcher_.SetUrlGreylist(urls);
    return true;
  }

  return false;
}

bool ScriptableObject::RemoveProperty(NPIdentifier name) {
  LOGNAME("RemoveProperty", name);
  return false;
}

bool ScriptableObject::Enumerate(NPIdentifier **identifier, uint32_t *count) {
  return false;
}

bool ScriptableObject::Construct(const NPVariant *args, uint32_t argCount,
                                NPVariant *result) {
  return false;
}

std::wstring ScriptableObject::VariantToWString(const NPVariant& variant) {
  const NPString& npstring = NPVARIANT_TO_STRING(variant);
  std::string url(npstring.UTF8Characters,
                  npstring.UTF8Characters + npstring.UTF8Length);

  int len = ::MultiByteToWideChar(CP_UTF8, 0,
                                  npstring.UTF8Characters, npstring.UTF8Length,
                                  NULL, 0);
  std::auto_ptr<wchar_t> buffer(new wchar_t[len]);
  len = ::MultiByteToWideChar(CP_UTF8, 0,
                              npstring.UTF8Characters, npstring.UTF8Length,
                              buffer.get(), len);

  return std::wstring(buffer.get(), buffer.get() + len);
}

std::wstring ScriptableObject::IdentifierToWString(
    const NPIdentifier& identifier) {
  DCHECK(NPN_IdentifierIsString(identifier));

  NPUTF8* utf_string = NPN_UTF8FromIdentifier(identifier);
  int len = ::MultiByteToWideChar(CP_UTF8, 0,
                                  utf_string, strlen(utf_string),
                                  NULL, 0);
  std::auto_ptr<wchar_t> buffer(new wchar_t[len]);
  len = ::MultiByteToWideChar(CP_UTF8, 0,
                              utf_string, strlen(utf_string),
                              buffer.get(), len);
  NPN_MemFree(utf_string);

  return std::wstring(buffer.get(), buffer.get() + len);
}

void ScriptableObject::WStringToVariant(const std::wstring string,
                                        NPVariant* variant) {
  int len = ::WideCharToMultiByte(CP_UTF8, 0,
                                  string.data(), string.length(),
                                  NULL, 0, NULL, NULL);
  NPUTF8* buffer = reinterpret_cast<NPUTF8*>(NPN_MemAlloc(len));
  len = ::WideCharToMultiByte(CP_UTF8, 0,
                              string.data(), string.length(),
                              buffer, len, NULL, NULL);
  STRINGN_TO_NPVARIANT(buffer, len, *variant);
}
