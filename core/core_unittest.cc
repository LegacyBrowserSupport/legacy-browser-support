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

#include <SDKDDKVer.h>

#include <Windows.h>

#include <gtest/gtest.h>

#include "core/browser_switcher_core.h"
#include "core/logging.h"

#include "core/ieem_site_list_parser.h"

class BrowserSwitcherCoreTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    ASSERT_TRUE(CreateTempFile(&temp_file_));
  }

  virtual void TearDown() {
    ::DeleteFile(temp_file_.c_str());
  }

  // Delegates calls to a the methods of the class.
  const std::wstring& GetConfigFileLocation() {
    return core_instance_.GetConfigFileLocation();
  }

  void SetConfigFileLocation(const std::wstring& path) {
    core_instance_.SetConfigFileLocationForTest(path);
  }

  void SetIESiteListCacheLocation(const std::wstring& path) {
    core_instance_.SetIESiteListCacheLocationForTest(path);
  }

  std::wstring CompileCommandLine(const std::wstring& raw_command_line,
                                  const std::wstring& url) {
    return core_instance_.CompileCommandLine(raw_command_line, url);
  }

  std::wstring SanitizeUrl(const std::wstring& url) {
    return core_instance_.SanitizeUrl(url);
  }

  bool DownloadIESiteList() {
    return core_instance_.DownloadIESiteList();
  }

  // Creates a temp file for tests that need to mock the config.
  bool CreateTempFile(std::wstring* path) {
    wchar_t temp_dir[MAX_PATH+1];
    wchar_t temp_file[MAX_PATH+1];
    if (!::GetTempPath(MAX_PATH+1, temp_dir))
      return false;
    if (!::GetTempFileName(temp_dir, L"core_test", 0, temp_file))
      return false;
    *path = std::wstring(temp_file);
    return true;
  }

  // Writes a string to a file.
  bool WriteStringToFile(const std::wstring file, const std::wstring text) {
    FILE *f = NULL;
    errno_t error = _wfopen_s(&f, file.c_str(), L"wt");
    if (error)
      return false;
    int bytes_written = fwprintf(f, L"%s", text.c_str());
    fclose(f);
    return bytes_written == text.size();
  }

  // Writes a string to a file.
  bool FileExists(const std::wstring file) {
    FILE *f = NULL;
    errno_t error = _wfopen_s(&f, file.c_str(), L"r");
    if (error)
      return false;
    fclose(f);
    return true;
  }

  std::wstring temp_file_;
  BrowserSwitcherCore core_instance_;
};

TEST_F(BrowserSwitcherCoreTest, ConfigPathNonEmpty) {
  EXPECT_FALSE(GetConfigFileLocation().empty());
}

TEST_F(BrowserSwitcherCoreTest, SetConfigPath) {
  SetConfigFileLocation(L"mytestfile");
  EXPECT_EQ(std::wstring(L"mytestfile"), GetConfigFileLocation());
}

TEST_F(BrowserSwitcherCoreTest, LoadConfig) {
  WriteStringToFile(
      temp_file_,
      L"1\ntest_browser\ntest_params\nchrome_browser\nmore_params\n"
      L"1\nsome_url\n1\nother_url");
  SetConfigFileLocation(temp_file_);
  ASSERT_FALSE(core_instance_.HasValidConfiguration());
  ASSERT_TRUE(core_instance_.LoadConfigFile());
  ASSERT_TRUE(core_instance_.HasValidConfiguration());
  EXPECT_EQ(std::wstring(L"test_browser"),
            core_instance_.GetAlternativeBrowserPath());
  EXPECT_EQ(std::wstring(L"test_params"),
            core_instance_.GetAlternativeBrowserParameters());
  EXPECT_EQ(std::wstring(L"chrome_browser"), core_instance_.GetChromePath());
  EXPECT_EQ(std::wstring(L"more_params"), core_instance_.GetChromeParameters());
  ASSERT_EQ(1, core_instance_.GetUrlsToRedirect().size());
  EXPECT_EQ(std::wstring(L"some_url"), core_instance_.GetUrlsToRedirect()[0]);
  ASSERT_EQ(1, core_instance_.GetUrlGreylist().size());
  EXPECT_EQ(std::wstring(L"other_url"), core_instance_.GetUrlGreylist()[0]);
}

TEST_F(BrowserSwitcherCoreTest, LoadConfigWrongVersion) {
  WriteStringToFile(
      temp_file_,
      L"2\ncompletely_unreadable_scrambled_eggs\t34332@#$@#$@#");
  SetConfigFileLocation(temp_file_);
  ASSERT_FALSE(core_instance_.LoadConfigFile());
  ASSERT_FALSE(core_instance_.HasValidConfiguration());
}

TEST_F(BrowserSwitcherCoreTest, EmptyConfig) {
  SetConfigFileLocation(temp_file_);
  ASSERT_FALSE(core_instance_.HasValidConfiguration());
  core_instance_.SetAlternativeBrowserParameters(L"");
  core_instance_.SetAlternativeBrowserPath(L"");
  core_instance_.SetChromeParameters(L"");
  core_instance_.SetChromePath(L"");
  core_instance_.SetUrlsToRedirect(BrowserSwitcherCore::UrlList());
  core_instance_.SetUrlGreylist(BrowserSwitcherCore::UrlList());
  core_instance_.SaveConfigFile();
  ASSERT_TRUE(core_instance_.HasValidConfiguration());
  ASSERT_TRUE(core_instance_.LoadConfigFile());
  ASSERT_TRUE(core_instance_.HasValidConfiguration());
  EXPECT_FALSE(core_instance_.GetAlternativeBrowserPath().empty());
  EXPECT_TRUE(core_instance_.GetAlternativeBrowserParameters().empty());
  EXPECT_FALSE(core_instance_.GetChromePath().empty());
  EXPECT_TRUE(core_instance_.GetChromeParameters().empty());
  ASSERT_EQ(0, core_instance_.GetUrlsToRedirect().size());
  ASSERT_EQ(0, core_instance_.GetUrlGreylist().size());
  ::DeleteFile(temp_file_.c_str());
  ASSERT_FALSE(core_instance_.LoadConfigFile());
  EXPECT_FALSE(core_instance_.GetAlternativeBrowserPath().empty());
  EXPECT_TRUE(core_instance_.GetAlternativeBrowserParameters().empty());
  EXPECT_FALSE(core_instance_.GetChromePath().empty());
  EXPECT_TRUE(core_instance_.GetChromeParameters().empty());
  ASSERT_EQ(0, core_instance_.GetUrlsToRedirect().size());
  ASSERT_EQ(0, core_instance_.GetUrlGreylist().size());
}

TEST_F(BrowserSwitcherCoreTest, ChangeConfig) {
  WriteStringToFile(
      temp_file_,
      L"1\ntest_browser\ntest_params\nchrome_browser\nmore_params\n"
      L"1\nsome_url\n1\nother_url");
  SetConfigFileLocation(temp_file_);
  ASSERT_TRUE(core_instance_.LoadConfigFile());
  core_instance_.SetAlternativeBrowserPath(L"another_browser");
  EXPECT_EQ(std::wstring(L"another_browser"),
            core_instance_.GetAlternativeBrowserPath());
  EXPECT_EQ(std::wstring(L"chrome_browser"), core_instance_.GetChromePath());
  ASSERT_EQ(1, core_instance_.GetUrlsToRedirect().size());
  EXPECT_EQ(std::wstring(L"some_url"), core_instance_.GetUrlsToRedirect()[0]);
  ASSERT_TRUE(core_instance_.SaveConfigFile());
  ASSERT_TRUE(core_instance_.LoadConfigFile());
  EXPECT_EQ(std::wstring(L"another_browser"),
            core_instance_.GetAlternativeBrowserPath());

  std::vector<std::wstring> test_urls;
  test_urls.push_back(L"url1");
  test_urls.push_back(L"url2");
  core_instance_.SetUrlsToRedirect(test_urls);
  ASSERT_EQ(2, core_instance_.GetUrlsToRedirect().size());
  ASSERT_TRUE(core_instance_.SaveConfigFile());
  ASSERT_TRUE(core_instance_.LoadConfigFile());
  ASSERT_EQ(2, core_instance_.GetUrlsToRedirect().size());

  // Last change both and then reload.
  core_instance_.SetAlternativeBrowserPath(L"yet_another_one");
  EXPECT_EQ(std::wstring(L"yet_another_one"),
            core_instance_.GetAlternativeBrowserPath());
  test_urls.clear();
  test_urls.push_back(L"url3");
  core_instance_.SetUrlsToRedirect(test_urls);
  ASSERT_EQ(1, core_instance_.GetUrlsToRedirect().size());
  ASSERT_TRUE(core_instance_.SaveConfigFile());
  ASSERT_TRUE(core_instance_.LoadConfigFile());
  EXPECT_EQ(std::wstring(L"yet_another_one"),
            core_instance_.GetAlternativeBrowserPath());
  ASSERT_EQ(1, core_instance_.GetUrlsToRedirect().size());
}

TEST_F(BrowserSwitcherCoreTest, PathVariablesSubstitution) {
  // For IE and Chorme we can check if the executable is there for the rest
  // only if the substitution took place as we might not have them installed.
  SetLogLevel(OFF);
  core_instance_.SetAlternativeBrowserPath(L"${ie}");
  EXPECT_TRUE(FileExists(core_instance_.GetAlternativeBrowserPath()));
  core_instance_.SetChromePath(L"${chrome}");
  EXPECT_TRUE(FileExists(core_instance_.GetChromePath()));
  core_instance_.SetAlternativeBrowserPath(L"${firefox}");
  EXPECT_EQ(core_instance_.GetAlternativeBrowserPath().npos,
            core_instance_.GetAlternativeBrowserPath().find(L"${firefox}"));
  core_instance_.SetAlternativeBrowserPath(L"${opera}");
  EXPECT_EQ(core_instance_.GetAlternativeBrowserPath().npos,
            core_instance_.GetAlternativeBrowserPath().find(L"${opera}"));
  core_instance_.SetAlternativeBrowserPath(L"${safari}");
  EXPECT_EQ(core_instance_.GetAlternativeBrowserPath().npos,
            core_instance_.GetAlternativeBrowserPath().find(L"${safari}"));
  ::SetEnvironmentVariable(L"LBS_TEST", L"SUCCESS");
  core_instance_.SetAlternativeBrowserPath(L"%LBS_TEST%");
  EXPECT_EQ(core_instance_.GetAlternativeBrowserPath(), L"SUCCESS");
  SetLogLevel(ERR);
}

TEST_F(BrowserSwitcherCoreTest, ParameterCompiler) {
  std::wstring raw_command_line(L"");
  // Sanitzation should also normalize the case.
  std::wstring url(L"http://exAmple.cOm");
  // empty command line.
  std::wstring result = CompileCommandLine(raw_command_line, url);
  EXPECT_EQ(L"http://example.com/", result);
  // only the url var.
  raw_command_line = L"${url}";
  result = CompileCommandLine(raw_command_line, url);
  EXPECT_EQ(L"http://example.com/", result);
  // some stuff and a var.
  raw_command_line = L"--url=${url} --kiosk";
  result = CompileCommandLine(raw_command_line, url);
  EXPECT_EQ(L"--url=http://example.com/ --kiosk", result);
  // some stuff and no var.
  raw_command_line = L"--kiosk";
  result = CompileCommandLine(raw_command_line, url);
  EXPECT_EQ(L"--kiosk http://example.com/", result);

  // Verify that no bad chars pass the sanitization.
  url = L"http://a.com/inject Param \"u think\"<script>f();</s>";
  raw_command_line.clear();
  result = CompileCommandLine(raw_command_line, url);
  EXPECT_EQ(
      L"http://a.com/inject%20Param%20%22u%20think%22%3Cscript%3Ef();%3C/s%3E",
      result);

  // Verify that env. variable substitution works for parameter strings too but
  // not inside the url passes by the browser.
  ::SetEnvironmentVariable(L"LBS_TEST", L"SUCCESS");
  core_instance_.SetAlternativeBrowserParameters(L"--param=%LBS_TEST%");
  url = L"http://%LBS_TEST%.com/";
  raw_command_line = core_instance_.GetAlternativeBrowserParameters();
  result = CompileCommandLine(raw_command_line, url);
  EXPECT_EQ(L"--param=SUCCESS http://%lbs_test%.com/", result);
}

TEST_F(BrowserSwitcherCoreTest, SanitizeUrl) {
  // Verify that no bad chars pass the poor man's sanitization.
  std::wstring url = L"http://a.com/inject Param \"u think\"<script>f();</s>";
  std::wstring raw_command_line;

  std::wstring result = SanitizeUrl(url);
  EXPECT_EQ(
      L"http://a.com/inject%20Param%20%22u%20think%22%3Cscript%3Ef();%3C/s%3E",
      result);
}

TEST_F(BrowserSwitcherCoreTest, ShouldInvokeAlternativeBrowser) {
  WriteStringToFile(
      temp_file_,
      L"1\ntest_browser\n\nchrome_browser\n\n3\nsome_url.com\n"
      L"https://test.com/some\n!www.another.co.uk\n"
      L"2\nhttp://www.another.co.uk/somedir\ngreyzone.com");
  SetConfigFileLocation(temp_file_);
  ASSERT_TRUE(core_instance_.LoadConfigFile());
  ASSERT_EQ(3, core_instance_.GetUrlsToRedirect().size());
  ASSERT_EQ(2, core_instance_.GetUrlGreylist().size());

  // Test urls that are not in the list but where the url might appear as
  // some part of the string.
  EXPECT_FALSE(core_instance_.ShouldOpenInAlternativeBrowser(L"http://a.ly/"));
  EXPECT_FALSE(core_instance_.ShouldOpenInAlternativeBrowser(
      L"http://www.some_url.org/"));
  EXPECT_FALSE(core_instance_.ShouldOpenInAlternativeBrowser(
      L"http://www.google.com/search?query=www.some_url.com"));
  EXPECT_FALSE(core_instance_.ShouldOpenInAlternativeBrowser(
      L"http://www.bla.com/search?query=https://test.com/some"));
  EXPECT_FALSE(core_instance_.ShouldOpenInAlternativeBrowser(
      L"http://www.google.com/www.some_url.com/trickme"));
  // Test some that are in the list.
  EXPECT_TRUE(core_instance_.ShouldOpenInAlternativeBrowser(
      L"http://some_url.com/"));
  EXPECT_TRUE(core_instance_.ShouldOpenInAlternativeBrowser(
      L"http://www.some_url.com/"));
  EXPECT_TRUE(core_instance_.ShouldOpenInAlternativeBrowser(
      L"http://www.some_url.com/somedir/somepage.php?params=100&more=xyz"));
  EXPECT_TRUE(core_instance_.ShouldOpenInAlternativeBrowser(
      L"http://www.another.co.uk/somedir/somepage.php?params=100&more=xyz"));
  EXPECT_TRUE(core_instance_.ShouldOpenInAlternativeBrowser(
      L"https://test.com/some/somedir/somepage.php?params=100&more=xyz"));
  // Check more precise greylist match is preferred always.
  EXPECT_FALSE(core_instance_.ShouldOpenInAlternativeBrowser(
    L"http://www.another.co.uk/someotherdir/"));
  EXPECT_TRUE(core_instance_.ShouldOpenInAlternativeBrowser(
    L"http://inthe.greyzone.com/"));

  // Check more precise entries override less precise entries.
  WriteStringToFile(
      temp_file_,
      L"1\ntest_browser\n\nchrome_browser\n\n4\n*\nbar.foo.com\n!foo.com\n"
      L"!http://bazinga.com/accounts\n3\nbaz.com\n!bar.com\n*\n");
  ASSERT_TRUE(core_instance_.LoadConfigFile());
  ASSERT_EQ(4, core_instance_.GetUrlsToRedirect().size());
  ASSERT_EQ(3, core_instance_.GetUrlGreylist().size());

  EXPECT_TRUE(core_instance_.ShouldOpenInAlternativeBrowser(L"http://a.ly/"));
  EXPECT_TRUE(core_instance_.ShouldOpenInAlternativeBrowser(
      L"http://bar.com/"));
  EXPECT_TRUE(core_instance_.ShouldOpenInAlternativeBrowser(
      L"http://www.another.co.uk/somedir/somepage.php?params=foo.com&more"));
  EXPECT_FALSE(core_instance_.ShouldOpenInAlternativeBrowser(
      L"https://bar.foo.com/bla"));
  EXPECT_FALSE(core_instance_.ShouldOpenInAlternativeBrowser(
      L"http://bazinga.com/accounts/signin?user=me&domain=mazinga.com"));

  // Test negative entries and wildcards in url list.
  WriteStringToFile(
      temp_file_,
      L"1\ntest_browser\n\nchrome_browser\n\n2\n*\n!google\n\n0\n");
  ASSERT_TRUE(core_instance_.LoadConfigFile());
  ASSERT_EQ(2, core_instance_.GetUrlsToRedirect().size());
  ASSERT_EQ(0, core_instance_.GetUrlGreylist().size());

  EXPECT_TRUE(core_instance_.ShouldOpenInAlternativeBrowser(L"http://a.ly/"));
  EXPECT_TRUE(core_instance_.ShouldOpenInAlternativeBrowser(
      L"http://gmail.com/"));
  EXPECT_FALSE(core_instance_.ShouldOpenInAlternativeBrowser(
      L"http://account.google.com"));
  EXPECT_FALSE(core_instance_.ShouldOpenInAlternativeBrowser(
      L"https://googlemail.com"));

  // Test wildcards in the greylist.
  WriteStringToFile(
      temp_file_,
      L"1\ntest_browser\n\nchrome_browser\n\n2\nyahoo\n!google\n1\n*\n");
  ASSERT_TRUE(core_instance_.LoadConfigFile());
  ASSERT_EQ(2, core_instance_.GetUrlsToRedirect().size());
  ASSERT_EQ(1, core_instance_.GetUrlGreylist().size());

  EXPECT_TRUE(core_instance_.ShouldOpenInAlternativeBrowser(L"http://a.ly/"));
  EXPECT_TRUE(core_instance_.ShouldOpenInAlternativeBrowser(
      L"http://yahoo.com/"));
  EXPECT_FALSE(core_instance_.ShouldOpenInAlternativeBrowser(
      L"http://account.google.com"));
}

TEST_F(BrowserSwitcherCoreTest, WithIESiteList) {
  SetConfigFileLocation(temp_file_);
  WriteStringToFile(temp_file_,
      L"1\ntest_browser\n\nchrome_browser\n\n0\n0\n");
  ASSERT_TRUE(core_instance_.LoadConfigFile());
  SetIESiteListCacheLocation(temp_file_);
  WriteStringToFile(temp_file_, L"1\n2\n!google.com\nyahoo.com");
  EXPECT_TRUE(core_instance_.LoadIESiteListCache());
  std::vector<std::wstring> list;
  ASSERT_TRUE(core_instance_.GetIESiteList(&list));
  EXPECT_EQ(2, list.size());
  EXPECT_TRUE(core_instance_.ShouldOpenInAlternativeBrowser(
    L"http://yahoo.com/"));
  EXPECT_FALSE(core_instance_.ShouldOpenInAlternativeBrowser(
    L"http://account.google.com"));
}

TEST_F(BrowserSwitcherCoreTest, InternaListAndIESiteList) {
  SetConfigFileLocation(temp_file_);
  WriteStringToFile(temp_file_,
    L"1\ntest_browser\n\nchrome_browser\n\n2\n!yahoo\ngoogle\n0\n");
  ASSERT_TRUE(core_instance_.LoadConfigFile());
  SetIESiteListCacheLocation(temp_file_);
  WriteStringToFile(temp_file_, L"1\n2\n!google.com\nyahoo.com");
  EXPECT_TRUE(core_instance_.LoadIESiteListCache());
  std::vector<std::wstring> list;
  ASSERT_TRUE(core_instance_.GetIESiteList(&list));
  EXPECT_EQ(2, list.size());
  // Internal list always wins!
  EXPECT_FALSE(core_instance_.ShouldOpenInAlternativeBrowser(
    L"http://yahoo.com/"));
  EXPECT_TRUE(core_instance_.ShouldOpenInAlternativeBrowser(
    L"http://account.google.com"));
}

TEST_F(BrowserSwitcherCoreTest, GreyListAndIESiteList) {
  SetConfigFileLocation(temp_file_);
  WriteStringToFile(temp_file_,
    L"1\ntest_browser\n\nchrome_browser\n\n0\n2\nwww.yahoo.com\ngoogle\n");
  ASSERT_TRUE(core_instance_.LoadConfigFile());
  SetIESiteListCacheLocation(temp_file_);
  WriteStringToFile(temp_file_, L"1\n2\ngoogle.com\n!yahoo.com");
  EXPECT_TRUE(core_instance_.LoadIESiteListCache());
  std::vector<std::wstring> list;
  ASSERT_TRUE(core_instance_.GetIESiteList(&list));
  EXPECT_EQ(2, list.size());
  // Regular rule ofr more precise wins is used.
  EXPECT_TRUE(core_instance_.ShouldOpenInAlternativeBrowser(
    L"http://www.yahoo.com/"));
  EXPECT_TRUE(core_instance_.ShouldOpenInAlternativeBrowser(
    L"http://account.google.com"));
}
TEST_F(BrowserSwitcherCoreTest, InvokeAlternativeBrowser) {
  // For IE we can check if the executable is there for the rest only if the
  // substitution took place as we might not have them installed.
  core_instance_.SetAlternativeBrowserPath(L"notepad.exe");
  HWND notepad_window = ::FindWindow(L"Notepad", NULL);
  // If this assert fails that means that there is notepad instance running and
  // and thus this test will be inconclusive. Stop all running notepad instances
  // before retrying the test.
  ASSERT_FALSE(notepad_window);
  ASSERT_TRUE(core_instance_.InvokeAlternativeBrowser(L""));
  ::Sleep(500);
  notepad_window = ::FindWindow(L"Notepad", NULL);
  ASSERT_NE(static_cast<HWND>(NULL), notepad_window);
  ::SendMessage(notepad_window, WM_CLOSE, 0, 0);
  ::Sleep(500);
  notepad_window = ::FindWindow(L"Notepad", NULL);
  ASSERT_FALSE(notepad_window);
}

TEST_F(BrowserSwitcherCoreTest, InvokeChrome) {
  // For IE we can check if the executable is there for the rest only if the
  // substitution took place as we might not have them installed.
  core_instance_.SetChromePath(L"notepad.exe");
  HWND notepad_window = ::FindWindow(L"Notepad", NULL);
  // If this assert fails that means that there is notepad instance running and
  // and thus this test will be inconclusive. Stop all running notepad instances
  // before retrying the test.
  ASSERT_FALSE(notepad_window);
  ASSERT_TRUE(core_instance_.InvokeChrome(L""));
  ::Sleep(500);
  notepad_window = ::FindWindow(L"Notepad", NULL);
  ASSERT_NE(static_cast<HWND>(NULL), notepad_window);
  ::SendMessage(notepad_window, WM_CLOSE, 0, 0);
  ::Sleep(500);
  notepad_window = ::FindWindow(L"Notepad", NULL);
  ASSERT_FALSE(notepad_window);
}

class IEEMSiteListParserTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    ASSERT_TRUE(CreateTempFile(&temp_file_));
  }

  virtual void TearDown() {
    ::DeleteFile(temp_file_.c_str());
  }

  // Delegates calls to a the methods of the class.
  bool ParseIEFileVersionOne() {
    return parser_.ParseIEFileVersionOne();
  }

  bool ParseIEFileVersionTwo() {
    return parser_.ParseIEFileVersionTwo();
  }

  void SetXmlReader(std::auto_ptr<XmlReader> reader) {
    parser_.reader_.reset(reader.release());
  }

  void SetXmlString(const std::string& xml) {
    xml_ = xml;
    std::auto_ptr<XmlReader> reader(new XmlReader);
    // The XMLReader class does not fail on bad XML so this will work ever.
    reader->Load(xml_);
    reader->SkipToElement();
    SetXmlReader(reader);
  }

  // Creates a temp file for tests that need to mock the config.
  bool CreateTempFile(std::wstring* path) {
    wchar_t temp_dir[MAX_PATH + 1];
    wchar_t temp_file[MAX_PATH + 1];
    if (!::GetTempPath(MAX_PATH + 1, temp_dir))
      return false;
    if (!::GetTempFileName(temp_dir, L"core_test", 0, temp_file))
      return false;
    *path = std::wstring(temp_file);
    return true;
  }

  std::wstring temp_file_;
  IEEMSiteListParser parser_;
  std::string xml_;
};

TEST_F(IEEMSiteListParserTest, XMLParseBadXml) {
  SetXmlString("thisisnotxml");
  EXPECT_FALSE(ParseIEFileVersionOne());
}

TEST_F(IEEMSiteListParserTest, XMLParseBadXmlParsed) {
  // Very subtle issue in the closing element for rules.
  SetXmlString("<rules version=\"424\"><unknown></unknown><rules>");
  EXPECT_TRUE(ParseIEFileVersionOne());
  EXPECT_EQ(0, parser_.GetList().size());
}

TEST_F(IEEMSiteListParserTest, XMLParseV1OnlyBogusElements) {
  SetXmlString("<rules version=\"424\">"
      "<unknown><more><docMode><domain>ignore.com</domain></docMode>"
      "</more><emie><domain>ignoretoo.com<path>/ignored_path</path>"
      "</domain></emie><domain>onemoreingored.com</domain>"
      "<path>/ignore_outside_of_domain></path></unknown></rules>");
  EXPECT_TRUE(ParseIEFileVersionOne());
  EXPECT_EQ(0, parser_.GetList().size());
}

TEST_F(IEEMSiteListParserTest, XMLParseV1Full) {
  SetXmlString("<rules version=\"424\"><unknown><more><docMode><domain>ignore"
      "</domain></docMode></more><emie><domain>ignoretoo.com<path>/ignored_path"
      "</path></domain></emie><domain>onemoreingored.com</domain><path>"
      "/ignore_outside_of_domain></path></unknown><emie><other><more><docMode>"
      "<domain>ignore.com</domain></docMode></more><emie><domain>ignoretoo.com"
      "<path>/ignored_path</path></domain></emie><domain>onemoreingored.com"
      "</domain><path>/ignore_outside_of_domain></path></other><!--<domain "
      "exclude=\"false\">hotscanacc.dbch.b-source.net<path exclude=\"false\">"
      "/HotScan/</path></domain>--><domain>inside.com<more><docMode><domain>"
      "ignore.com</domain></docMode></more><emie><domain>ignoretoo.com<path>"
      "/ignored_path</path></domain></emie><domain>onemoreingored.com</domain>"
      "<path>/in_domain<more><docMode><domain>ignore.com</domain></docMode>"
      "</more><emie><domain>ignoretoo.com<path>/ignored_path</path></domain>"
      "</emie><domain>onemoreingored.com</domain><path>/ignore_nested_path>"
      "</path></path></domain><domain>google.com</domain><domain "
      "exclude=\"true\">good.com</domain><domain exclude=\"false\">more.com"
      "</domain><domain>e100.com<path>/path1</path><path exclude=\"true\">/pa2"
      "</path><path exclude=\"false\">/path3</path></domain><domain "
      "exclude=\"true\">e200.com<path>/path1</path><path exclude=\"true\">/pth2"
      "</path><path exclude=\"false\">/path3</path></domain><domain "
      "exclude=\"false\">e300.com<path>/path1</path><path exclude=\"true\">/pt2"
      "</path><path exclude=\"false\">/path3</path></domain><domain "
      "exclude=\"true\">random.com<path exclude=\"true\">/path1/</path><path "
      "exclude=\"false\" forceCompatView=\"true\">/path2<path exclude=\"true\">"
      "/TEST</path></path></domain></emie><docMode><domain docMode=\"8\">"
      "moredomains.com</domain><domain docMode=\"5\">evenmore.com<path "
      "docMode=\"5\">/r1</path><path docMode=\"5\">/r2</path></domain><domain "
      "docMode=\"5\" exclude=\"true\">domainz.com<path docMode=\"5\">/r2</path>"
      "<path docMode=\"5\" exclude=\"true\">/r5</path><path docMode=\"5\" "
      "exclude=\"false\">/r6</path></domain><domain docMode=\"5\" "
      "exclude=\"false\">howmanydomainz.com<path docMode=\"5\">/r8</path><path "
      "docMode=\"5\" exclude=\"true\">/r9</path><path docMode=\"5\" "
      "exclude=\"false\">/r10</path></domain></docMode></rules>");
  EXPECT_TRUE(ParseIEFileVersionOne());
  auto list = parser_.GetList();
  EXPECT_EQ(32, list.size());
  for (auto element : list) {
    EXPECT_TRUE(element.find(L"ignore") == element.npos);
  }
}

TEST_F(IEEMSiteListParserTest, XMLParseV2Full) {
  // Very subtle issue in the closing element for rules.
  SetXmlString("<site-list version=\"205\"><!-- File creation header -->"
      "<created-by><tool>EnterpriseSitelistManager</tool><version>10240"
      "</version><date-created>20150728.135021</date-created></created-by>"
      "<!-- unknown tags --><unknown><test><mest>test</mest></test>"
      "<!-- comments --></unknown><!-- no url attrib --><site><open-in>none"
      "</open-in></site><!-- nested site list --><site-list><site "
      "url=\"ignore!\"/></site-list><!-- nested site --><site "
      "url=\"google.com\"><site url=\"nested ignore!\"></site></site><!-- "
      "unknown tags in a site on multiple levels --><site url=\"good.site\">"
      "<!-- nested comments --><somethings>klj<other some=\"none\"/>jkh"
      "</somethings></site><!-- good sites --> <site url=\"www.cpandl.com\">"
      "<compat-mode>IE8Enterprise</compat-mode><open-in>MSEdge</open-in></site>"
      "<site url=\"contoso.com\"><compat-mode>default</compat-mode><open-in>"
      "none</open-in></site><site url=\"relecloud.com\"/><site "
      "url=\"relecloud.com/about\"><compat-mode>IE8Enterprise</compat-mode>"
      "</site></site-list><!-- trailing gibberish <trailing><site "
      "url=\"ignore after site list!\">  <compat-mode>IE8Enterprise\""
      "</compat-mode></site><gibberish>Lorem ipsum sit...</gibberish>"
      "</trailing>-->");
  EXPECT_TRUE(ParseIEFileVersionTwo());
  auto list = parser_.GetList();
  EXPECT_EQ(6, list.size());
  for (auto element : list) {
    EXPECT_TRUE(element.find(L"ignore") == element.npos);
  }
}

int main(int argc, wchar_t* argv[]) {
  SetLogLevel(ERR);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
