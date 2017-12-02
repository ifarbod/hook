// Code pattern matching utilities
// Author(s):       iFarbod <ifarbod@outlook.com>
//
// Copyright (c) 2013-2017 CTNorth Team
//
// Distributed under the MIT license (See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT)

#pragma once

#include <stddef.h>
#include <stdint.h>

#include <cassert>
#include <vector>

#include "build/BuildConfig.hpp"

namespace hook
{

// TODO: Integrate with MemoryPointer
extern ptrdiff_t baseAddressDifference;

// Sets the base address difference based on an obtained pointer
inline void SetBase(uintptr_t address)
{
#ifdef ARCH_CPU_X86
    uintptr_t addressDiff = (address - 0x400000);
#elif defined(ARCH_CPU_X86_64)
    uintptr_t addressDiff = (address - 0x140000000);
#endif

    // Pointer-style cast to ensure unsigned overflow ends up copied directly into a signed value
    baseAddressDifference = *reinterpret_cast<ptrdiff_t*>(&addressDiff);
}

// Sets the base to the process main base
void SetBase();

template <typename T>
inline T* GetRVA(uintptr_t rva)
{
    SetBase();
#ifdef ARCH_CPU_X86
    return (T*)(baseAddressDifference + 0x400000 + rva);
#elif defined(ARCH_CPU_X86_64)
    return (T*)(0x140000000 + rva);
#endif
}

class PatternMatch
{
public:
    PatternMatch(void* pointer) :
        m_pointer(pointer)
    {}

    template <typename T>
    T* Get(ptrdiff_t offset = 0) const
    {
        char* ptr = reinterpret_cast<char*>(m_pointer);
        return reinterpret_cast<T*>(ptr + offset);
    }

private:
    void* m_pointer;
};

class Pattern
{
public:
    template <size_t Len>
    Pattern(const char(&pattern)[Len]) :
        Pattern(GetRVA<void>(0))
    {
        Initialize(pattern, Len - 1);
    }

    Pattern& Count(uint32_t expected)
    {
        EnsureMatches(expected);
        assert(m_matches.size() == expected);
        return *this;
    }

    Pattern& CountHint(uint32_t expected)
    {
        EnsureMatches(expected);
        return *this;
    }

    Pattern& Clear()
    {
        m_matches.clear();
        m_matched = false;
        return *this;
    }

    size_t Size()
    {
        EnsureMatches(UINT32_MAX);
        return m_matches.size();
    }

    bool Empty() { return Size() == 0; }

    PatternMatch Get(size_t index)
    {
        EnsureMatches(UINT32_MAX);
        return GetInternal(index);
    }

    PatternMatch GetOne() { return Count(1).GetInternal(0); }

    template <typename T = void>
    auto GetFirst(ptrdiff_t offset = 0)
    {
        return GetOne().Get<T>(offset);
    }

protected:
    Pattern(void* module) : m_module(module), m_rangeEnd(0), m_matched(false) {}

    Pattern(uintptr_t begin, uintptr_t end) : m_rangeStart(begin), m_rangeEnd(end), m_matched(false) {}

    void Initialize(const char* pattern, size_t length);

private:
    bool ConsiderMatch(uintptr_t offset);

    void EnsureMatches(uint32_t maxCount);

    PatternMatch GetInternal(size_t index) const { return m_matches[index]; }

    std::string m_bytes;
    std::string m_mask;

    std::vector<PatternMatch> m_matches;

    bool m_matched;

    union
    {
        void* m_module;
        struct
        {
            uintptr_t m_rangeStart;
            uintptr_t m_rangeEnd;
        };
    };
};

class ModulePattern : public Pattern
{
public:
    template <size_t Len>
    ModulePattern(void* module, const char (&pattern)[Len]) :
        Pattern(module)
    {
        Initialize(pattern, Len - 1);
    }
};

class RangePattern : public Pattern
{
public:
    template <size_t Len>
    RangePattern(uintptr_t begin, uintptr_t end, const char (&pattern)[Len]) :
        Pattern(begin, end)
    {
        Initialize(pattern, Len - 1);
    }
};

template <typename T = void, size_t Len>
auto GetPattern(const char (&pattern_string)[Len], ptrdiff_t offset = 0)
{
    return Pattern(pattern_string).GetFirst<T>(offset);
}

}  // namespace hook
