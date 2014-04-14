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

#include <iutil/cfgmgr.h>



#include "eeditfpstoolbox.h"

#include "eeditglobals.h"



#include "paws/pawsmanager.h"

#include "paws/pawstextbox.h"

#include "paws/pawsspinbox.h"


EEditFPSToolbox::EEditFPSToolbox() : scfImplementationType(this)

{
    fps = 60.0f;

    framesCount = 0;

    framesElapsed = 0;

}



EEditFPSToolbox::~EEditFPSToolbox()

{

    editApp->SetConfigFloat("EEdit.TargetFPS", fpsTarget->GetValue());

}



float EEditFPSToolbox::GetFPS() const

{

    return fps;

}



float EEditFPSToolbox::GetTargetFPS() const

{

    return fpsTarget->GetValue();

}





void EEditFPSToolbox::Update(unsigned int elapsed)

{

    ++framesCount;

    framesElapsed += elapsed;

    if (framesElapsed > 1000)

    {

        fps = ((float)framesCount / (float)framesElapsed) * 1000.0f;

        framesCount = 0;

        framesElapsed = 0;



        csString fpsText;

        fpsText.Format("%5.1f", fps);

        fpsDisplay->SetText(fpsText);

        SetTitle("FPS - " + fpsText);

    }

}



size_t EEditFPSToolbox::GetType() const

{

    return T_FPS;

}



const char * EEditFPSToolbox::GetName() const

{

    return "FPS";

}

    

bool EEditFPSToolbox::PostSetup()

{

    fpsDisplay = (pawsTextBox *) FindWidget("fps");        CS_ASSERT(fps);

    fpsTarget  = (pawsSpinBox *) FindWidget("fps_target"); CS_ASSERT(fpsTarget);

    float target = editApp->GetConfigFloat("EEdit.TargetFPS", 60.0f);

    if (target < 1.0f)

        target = 1.0f;

    fpsTarget->SetValue(target);

    

    return true;

}



bool EEditFPSToolbox::OnButtonPressed(int mouseButton, int keyModifier, pawsWidget * widget)

{

    return false;

}



