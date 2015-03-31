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

#include <cassert>
#include <fstream>

enum LogLevels { OFF = 0, ERR = 1, WARN = 2, INFO = 3};

extern std::wostream *gLogStream;
extern LogLevels gLogLevel;

void InitLog(const std::wstring& file);
void CloseLog();
void SetLogLevel(LogLevels level);

#define INFO_MSG "[info] : "
#define WARN_MSG "[WARN] : "
#define ERR_MSG "[*ERROR!*] : "

#define LOG(a) if (gLogLevel >= a) \
    *gLogStream << (a##_MSG) << __FILE__ << ":" << __LINE__ << " : "

#define DCHECK(a) assert(a)
#define NOTREACHED() assert(false)
