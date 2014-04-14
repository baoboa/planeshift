/*
* peEditableWidget.h
*
* Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
*
* Credits : 
*            Michael Cummings <cummings.michael@gmail.com>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation (version 2
* of the License).
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*
* Creation Date: 1/20/2005
* Description : widget used to load windows, replaces libgui specific 
*                functionality with generic widget editing.
*/

#include <psconfig.h>
#include "peeditablewidget.h"
#include "pawseditorglobals.h"

#include "paws/pawsmanager.h"
#include "paws/pawsbutton.h"
#include "paws/pawsmainwidget.h"

peEditableWidget::peEditableWidget() : scfImplementationType(this)
{
}

peEditableWidget::~peEditableWidget()
{
}

bool peEditableWidget::PostSetup()
{
    return true;
}

bool peEditableWidget::OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget)
{
    return true;
}
