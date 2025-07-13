/** @file overclock.cpp
 *
 *  A minimalist, cross-platform puzzle-platformer, designed
 *  especially for the Nokia N-Gage.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifdef __SYMBIAN32__
#include <e32std.h>
#include <e32svr.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#ifdef __SYMBIAN32__
_LIT(KNinjaDll, "E:\\System\\Apps\\kagekero\\ninja.dll");
_LIT(KTurtleDll, "E:\\System\\Apps\\kagekero\\turtle.dll");

static TBool is_overclock_enabled = EFalse;

void disable_overclock(void)
{
    RLibrary lib;

    if (!is_overclock_enabled)
    {
        return;
    }

    if (lib.Load(KTurtleDll) == KErrNone)
    {
        UserSvr::ChangeLocale(lib);
        lib.Close();
        is_overclock_enabled = EFalse;
    }
}

void enable_overclock(void)
{
    RLibrary lib;

    if (is_overclock_enabled)
    {
        return;
    }

    if (lib.Load(KNinjaDll) == KErrNone)
    {
        UserSvr::ChangeLocale(lib);
        lib.Close();
        is_overclock_enabled = ETrue;
    }
}
#else
void disable_overclock(void)
{
    return;
}

void enable_overclock(void)
{
    return;
}
#endif

#ifdef __cplusplus
}
#endif // __cplusplus
