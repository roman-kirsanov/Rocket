/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <Rocket/Window/App.hpp>

namespace Rocket {
namespace App {

void _Update();

void _Close();

std::string const& __GetExecPath();

std::string const& __GetExecDir();

std::string const& __GetWorkDir();

std::string const& __GetUserDir();

void __Exit();

void __Run(int argc, char const** argv);

void __Tick(bool update);

} /* namespace App */
} /* namespace Rocket */