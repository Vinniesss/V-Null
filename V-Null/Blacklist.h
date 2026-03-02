#pragma once
#include <string>
#include <vector>

enum class MatchMode { Exact , Prefix };

struct BlacklistEntry
{
    std::wstring exe_name;
    std::wstring category;
    MatchMode    mode = MatchMode::Exact;
};

inline std::vector<BlacklistEntry> get_blacklist ( )
{
    return
    {
        { L"notepad.exe"  , L"Example"  , MatchMode::Exact  },
        { L"calc.exe"     , L"Example"  , MatchMode::Exact  },
        { L"Ocean-"       , L"Ocean"    , MatchMode::Prefix },
    };
}

inline bool is_blacklisted ( const std::wstring& exe_name ,
                             const std::vector<BlacklistEntry>& blacklist ,
                             std::wstring& out_category )
{
    for ( const auto& entry : blacklist )
    {
        if ( entry.mode == MatchMode::Exact )
        {
            if ( _wcsicmp ( exe_name.c_str ( ) , entry.exe_name.c_str ( ) ) == 0 )
            {
                out_category = entry.category;
                return true;
            }
        }
        else if ( entry.mode == MatchMode::Prefix )
        {
            if ( _wcsnicmp ( exe_name.c_str ( ) , entry.exe_name.c_str ( ) , entry.exe_name.size ( ) ) == 0 )
            {
                out_category = entry.category;
                return true;
            }
        }
    }
    return false;
}
