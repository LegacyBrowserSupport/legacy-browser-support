// Copyright 2017 Google Inc.
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

#include "core/browser_switcher_core.h"
#include "core/libxml_utils.h"

#include <memory>

class IEEMSiteListParser {
 public:
  IEEMSiteListParser();
  ~IEEMSiteListParser();

  bool LoadFile(const std::string& filename);
  bool LoadFile(const std::wstring& filename);

  BrowserSwitcherCore::UrlList GetList() const { return list_; }

 private:
  friend class IEEMSiteListParserTest;

  // Parses Enterprise Mode schema 1 files according to:
  // https://technet.microsoft.com/itpro/internet-explorer/ie11-deploy-guide/enterprise-mode-schema-version-1-guidance
  // and returns a UrlList of the found URLs.
  bool ParseIEFileVersionOne();

  // Parses Enterprise Mode schema 2 files according to:
  // https://technet.microsoft.com/itpro/internet-explorer/ie11-deploy-guide/enterprise-mode-schema-version-2-guidance
  // and returns a UrlList of the found URLs.
  bool ParseIEFileVersionTwo();

  std::auto_ptr<XmlReader> reader_;
  BrowserSwitcherCore::UrlList list_;
};