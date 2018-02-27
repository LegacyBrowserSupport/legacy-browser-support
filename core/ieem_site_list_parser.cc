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

#include "core/ieem_site_list_parser.h"

namespace {

// Convert std::string to std::wstring.
std::wstring StringToWString(const std::string& utf8_string) {
  int len = ::MultiByteToWideChar(CP_UTF8, 0, utf8_string.data(),
    utf8_string.length(), NULL, 0);
  std::auto_ptr<wchar_t> buffer(new wchar_t[len]);
  len = ::MultiByteToWideChar(CP_UTF8, 0, utf8_string.data(),
    utf8_string.length(), buffer.get(), len);
  return std::wstring(buffer.get(), buffer.get() + len);
}

// Convert std::string to std::wstring.
std::string WStringToString(const std::wstring& utf8_string) {
  int len = ::WideCharToMultiByte(CP_UTF8, 0, utf8_string.data(),
    utf8_string.length(), NULL, 0, NULL, NULL);
  std::auto_ptr<char> buffer(new char[len]);
  len = ::WideCharToMultiByte(CP_UTF8, 0, utf8_string.data(),
    utf8_string.length(), buffer.get(), len, NULL, NULL);
  return std::string(buffer.get(), buffer.get() + len);
}

const char Schema1RulesElement[] = "rules";
const char Schema1EmieElement[] = "emie";
const char Schema1DocModeElement[] = "docMode";
const char Schema1DomainElement[] = "domain";
const char Schema1PathElement[] = "path";
const char Schema1ExcludeAttribute[] = "exclude";
const char Schema1TrueValue[] = "true";

const char Schema2SiteListElement[] = "site-list";
const char Schema2SiteElement[] = "site";
const char Schema2SiteUrlAttribute[] = "url";
const char Schema2SiteOpenInElement[] = "open-in";
}  // namespace

IEEMSiteListParser::IEEMSiteListParser() {
}

IEEMSiteListParser::~IEEMSiteListParser() {
}

bool IEEMSiteListParser::LoadFile(const std::string& filename) {
  reader_.reset(new XmlReader);
  // Try to open the file and advance to first actual element.
  if (!reader_->LoadFile(filename) || !reader_->SkipToElement())
    return false;
  // Enterprise Mode schema v.1 has rules element at its top level.
  if (reader_->NodeName() == Schema1RulesElement) {
    return ParseIEFileVersionOne();
  } else
    return ParseIEFileVersionTwo();
}

bool IEEMSiteListParser::LoadFile(const std::wstring& filename) {
  return LoadFile(WStringToString(filename));
}

bool IEEMSiteListParser::ParseIEFileVersionOne() {
  // Enterprise Mode schema v.1 has rules element at its top level.
  if (reader_->NodeName() != Schema1RulesElement || !reader_->SkipToElement())
    return false;
  while (!(reader_->IsClosingElement() && reader_->Depth() == 0)) {
    // Skip over anything that is not a emie or docMode element.
    if (reader_->NodeName() != Schema1EmieElement &&
        reader_->NodeName() != Schema1DocModeElement) {
      if (!reader_->IsEmptyElement()) {
        while (!(reader_->IsClosingElement() && reader_->Depth() == 1))
          if (!reader_->Read(true))
            break;
      }
    } else {
      if (!reader_->SkipToElement())
        break;
      while (!(reader_->IsClosingElement() && reader_->Depth() == 1)) {
        // Skip over anything that is not a domain element.
        if (reader_->NodeName() != Schema1DomainElement) {
          if (!reader_->IsEmptyElement()) {
            while (!(reader_->IsClosingElement() && reader_->Depth() == 2))
              if (!reader_->Read(true))
                break;
          }
        } else {
          bool exclude = false;
          std::string exclude_attrib;
          reader_->NodeAttribute(Schema1ExcludeAttribute, &exclude_attrib);
          exclude = exclude_attrib == Schema1TrueValue;
          std::string domain;
          reader_->ReadElementText(&domain);
          if (!domain.empty()) {
            if (exclude)
              list_.push_back(StringToWString("!" + domain));
            else
              list_.push_back(StringToWString(domain));
          }
          // Read all sub-elements and keep the content of the path element.
          if (!reader_->IsEmptyElement()) {
            while (!(reader_->IsClosingElement() && reader_->Depth() == 2)) {
              if (reader_->NodeName() == Schema1PathElement &&
                  reader_->Depth() == 3 && !reader_->IsClosingElement()) {
                bool exclude_path = false;
                reader_->NodeAttribute(Schema1ExcludeAttribute,
                                       &exclude_attrib);
                exclude_path = exclude_attrib == Schema1TrueValue;
                std::string path;
                reader_->ReadElementText(&path);
                if (!path.empty() && !domain.empty()) {
                  if (exclude_path)
                    list_.push_back(StringToWString("!" + domain + path));
                  else
                    list_.push_back(StringToWString(domain + path));
                }
              }
              if (!reader_->Read(true))
                break;
            }
          }
        }
        if (!reader_->SkipToElement())
          break;
      }
    }
    if (!reader_->SkipToElement())
      break;
  }
  return true;
}

bool IEEMSiteListParser::ParseIEFileVersionTwo() {
  // Enterprise Mode schema v.1 has site-list element at its top level.
  if (reader_->NodeName() != Schema2SiteListElement ||
      !reader_->SkipToElement())
    return false;
  while (!(reader_->IsClosingElement() && reader_->Depth() == 0)) {
    // Skip over anything that is not a site (notably created-by elements).
    if (reader_->NodeName() != Schema2SiteElement) {
      if (!reader_->IsEmptyElement()) {
        while (!(reader_->IsClosingElement() && reader_->Depth() == 1))
          reader_->Read(true);
      }
    } else {
      std::string url;
      std::string mode;
      reader_->NodeAttribute(Schema2SiteUrlAttribute, &url);
      // Read all sub-elements and keep the content of the open-in element.
      if (!reader_->IsEmptyElement()) {
        while (!(reader_->IsClosingElement() && reader_->Depth() == 1)) {
          if (reader_->NodeName() == Schema2SiteOpenInElement &&
              reader_->Depth() == 2 && !reader_->IsClosingElement()) {
            reader_->ReadElementText(&mode);
          }
          reader_->Read(true);
        }
      }
      if (!url.empty()) {
        if (!mode.empty() && mode != "none")
          list_.push_back(StringToWString(url));
        else
          list_.push_back(StringToWString("!" + url));
      }
    }
    reader_->SkipToElement();
  }
  return true;
}
