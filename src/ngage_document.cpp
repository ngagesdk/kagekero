/** @file ngage_document.cpp
 *
 *  A minimalist, cross-platform puzzle-platformer, designed
 *  especially for the Nokia N-Gage.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include "ngage_document.h"
#include "ngage_appui.h"

CNGageDocument *CNGageDocument::NewL(CEikApplication &aApp)
{
    CNGageDocument *self = NewLC(aApp);
    CleanupStack::Pop(self);
    return self;
}

CNGageDocument *CNGageDocument::NewLC(CEikApplication &aApp)
{
    CNGageDocument *self = new (ELeave) CNGageDocument(aApp);
    CleanupStack::PushL(self);
    self->ConstructL();
    return self;
}

void CNGageDocument::ConstructL()
{
    /* No implementation required. */
}

CNGageDocument::CNGageDocument(CEikApplication &aApp) : CAknDocument(aApp)
{
    /* No implementation required. */
}

CNGageDocument::~CNGageDocument()
{
    /* No implementation required. */
}

CEikAppUi *CNGageDocument::CreateAppUiL()
{
    CEikAppUi *appUi = new (ELeave) CNGageAppUi;
    return appUi;
}
