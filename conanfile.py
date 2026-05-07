# Copyright 2026 Madeline Schneider and the libhal contributors
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

from conan import ConanFile


required_conan_version = ">=2.0.14"


class scatter_span_conan(ConanFile):
    name = "scatter-span"
    license = "Apache-2.0"
    homepage = "https://github.com/libhal/scatter-span"
    description = ("A data structure library for housing arrays of unowned data.")
    topics = ("span", "scatter", "data-structures", "non-owning", "view")
    settings = "compiler", "build_type", "os", "arch"

    python_requires = "libhal-bootstrap/[>=4.3.0 <5]"
    python_requires_extend = "libhal-bootstrap.library"

    def set_version(self):
        if not self.version:
            self.version = "latest"

    def requirements(self):
        pass

    def package_info(self):
        self.cpp_info.bindirs = []
        self.cpp_info.libdirs = []
        self.cpp_info.set_property("cmake_target_name", "scatter-span::scatter-span")
