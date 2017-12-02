// Code pattern matching utilities
// Author(s):       iFarbod <ifarbod@outlook.com>
//
// Copyright (c) 2013-2017 CTNorth Team
//
// Distributed under the MIT license (See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT)

#include "client/hook/Pattern.hpp"

#include <windows.h>

#include <algorithm>
#include <string_view>
#include "base/Macros.hpp"

namespace hook
{

ptrdiff_t baseAddressDifference;

// sets the base to the process main base
void SetBase()
{
    SetBase(reinterpret_cast<uintptr_t>(GetModuleHandle(nullptr)));
}

static void TransformPattern(std::string_view pattern, std::string& data, std::string& mask)
{
    uint8_t tempDigit = 0;
    bool tempFlag = false;

    auto tol = [](char ch) -> uint8_t
    {
        if (ch >= 'A' && ch <= 'F')
            return uint8_t(ch - 'A' + 10);
        if (ch >= 'a' && ch <= 'f')
            return uint8_t(ch - 'a' + 10);
        return uint8_t(ch - '0');
    };

    for (auto ch : pattern)
    {
        if (ch == ' ')
        {
            continue;
        }
        else if (ch == '?')
        {
            data.push_back(0);
            mask.push_back('?');
        }
        else if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f'))
        {
            uint8_t thisDigit = tol(ch);

            if (!tempFlag)
            {
                tempDigit = thisDigit << 4;
                tempFlag = true;
            }
            else
            {
                tempDigit |= thisDigit;
                tempFlag = false;

                data.push_back(tempDigit);
                mask.push_back('x');
            }
        }
    }
}

class ExecutableMeta
{
public:
    template <typename TReturn, typename TOffset>
    TReturn* GetRVA(TOffset rva)
    {
        return (TReturn*)(m_begin + rva);
    }

    explicit ExecutableMeta(void* module) :
        m_begin((uintptr_t)module)
    {
        PIMAGE_DOS_HEADER dosHeader = GetRVA<IMAGE_DOS_HEADER>(0);
        PIMAGE_NT_HEADERS ntHeader = GetRVA<IMAGE_NT_HEADERS>(dosHeader->e_lfanew);

        m_end = m_begin + ntHeader->OptionalHeader.SizeOfCode;
    }

    ExecutableMeta(uintptr_t begin, uintptr_t end) :
        m_begin(begin),
        m_end(end)
    {}

    inline uintptr_t begin() const
    {
        return m_begin;
    }

    inline uintptr_t end() const
    {
        return m_end;
    }

private:
    uintptr_t m_begin;
    uintptr_t m_end;
};

void Pattern::Initialize(const char* pattern, size_t length)
{
    // Transform the base pattern from IDA format to canonical format
    TransformPattern(std::string_view(pattern, length), m_bytes, m_mask);
}

void Pattern::EnsureMatches(uint32_t maxCount)
{
    if (m_matched)
    {
        return;
    }

    // Scan the executable for code
    ExecutableMeta executable =
        m_rangeStart != 0 && m_rangeEnd != 0 ? ExecutableMeta(m_rangeStart, m_rangeEnd) : ExecutableMeta(m_module);

    auto matchSuccess = [&](uintptr_t address)
    {
        ignore_result(address);

        return (m_matches.size() == maxCount);
    };

    const uint8_t* pattern = reinterpret_cast<const uint8_t*>(m_bytes.c_str());
    const char* mask = m_mask.c_str();
    size_t maskSize = m_mask.size();
    size_t lastWild = m_mask.find_last_of('?');

    ptrdiff_t Last[256];

    std::fill(std::begin(Last), std::end(Last), lastWild == std::string::npos ? -1 : static_cast<ptrdiff_t>(lastWild));

    for (ptrdiff_t i = 0; i < static_cast<ptrdiff_t>(maskSize); ++i)
    {
        if (Last[pattern[i]] < i)
        {
            Last[pattern[i]] = i;
        }
    }

    for (uintptr_t i = executable.begin(), end = executable.end() - maskSize; i <= end;)
    {
        uint8_t* ptr = reinterpret_cast<uint8_t*>(i);
        ptrdiff_t j = maskSize - 1;

        while ((j >= 0) && (mask[j] == '?' || pattern[j] == ptr[j]))
            j--;

        if (j < 0)
        {
            m_matches.emplace_back(ptr);

            if (matchSuccess(i))
            {
                break;
            }
            i++;
        }
        else
        {
            // TODO: Replace w/ custom impl
            i += std::max(1, j - Last[ptr[j]]);
        }
    }

    m_matched = true;
}

bool Pattern::ConsiderMatch(uintptr_t offset)
{
    const char* pattern = m_bytes.c_str();
    const char* mask = m_mask.c_str();

    char* ptr = reinterpret_cast<char*>(offset);

    for (size_t i = 0, j = m_mask.size(); i < j; i++)
    {
        if (mask[i] == '?')
        {
            continue;
        }

        if (pattern[i] != ptr[i])
        {
            return false;
        }
    }

    m_matches.emplace_back(ptr);

    return true;
}

}  // namespace hook
