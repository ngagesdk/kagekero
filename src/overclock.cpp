/** @file overclock.cpp
 *
 *  A minimalist, cross-platform puzzle-platformer, designed
 *  especially for the Nokia N-Gage.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include <SDL3/SDL.h>

#ifdef __SYMBIAN32__
#include <e32std.h>
#include <e32svr.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

static bool overclock_state = false;

#ifdef __SYMBIAN32__
_LIT(KNinjaDll, "E:\\System\\Apps\\kagekero\\ninja.dll");
_LIT(KTurtleDll, "E:\\System\\Apps\\kagekero\\turtle.dll");

void disable_overclock(void)
{
    RLibrary lib;

    if (!overclock_state)
    {
        return;
    }

    if (lib.Load(KTurtleDll) == KErrNone)
    {
        UserSvr::ChangeLocale(lib);
        lib.Close();
        overclock_state = false;
    }
}

void enable_overclock(void)
{
    RLibrary lib;

    if (overclock_state)
    {
        return;
    }

    if (lib.Load(KNinjaDll) == KErrNone)
    {
        UserSvr::ChangeLocale(lib);
        lib.Close();
        overclock_state = true;
    }
}
#else
void disable_overclock(void)
{
    overclock_state = false;
    return;
}

void enable_overclock(void)
{
    overclock_state = true;
    return;
}
#endif

bool is_overclock_enabled(void)
{
    return overclock_state;
}

#ifdef __cplusplus
}
#endif // __cplusplus
