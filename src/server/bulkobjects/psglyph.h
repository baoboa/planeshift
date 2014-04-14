/*
 * psglyph.h
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

#ifndef __PSGLYPH_H__
#define __psglyph_h__
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/csstring.h>
#include <csutil/refcount.h>
#include <csutil/refarr.h>
#include <csutil/array.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/psconst.h"

#include <idal.h>

//=============================================================================
// Local Includes
//=============================================================================
#include "psitem.h"
#include "psstdint.h"

class psCharacter;


/**
 */
class psGlyph : public psItem
{
public:
    psGlyph();
    virtual ~psGlyph() { };

    /** Return a string identifying this object as a Glyph
     */
    virtual const char* GetItemType()
    {
        return "Glyph";
    }

    virtual bool Load(iResultRow &row);

    virtual psItem* CreateNew();
    virtual void Copy(psGlyph* target);

    virtual void SetOwningCharacter(psCharacter* owner);

    csString GlyphToXML();
    virtual int GetPurifyStatus() const;
    void PurifyingStarted();
    void PurifyingFinished();
    void UnPurify();
    bool Purified();

    virtual psItem* Clone();

    ///  The new operator is overriden to call PoolAllocator template functions
    void* operator new(size_t);

    ///  The delete operator is overriden to call PoolAllocator template functions
    void operator delete(void*);

private:
    /// Static reference to the pool for all psGlyph objects
    static PoolAllocator<psGlyph> glyphpool;

};

#endif
