// Static hook installer
// Author(s):       iFarbod <ifarbod@outlook.com>
//
// Copyright (c) 2013-2017 CTNorth Team
//
// Distributed under the MIT license (See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT)

#include "client/hook/HookFunction.hpp"

namespace hook
{

static HookFunctionBase* hookFunctions;

void HookFunctionBase::Register()
{
    m_next = hookFunctions;
    hookFunctions = this;
}

void HookFunctionBase::RunAll()
{
    for (auto func = hookFunctions; func; func = func->m_next)
    {
        func->Run();
    }
}

}  // namespace hook
