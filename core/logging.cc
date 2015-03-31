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

#include "core/logging.h"

#include <iostream>

std::wostream* gLogStream = &std::wcout;
std::wofstream gLogFileStream;
LogLevels gLogLevel = INFO;

// Must be called before the first logging call is made.
void InitLog(const std::wstring& file) {
  gLogFileStream.open(file, std::ios_base::out | std::ios_base::trunc);
  gLogStream = &gLogFileStream;
}

// Closes the log file. No more logging is possible after this call.
void CloseLog() {
  gLogFileStream.close();
}

void SetLogLevel(LogLevels level) {
  gLogLevel = level;
}
