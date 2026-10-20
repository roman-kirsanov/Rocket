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

/**
 * __Run ticks with `update` set; the loops that block inside a tick of their
 * own (a modal window, a file dialog) tick without it, so App timeouts,
 * intervals and UpdateAppEvent do not run while they block, as on macOS.
 */
void __Tick(bool update);

} /* namespace App */
} /* namespace Rocket */