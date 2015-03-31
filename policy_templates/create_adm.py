# Copyright 2014 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


"""Generates localized ADM and ADMX from the translated resources.

Usage: run the script in each language directory.
"""
import codecs
import json


def doswrite(f, text):
  t2 = text.replace("\n", "\r\n")
  f.write(t2)

output_json = json.load(open("policy_templates.json"))

adm_file = codecs.open("LegacyBrowserSupport.adm", encoding="utf-16", mode="w")
adml_file = open("LegacyBrowserSupport.adml", "w")

doswrite(adml_file, """<?xml version="1.0" encoding="utf-8"?>
<policyDefinitionResources revision="1.0" schemaVersion="1.0">
  <displayName>
  </displayName>
  <description>
  </description>
  <resources>
    <stringTable>
""")

doswrite(adm_file, """CLASS MACHINE
  CATEGORY !!Cat_Google
    CATEGORY !!Cat_LegacyBrowserSupport
      KEYNAME "Software\\Policies\\Google\\Chrome\\3rdparty\\Extensions\\heildphpnddilhkemkielfhnkaagiabh\\policy"

      POLICY !!AlternativeBrowserPath
        #if version >= 4
            SUPPORTED !!SUPPORTED_WINXPSP2
        #endif
        EXPLAIN !!AlternativeBrowserPath_Explain
        PART !!AlternativeBrowserPath_Part  EDITTEXT
          VALUENAME "alternative_browser_path"
        END PART
      END POLICY

      POLICY !!AlternativeBrowserArguments
        #if version >= 4
            SUPPORTED !!SUPPORTED_WINXPSP2
        #endif
        EXPLAIN !!AlternativeBrowserArguments_Explain
        PART !!AlternativeBrowserArguments_Part  EDITTEXT
          VALUENAME "alternative_browser_arguments"
        END PART
      END POLICY

      POLICY !!ChromePath
        #if version >= 4
            SUPPORTED !!SUPPORTED_WINXPSP2
        #endif
        EXPLAIN !!ChromePath_Explain
        PART !!ChromePath_Part  EDITTEXT
          VALUENAME "chrome_path"
        END PART
      END POLICY

      POLICY !!ChromeArguments
        #if version >= 4
            SUPPORTED !!SUPPORTED_WINXPSP2
        #endif
        EXPLAIN !!ChromeArguments_Explain
        PART !!ChromeArguments_Part  EDITTEXT
          VALUENAME "chrome_arguments"
        END PART
      END POLICY

      POLICY !!URLList_Policy
        #if version >= 4
            SUPPORTED !!SUPPORTED_WINXPSP2
        #endif
        EXPLAIN !!URLList_Explain
        PART !!URLList_Part  LISTBOX
          KEYNAME "Software\\Policies\\Google\\Chrome\\3rdparty\\Extensions\\heildphpnddilhkemkielfhnkaagiabh\\policy\\url_list"
          VALUEPREFIX ""
        END PART
      END POLICY

      POLICY !!URLGreyList_Policy
        #if version >= 4
            SUPPORTED !!SUPPORTED_WINXPSP2
        #endif
        EXPLAIN !!URLGreyList_Explain
        PART !!URLGreyList_Part  LISTBOX
          KEYNAME "Software\\Policies\\Google\\Chrome\\3rdparty\\Extensions\\heildphpnddilhkemkielfhnkaagiabh\\policy\\url_greylist"
          VALUEPREFIX ""
        END PART
      END POLICY

      POLICY !!KeepLastChromeTab_Policy
        #if version >= 4
            SUPPORTED !!SUPPORTED_WINXPSP2
        #endif
        EXPLAIN !!KeepLastChromeTab_Explain
        VALUENAME "keep_last_chrome_tab"
        VALUEON NUMERIC 1
        VALUEOFF NUMERIC 0
      END POLICY
    END CATEGORY
  END CATEGORY

[Strings]

""")

for majorkey, subdict in output_json.iteritems():
  if majorkey == "PolicySchema":
    continue

  adm_file.write(majorkey + "=\"" +
                 subdict["message"].replace("$$", "$").replace("\\\\", "\\")
                 .replace("\n", "\\n") + "\"\r\n")

  doswrite(adml_file,
           "      <string id=\"" + majorkey.encode("utf-8") + "\">" +
           subdict["message"].replace("$$", "$").replace("\\\\", "\\")
           .encode("utf-8") + "</string>\n")

doswrite(adml_file, """</stringTable>
  <presentationTable>
    <presentation id="AlternativeBrowserPath">
      <textBox refId="AlternativeBrowserPath_Part">
        <label>""")
doswrite(adml_file,
         output_json["AlternativeBrowserPath"]["message"].replace("$$", "$")
         .replace("\\\\", "\\").encode("utf-8"))
doswrite(adml_file, """</label>
        <defaultValue></defaultValue>
      </textBox>
    </presentation>
    <presentation id="AlternativeBrowserArguments">
      <textBox refId="AlternativeBrowserArguments_Part">
        <label>""")
doswrite(adml_file, output_json["AlternativeBrowserArguments"]["message"]
         .replace("$$", "$").replace("\\\\", "\\").encode("utf-8"))
doswrite(adml_file, """</label>
        <defaultValue></defaultValue>
      </textBox>
    </presentation>
    <presentation id="ChromePath">
      <textBox refId="ChromePath_Part">
        <label>""")
doswrite(adml_file, output_json["ChromePath"]["message"].replace("$$", "$")
         .replace("\\\\", "\\").encode("utf-8"))
doswrite(adml_file, """</label>
        <defaultValue></defaultValue>
      </textBox>
    </presentation>
    <presentation id="ChromeArguments">
      <textBox refId="ChromeArguments_Part">
        <label>""")
doswrite(adml_file, output_json["ChromeArguments"]["message"]
         .replace("$$", "$").replace("\\\\", "\\").encode("utf-8"))
doswrite(adml_file, """</label>
        <defaultValue></defaultValue>
      </textBox>
    </presentation>
    <presentation id="URLList_Policy">
      <listBox refId="URLList_Part">""")
doswrite(adml_file, output_json["URLList_Policy"]["message"]
         .replace("$$", "$").replace("\\\\", "\\").encode("utf-8"))
doswrite(adml_file, """</listBox>
    </presentation>
    <presentation id="URLGreyList_Policy">
      <listBox refId="URLGreyList_Part">""")
doswrite(adml_file, output_json["URLGreyList_Policy"]["message"]
         .replace("$$", "$").replace("\\\\", "\\").encode("utf-8"))
doswrite(adml_file, """</listBox>
    </presentation>
    <presentation id="KeepLastChromeTab_Policy"/>
  </presentationTable>
</resources>
</policyDefinitionResources>
""")

adml_file.close()
adm_file.close()
