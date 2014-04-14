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

#include "eedittargettoolbox.h"
#include "eeditglobals.h"

#include "paws/pawsmanager.h"
#include "paws/pawsbutton.h"
#include "paws/pawstextbox.h"
#include "paws/pawsspinbox.h"
#include "paws/pawsradio.h"

EEditTargetToolbox::EEditTargetToolbox() : scfImplementationType(this)
{
    prevTar = csVector3(0,0,0);
    prevRot = 0.0f;
}

EEditTargetToolbox::~EEditTargetToolbox()
{
}

void EEditTargetToolbox::SetMeshFile(const csString & file)
{
    meshFileText->SetText(file);
}

void EEditTargetToolbox::FillAnimList(iSpriteCal3DState * cal3d)
{
    for (int a=0; a<cal3d->GetAnimCount(); ++a)
    {
        csString anim = cal3d->GetAnimName(a);
        anims.Push(anim);
    }
    animButton->Show();
}

void EEditTargetToolbox::SetPosition(const csVector3 & pos)
{
    tarX->SetValue(pos.x);
    tarY->SetValue(pos.y);
    tarZ->SetValue(pos.z);
}

void EEditTargetToolbox::Update(unsigned int elapsed)
{
    bool changeTar = false;

    if (tarX->GetValue() != prevTar.x)
    {
        changeTar = true;
        prevTar.x = tarX->GetValue();
    }
    if (tarY->GetValue() != prevTar.y)
    {
        changeTar = true;
        prevTar.y = tarY->GetValue();
    }
    if (tarZ->GetValue() != prevTar.z)
    {
        changeTar = true;
        prevTar.z = tarZ->GetValue();
    }
    if (rot->GetValue() != prevRot)
    {
        changeTar = true;
        prevRot = rot->GetValue();
    }

    if (changeTar)
        editApp->SetTargetData(prevTar, prevRot * PI / 180.0f);
}

size_t EEditTargetToolbox::GetType() const
{
    return T_TARGET;
}

const char * EEditTargetToolbox::GetName() const
{
    return "Target";
}
    
bool EEditTargetToolbox::PostSetup()
{
    moreLessButton = (pawsButton *) FindWidget("more_less");                CS_ASSERT(moreLessButton);
    animButton     = (pawsButton *) FindWidget("anim");                     CS_ASSERT(animButton);
    browseMesh     = (pawsButton *) FindWidget("tar_mesh_browse");          CS_ASSERT(browseMesh);
    meshFileText   = (pawsEditTextBox *) FindWidget("tar_mesh_file");       CS_ASSERT(meshFileText);
    tarX           = (pawsSpinBox *) FindWidget("tar_x");                   CS_ASSERT(tarX);
    tarY           = (pawsSpinBox *) FindWidget("tar_y");                   CS_ASSERT(tarY);
    tarZ           = (pawsSpinBox *) FindWidget("tar_z");                   CS_ASSERT(tarZ);
    rot            = (pawsSpinBox *) FindWidget("tar_rot");                 CS_ASSERT(rot);
    tarTypeNone    = (pawsRadioButton *) FindWidget("tar_type_none");       CS_ASSERT(tarTypeNone);
    tarTypeAxis    = (pawsRadioButton *) FindWidget("tar_type_axis");       CS_ASSERT(tarTypeAxis);
    tarTypeCustom  = (pawsRadioButton *) FindWidget("tar_type_custom");     CS_ASSERT(tarTypeCustom);
    
    animButton->Hide();
    
    tarTypeNone->SetState(true);
    tarTypeAxis->SetState(false);
    tarTypeCustom->SetState(false);
    
    return true;
}

bool EEditTargetToolbox::OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget )
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
        tarTypeNone->SetState(false);
        tarTypeAxis->SetState(false);
        tarTypeCustom->SetState(true);
        pawsFileNavigation::Create(meshFileText->GetText(),"|*.cal3d|*.spr|*",
                                   new OnFileSelected(meshFileText), "data/eedit/filenavigation.xml");
    }
    else if (widget == tarTypeNone || widget == tarTypeAxis || widget == tarTypeCustom)
    {
        if (tarTypeNone->GetState())
            editApp->LoadTargetMesh("");
        else if (tarTypeAxis->GetState())
            editApp->LoadTargetMesh("axis");
        else
            editApp->LoadTargetMesh(meshFileText->GetText());
    }

    return false;
}

void EEditTargetToolbox::OnFileSelected::Execute(const csString & text)
{
    if (text.Length() > 0)
        textBox->SetText(text);
    editApp->LoadTargetMesh(text);
}

void EEditTargetToolbox::OnAnimSelected::Select(const csString & value, size_t index)
{
    editApp->SetTargetAnim(index);
}

