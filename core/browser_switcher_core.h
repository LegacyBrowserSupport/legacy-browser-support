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

#include <Windows.h>

#include <string>
#include <vector>

// Implements the browser switching logic for both Chrome and the alternative
// browser.
class BrowserSwitcherCore {
 public:
  // Defines the type of the list of domains to open in the alternative browser.
  typedef std::vector<std::wstring> UrlList;

  BrowserSwitcherCore();
  virtual ~BrowserSwitcherCore();

  // Invokes the configured alternative browser and loads |url| in it.
  bool InvokeAlternativeBrowser(const std::wstring& url) const;
  // Invokes Chrome and loads |url| in it.
  bool InvokeChrome(const std::wstring& url) const;

  // Setter and getter for the alternative browser executable. |path| can be
  // either a fully qualified path to an executable or one of the following
  // variables. If any of those variables is used then they should be the only
  // content of the parameter as they will resolve to the fully qualified path
  // of one of the browsers.
  //     ${ie}      - The default location of Internet Explorer as defined in
  //                  the registry.
  //     ${firefox} - The default location of Firefox as defined in the
  //                  registry.
  //     ${opera}   - The default location of Opera as defined in the registry.
  void SetAlternativeBrowserPath(const std::wstring& path);
  const std::wstring& GetAlternativeBrowserPath() const;

  // Setter and getter for the alternative browser command line parameters.
  // |parameters| can contain the following variable in which case this will be
  // the position of the url to be opened, otherwise it will be appended at the
  // end:
  //     ${url} - The location of the url parameter in the command line.
  void SetAlternativeBrowserParameters(const std::wstring& parameters);
  const std::wstring& GetAlternativeBrowserParameters() const;

  // Setter and getter for Chrome's executable. |path| can be either a fully
  // qualified path to an executable or the following variable. If this variable
  // is used then it should be the only content of the parameter as it will
  // resolve to the fully qualified path of the browser.
  //     ${chrome}  - The default location of Chrome as defined in the registry.
  void SetChromePath(const std::wstring& path);
  const std::wstring& GetChromePath() const;

  // Setter and getter for the Chrome command line parameters.
  // |parameters| can contain the following variable in which case this will be
  // the position of the url to be opened, otherwise it will be appended at the
  // end:
  //     ${url} - The location of the url parameter in the command line.
  void SetChromeParameters(const std::wstring& parameters);
  const std::wstring& GetChromeParameters() const;

  // Setter and getter for the list of urls to be opened in the alternative
  // browser.
  const UrlList& GetUrlsToRedirect() const;
  void SetUrlsToRedirect(const UrlList& urls);

  // Setter and getter for the list of urls to be opened in both browsers. This
  // is the set of urls that should not trigger transition. Such set might be
  // required if for example there are third party authentication pages that
  // need to be accessible by both legacy and normal applications.
  const UrlList& GetUrlGreylist() const;
  void SetUrlGreylist(const UrlList& urls);

  // Setter and getter for the list of urls to be opened in both browsers. This
  // is the set of urls that should not trigger transition. Such set might be
  // required if for example there are third party authentication pages that
  // need to be accessible by both legacy and normal applications.
  bool GetIESiteList(UrlList* list) const;
  void SetIESiteList(const UrlList& urls);

  // Checks if an url should be opened in the alternative browser. Returns true
  // if the hostname (or part of it) of the url is contained in the url lists.
  // This function should be used by external browsers to verify if they should
  // bounce back to Chrome. Chrome itself uses different logic to decide if the
  // url should be opened in the external browser.
  bool ShouldOpenInAlternativeBrowser(const std::wstring& url);

  // Loader and saver for the configuraton file.
  bool LoadConfigFile();
  bool SaveConfigFile();

  // Loader and saver for the site list cache file. The cache is used to speed
  // up the start time of LBS since retrieving the original list might require
  // network access.
  bool LoadIESiteListCache();
  bool SaveIESiteListCache();

  // Returns true if the configuration has been loaded or saved successfully.
  // Used mainly to verify the course of action in the alternative browser
  // which has no direct access to policy and relies on properly loaded confi-
  // guration.
  bool HasValidConfiguration() const;

  // Starts the download thread that calls DownloadIESiteList.
  bool StartIESiteListDownload();

  // Locates and downloads the IE Site List file and stores it in a cache file.
  // This function can take arbitrary amount of time. Never call on the main
  // thread.
  bool DownloadIESiteList();

 private:
  friend class BrowserSwitcherCoreTest;

  enum UrlListEntryType {HOST, PREFIX, NEGATED_HOST, NEGATED_PREFIX, WILDCARD};
  typedef std::vector<UrlListEntryType> UrlListTypes;

  // Performs initalization of the class and loads the config file.
  void Initialize();

  // Retrieves the configuration files path based on %LOCALAPPDATA%.
  const std::wstring GetConfigPath() const;
  const std::wstring& GetConfigFileLocation();
  const std::wstring& GetIESiteListCacheLocation();
  // Used for tests only to mock the config file.
  void SetConfigFileLocationForTest(const std::wstring& path);
  void SetIESiteListCacheLocationForTest(const std::wstring& path);

  // Compiles the final command line to start a browser. It will replace the
  // ${url} variable with the supplied url if it is present or append the url to
  // the end if the variable is not present in the command line.
  // The function will also attempt to canonicalize the url to make sure it is
  // not passing potentially dangerous argument to the browser.
  std::wstring CompileCommandLine(const std::wstring& raw_command_line,
                                  const std::wstring& url) const;

  // Poor man's implementation of URL sanitization, used if the call to the
  // WinInet API InternetCanonicalizeUrl fails for some reason.
  std::wstring SanitizeUrl(const std::wstring url) const;

  // Processes a list of url patterns creating a parallel list with the pattern
  // types for each entry. This is done to speed up searching for matches when
  // deciding for redirecting.
  void ProcessUrlList(UrlList* list, UrlListTypes* types);

  // Retrieves the location of various browsers. Using the values in the
  // registry under HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\.
  // Returns either the executable location or an empty string if not found.
  std::wstring GetBrowserLocation(const wchar_t* key_name) const;

  // Retrieves a string value from the registry stored at |key| or an empty
  // string if missing.
  std::wstring ReadRegValue(HKEY key, const wchar_t* name) const;

  // Expands any environment variables the input string |str| might contain.
  std::wstring ExpandEnvironmentVariables(const std::wstring& str) const;

  std::wstring alt_browser_path_;
  std::wstring alt_browser_dde_host_;
  std::wstring chrome_path_;

  std::wstring alt_browser_parameters_;
  std::wstring chrome_parameters_;

  UrlList urls_to_redirect_;
  UrlListTypes urls_to_redirect_type_;
  UrlList url_greylist_;
  UrlListTypes url_greylist_type_;
  UrlList urls_from_site_list_;
  UrlListTypes urls_from_site_list_type_;

  HANDLE site_list_mutex_;

  std::wstring config_file_path_;
  std::wstring site_list_cache_file_path_;
  // Tracks the validity of the cached configuration. If a load has succeeded
  // once this status is flipped to true. As long as no load has been successful
  // and no configuration has been set and successfully saved this status stays
  // false.
  bool configuration_valid_;
};

