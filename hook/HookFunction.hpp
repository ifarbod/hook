// Static hook installer
// Author(s):       iFarbod <ifarbod@outlook.com>
//
// Copyright (c) 2013-2017 CTNorth Team
//
// Distributed under the MIT license (See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT)

#pragma once

namespace hook
{

// Initialization function that will be called after the game is loaded.
class HookFunctionBase
{
public:
    HookFunctionBase() { Register(); }

    virtual void Run() = 0;

    static void RunAll();
    void Register();

private:
    HookFunctionBase* m_next;
};

class HookFunction : public HookFunctionBase
{
public:
    HookFunction(void (*function)()) { m_function = function; }

    virtual void Run() { m_function(); }

private:
    void (*m_function)();
};

}  // namespace hook
