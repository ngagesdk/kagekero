/** @file ngage_application.cpp
 *
 *  A minimalist, cross-platform puzzle-platformer, designed
 *  especially for the Nokia N-Gage.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include "ngage_application.h"
#include "ngage_document.h"

static const TUid KUidNGageApp = { 0x1badc0de };

CApaDocument *CNGageApplication::CreateDocumentL()
{
    CApaDocument *document = CNGageDocument::NewL(*this);
    return document;
}

TUid CNGageApplication::AppDllUid() const
{
    return KUidNGageApp;
}
