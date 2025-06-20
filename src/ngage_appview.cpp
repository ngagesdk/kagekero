/** @file ngage_appview.cpp
 *
 *  A minimalist, cross-platform puzzle-platformer, designed
 *  especially for the Nokia N-Gage.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include "ngage_appview.h"
#include <coemain.h>

CNGageAppView *CNGageAppView::NewL(const TRect &aRect)
{
    CNGageAppView *self = CNGageAppView::NewLC(aRect);
    CleanupStack::Pop(self);
    return self;
}

CNGageAppView *CNGageAppView::NewLC(const TRect &aRect)
{
    CNGageAppView *self = new (ELeave) CNGageAppView;
    CleanupStack::PushL(self);
    self->ConstructL(aRect);
    return self;
}

CNGageAppView::CNGageAppView()
{
    /* No implementation required. */
}

CNGageAppView::~CNGageAppView()
{
    /* No implementation required. */
}

void CNGageAppView::ConstructL(const TRect &aRect)
{
    CreateWindowL();
    SetRect(aRect);
    ActivateL();
}

// Draw this application's view to the screen
void CNGageAppView::Draw(const TRect & /*aRect*/) const
{
    CWindowGc &gc = SystemGc();
    TRect rect = Rect();

    gc.Clear(rect);
}
