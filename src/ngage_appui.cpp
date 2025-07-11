/** @file ngage_appui.cpp
 *
 *  A minimalist, cross-platform puzzle-platformer, designed
 *  especially for the Nokia N-Gage.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include <aknnotewrappers.h>
#include <avkon.hrh>

#include "ngage_appui.h"
#include "ngage_appview.h"

void CNGageAppUi::ConstructL()
{
    BaseConstructL();

    iAppView = CNGageAppView::NewL(ClientRect());

    AddToStackL(iAppView);
}

_LIT(KNinjaDll, "E:\\System\\Apps\\kagekero\\ninja.dll");
_LIT(KTurtleDll, "E:\\System\\Apps\\kagekero\\turtle.dll");

CNGageAppUi::CNGageAppUi()
{
    RProcess Proc;

    iAppView = NULL;

    RLibrary lib;
    if (lib.Load(KNinjaDll) == KErrNone)
    {
        UserSvr::ChangeLocale(lib);
        lib.Close();

        if (KErrNone == Proc.Create(_L("E:\\System\\Apps\\kagekero\\kagekero.exe"), _L("")))
        {
            TRequestStatus status;
            Proc.Logon(status);
            Proc.Resume();
            User::WaitForRequest(status);
            Proc.Close();
            Exit();
        }
    }
    else
    {
        Exit();
    }
}

CNGageAppUi::~CNGageAppUi()
{
    RLibrary lib;
    if (lib.Load(KTurtleDll) == KErrNone)
    {
        UserSvr::ChangeLocale(lib);
        lib.Close();
    }

    if (iAppView)
    {
        RemoveFromStack(iAppView);
        delete iAppView;
        iAppView = NULL;
    }
}

void CNGageAppUi::HandleCommandL(TInt aCommand)
{
    /* No implementation required. */
}
