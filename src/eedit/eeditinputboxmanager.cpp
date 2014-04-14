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
#include "eeditinputboxmanager.h"
#include "eeditglobals.h"

#include "paws/pawsmanager.h"
#include "paws/pawswidget.h"

#include "eeditselectfloat.h"
#include "eeditselectstring.h"
#include "eeditselectvec3.h"
#include "eeditselectyesno.h"
#include "eeditselectlist.h"
#include "eeditselectnewanchor.h"
#include "eeditselecteditanchor.h"
#include "eeditselectnewanchorkeyframe.h"

EEditInputboxManager::EEditInputboxManager()
{
    selectFloat = 0;
    selectString = 0;
    selectVec3 = 0;
    selectYesNo = 0;
    selectList = 0;
    selectNewAnchor = 0;
    selectEditAnchor = 0;
    selectNewAnchorKeyFrame = 0;
}

EEditInputboxManager::~EEditInputboxManager()
{
}
 
#define EEDIT_SELECT_FLOAT_FILE              "data/eedit/inputbox/float.xml"
#define EEDIT_SELECT_STRING_FILE             "data/eedit/inputbox/string.xml"
#define EEDIT_SELECT_VEC3_FILE               "data/eedit/inputbox/vec3.xml"
#define EEDIT_SELECT_YESNO_FILE              "data/eedit/inputbox/yesno.xml"
#define EEDIT_SELECT_LIST_FILE               "data/eedit/inputbox/list.xml"
#define EEDIT_SELECT_NEWANCHOR_FILE          "data/eedit/inputbox/newanchor.xml"
#define EEDIT_SELECT_EDITANCHOR_FILE         "data/eedit/inputbox/editanchor.xml"
#define EEDIT_SELECT_NEWANCHOR_KEYFRAME_FILE "data/eedit/inputbox/newanchorkeyframe.xml"
bool EEditInputboxManager::LoadWidgets(PawsManager * paws)
{
    bool succeeded = true;
    
    // load select float
    if (!paws->LoadWidget(EEDIT_SELECT_FLOAT_FILE))
    {
        printf("Warning: Loading of '%s' failed!\n", EEDIT_SELECT_FLOAT_FILE);
        succeeded = false;
    }
    selectFloat = (EEditSelectFloat *)paws->FindWidget("SelectFloat");
    if (!selectFloat)
    {
        printf("Warning: Could not find SelectFloat window!\n");
        succeeded = false;
    }
    
    // load select string
    if (!paws->LoadWidget(EEDIT_SELECT_STRING_FILE))
    {
        printf("Warning: Loading of '%s' failed!\n", EEDIT_SELECT_STRING_FILE);
        succeeded = false;
    }
    selectString = (EEditSelectString *)paws->FindWidget("SelectString");
    if (!selectString)
    {
        printf("Warning: Could not find SelectString window!\n");
        succeeded = false;
    }
    
    // load select vec3
    if (!paws->LoadWidget(EEDIT_SELECT_VEC3_FILE))
    {
        printf("Warning: Loading of '%s' failed!\n", EEDIT_SELECT_VEC3_FILE);
        succeeded = false;
    }
    selectVec3 = (EEditSelectVec3 *)paws->FindWidget("SelectVec3");
    if (!selectVec3)
    {
        printf("Warning: Could not find SelectVec3 window!\n");
        succeeded = false;
    }

    // load select yesno
    if (!paws->LoadWidget(EEDIT_SELECT_YESNO_FILE))
    {
        printf("Warning: Loading of '%s' failed!\n", EEDIT_SELECT_YESNO_FILE);
        succeeded = false;
    }
    selectYesNo = (EEditSelectYesNo *)paws->FindWidget("SelectYesNo");
    if (!selectYesNo)
    {
        printf("Warning: Could not find SelectYesNo window!\n");
        succeeded = false;
    }

    // load select list
    if (!paws->LoadWidget(EEDIT_SELECT_LIST_FILE))
    {
        printf("Warning: Loading of '%s' failed!\n", EEDIT_SELECT_LIST_FILE);
        succeeded = false;
    }
    selectList = (EEditSelectList *)paws->FindWidget("SelectList");
    if (!selectList)
    {
        printf("Warning: Could not find SelectList window!\n");
        succeeded = false;
    }
    
    // load select newanchor
    if (!paws->LoadWidget(EEDIT_SELECT_NEWANCHOR_FILE))
    {
        printf("Warning: Loading of '%s' failed!\n", EEDIT_SELECT_NEWANCHOR_FILE);
        succeeded = false;
    }
    selectNewAnchor = (EEditSelectNewAnchor *)paws->FindWidget("SelectNewAnchor");
    if (!selectNewAnchor)
    {
        printf("Warning: Could not find SelectNewAnchor window!\n");
        succeeded = false;
    }
    
    // load select editanchor
    if (!paws->LoadWidget(EEDIT_SELECT_EDITANCHOR_FILE))
    {
        printf("Warning: Loading of '%s' failed!\n", EEDIT_SELECT_EDITANCHOR_FILE);
        succeeded = false;
    }
    selectEditAnchor = (EEditSelectEditAnchor *)paws->FindWidget("SelectEditAnchor");
    if (!selectEditAnchor)
    {
        printf("Warning: Could not find SelectEditAnchor window!\n");
        succeeded = false;
    }
    
    // load select newanchorkeyframe
    if (!paws->LoadWidget(EEDIT_SELECT_NEWANCHOR_KEYFRAME_FILE))
    {
        printf("Warning: Loading of '%s' failed!\n", EEDIT_SELECT_NEWANCHOR_KEYFRAME_FILE);
        succeeded = false;
    }
    selectNewAnchorKeyFrame = (EEditSelectNewAnchorKeyFrame *)paws->FindWidget("SelectNewAnchorKeyFrame");
    if (!selectNewAnchorKeyFrame)
    {
        printf("Warning: Could not find SelectNewAnchorKeyFrame window!\n");
        succeeded = false;
    }
    
    return succeeded;
}

bool EEditInputboxManager::RegisterFactories() const
{
    pawsWidgetFactory * factory;
    factory = new EEditSelectFloatFactory();
    CS_ASSERT(factory);
    factory = new EEditSelectStringFactory();
    CS_ASSERT(factory);
    factory = new EEditSelectVec3Factory();
    CS_ASSERT(factory);
    factory = new EEditSelectYesNoFactory();
    CS_ASSERT(factory);
    factory = new EEditSelectListFactory();
    CS_ASSERT(factory);
    factory = new EEditSelectNewAnchorFactory();
    CS_ASSERT(factory);
    factory = new EEditSelectEditAnchorFactory();
    CS_ASSERT(factory);
    factory = new EEditSelectNewAnchorKeyFrameFactory();
    CS_ASSERT(factory);
    return true;
}

void EEditInputboxManager::SelectFloat(float startValue, iSelectFloat * callback)
{
    if (selectFloat)
        selectFloat->Select(startValue, callback, editApp->GetMousePointer());
    else
        printf("Cannot display SelectFloat box, because it isn't found!\n");
}

void EEditInputboxManager::SelectString(const csString & startValue, iSelectString * callback)
{
    if (selectString)
        selectString->Select(startValue, callback, editApp->GetMousePointer());
    else
        printf("Cannot display SelectString box, because it isn't found!\n");
}

void EEditInputboxManager::SelectVec3(const csVector3 & startValue, iSelectVec3 * callback)
{
    if (selectVec3)
        selectVec3->Select(startValue, callback, editApp->GetMousePointer());
    else
        printf("Cannot display SelectVec3 box, because it isn't found!\n");
}

void EEditInputboxManager::SelectYesNo(bool startValue, iSelectYesNo * callback)
{
    if (selectYesNo)
        selectYesNo->Select(startValue, callback, editApp->GetMousePointer());
    else
        printf("Cannot display SelectYesNo box, because it isn't found!\n");
}

void EEditInputboxManager::SelectList(csString * list, size_t listCount, const csString & defaultValue, iSelectList * callback)
{
    if (selectList)
        selectList->Select(list, listCount, defaultValue, callback, editApp->GetMousePointer());
    else
        printf("Cannot display SelectList box, because it isn't found!\n");
}

void EEditInputboxManager::SelectNewAnchor(iSelectNewAnchor * callback)
{
    if (selectNewAnchor)
        selectNewAnchor->Select(callback, editApp->GetMousePointer());
    else
        printf("Cannot display SelectNewAnchor box, because it isn't found!\n");
}

void EEditInputboxManager::SelectEditAnchor(const csString & currName, iSelectEditAnchor * callback)
{
    if (selectEditAnchor)
        selectEditAnchor->Select(currName, callback, editApp->GetMousePointer());
    else
        printf("Cannot display SelectEditAnchor box, because it isn't found!\n");
}

void EEditInputboxManager::SelectNewAnchorKeyFrame(iSelectNewAnchorKeyFrame * callback)
{
    if (selectNewAnchorKeyFrame)
        selectNewAnchorKeyFrame->Select(callback, editApp->GetMousePointer());
    else
        printf("Cannot display SelectNewAnchorKeyFrame box, because it isn't found!\n");
}

