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

#include <psconfig.h>
#include "eedittoolboxmanager.h"
#include "eeditglobals.h"

#include "paws/pawsmanager.h"
#include "paws/pawswidget.h"
#include "paws/pawsborder.h"
#include "eeditpositiontoolbox.h"
#include "eedittargettoolbox.h"
#include "eeditcameratoolbox.h"
#include "eeditrendertoolbox.h"
#include "eeditloadeffecttoolbox.h"
#include "eeditpartlisttoolbox.h"
#include "eeditediteffecttoolbox.h"
#include "eeditloadmaptoolbox.h"
#include "eediterrortoolbox.h"
#include "eeditfpstoolbox.h"
#include "eeditshortcutstoolbox.h"

bool RegisterToolboxFactories()
{
    pawsWidgetFactory * factory;
    factory = new EEditPositionToolboxFactory();
    CS_ASSERT(factory);
    factory = new EEditTargetToolboxFactory();
    CS_ASSERT(factory);
    factory = new EEditCameraToolboxFactory();
    CS_ASSERT(factory);
    factory = new EEditRenderToolboxFactory();
    CS_ASSERT(factory);
    factory = new EEditParticleListToolboxFactory();
    CS_ASSERT(factory);
    factory = new EEditLoadEffectToolboxFactory();
    CS_ASSERT(factory);
    factory = new EEditEditEffectToolboxFactory();
    CS_ASSERT(factory);
    factory = new EEditLoadMapToolboxFactory();
    CS_ASSERT(factory);
    factory = new EEditErrorToolboxFactory();
    CS_ASSERT(factory);
    factory = new EEditFPSToolboxFactory();
    CS_ASSERT(factory);
    factory = new EEditShortcutsToolboxFactory();
    CS_ASSERT(factory);
    return true;
}

EEditToolboxManager::EEditToolboxManager()
{
    for (int a=0; a<EEditToolbox::T_COUNT; ++a)
        toolboxes[a] = 0;

    widgetFiles[EEditToolbox::T_POSITION]       = "data/eedit/position.xml";
    widgetFiles[EEditToolbox::T_TARGET]         = "data/eedit/target.xml";
    widgetFiles[EEditToolbox::T_CAMERA]         = "data/eedit/camera.xml";
    widgetFiles[EEditToolbox::T_RENDER]         = "data/eedit/render.xml";
    widgetFiles[EEditToolbox::T_LOAD_EFFECT]    = "data/eedit/loadeffect.xml";
    widgetFiles[EEditToolbox::T_EDIT_EFFECT]    = "data/eedit/editeffect.xml";
    widgetFiles[EEditToolbox::T_LOAD_MAP]       = "data/eedit/loadmap.xml";
    widgetFiles[EEditToolbox::T_ERROR]          = "data/eedit/error.xml";
    widgetFiles[EEditToolbox::T_FPS]            = "data/eedit/fps.xml";
    widgetFiles[EEditToolbox::T_SHORTCUTS]      = "data/eedit/shortcuts.xml";
    widgetFiles[EEditToolbox::T_PARTICLES]      = "data/eedit/partlist.xml";
    
    widgetNames[EEditToolbox::T_POSITION]       = "PositionToolbox";
    widgetNames[EEditToolbox::T_TARGET]         = "TargetToolbox";
    widgetNames[EEditToolbox::T_CAMERA]         = "CameraToolbox";
    widgetNames[EEditToolbox::T_RENDER]         = "RenderToolbox";
    widgetNames[EEditToolbox::T_LOAD_EFFECT]    = "LoadEffectToolbox";
    widgetNames[EEditToolbox::T_EDIT_EFFECT]    = "EditEffectToolbox";
    widgetNames[EEditToolbox::T_LOAD_MAP]       = "LoadMapToolbox";
    widgetNames[EEditToolbox::T_ERROR]          = "ErrorToolbox";
    widgetNames[EEditToolbox::T_FPS]            = "FPSToolbox";
    widgetNames[EEditToolbox::T_SHORTCUTS]      = "ShortcutsToolbox";
    widgetNames[EEditToolbox::T_PARTICLES]      = "PartListToolbox";
}

EEditToolboxManager::~EEditToolboxManager()
{
}

void EEditToolboxManager::UpdateAll(unsigned int elapsed)
{
    for (int a=0; a<EEditToolbox::T_COUNT; ++a)
    {
        if (toolboxes[a])
            toolboxes[a]->Update(elapsed);
    }
}

bool EEditToolboxManager::LoadWidgets(PawsManager * paws)
{
    bool succeeded = true;
    for (int a=0; a<EEditToolbox::T_COUNT; ++a)
    {
        // load the widget associated with this file
        if (!paws->LoadWidget(widgetFiles[a]))
        {
            printf("Warning: Loading of '%s' failed!\n", widgetFiles[a]);
            succeeded = false;
            toolboxes[a] = 0;
        }
        else
        {
            pawsWidget * toolboxWidget = paws->FindWidget(widgetNames[a]);
            
            toolboxes[a] = dynamic_cast<EEditToolbox *>(toolboxWidget);
            if (!toolboxes[a])
            {
                printf("Warning: Could not find widget: '%s' in '%s'!\n", widgetNames[a], widgetFiles[a]);
                succeeded = false;
            }
            else
            {
                pawsWidget * w = dynamic_cast<pawsWidget *>(toolboxes[a]);
                w->SetAlwaysOnTop(true);
            }
        }
    }
    return succeeded;
}

bool EEditToolboxManager::RegisterFactories() const
{
    return RegisterToolboxFactories();
}

size_t EEditToolboxManager::GetToolboxCount() const
{
    return EEditToolbox::T_COUNT;
}

EEditToolbox * EEditToolboxManager::GetToolbox(size_t type) const
{
    return toolboxes[type];
}

pawsWidget * EEditToolboxManager::GetToolboxWidget(size_t type) const
{
    return dynamic_cast<pawsWidget *>(toolboxes[type]);
}
