/*
 * pstoggle.cpp
 *
 * Copyright (C) 2001-2010 Atomic Blue (info@planeshift.it, http://www.planeshift.it)
 *
 * Credits : Saul Leite <leite@engineer.com>
 *           Mathias 'AgY' Voeroes <agy@operswithoutlife.net>
 *           and all past and present planeshift coders
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include "pstoggle.h"

psToggle::psToggle()
{
    state = true;
    hasCallback = false;
}

psToggle::~psToggle()
{   
}

void psToggle::Activate()
{
    state = true;
    Callback();
}

void psToggle::Deactivate()
{
    state = false;
    Callback();
}

bool psToggle::GetToggle()
{
    return state;
}

void psToggle::SetToggle(bool newstate)
{
    state = newstate;
    Callback();
}

void psToggle::SetCallback(void (*object), void (*function) (void *))
{
    callbackobject = object;
    callbackfunction = function;
    hasCallback = true; 
}

void psToggle::RemoveCallback()
{
    hasCallback = false;
}

void psToggle::Callback()
{
    if (hasCallback == true)
    {
        callbackfunction(callbackobject);
    }
}
