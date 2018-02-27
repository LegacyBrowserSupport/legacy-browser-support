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

// This application is a wrapper around the BrowserSwitcherCore that is capable
// of talking the native messaging API protocol of Chrome.
// The application is started by Chrome automatically when the extension is
// loaded and uses the standard input and output to exchange json formatted
// blobs of data.
// The expected input objects have the following fields:
//   'id'      Identifier used to disambiguate response to the right command.
//   'command' A command to be parsed to the core. Can be one of the following:
//             invokeAlternativeBrowser, setProperties, saveSettings, logError.
//   Further command specific fields like url, properties or error message.
// The response has the following fields always:
//   'id'      The identifier sent with the command.
//   'success' Whether the command completed successfully.
//   'error'   If the command failed, contains the human readable error message.

#include "stdafx.h"
#include <Windows.h>
#include <ShellAPI.h>
#include <ShlObj.h>
#include <fcntl.h>
#include <io.h>

#include <core/browser_switcher_core.h>
#include <core/logging.h>

#include <third_party/json/include/reader.h>
#include <third_party/json/include/writer.h>

// To quote Bill Gates - "640KB should be enough for everyone!"
const int kMaxSafeLength = 640 * 1024;
const int kInitialBufferLength = 4 * 1024;  // Suffices for most cases.

// Names of the different methods and properties of this native host.
const char kInvokeAlternativeBrowser[] = "invokeAlternativeBrowser";
const char kSetProperties[] = "setProperties";
const char kSaveSettings[] = "saveSettings";
const char kDownloadIESiteList[] = "downloadIESiteList";
const char kGetIESiteList[] = "getIESiteList";
const char kLogError[] = "logError";

const char kAlternativeBrowserProperty[] = "alternative_browser";
const char kAltBrowserParametersProperty[] = "alternative_browser_arguments";
const char kChromeBrowserProperty[] = "chrome_browser";
const char kChromeParametersProperty[] = "chrome_arguments";
const char kUrlListProperty[] = "urls_to_redirect";
const char kUrlGreyListProperty[] = "url_greylist";
const char kUseIESiteList[] = "use_ie_site_list";

// The singleton core object. Created later after we have set up logging to not
// smear stdout and cause our channel to Chrome to be closed.
BrowserSwitcherCore* browser_switcher = NULL;

void InvokeAlternativeBrowser(const Json::Value& input, Json::Value* output) {
  std::string url = input.get("url", "").asString();

  // Convert to std::wstring and try to invoke the alt.browser.
  std::wstring wide_url(url.begin(), url.end());
  if (browser_switcher->ShouldOpenInAlternativeBrowser(wide_url)) {
    if (browser_switcher->InvokeAlternativeBrowser(wide_url))
      (*output)["success"] = true;
    else
      (*output)["error"] = "Cound not invoke alt browser.";
  } else {
    (*output)["error"] =
      "This address is not intended to be loaded in the alt browser.";
  }
}

std::wstring GetPropertyValueAsWideString(const Json::Value& prop) {
  std::string value = prop["value"].asString();
  std::wstring wide_value(value.begin(), value.end());
  return wide_value;
}

std::vector<std::wstring> GetPropertyValueAsWideStringArray(
    const Json::Value& property_pair) {
  std::vector<std::wstring> string_array;
  const Json::Value values = property_pair["value"];
  for (size_t i = 0; i < values.size(); ++i) {
    std::string value = values[i].asString();
    std::wstring wide_value(value.begin(), value.end());
    string_array.push_back(wide_value);
  }
  return string_array;
}

void SetProperties(const Json::Value& input, Json::Value* output) {
  const Json::Value properties = input["properties"];
  for (size_t i = 0; i < properties.size(); ++i) {
    std::string name = properties[i]["name"].asString();
    if (name == kAlternativeBrowserProperty) {
      browser_switcher->SetAlternativeBrowserPath(
        GetPropertyValueAsWideString(properties[i]));
    } else if (name == kAltBrowserParametersProperty) {
      browser_switcher->SetAlternativeBrowserParameters(
        GetPropertyValueAsWideString(properties[i]));
    } else if (name == kChromeBrowserProperty) {
      browser_switcher->SetChromePath(
        GetPropertyValueAsWideString(properties[i]));
    } else if (name == kChromeParametersProperty) {
      browser_switcher->SetChromeParameters(
        GetPropertyValueAsWideString(properties[i]));
    } else if (name == kUrlListProperty) {
      browser_switcher->SetUrlsToRedirect(
        GetPropertyValueAsWideStringArray(properties[i]));
    } else if (name == kUrlGreyListProperty) {
      browser_switcher->SetUrlGreylist(
        GetPropertyValueAsWideStringArray(properties[i]));
    } else if (name == kUseIESiteList) {
      if (!properties[i]["value"].asBool()) {
        // If disabled by policy clear the list if present.
        browser_switcher->SetIESiteList(BrowserSwitcherCore::UrlList());
        browser_switcher->SaveIESiteListCache();
      }
    } else {
      (*output)["error"] = "W:Unknown property present. Skipping " + name;
    }
  }
  (*output)["success"] = true;
}

void SaveSettings(const Json::Value& input, Json::Value* output) {
  (*output)["success"] = browser_switcher->SaveConfigFile();
}

void DownloadIESiteList(const Json::Value& input, Json::Value* output) {
  (*output)["success"] = browser_switcher->StartIESiteListDownload();
}

void GetIESiteList(const Json::Value& input, Json::Value* output) {
  BrowserSwitcherCore::UrlList list;
  bool success = browser_switcher->GetIESiteList(&list);
  (*output)["success"] = success;
  if (!success)
    return;

  Json::Value json_list(Json::arrayValue);
  for (const auto& item : list) {
    int len = ::WideCharToMultiByte(
        CP_UTF8, 0, item.data(), item.length(), NULL, 0, NULL, NULL);
    std::auto_ptr<char> buffer(new char[len]);
    len = ::WideCharToMultiByte(
        CP_UTF8, 0, item.data(), item.length(), buffer.get(), len, NULL, NULL);
    std::string url(buffer.get(), buffer.get() + len);
    json_list.append(Json::Value(url));
  }
  (*output)["items"] = json_list;
}

void LogError(const Json::Value& input, Json::Value* output) {
  LOG(ERR) << "Runtime error from javascript : "
           << input["error"].asString().c_str() << std::endl;
  (*output)["success"] = true;
}

int _tmain(int argc, _TCHAR* argv[]) {
  // Set up logging first before it has printed anything on stdout.
  wchar_t path[MAX_PATH];
  if (::SHGetSpecialFolderPath(0, path, CSIDL_LOCAL_APPDATA, false)) {
    std::wstring log_file_path(path);
    ::CreateDirectory(log_file_path.append(L"\\Google").c_str(), NULL);
    ::CreateDirectory(log_file_path.append(L"\\BrowserSwitcher").c_str(), NULL);
    log_file_path.append(L"\\native_log.txt");
    InitLog(log_file_path.c_str());
  }

  browser_switcher = new BrowserSwitcherCore();
  int buffer_size = kInitialBufferLength;
  char* input_buffer = new char[buffer_size];

  if (!browser_switcher || !input_buffer) {
    LOG(ERR) << "Not enough free memory to initialize host!" << std::endl;
    return 1;
  }

  // Change stdio mode to binary to avoid converting 0x10 into 0x10+0x13.
  _setmode(_fileno(stdin), _O_BINARY);

  while (true) {
    int len = 0;
    int bytes = fread(&len, 4, 1, stdin);
    if (len <= 0 || len > kMaxSafeLength)
      return 0;

    if (len + 1 > buffer_size) {
      buffer_size = len + 1;
      delete[] input_buffer;
      input_buffer = new char[buffer_size];
      if (!input_buffer) {
        LOG(ERR) << "Not enough free memory to resize buffer!" << std::endl;
        return 1;
      }
    }
    bytes = fread(input_buffer, 1, len, stdin);
    if (bytes != len) {
      LOG(ERR) << "Can't read whole message, quitting..." << std::endl;
      return 0;
    }
    input_buffer[len] = 0;

    Json::Value output;
    Json::Value input;   // will contain the root value after parsing.
    Json::Reader reader;
    output["success"] = false;

    bool result = reader.parse(input_buffer, input_buffer+len, input, false);
    if (result) {
      output["id"] = input["id"];
      if (input["command"] == kInvokeAlternativeBrowser) {
        InvokeAlternativeBrowser(input, &output);
      } else if (input["command"] == kSetProperties) {
        SetProperties(input, &output);
      } else if (input["command"] == kSaveSettings) {
        SaveSettings(input, &output);
      } else if (input["command"] == kDownloadIESiteList) {
        DownloadIESiteList(input, &output);
      } else if (input["command"] == kGetIESiteList) {
        GetIESiteList(input, &output);
      } else if (input["command"] == kLogError) {
        LogError(input, &output);
      } else {
        output["error"] = "Unknown command";
        LOG(ERR) << "Unknown command : "
                 << input["command"].asString().c_str() << std::endl;
      }
    } else {
      // report to the user the failure and their locations in the document.
      output["error"] = "Parse error : " + reader.getFormatedErrorMessages();
    }
    // Report the results back.
    Json::FastWriter writer;
    std::string output_string = writer.write(output);
    len = output_string.length();
    fwrite(&len, 4, 1, stdout);
    fwrite(output_string.c_str(), len, 1, stdout);
    fflush(stdout);
    LOG(INFO) << "Command finished with response : "
              << output_string.c_str() << std::endl;
  }
  return 0;
}
