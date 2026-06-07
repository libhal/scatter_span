# Copyright 2026 Madeline Schneider and the libhal contributors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 4.0)

set(CMAKE_COLOR_DIAGNOSTICS ON)

project(scatter-span LANGUAGES CXX)

find_package(LibhalCMakeUtil REQUIRED)

libhal_project_init()
libhal_add_library(scatter-span
    MODULES
        modules/scatter_span.cppm
        modules/core.cppm
)

libhal_apply_compile_options(scatter-span)
libhal_install_library(scatter-span NAMESPACE scatter-span)
libhal_add_tests(scatter-span
    TEST_NAMES
        scatter_span
)
