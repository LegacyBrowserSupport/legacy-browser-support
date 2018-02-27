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

// This is a copy of the Chromium adapter for lib xml modified for the 
// specific structire of the XML in the site lists.

#include "libxml_utils.h"

#include "third_party/libxml/src/include/libxml/parser.h"

std::string XmlStringToStdString(const xmlChar* xmlstring) {
  // xmlChar*s are UTF-8, so this cast is safe.
  if (xmlstring)
    return std::string(reinterpret_cast<const char*>(xmlstring));
  else
    return "";
}

XmlReader::XmlReader() : reader_(NULL), has_error_(false) {
}

XmlReader::~XmlReader() {
  if (reader_)
    xmlFreeTextReader(reader_);
}

bool XmlReader::Load(const std::string& input) {
  const int kParseOptions = /*XML_PARSE_RECOVER |*/  // recover on errors
                            XML_PARSE_NONET;     // forbid network access
  // TODO(evanm): Verify it's OK to pass NULL for the URL and encoding.
  // The libxml code allows for these, but it's unclear what effect is has.
  reader_ = xmlReaderForMemory(input.data(), static_cast<int>(input.size()),
                               NULL, NULL, kParseOptions);
  xmlTextReaderSetErrorHandler(reader_, &XmlReader::ReaderErrorFunc, 
                               &has_error_);
  return reader_ != NULL;
}

bool XmlReader::LoadFile(const std::string& file_path) {
  const int kParseOptions = XML_PARSE_RECOVER |  // recover on errors
                            XML_PARSE_NONET;     // forbid network access
  reader_ = xmlReaderForFile(file_path.c_str(), NULL, kParseOptions);
  return reader_ != NULL;
}

bool XmlReader::Read(bool skip_ws) { 
  if (has_error_)
    return false;
  if (!xmlTextReaderRead(reader_))
    return false;
  while (skip_ws && 
         (NodeType() == XML_READER_TYPE_WHITESPACE || 
          NodeType() == XML_READER_TYPE_SIGNIFICANT_WHITESPACE)) {
    if (!xmlTextReaderRead(reader_))
      return false;
  }
  return true;
}

bool XmlReader::NodeAttribute(const char* name, std::string* out) {
  xmlChar* value = xmlTextReaderGetAttribute(reader_, BAD_CAST name);
  if (!value)
    return false;
  *out = XmlStringToStdString(value);
  xmlFree(value);
  return true;
}

bool XmlReader::IsClosingElement() {
  return NodeType() == XML_READER_TYPE_END_ELEMENT;
}

bool XmlReader::IsEmptyElement() {
  return xmlTextReaderIsEmptyElement(reader_) != 0;
}

bool XmlReader::ReadElementContent(std::string* content) {
  const int start_depth = Depth();

  if (xmlTextReaderIsEmptyElement(reader_)) {
    // Empty tag.  We succesfully read the content, but it's
    // empty.
    *content = "";
    // Advance past this empty tag.
    if (!Read(false))
      return false;
    return true;
  }

  // Advance past opening element tag.
  if (!Read(false))
    return false;

  // Read the text.  We read up until we hit a closing tag at the
  // same level as our starting point.
  while (NodeType() != XML_READER_TYPE_END_ELEMENT || Depth() != start_depth) {
    *content += XmlStringToStdString(xmlTextReaderConstValue(reader_));
    if (!Read(true))
      return false;
  }

  // Advance past ending element tag.
  Read(false);
  return true;
}

bool XmlReader::ReadElementText(std::string* content) {
  if (xmlTextReaderIsEmptyElement(reader_)) {
    // Empty tag.  We succesfully read the content, but it's
    // empty.
    *content = "";
    return true;
  }

  // Advance past opening element tag.
  if (!Read(false))
    return false;

  // Read the text.  We read up until we hit a closing tag at the
  // same level as our starting point.
  if (NodeType() == XML_READER_TYPE_TEXT) {
    *content = XmlStringToStdString(xmlTextReaderConstValue(reader_));
  }
  
  return true;
}

bool XmlReader::SkipToElement() {
  if (NodeType() == XML_READER_TYPE_ELEMENT && 
      xmlTextReaderIsEmptyElement(reader_)) {
    if (!Read(true))
      return false;
    return true;
  }
  if (!Read(true))
    return false;
  do {
    switch (NodeType()) {
    case XML_READER_TYPE_ELEMENT:
      return true;
    case XML_READER_TYPE_END_ELEMENT:
      return true;
    case XML_READER_TYPE_NONE:
      return false;
    default:
      // Skip all other node types.
      continue;
    }
  } while (Read(true));
  return false;
}

// static 
void XmlReader::ReaderErrorFunc(void* arg, const char*,
    xmlParserSeverities severity, xmlTextReaderLocatorPtr) {
  bool* has_error = reinterpret_cast<bool*>(arg);
  if (severity == XML_PARSER_SEVERITY_ERROR || 
      severity == XML_PARSER_SEVERITY_VALIDITY_WARNING) {
    *has_error = true;
  }
}
