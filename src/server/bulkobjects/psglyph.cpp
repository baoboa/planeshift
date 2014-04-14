/*
 * psglyph.cpp
 *
 * Copyright (C) 2001-2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2 of the License)
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include "psconfig.h"
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/csstring.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/psstring.h"
#include "util/log.h"
#include "util/psconst.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "psglyph.h"
#include "pscharacter.h"


psGlyph::psGlyph()
{
}

bool psGlyph::Load(iResultRow &row)
{
    psString flagstr(row["flags"]);

    // Set purify status
    if(flagstr.FindSubString("PURIFYING",0,true)!=-1)
    {
        flags |= PSITEM_FLAG_PURIFYING;
    }

    if(flagstr.FindSubString("PURIFIED",0,true)!=-1)
    {
        flags |= PSITEM_FLAG_PURIFIED;
    }

    return psItem::Load(row);
}

void psGlyph::PurifyingStarted()
{
    flags &= ~PSITEM_FLAG_PURIFIED;
    flags |= PSITEM_FLAG_PURIFYING;
    Save(false);
}

void psGlyph::PurifyingFinished()
{
    flags |= PSITEM_FLAG_PURIFIED;
    flags &= ~PSITEM_FLAG_PURIFYING;
    Save(false);
}

void psGlyph::UnPurify()
{
    flags &= ~PSITEM_FLAG_PURIFYING;
    flags &= ~PSITEM_FLAG_PURIFIED;
}

bool psGlyph::Purified()
{
    return (flags & PSITEM_FLAG_PURIFIED) ? true : false;
}

psItem* psGlyph::Clone()
{
    return new psGlyph();
}


// Definition of the glyphpool for psGlyphs
PoolAllocator<psGlyph> psGlyph::glyphpool;

void* psGlyph::operator new(size_t allocSize)
{
    CS_ASSERT(allocSize<=sizeof(psGlyph));
    return (void*)glyphpool.CallFromNew();
}

void psGlyph::operator delete(void* releasePtr)
{
    glyphpool.CallFromDelete((psGlyph*)releasePtr);
}

psItem* psGlyph::CreateNew()
{
    return new psGlyph();
}

void psGlyph::Copy(psGlyph* target)
{
    psItem::Copy(target);
    target->SetFlags(flags & ~(PSITEM_FLAG_PURIFIED | PSITEM_FLAG_PURIFYING));
}

void psGlyph::SetOwningCharacter(psCharacter* owner)
{
    if(loaded && owner != GetOwningCharacter())
        UnPurify();
    psItem::SetOwningCharacter(owner);
}

int psGlyph::GetPurifyStatus() const
{
    if(flags & PSITEM_FLAG_PURIFIED)
        return 2;
    else if(flags & PSITEM_FLAG_PURIFYING)
        return 1;
    else
        return 0;
}
