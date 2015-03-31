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
  rm *.adm *.adml
  python ../../create_adm.py
  cp LegacyBrowserSupport.adml ../../admx/$dir/
  cp LegacyBrowserSupport.adm ../../adm/$dir/
  echo "Done $dir"
  cd -
done
cd ..
echo "All done!"
