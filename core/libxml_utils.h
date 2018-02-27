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

#pragma once

#include <string>

#include "third_party/libxml/src/include/libxml/xmlreader.h"
#include "third_party/libxml/src/include/libxml/xmlwriter.h"

// Converts a libxml xmlChar* into a UTF-8 std::string.
// NULL inputs produce an empty string.
std::string XmlStringToStdString(const xmlChar* xmlstring);

// libxml uses a global error function pointer for reporting errors.
// A ScopedXmlErrorFunc object lets you change the global error pointer
// for the duration of the object's lifetime.
class ScopedXmlErrorFunc {
 public:
  ScopedXmlErrorFunc(void* context, xmlGenericErrorFunc func) {
    old_error_func_ = xmlGenericError;
    old_error_context_ = xmlGenericErrorContext;
    xmlSetGenericErrorFunc(context, func);
  }
  ~ScopedXmlErrorFunc() {
    xmlSetGenericErrorFunc(old_error_context_, old_error_func_);
  }

 private:
  xmlGenericErrorFunc old_error_func_;
  void* old_error_context_;
};

// XmlReader is a wrapper class around libxml's xmlReader,
// providing a simplified C++ API.
class XmlReader {
 public:
  XmlReader();
  ~XmlReader();

  // Load a document into the reader from memory.  |input| must be UTF-8 and
  // exist for the lifetime of this object.  Returns false on error.
  // TODO(evanm): handle encodings other than UTF-8?
  bool Load(const std::string& input);

  // Load a document into the reader from a file.  Returns false on error.
  bool LoadFile(const std::string& file_path);

  // Wrappers around libxml functions -----------------------------------------

  // Read() advances to the next node.  Returns false on EOF or error.
  bool Read(bool skip_ws);

  // Next(), when pointing at an opening tag, advances to the node after
  // the matching closing tag.  Returns false on EOF or error.
  bool Next() { return xmlTextReaderNext(reader_) == 1; }

  // Return the depth in the tree of the current node.
  int Depth() { return xmlTextReaderDepth(reader_); }

  // Returns the "local" name of the current node.
  // For a tag like <foo:bar>, this is the string "foo:bar".
  std::string NodeName() {
    return XmlStringToStdString(xmlTextReaderConstLocalName(reader_));
  }

  // When pointing at a tag, retrieves the value of an attribute.
  // Returns false on failure.
  // E.g. for <foo bar:baz="a">, NodeAttribute("bar:baz", &value)
  // returns true and |value| is set to "a".
  bool NodeAttribute(const char* name, std::string* value);

  // Returns true if the node is a closing element (e.g. </foo>).
  bool IsClosingElement();

  // Returns true if the node is a empty element (e.g. <foo/>).
  bool IsEmptyElement();

  // Helper functions not provided by libxml ----------------------------------

  // Return the string content within an element.
  // "<foo>bar</foo>" is a sequence of three nodes:
  // (1) open tag, (2) text, (3) close tag.
  // With the reader currently at (1), this returns the text of (2),
  // and advances past (3).
  // Returns false on error.
  bool ReadElementContent(std::string* content);


  // Return the string content within an element.
  // "<foo>bar</foo>" is a sequence of three nodes:
  // (1) open tag, (2) text, (3) close tag.
  // With the reader currently at (1), this returns the text of (2),
  // and advances past (3).
  // Returns false on error.
  bool ReadElementText(std::string* content);

  // Skip to the next opening tag, returning false if we reach a closing
  // tag or EOF first.
  // If currently on an opening tag, doesn't advance at all.
  bool SkipToElement();

  // Returns the libxml node type of the current node.
  int NodeType() { return xmlTextReaderNodeType(reader_); }
private:
  // The underlying libxml xmlTextReader.
  xmlTextReaderPtr reader_;

  bool has_error_;

  static void ReaderErrorFunc(void* arg, const char * msg,
    xmlParserSeverities severity, xmlTextReaderLocatorPtr locator);
};
