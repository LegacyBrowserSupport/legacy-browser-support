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

#include "core/browser_switcher_core.h"

#include <ShlObj.h>
#include <WinInet.h>

#include <algorithm>
#include <memory>
#include <vector>

#include "core/logging.h"
#include "core/ieem_site_list_parser.h"

namespace {
const wchar_t kIExploreKey[] =
    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\IEXPLORE.EXE";
const wchar_t kFirefoxKey[] =
    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\firefox.exe";
// Opera does not register itself here for now but it's no harm to keep this.
const wchar_t kOperaKey[] =
    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\opera.exe";
const wchar_t kSafariKey[] =
    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\safari.exe";
const wchar_t kChromeKey[] =
    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\chrome.exe";

const wchar_t kIExploreDdeHost[] = L"IExplore";

const wchar_t kChromeVarName[] = L"${chrome}";
const wchar_t kIEVarName[] = L"${ie}";
const wchar_t kFirefoxVarName[] = L"${firefox}";
const wchar_t kOperaVarName[] = L"${opera}";
const wchar_t kSafariVarName[] = L"${safari}";
const wchar_t kUrlVarName[] = L"${url}";

const wchar_t kWildcardUrl[] = L"*";

const wchar_t* kBrowsersToKeysMapping[][2] = {
    {kChromeVarName, kChromeKey},
    {kIEVarName, kIExploreKey},
    {kFirefoxVarName, kFirefoxKey},
    {kOperaVarName, kOperaKey},
    {kSafariVarName, kSafariKey}};

const wchar_t* kBrowsersToDdeHostsMapping[][2] = {
    { kChromeVarName, L"" },
    { kIEVarName, kIExploreDdeHost },
    { kFirefoxVarName, L"" },  // Firefox claims to support this but why bother.
    { kOperaVarName, L"" },    // Opera claims to support this but why bother.
    { kSafariVarName, L"" } };

const int kMinSupportedFileVersion = 1;
const int kCurrentFileVersion = 1;

const size_t kMaxUrlFilterSize = 10000;

// Reads a line from a file and returns true on success and false otherwise.
bool ReadLineFromFile(std::wifstream* stream, std::wstring* line) {
  if (stream->eof())
    return false;
  std::getline(*stream, *line);
  if (stream->fail())
    return false;
  return true;
}

// DDE Callback function which is not used in our case at all.
HDDEDATA CALLBACK DdeCallback(
    UINT type, UINT format, HCONV handle, HSZ string1, HSZ string2,
    HDDEDATA data, ULONG_PTR data1, ULONG_PTR data2) {
  return NULL;
}

DWORD WINAPI DownloadThreadMain(LPVOID param) {
  BrowserSwitcherCore* core = reinterpret_cast<BrowserSwitcherCore*>(param);
  return core->DownloadIESiteList();
}

}  // namespace

BrowserSwitcherCore::BrowserSwitcherCore() {
  Initialize();
}

BrowserSwitcherCore::~BrowserSwitcherCore() {
  ::CloseHandle(site_list_mutex_);
}

bool BrowserSwitcherCore::InvokeAlternativeBrowser(
    const std::wstring& url) const {
  // Use DDE if possible in order to respect IE's tab settings for apps. This
  // will only work if there is a running instance of the respective browser
  // otherwise it will fail but in this case a new tab or a new window is the
  // same.
  bool dde_success = false;
  if (!alt_browser_dde_host_.empty()) {
    DWORD dde_instance = 0;
    if (DdeInitialize(&dde_instance, DdeCallback, CBF_FAIL_ALLSVRXACTIONS, 0) ==
        DMLERR_NO_ERROR) {
      HSZ service;
      HSZ openurl_topic;
      HSZ activate_topic;
      HCONV openurl_service_instance;
      HCONV activate_service_instance;

      service = DdeCreateStringHandle(
          dde_instance, alt_browser_dde_host_.c_str(), CP_WINUNICODE);
      openurl_topic = DdeCreateStringHandle(
          dde_instance, L"WWW_OpenURL", CP_WINUNICODE);
      activate_topic = DdeCreateStringHandle(
          dde_instance, L"WWW_Activate", CP_WINUNICODE);
      openurl_service_instance = DdeConnect(dde_instance, service, openurl_topic, NULL);
      activate_service_instance = DdeConnect(dde_instance, service, activate_topic, NULL);
      DdeFreeStringHandle(dde_instance, service);
      DdeFreeStringHandle(dde_instance, openurl_topic);
      DdeFreeStringHandle(dde_instance, activate_topic);

      if (openurl_service_instance) {
        // Percent-encode commas and spaces because those mean something else
        // for the WWW_OpenURL verb and the url is trimmed on the first one.
        std::wstring encoded_url(url);
        size_t pos = encoded_url.find(L",");
        while (pos != std::wstring::npos) {
           encoded_url.replace(pos, 1, L"%2C");
           pos = encoded_url.find(L",", pos);
        }
        pos = encoded_url.find(L" ");
        while (pos != std::wstring::npos) {
           encoded_url.replace(pos, 1, L"%20");
           pos = encoded_url.find(L" ", pos);
        }
        dde_success = DdeClientTransaction(
            reinterpret_cast<LPBYTE>(const_cast<wchar_t*>(encoded_url.data())),
            encoded_url.size() * sizeof(wchar_t), openurl_service_instance, 0,
            0, XTYP_EXECUTE, TIMEOUT_ASYNC, NULL) != 0;
        DdeDisconnect(openurl_service_instance);
        if (activate_service_instance) {
          if (dde_success) {
            wchar_t cmd[] = L"0xFFFFFFFF,0x0";
            DdeClientTransaction(
                reinterpret_cast<LPBYTE>(cmd), sizeof(cmd),
                activate_service_instance, 0, 0, XTYP_EXECUTE, TIMEOUT_ASYNC,
                NULL);
          }
          DdeDisconnect(activate_service_instance);
        }
      }
      DdeUninitialize(dde_instance);
    }
  }

  if (dde_success)
    return true;

  std::wstring command_line =
      CompileCommandLine(GetAlternativeBrowserParameters(), url);

  std::wstring new_command_line = L"";
  new_command_line.append(L"\"").append(alt_browser_path_.c_str()).append(L"\"");
  new_command_line.append(L" ").append(command_line.c_str());

  STARTUPINFO si = { sizeof(si) };
  PROCESS_INFORMATION pi;

  bool rv = CreateProcess(alt_browser_path_.c_str(), const_cast<LPWSTR>(new_command_line.c_str()), NULL, NULL, TRUE, CREATE_BREAKAWAY_FROM_JOB, NULL, NULL, &si, &pi);
  if (rv) {
    LOG(ERR) << "Could not start the alternative browser! Handle: "
             << rv << " " << ::GetLastError() << std::endl;
    return false;
  }
  return true;
}

bool BrowserSwitcherCore::InvokeFirefox(const std::wstring& url) const {
  std::wstring command_line = CompileCommandLine(GetFirefoxParameters(), url);

  std::wstring new_command_line = L"";
  new_command_line.append(L"\"").append(firefox_path_.c_str()).append(L"\"");
  new_command_line.append(L" ").append(command_line.c_str());

  STARTUPINFO si = { sizeof(si) };
  PROCESS_INFORMATION pi;

  bool rv = CreateProcess(firefox_path_.c_str(), const_cast<LPWSTR>(new_command_line.c_str()), NULL, NULL, TRUE, CREATE_BREAKAWAY_FROM_JOB, NULL, NULL, &si, &pi);
  if (rv) {
    LOG(ERR) << "Could not start Firefox! Handle: " << rv
                << " " << ::GetLastError() << std::endl;
    return false;
  }
  return true;
}

void BrowserSwitcherCore::SetAlternativeBrowserPath(const std::wstring& path) {
  alt_browser_path_ = path;
  alt_browser_dde_host_.clear();
  if (alt_browser_path_.empty()) {
    alt_browser_path_ = GetBrowserLocation(kIExploreKey);
    alt_browser_dde_host_ = kIExploreDdeHost;
    return;
  }
  for (size_t i = 0;i < _ARRAYSIZE(kBrowsersToKeysMapping); ++i) {
    if (alt_browser_path_.compare(kBrowsersToKeysMapping[i][0]) == 0) {
      alt_browser_path_ = GetBrowserLocation(kBrowsersToKeysMapping[i][1]);
      alt_browser_dde_host_ = kBrowsersToDdeHostsMapping[i][1];
      break;
    }
  }
  alt_browser_path_ = ExpandEnvironmentVariables(alt_browser_path_);
}

const std::wstring& BrowserSwitcherCore::GetAlternativeBrowserPath() const {
  return alt_browser_path_;
}

void BrowserSwitcherCore::SetAlternativeBrowserParameters(
    const std::wstring& parameters) {
  alt_browser_parameters_ = parameters;
  alt_browser_parameters_ = ExpandEnvironmentVariables(alt_browser_parameters_);
}

const std::wstring&
BrowserSwitcherCore::GetAlternativeBrowserParameters() const {
  return alt_browser_parameters_;
}

void BrowserSwitcherCore::SetFirefoxPath(const std::wstring& path) {
  firefox_path_ = path;
  if (firefox_path_.empty() || firefox_path_.compare(kFirefoxVarName) == 0)
    firefox_path_ = GetBrowserLocation(kFirefoxKey);
  alt_browser_path_ = ExpandEnvironmentVariables(alt_browser_path_);
}

const std::wstring& BrowserSwitcherCore::GetFirefoxPath() const {
  return firefox_path_;
}

void BrowserSwitcherCore::SetFirefoxParameters(const std::wstring& parameters) {
  firefox_parameters_ = parameters;
  firefox_parameters_ = ExpandEnvironmentVariables(firefox_parameters_);
}

const std::wstring& BrowserSwitcherCore::GetFirefoxParameters() const {
  return firefox_parameters_;
}

const BrowserSwitcherCore::UrlList& 
BrowserSwitcherCore::GetUrlsToRedirect() const {
  return urls_to_redirect_;
}

void BrowserSwitcherCore::SetUrlsToRedirect(const UrlList& urls) {
  urls_to_redirect_ = urls;
  ProcessUrlList(&urls_to_redirect_, &urls_to_redirect_type_);
}

const BrowserSwitcherCore::UrlList&
BrowserSwitcherCore::GetUrlGreylist() const {
  return url_greylist_;
}

void BrowserSwitcherCore::SetUrlGreylist(const UrlList& urls) {
  url_greylist_ = urls;
  ProcessUrlList(&url_greylist_, &url_greylist_type_);
}

bool BrowserSwitcherCore::GetIESiteList(
    BrowserSwitcherCore::UrlList* list) const {
  // Wait for max 1s to avoid blocking the caller indefinetely.
  if (site_list_mutex_ &&
      ::WaitForSingleObject(site_list_mutex_, 1000) == WAIT_OBJECT_0) {
    *list = urls_from_site_list_;
    ::ReleaseMutex(site_list_mutex_);
    return true;
  }
  return false;
}

void BrowserSwitcherCore::SetIESiteList(const UrlList& urls) {
  urls_from_site_list_ = urls;
  ProcessUrlList(&urls_from_site_list_, &urls_from_site_list_type_);
}

void BrowserSwitcherCore::ProcessUrlList(UrlList* list,
                                         UrlListTypes* types) {
  // Sort will push negative entries first because those should have higher
  // priority.
  std::sort(list->begin(), list->end());
  types->resize(list->size());
  for (size_t i = 0; i < list->size(); ++i) {
    if ((*list)[i].compare(kWildcardUrl) == 0) {
      (*types)[i] = WILDCARD;
      continue;
    }

    if ((*list)[i].find('/') != (*list)[i].npos)
      (*types)[i] = PREFIX;
    else
      (*types)[i] = HOST;

    if ((*list)[i].find('!') == 0)
      (*types)[i] = ((*types)[i] == HOST ? NEGATED_HOST : NEGATED_PREFIX);
  }
}

bool BrowserSwitcherCore::ShouldOpenInAlternativeBrowser(
    const std::wstring& url) {
  enum TransitionDecision { NONE, FIREFOX, ALT_BROWSER };
  TransitionDecision decision = NONE;

  // Since we can not decide in this case we should assume it is ok to use the
  // alternative browser.
  if (!HasValidConfiguration())
    return true;
  // In case the url cracking fails at least compare the whole url.
  std::wstring hostname = url;

  URL_COMPONENTS parsed_url;
  memset(&parsed_url, 0, sizeof(parsed_url));
  parsed_url.dwStructSize = sizeof(parsed_url);
  parsed_url.dwHostNameLength = static_cast<DWORD>(-1);
  parsed_url.dwSchemeLength = static_cast<DWORD>(-1);
  parsed_url.dwUrlPathLength = static_cast<DWORD>(-1);
  parsed_url.dwExtraInfoLength = static_cast<DWORD>(-1);
  if (InternetCrackUrl(url.c_str(), 0, 0, &parsed_url))
    hostname.assign(parsed_url.lpszHostName, parsed_url.dwHostNameLength);
  else
    LOG(ERR) << "URL Parsing failed!" << std::endl;

  bool all_in_alternative_browser = false;
  std::wstring decision_rule;
  for (size_t i = 0; i < urls_to_redirect_.size(); ++i) {
    // Employ a simple, yet powerful heuristic on the entries in the list:
    // If the entry has no slashes it is assumed to be a host name or substring
    // of one. In that case we match only the host part of the url to the entry.
    // If on the other hand we have at least one slash in the string it is
    // assumed to be a proper url prefix like "http://example.com/somepath". In
    // this case we compare the beginning whole url with the list entry.
    // Lastly if the entry starts with a '!' we negate the check.
    // An entry consisting of only '*' means all should be opened in the
    // alternative browser except the negated ones.
    switch (urls_to_redirect_type_[i]) {
      case HOST:
        if (hostname.find(urls_to_redirect_[i]) != hostname.npos)
          decision = ALT_BROWSER;
        break;
      case PREFIX:
        if (url.find(urls_to_redirect_[i]) == 0)
          decision = ALT_BROWSER;
        break;
      case NEGATED_HOST:
        if (hostname.find(urls_to_redirect_[i].substr(1)) != hostname.npos)
          decision = FIREFOX;
        break;
      case NEGATED_PREFIX:
        if (url.find(urls_to_redirect_[i].substr(1)) == 0)
          decision = FIREFOX;
        break;
      case WILDCARD:
        all_in_alternative_browser = true;
        break;
    }
    if (decision != NONE) {
      decision_rule = urls_to_redirect_[i];
      break;
    }
  }

  // Since the gray list can only contribute to staying in the alt and the
  // internal list is higher prio than site list, if there is a decision exit.
  if (decision == ALT_BROWSER || all_in_alternative_browser)
    return true;

  if (decision == NONE && site_list_mutex_) {
    if (::WaitForSingleObject(site_list_mutex_, 500) == WAIT_OBJECT_0) {
      for (size_t i = 0; i < urls_from_site_list_.size(); ++i) {
        switch (urls_from_site_list_type_[i]) {
        case HOST:
          if (hostname.find(urls_from_site_list_[i]) != hostname.npos)
            decision = ALT_BROWSER;
          break;
        case PREFIX:
          if (url.find(urls_from_site_list_[i]) == 0)
            decision = ALT_BROWSER;
          break;
        case NEGATED_HOST:
          if (hostname.find(urls_from_site_list_[i].substr(1)) != hostname.npos)
            decision = FIREFOX;
          break;
        case NEGATED_PREFIX:
          if (url.find(urls_from_site_list_[i].substr(1)) == 0)
            decision = FIREFOX;
          break;
        case WILDCARD:
          all_in_alternative_browser = true;
          break;
        }
        if (decision != NONE) {
          decision_rule = urls_from_site_list_[i];
          break;
        }
      }
      ::ReleaseMutex(site_list_mutex_);

      if (decision == ALT_BROWSER)
        return true;
    }
  }

  for (size_t i = 0; i < url_greylist_.size(); ++i) {
    // See comments on the matching behavior above.
    switch (url_greylist_type_[i]) {
      case HOST:
        // Pick the greylist decision over the other one if it is more precise.
        if (hostname.find(url_greylist_[i]) != hostname.npos) {
          if (decision == NONE ||
              url_greylist_[i].length() > decision_rule.length()) {
            return true;
          }
        }
        break;
      case PREFIX:
        // Pick the greylist decision over the other one if it is more precise.
        if (url.find(url_greylist_[i]) == 0) {
          if (decision == NONE ||
              url_greylist_[i].length() > decision_rule.length()) {
            return true;
          }
        }
        break;
      // Negative entries have no meaning in the greylist.
      case NEGATED_HOST:
      case NEGATED_PREFIX:
        break;
      case WILDCARD:
        all_in_alternative_browser = true;
        break;
    }
  }

  if (decision != NONE)
    return decision == ALT_BROWSER;
  return all_in_alternative_browser;
}

void BrowserSwitcherCore::Initialize() {
  alt_browser_path_ = GetBrowserLocation(kIExploreKey);
  firefox_path_ = GetBrowserLocation(kFirefoxKey);
  configuration_valid_ = false;
  if (!LoadConfigFile())
    LOG(ERR) << "Confing file could not be loaded!" << std::endl;
  if (!LoadIESiteListCache())
    LOG(INFO) << "No IE Site List found or file can't be read." << std::endl;

  site_list_mutex_ = ::CreateMutex(NULL, FALSE, NULL);
  if (!site_list_mutex_) {
    LOG(ERR) << "Could not create mutex object for IE Site List thread. "
             << "Site list will not get updated at this run." << std::endl;
  }

  LOG(INFO) << "BrowserSwitcherCore::Initialize "
            << alt_browser_path_.c_str() << std::endl;
}

bool BrowserSwitcherCore::LoadConfigFile() {
  std::wstring path_string(GetConfigFileLocation());
  // Protect against failed config file location retrieval.
  if (path_string.empty())
    return false;

  LOG(INFO) << "Loading cache from : " << path_string.c_str() << std::endl;

  std::wifstream config_file(path_string);
  if (config_file.bad()) {
    LOG(ERR) << "Can't open config file : " << ::GetLastError() << std::endl;
    return false;
  }

  int file_version = 0;
  config_file >> file_version;
  if (config_file.fail())
    return false;
  LOG(INFO) << "file_version : '" << file_version << "'" << std::endl;
  if (file_version < kMinSupportedFileVersion ||
      file_version > kCurrentFileVersion) {
    return false;
  }
  std::wstring skip_to_eol;
  std::getline(config_file, skip_to_eol);

  std::wstring alternative_browser_path;
  if (!ReadLineFromFile(&config_file, &alternative_browser_path))
    return false;
  LOG(INFO) << "alternative_browser_path : '" << alternative_browser_path
            << "'" << std::endl;
  std::wstring alternative_browser_parameters;
  if (!ReadLineFromFile(&config_file, &alternative_browser_parameters))
    return false;
  LOG(INFO) << "alternative_browser_parameters : '"
            << alternative_browser_parameters << "'" << std::endl;
  std::wstring firefox_path;
  if (!ReadLineFromFile(&config_file, &firefox_path))
    return false;
  LOG(INFO) << "firefox_path : '" << firefox_path << "'" << std::endl;
  std::wstring firefox_parameters;
  if (!ReadLineFromFile(&config_file, &firefox_parameters))
    return false;
  LOG(INFO) << "firefox_parameters : '" << firefox_parameters << "'" << std::endl;

  size_t urls_to_load = 0;
  config_file >> urls_to_load;
  if (config_file.fail())
    return false;
  LOG(INFO) << "url list size : '" << urls_to_load << "'" << std::endl;
  if (urls_to_load > kMaxUrlFilterSize) {
    return false;
  }
  std::getline(config_file, skip_to_eol);

  UrlList urls_to_redirect;
  std::wstring url;
  for (size_t i = 0;i < urls_to_load; ++i) {
    if (!ReadLineFromFile(&config_file, &url))
      return false;
    LOG(INFO) << "url : '" << url << "'" << std::endl;
    urls_to_redirect.push_back(url);
  }

  config_file >> urls_to_load;
  if (config_file.fail())
    return false;
  LOG(INFO) << "url grey list size : '" << urls_to_load << "'" << std::endl;
  if (urls_to_load > kMaxUrlFilterSize) {
    return false;
  }
  std::getline(config_file, skip_to_eol);

  UrlList url_greylist;
  for (size_t i = 0;i < urls_to_load; ++i) {
    if (!ReadLineFromFile(&config_file, &url))
      return false;
    LOG(INFO) << "url : '" << url << "'" << std::endl;
    url_greylist.push_back(url);
  }

  SetAlternativeBrowserPath(alternative_browser_path);
  SetAlternativeBrowserParameters(alternative_browser_parameters);
  SetFirefoxPath(firefox_path);
  SetFirefoxParameters(firefox_parameters);
  SetUrlsToRedirect(urls_to_redirect);
  SetUrlGreylist(url_greylist);
  configuration_valid_ = true;
  return true;
}

bool BrowserSwitcherCore::SaveConfigFile() {
  std::wstring config_path(GetConfigFileLocation());
  // Protect against failed config file location retrieval.
  if (config_path.empty())
    return false;

  LOG(INFO) << "Saving cache to : " << config_path.c_str() << std::endl;

  std::wstring temp_config_path(config_path);
  temp_config_path.append(L".tmp");
  std::wofstream config_file(temp_config_path);
  if (config_file.bad()) {
    LOG(ERR) << "Can't open temp config file : "
             << ::GetLastError() << std::endl;
    return false;
  }

  config_file << kCurrentFileVersion << std::endl;
  config_file << GetAlternativeBrowserPath() << std::endl;
  config_file << GetAlternativeBrowserParameters() << std::endl;
  config_file << GetFirefoxPath() << std::endl;
  config_file << GetFirefoxParameters() << std::endl;
  config_file << urls_to_redirect_.size() << std::endl;
  for (size_t i = 0; i < urls_to_redirect_.size(); ++i)
    config_file << urls_to_redirect_[i] << std::endl;
  config_file << url_greylist_.size() << std::endl;
  for (size_t i = 0; i < url_greylist_.size(); ++i)
    config_file << url_greylist_[i] << std::endl;
  config_file.close();

  if (!::MoveFileEx(temp_config_path.c_str(), config_path.c_str(),
                    MOVEFILE_COPY_ALLOWED |
                    MOVEFILE_REPLACE_EXISTING |
                    MOVEFILE_WRITE_THROUGH)) {
    LOG(ERR) << "Could not move temp config file in place! "
             << ::GetLastError() << std::endl;
    return false;
  }
  configuration_valid_ = true;
  return true;
}


bool BrowserSwitcherCore::LoadIESiteListCache() {
  std::wstring path_string(GetIESiteListCacheLocation());
  // Protect against failed config file location retrieval.
  if (path_string.empty())
    return false;

  LOG(INFO) << "Loading IE Site List cache from : " << path_string.c_str()
            << std::endl;

  std::wifstream config_file(path_string);
  if (config_file.bad()) {
    LOG(ERR) << "Can't open config file : " << ::GetLastError() << std::endl;
    return false;
  }

  int file_version = 0;
  config_file >> file_version;
  if (config_file.fail())
    return false;
  LOG(INFO) << "file_version : '" << file_version << "'" << std::endl;
  if (file_version < kMinSupportedFileVersion ||
    file_version > kCurrentFileVersion) {
    return false;
  }
  std::wstring skip_to_eol;
  std::getline(config_file, skip_to_eol);

  size_t urls_to_load = 0;
  config_file >> urls_to_load;
  if (config_file.fail())
    return false;
  LOG(INFO) << "url list size : '" << urls_to_load << "'" << std::endl;
  if (urls_to_load > kMaxUrlFilterSize) {
    return false;
  }
  std::getline(config_file, skip_to_eol);

  UrlList urls_to_redirect;
  std::wstring url;
  for (size_t i = 0; i < urls_to_load; ++i) {
    if (!ReadLineFromFile(&config_file, &url))
      return false;
    LOG(INFO) << "url : '" << url << "'" << std::endl;
    urls_to_redirect.push_back(url);
  }

  SetIESiteList(urls_to_redirect);
  return true;
}

bool BrowserSwitcherCore::SaveIESiteListCache() {
  std::wstring config_path(GetIESiteListCacheLocation());
  // Protect against failed config file location retrieval.
  if (config_path.empty())
    return false;

  LOG(INFO) << "Saving IE Site List cache to : " << config_path.c_str()
            << std::endl;

  std::wstring temp_config_path(config_path);
  temp_config_path.append(L".tmp");
  std::wofstream config_file(temp_config_path);
  if (config_file.bad()) {
    LOG(ERR) << "Can't open temp config file : "
      << ::GetLastError() << std::endl;
    return false;
  }

  config_file << kCurrentFileVersion << std::endl;
  config_file << urls_from_site_list_.size() << std::endl;
  for (size_t i = 0; i < urls_from_site_list_.size(); ++i)
    config_file << urls_from_site_list_[i] << std::endl;
  config_file.close();

  if (!::MoveFileEx(temp_config_path.c_str(), config_path.c_str(),
    MOVEFILE_COPY_ALLOWED |
    MOVEFILE_REPLACE_EXISTING |
    MOVEFILE_WRITE_THROUGH)) {
    LOG(ERR) << "Could not move temp config file in place! "
      << ::GetLastError() << std::endl;
    return false;
  }
  return true;
}

bool BrowserSwitcherCore::HasValidConfiguration() const {
  return configuration_valid_;
}

void BrowserSwitcherCore::SetConfigFileLocationForTest(
    const std::wstring& path) {
  config_file_path_ = path;
  configuration_valid_ = false;
}

void BrowserSwitcherCore::SetIESiteListCacheLocationForTest(
    const std::wstring& path) {
  site_list_cache_file_path_ = path;
}

std::wstring BrowserSwitcherCore::CompileCommandLine(
    const std::wstring& raw_command_line,
    const std::wstring& url) const {
  std::wstring sanitized_url;
  // In almost every case should this be enough for the sanitization because
  // any ASCII char will expand to at most 3 chars - %[0-9A-F][0-9A-F].
  DWORD length = static_cast<DWORD>(url.length() * 3 + 1);
  std::auto_ptr<wchar_t> buffer(new wchar_t[length]);
  if (!::InternetCanonicalizeUrl(url.c_str(), buffer.get(), &length, 0)) {
    DWORD error = ::GetLastError();
    if (error == ERROR_INSUFFICIENT_BUFFER) {
      // If we get this error it means that the buffer is too small to hold the
      // canoncial url. In that case resize the buffer to what the requested
      // size is (returned in |length| and try again.
      buffer.reset(new wchar_t[length]);
      if (::InternetCanonicalizeUrl(url.c_str(), buffer.get(), &length, 0)) {
        sanitized_url = buffer.get();
      }
    }
  } else {
    sanitized_url = buffer.get();
  }
  // If the API failed, do some poor man's sanitizing at least.
  if (sanitized_url.empty()) {
    LOG(WARN) << "::InternetCanonicalizeUrl failed : "
              << ::GetLastError() << std::endl;
    sanitized_url = SanitizeUrl(url);
  }

  std::wstring command_line = raw_command_line;
  size_t pos = command_line.find(kUrlVarName);
  if (pos != command_line.npos) {
    command_line =
        command_line.replace(pos, wcslen(kUrlVarName), sanitized_url);
  } else {
    if (command_line.empty())
      command_line = sanitized_url;
    else
      command_line.append(L" ").append(sanitized_url);
  }
  return command_line;
}

std::wstring BrowserSwitcherCore::SanitizeUrl(const std::wstring url) const {
  // In almost every case should this be enough for the sanitization because
  // any ASCII char will expand to at most 3 chars - %[0-9A-F][0-9A-F].
  std::wstring::const_iterator it = url.begin();
  std::wstring untranslated_chars(L".:/\\_-@~();");
  std::auto_ptr<wchar_t> sanitized_url(new wchar_t[url.length() * 3 + 1]);
  wchar_t* output = sanitized_url.get();

  while (it != url.end()) {
    if (isalnum(*it) || untranslated_chars.find(*it) != std::wstring::npos) {
      *output++ = *it;
    } else {
      // Will only work for ASCII chars but hey it's at least something.
      // Any unicode char will be truncated to its first 8 bits and encoded.
      *output++ = '%';
      int nibble = (*it & 0xf0) >> 4;
      *output++ = nibble > 9 ? nibble - 10 + 'A' : nibble + '0';
      nibble = *it & 0xf;
      *output++ = nibble > 9 ? nibble - 10 + 'A' : nibble + '0';
    }
    it++;
  }
  *output = '\0';

  return std::wstring(sanitized_url.get());
}

const std::wstring BrowserSwitcherCore::GetConfigPath() const {
  std::wstring config_path;
  wchar_t path[MAX_PATH];
  if (!::SHGetSpecialFolderPath(0, path, CSIDL_LOCAL_APPDATA, false)) {
    LOG(ERR) << "Error locating %LOCAL_APPDATA%!" << std::endl;
    NOTREACHED();
    return config_path;
  }
  config_path.assign(path);
  ::CreateDirectory(config_path.append(L"\\Mozilla").c_str(), NULL);
  ::CreateDirectory(config_path.append(L"\\BrowserSwitcher").c_str(),
                    NULL);
  return config_path;
}

const std::wstring& BrowserSwitcherCore::GetConfigFileLocation() {
  if (config_file_path_.empty()) {
    config_file_path_ = GetConfigPath();
    config_file_path_.append(L"\\cache.dat");
  }
  return config_file_path_;
}

const std::wstring& BrowserSwitcherCore::GetIESiteListCacheLocation() {
  if (site_list_cache_file_path_.empty()) {
    site_list_cache_file_path_ = GetConfigPath();
    site_list_cache_file_path_.append(L"\\sitelistcache.dat");
  }
  return site_list_cache_file_path_;
}

std::wstring BrowserSwitcherCore::GetBrowserLocation(
    const wchar_t* key_name) const {
  HKEY key;
  if (ERROR_SUCCESS != ::RegOpenKey(HKEY_LOCAL_MACHINE, key_name, &key) &&
      ERROR_SUCCESS != ::RegOpenKey(HKEY_CURRENT_USER, key_name, &key)) {
    LOG(ERR) << "Could not open registry key " << key_name << "! Error Code:"
             << ::GetLastError() << std::endl;
    return std::wstring();
  }
  return ReadRegValue(key, NULL);
}

std::wstring BrowserSwitcherCore::ReadRegValue(
    HKEY key, const wchar_t* name) const {
  DWORD length = 0;
  if (ERROR_SUCCESS !=
      ::RegQueryValueEx(key, name, NULL, NULL, NULL, &length)) {
    LOG(ERR) << "Could not get size of the value!"
             << ::GetLastError() << std::endl;
    return std::wstring();
  }
  std::auto_ptr<wchar_t> browser_path(new wchar_t[length]);
  if (ERROR_SUCCESS !=
      ::RegQueryValueEx(key, name, NULL, NULL,
                        reinterpret_cast<LPBYTE>(browser_path.get()),
                        &length)) {
    LOG(ERR) << "Could not get the value!"
             << ::GetLastError() << std::endl;
    return std::wstring();
  }

  return std::wstring(browser_path.get());
}

std::wstring BrowserSwitcherCore::ExpandEnvironmentVariables(
    const std::wstring& str) const {
  std::wstring output = str;
  DWORD expanded_size = 0;
  expanded_size = ::ExpandEnvironmentStrings(str.c_str(), NULL, expanded_size);
  if (expanded_size != 0) {
    // The expected buffer length as defined in MSDN is chars + null + 1.
    std::auto_ptr<wchar_t> expanded_path(new wchar_t[expanded_size + 2]);
    expanded_size = ::ExpandEnvironmentStrings(str.c_str(),
                                               expanded_path.get(),
                                               expanded_size);
    if (expanded_size != 0)
      output = expanded_path.get();
  }
  return output;
}

bool BrowserSwitcherCore::StartIESiteListDownload() {
  // No mutex, no thread!
  if (!site_list_mutex_)
    return false;

  DWORD thread_id;
  HANDLE thread_handle =
      ::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)DownloadThreadMain,
                     reinterpret_cast<void*>(this), 0, &thread_id);
  return thread_handle != NULL;
}

bool BrowserSwitcherCore::DownloadIESiteList() {
  HKEY key;
  std::wstring name(L"SOFTWARE\\Policies\\Microsoft\\Internet Explorer\\Main\\"
                    L"EnterpriseMode");
  if (ERROR_SUCCESS != ::RegOpenKey(HKEY_LOCAL_MACHINE, name.c_str(), &key) &&
      ERROR_SUCCESS != ::RegOpenKey(HKEY_CURRENT_USER, name.c_str(), &key)) {
    LOG(INFO) << "Could not open registry key " << name << "! Error Code:"
              << ::GetLastError() << std::endl;
    return false;
  }
  std::wstring value(L"SiteList");
  std::wstring site_list_location = ReadRegValue(key, value.c_str());

  wchar_t local_file[MAX_PATH];
  if (S_OK != ::URLDownloadToCacheFile(NULL, site_list_location.c_str(),
                                       local_file, MAX_PATH, 0, NULL)) {
    LOG(ERR) << "Could not download IE Site List file from :"
             << site_list_location << " error: " << ::GetLastError()
             << std::endl;
    return false;
  }

  IEEMSiteListParser parser;
  if (!parser.LoadFile(std::wstring(local_file))) {
    LOG(ERR) << "Could not parse IE Site List file." << std::endl;
    return false;
  }

  if (::WaitForSingleObject(site_list_mutex_, INFINITE) == WAIT_OBJECT_0) {
    SetIESiteList(parser.GetList());
    SaveIESiteListCache();
    ::ReleaseMutex(site_list_mutex_);
    LOG(INFO) << "Site List successfully updated." << std::endl;
  }
  return true;
}
