#!/bin/sh
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


cd langs
for dir in *; do
  cd $dir
  python ../../create_adm.py
  mkdir -p ../../output/admx/$dir
  cp LegacyBrowserSupportFirefox.adml ../../output/admx/$dir/
  cp ../../admx/mozilla.adml ../../output/admx/$dir/
  unix2dos ../../output/admx/$dir/mozilla.adml
  mkdir -p ../../output/adm/$dir
  cp LegacyBrowserSupportFirefox.adm ../../output/adm/$dir/
  echo "Done $dir"
  cd -
done
cd ..
rm -fr ./langs
# Zip up the output.
cd output
cp ../admx/*.admx ./admx/
unix2dos ./admx/*.admx
zip -Dr policy_templates.zip *
cd ..
echo "All done!"
