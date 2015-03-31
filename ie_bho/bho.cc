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

#include "bho.h"

#include <atlstr.h>
#include <time.h>
#include <wininet.h>

#include "core/logging.h"

namespace {
const wchar_t kHttpPrefix[] = L"http://";
const wchar_t kHttpsPrefix[] = L"https://";
const wchar_t kFilePrefix[] = L"file://";
}  // namespace

// Implementation of IObjectWithSiteImpl::SetSite.
STDMETHODIMP CBrowserSwitcherBHO::SetSite(IUnknown* site) {
  if (site != NULL) {
    HRESULT hr = site->QueryInterface(IID_IWebBrowser2,
                                      reinterpret_cast<void **>(&web_browser_));
    if (SUCCEEDED(hr)) {
      hr = DispEventAdvise(web_browser_);
      advised_ = true;
    }
  } else {  // site == NULL
    if (advised_) {
      DispEventUnadvise(web_browser_);
      advised_ = false;
    }
    web_browser_.Release();
  }
  return IObjectWithSiteImpl<CBrowserSwitcherBHO>::SetSite(site);
}

// If enabled, monitors navigations and redirects them to Chrome if they are
// not intended to happen in IE according to the Legacy Browser Support policy.
// This only applies to top-level documents (not to frames).
void STDMETHODCALLTYPE CBrowserSwitcherBHO::BeforeNavigate(
    IDispatch *disp,
    VARIANT *url,
    VARIANT *flags,
    VARIANT *target_frame_name,
    VARIANT *post_data,
    VARIANT *headers,
    VARIANT_BOOL *cancel) {
  if (web_browser_ != NULL && disp != NULL) {
    ATL::CComPtr<IUnknown> unknown1;
    ATL::CComPtr<IUnknown> unknown2;
    if (SUCCEEDED(web_browser_->QueryInterface(
                      IID_IUnknown, reinterpret_cast<void**>(&unknown1))) &&
        SUCCEEDED(disp->QueryInterface(
                      IID_IUnknown, reinterpret_cast<void**>(&unknown2)))) {
      // check if this is the outter frame.
      if (unknown1 == unknown2) {
        bool result = CheckUrl((LPOLESTR)url->bstrVal,
                               *cancel != VARIANT_FALSE);
        if (result)
          web_browser_->Quit();
        *cancel = result ? VARIANT_TRUE : VARIANT_FALSE;
      }
    }
  }
}

// Checks if an url should be loaded in IE or forwarded to the default browser.
bool CBrowserSwitcherBHO::CheckUrl(LPOLESTR url, bool cancel) {
  // Only verify the url if it is http[s] link.
  if ((!_wcsnicmp(url, kHttpPrefix, wcslen(kHttpPrefix)) ||
       !_wcsnicmp(url, kHttpsPrefix, wcslen(kHttpsPrefix)) ||
       !_wcsnicmp(url, kFilePrefix, wcslen(kFilePrefix))) &&
      !browser_switcher_.ShouldOpenInAlternativeBrowser(url)) {
    LOG(INFO) << "\tTriggering redirect" << std::endl;
    if (!browser_switcher_.InvokeChrome(url)) {
      LOG(ERR) << "Cound not invoke alternative browser! "
               << "Will resume loading in IE!" << std::endl;
    } else {
      cancel = true;
    }
  }
  return cancel;
}
