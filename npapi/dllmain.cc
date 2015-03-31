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

#include "core/logging.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call,
                      LPVOID lpReserved) {
  switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
      wchar_t path[MAX_PATH];
      if (!::SHGetSpecialFolderPath(0, path, CSIDL_LOCAL_APPDATA, false))
        break;
      std::wstring log_file_path(path);
      ::CreateDirectory(log_file_path.append(L"\\Google").c_str(), NULL);
      ::CreateDirectory(log_file_path.append(L"\\BrowserSwitcher").c_str(),
                        NULL);
      log_file_path.append(L"\\plugin_log.txt");
      InitLog(log_file_path.c_str());
      break;
    }
    case DLL_PROCESS_DETACH:
      CloseLog();
      break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
      break;
  }
  return TRUE;
}

