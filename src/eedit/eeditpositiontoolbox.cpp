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
#include <imesh/spritecal3d.h>

#include "eeditpositiontoolbox.h"
#include "eeditglobals.h"

#include "paws/pawsmanager.h"
#include "paws/pawsbutton.h"
#include "paws/pawstextbox.h"
#include "paws/pawsspinbox.h"
#include "paws/pawsradio.h"

EEditPositionToolbox::EEditPositionToolbox() : scfImplementationType(this)
{
    prevPos = csVector3(0,0,0);
    prevRot = 0.0f;
}

EEditPositionToolbox::~EEditPositionToolbox()
{
}

void EEditPositionToolbox::SetMeshFile(const csString & file)
{
    meshFileText->SetText(file);
}

void EEditPositionToolbox::FillAnimList(iSpriteCal3DState * cal3d)
{
    for (int a=0; a<cal3d->GetAnimCount(); ++a)
    {
        csString anim = cal3d->GetAnimName(a);
        anims.Push(anim);
    }
    animButton->Show();
}
    
void EEditPositionToolbox::SetPosition(const csVector3 & pos)
{
    posX->SetValue(pos.x);
    posY->SetValue(pos.y);
    posZ->SetValue(pos.z);
}

void EEditPositionToolbox::Update(unsigned int elapsed)
{
    bool changePos = false;

    if (posX->GetValue() != prevPos.x)
    {
        changePos = true;
        prevPos.x = posX->GetValue();
    }
    if (posY->GetValue() != prevPos.y)
    {
        changePos = true;
        prevPos.y = posY->GetValue();
    }
    if (posZ->GetValue() != prevPos.z)
    {
        changePos = true;
        prevPos.z = posZ->GetValue();
    }
    if (rot->GetValue() != prevRot)
    {
        changePos = true;
        prevRot = rot->GetValue();
    }

    if (changePos)
        editApp->SetPositionData(prevPos, prevRot * PI / 180.0f);
}

size_t EEditPositionToolbox::GetType() const
{
    return T_POSITION;
}

const char * EEditPositionToolbox::GetName() const
{
    return "Position";
}
    
bool EEditPositionToolbox::PostSetup()
{
    moreLessButton = (pawsButton *) FindWidget("more_less");                CS_ASSERT(moreLessButton);
    animButton     = (pawsButton *) FindWidget("anim");                     CS_ASSERT(animButton);
    browseMesh     = (pawsButton *) FindWidget("pos_mesh_browse");          CS_ASSERT(browseMesh);
    meshFileText   = (pawsEditTextBox *) FindWidget("pos_mesh_file");       CS_ASSERT(meshFileText);
    posX           = (pawsSpinBox *) FindWidget("pos_x");                   CS_ASSERT(posX);
    posY           = (pawsSpinBox *) FindWidget("pos_y");                   CS_ASSERT(posY);
    posZ           = (pawsSpinBox *) FindWidget("pos_z");                   CS_ASSERT(posZ);
    rot            = (pawsSpinBox *) FindWidget("pos_rot");                 CS_ASSERT(rot);
    posTypeNone    = (pawsRadioButton *) FindWidget("pos_type_none");       CS_ASSERT(posTypeNone);
    posTypeAxis    = (pawsRadioButton *) FindWidget("pos_type_axis");       CS_ASSERT(posTypeAxis);
    posTypeCustom  = (pawsRadioButton *) FindWidget("pos_type_custom");     CS_ASSERT(posTypeCustom);
   
    animButton->Hide();
    
    posTypeNone->SetState(true);
    posTypeAxis->SetState(false);
    posTypeCustom->SetState(false);
    
    return true;
}

bool EEditPositionToolbox::OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget )
{
    if (widget == moreLessButton)
    {
        if (csString(moreLessButton->GetText()) == csString("More >>"))
        {
            moreLessButton->SetText("Less <<");
            SetSize(GetActualWidth(210), GetActualHeight(250));
        }
        else
        {
            moreLessButton->SetText("More >>");
            SetSize(GetActualWidth(210), GetActualHeight(160));
        }
        return true;
    }
    else if (widget == animButton)
    {
        editApp->GetInputboxManager()->SelectList(anims.GetArray(), anims.GetSize(), anims[0], new OnAnimSelected());
    }
    else if (widget == browseMesh)
    {
        posTypeNone->SetState(false);
        posTypeAxis->SetState(false);
        posTypeCustom->SetState(true);
        pawsFileNavigation::Create(meshFileText->GetText(),"|*.cal3d|*.spr|*",
                                   new OnFileSelected(meshFileText), "data/eedit/filenavigation.xml");
    }
    else if (widget == posTypeNone || widget == posTypeAxis || widget == posTypeCustom)
    {
        if (posTypeNone->GetState())
            editApp->LoadPositionMesh("");
        else if (posTypeAxis->GetState())
            editApp->LoadPositionMesh("axis");
        else
            editApp->LoadPositionMesh(meshFileText->GetText());
    }

    return false;
}

void EEditPositionToolbox::OnFileSelected::Execute(const csString & text)
{
    if (text.Length() > 0)
        textBox->SetText(text);
    editApp->LoadPositionMesh(text);
}

void EEditPositionToolbox::OnAnimSelected::Select(const csString & value, size_t index)
{
    editApp->SetPositionAnim(index);
}

