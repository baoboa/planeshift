/*
* Author: Andrew Robberts
*
* Copyright (C) 2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef PS_EFFECT_OBJ_TEXTABLE_HEADER
#define PS_EFFECT_OBJ_TEXTABLE_HEADER

#include <csutil/csstring.h>

/**
 * \addtogroup common_effects
 * @{ */

enum psEffectTextAlignment
{
    ETA_LEFT,
    ETA_CENTER,
    ETA_RIGHT
};

struct psEffectTextRow
{
    csString        text;
    int             colour;
    bool            hasShadow;
    int             shadowColour;
    bool            hasOutline;
    int             outlineColour;
    psEffectTextAlignment   align;
};

struct psEffectTextElement : public psEffectTextRow
{
    int     x;
    int     y;
    int     width;
    int     height;
};

class psEffectObjTextable
{
public:
    virtual bool SetText(const csArray<psEffectTextElement> &elements) = 0;
    virtual bool SetText(const csArray<psEffectTextRow> &rows) = 0;
    virtual bool SetText(int rows, ...) = 0;
    virtual ~psEffectObjTextable() {};
};

/** @} */

#endif // PS_EFFECT_OBJ_TEXTABLE_HEADER

