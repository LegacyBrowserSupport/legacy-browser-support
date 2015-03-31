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

}  // namespace

BrowserSwitcherCore::BrowserSwitcherCore() {
  Initialize();
}

BrowserSwitcherCore::~BrowserSwitcherCore() {
}

bool BrowserSwitcherCore::InvokeAlternativeBrowser(const std::wstring& url) {
  std::wstring command_line =
      CompileCommandLine(GetAlternativeBrowserParameters(), url);
  HINSTANCE browser_instance =
      ::ShellExecute(NULL, NULL, alt_browser_path_.c_str(),
                     command_line.c_str(), NULL, SW_SHOWNORMAL);
  if (reinterpret_cast<int>(browser_instance) <= 32) {
    LOG(ERR) << "Could not start the alternative browser! Handle: "
             << browser_instance << " " << ::GetLastError() << std::endl;
    return false;
  }
  return true;
}

bool BrowserSwitcherCore::InvokeChrome(const std::wstring& url) {
  std::wstring command_line = CompileCommandLine(GetChromeParameters(), url);
  HINSTANCE browser_instance =
      ::ShellExecute(NULL, NULL, chrome_path_.c_str(),
                     command_line.c_str(), NULL, SW_SHOWNORMAL);
  if (reinterpret_cast<int>(browser_instance) <= 32) {
    LOG(ERR) << "Could not start Chrome! Handle: " << browser_instance
                << " " << ::GetLastError() << std::endl;
    return false;
  }
  return true;
}

void BrowserSwitcherCore::SetAlternativeBrowserPath(const std::wstring& path) {
  alt_browser_path_ = path;
  if (alt_browser_path_.empty()) {
    alt_browser_path_ = GetBrowserLocation(kIExploreKey);
    return;
  }
  for (size_t i = 0;i < _ARRAYSIZE(kBrowsersToKeysMapping); ++i) {
    if (alt_browser_path_.compare(kBrowsersToKeysMapping[i][0]) == 0) {
      alt_browser_path_ = GetBrowserLocation(kBrowsersToKeysMapping[i][1]);
      break;
    }
  }
  alt_browser_path_ = ExpandEnvironmentVariables(alt_browser_path_);
}

const std::wstring& BrowserSwitcherCore::GetAlternativeBrowserPath() {
  return alt_browser_path_;
}

void BrowserSwitcherCore::SetAlternativeBrowserParameters(
    const std::wstring& parameters) {
  alt_browser_parameters_ = parameters;
  alt_browser_parameters_ = ExpandEnvironmentVariables(alt_browser_parameters_);
}

const std::wstring& BrowserSwitcherCore::GetAlternativeBrowserParameters() {
  return alt_browser_parameters_;
}

void BrowserSwitcherCore::SetChromePath(const std::wstring& path) {
  chrome_path_ = path;
  if (chrome_path_.empty() || chrome_path_.compare(kChromeVarName) == 0)
    chrome_path_ = GetBrowserLocation(kChromeKey);
  alt_browser_path_ = ExpandEnvironmentVariables(alt_browser_path_);
}

const std::wstring& BrowserSwitcherCore::GetChromePath() {
  return chrome_path_;
}

void BrowserSwitcherCore::SetChromeParameters(const std::wstring& parameters) {
  chrome_parameters_ = parameters;
  chrome_parameters_ = ExpandEnvironmentVariables(chrome_parameters_);
}

const std::wstring& BrowserSwitcherCore::GetChromeParameters() {
  return chrome_parameters_;
}

const BrowserSwitcherCore::UrlList& BrowserSwitcherCore::GetUrlsToRedirect() {
  return urls_to_redirect_;
}

void BrowserSwitcherCore::SetUrlsToRedirect(const UrlList& urls) {
  urls_to_redirect_ = urls;
  // Sort will push negative entries first because those should have higher
  // priority.
  std::sort(urls_to_redirect_.begin(), urls_to_redirect_.end());
  ProcessUrlList(urls_to_redirect_, &urls_to_redirect_type_);
}

const BrowserSwitcherCore::UrlList& BrowserSwitcherCore::GetUrlGreylist() {
  return url_greylist_;
}

void BrowserSwitcherCore::SetUrlGreylist(const UrlList& urls) {
  url_greylist_ = urls;
  // Sort will push negative entries first because those should have higher
  // priority.
  std::sort(url_greylist_.begin(), url_greylist_.end());
  ProcessUrlList(url_greylist_, &url_greylist_type_);
}

void BrowserSwitcherCore::ProcessUrlList(const UrlList& list,
                                         UrlListTypes* types) {
  types->resize(list.size());
  for (size_t i = 0; i < list.size(); ++i) {
    if (list[i].compare(kWildcardUrl) == 0) {
      (*types)[i] = WILDCARD;
      continue;
    }

    if (list[i].find('/') != urls_to_redirect_[i].npos)
      (*types)[i] = PREFIX;
    else
      (*types)[i] = HOST;

    if (list[i].find('!') == 0)
      (*types)[i] = ((*types)[i] == HOST ? NEGATED_HOST : NEGATED_PREFIX);
  }
}

bool BrowserSwitcherCore::ShouldOpenInAlternativeBrowser(
    const std::wstring& url) {
  enum TransitionDecision { NONE, CHROME, ALT_BROWSER };
  TransitionDecision decision = NONE;
  int decision_idx = -1;

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
          decision = CHROME;
        break;
      case NEGATED_PREFIX:
        if (url.find(urls_to_redirect_[i].substr(1)) == 0)
          decision = CHROME;
        break;
      case WILDCARD:
        all_in_alternative_browser = true;
        break;
    }
    if (decision != NONE) {
      decision_idx = i;
      break;
    }
  }
  // Since the gray list can only contribute to staying in the alt. browser
  // if this is already the decision exit early.
  if (decision == ALT_BROWSER || all_in_alternative_browser)
    return true;

  for (size_t i = 0; i < url_greylist_.size(); ++i) {
    // See comments on the matching behavior above.
    switch (url_greylist_type_[i]) {
      case HOST:
        // Pick the greylist decision over the other one if it is more precise.
        if (hostname.find(url_greylist_[i]) != hostname.npos) {
          if (decision == NONE || url_greylist_[i].length() >
                  urls_to_redirect_[decision_idx].length()) {
            return true;
          }
        }
        break;
      case PREFIX:
        // Pick the greylist decision over the other one if it is more precise.
        if (url.find(url_greylist_[i]) == 0) {
          if (decision == NONE || url_greylist_[i].length() >
                  urls_to_redirect_[decision_idx].length()) {
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
  chrome_path_ = GetBrowserLocation(kChromeKey);
  configuration_valid_ = false;
  if (!LoadConfigFile())
    LOG(ERR) << "Confing file could not be loaded!" << std::endl;
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
  std::wstring chrome_path;
  if (!ReadLineFromFile(&config_file, &chrome_path))
    return false;
  LOG(INFO) << "chrome_path : '" << chrome_path << "'" << std::endl;
  std::wstring chrome_parameters;
  if (!ReadLineFromFile(&config_file, &chrome_parameters))
    return false;
  LOG(INFO) << "chrome_parameters : '" << chrome_parameters << "'" << std::endl;

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
  SetChromePath(chrome_path);
  SetChromeParameters(chrome_parameters);
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
  config_file << GetChromePath() << std::endl;
  config_file << GetChromeParameters() << std::endl;
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

bool BrowserSwitcherCore::HasValidConfiguration() {
  return configuration_valid_;
}

void BrowserSwitcherCore::SetConfigFileLocationForTest(
    const std::wstring& path) {
  config_file_path_ = path;
  configuration_valid_ = false;
}

std::wstring BrowserSwitcherCore::CompileCommandLine(
    const std::wstring& raw_command_line,
    const std::wstring& url) {
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

std::wstring BrowserSwitcherCore::SanitizeUrl(const std::wstring url) {
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

const std::wstring& BrowserSwitcherCore::GetConfigFileLocation() {
  if (config_file_path_.empty()) {
    wchar_t path[MAX_PATH];
    if (!::SHGetSpecialFolderPath(0, path, CSIDL_LOCAL_APPDATA, false)) {
      LOG(ERR) << "Error locating %LOCAL_APPDATA%!" << std::endl;
      NOTREACHED();
      return config_file_path_;
    }
    config_file_path_.assign(path);
    ::CreateDirectory(config_file_path_.append(L"\\Google").c_str(), NULL);
    ::CreateDirectory(config_file_path_.append(L"\\BrowserSwitcher").c_str(),
                      NULL);
    config_file_path_.append(L"\\cache.dat");
  }

  return config_file_path_;
}

std::wstring BrowserSwitcherCore::GetBrowserLocation(const wchar_t* key_name) {
  HKEY key;
  if (ERROR_SUCCESS != ::RegOpenKey(HKEY_LOCAL_MACHINE, key_name, &key) &&
      ERROR_SUCCESS != ::RegOpenKey(HKEY_CURRENT_USER, key_name, &key)) {
    LOG(ERR) << "Could not open registry key " << key_name << "! Error Code:"
             << ::GetLastError() << std::endl;
    return std::wstring();
  }
  return ReadDefaultRegValue(key);
}

std::wstring BrowserSwitcherCore::ReadDefaultRegValue(HKEY key) {
  DWORD length = 0;
  if (ERROR_SUCCESS !=
      ::RegQueryValueEx(key, NULL, NULL, NULL, NULL, &length)) {
    LOG(ERR) << "Could not get size of the default value!"
             << ::GetLastError() << std::endl;
    return std::wstring();
  }
  std::auto_ptr<wchar_t> browser_path(new wchar_t[length]);
  if (ERROR_SUCCESS !=
      ::RegQueryValueEx(key, NULL, NULL, NULL,
                        reinterpret_cast<LPBYTE>(browser_path.get()),
                        &length)) {
    LOG(ERR) << "Could not get the default value!"
             << ::GetLastError() << std::endl;
    return std::wstring();
  }

  return std::wstring(browser_path.get());
}

std::wstring BrowserSwitcherCore::ExpandEnvironmentVariables(
  const std::wstring& str) {
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
