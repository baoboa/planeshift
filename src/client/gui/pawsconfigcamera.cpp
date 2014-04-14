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

// CS INCLUDES
#include <psconfig.h>
#include <csutil/xmltiny.h>
#include <csutil/objreg.h>

// Client INCLUDES
#include "globals.h"
#include "pscamera.h"

// COMMON INCLUDES
#include "../../common/util/log.h"

// PAWS INCLUDES
#include "pawsconfigcamera.h"
#include "paws/pawstextbox.h"
#include "paws/pawsmanager.h"
#include "paws/pawsmainwidget.h"
#include "paws/pawsspinbox.h"

//////////////////////////////////////////////////////////////////////
//
//                        pawsConfigCamera
//
//////////////////////////////////////////////////////////////////////

pawsConfigCamera::pawsConfigCamera()
{
    labelWidth = 230;
    inputWidth = 70;
}

pawsConfigCamera::~pawsConfigCamera()
{
}

bool pawsConfigCamera::Initialize()
{
    psengine->GetPSCamera()->LoadFromFile(false,false);
    if (!CreateTree())
        return false;

    UseBorder("line");
    return true;
}

bool pawsConfigCamera::LoadConfig()
{
    return psengine->GetPSCamera()->LoadFromFile(false,false);
}

bool pawsConfigCamera::SaveConfig()
{
    pawsTreeNode *root = tree->GetRoot();
    if (root != NULL)
        SaveConfigByTreeNodes(root);

    return psengine->GetPSCamera()->SaveToFile();
}

void pawsConfigCamera::SetDefault()
{
    psengine->GetPSCamera()->LoadFromFile(true);
    SetCameraValues(tree->GetRoot());
    dirty = true;
}

bool pawsConfigCamera::CreateTree()
{
    pawsTreeNode * root;

    /*tree = new pawsTree();
    tree->MoveTo(screenFrame.xmin, screenFrame.ymin);
    tree->SetSize(screenFrame.Width(), screenFrame.Height());
    tree->SetScrollBars(false, true);
    tree->SetTreeLayout(new pawsStdTreeLayout(tree, 5, 20));
    tree->SetTreeDecorator(new pawsStdTreeDecorator(tree, graphics2D, 0x0000ff, 0x00ffff, 13));
    AddChild(tree);
    */
    if (!LoadFromFile("configcamera.xml"))
        return false;

    tree = dynamic_cast<pawsTree *>(children[0]);
    tree->SetRelativeFrameSize(parent->GetScreenFrame().Width()-20, parent->GetScreenFrame().Height()-20);

    root = tree->GetRoot();
    if (root != NULL)
        CreateTreeWidgets(root);

    tree->SetScrollBars(false,true);

    return true;
}

void pawsConfigCamera::CreateTreeWidgets(pawsTreeNode * subtreeRoot)
{
    pawsTextBox * label;
    pawsButton * button;
    pawsSeqTreeNode * rootAsSeq;
    pawsTreeNode * child;

    rootAsSeq = dynamic_cast<pawsSeqTreeNode*> (subtreeRoot);
    if (rootAsSeq != NULL)
    {
        csString cammode = subtreeRoot->GetAttr("cammode");
        int currCamMode = -1;
        if (cammode == "First Person")
            currCamMode = psCamera::CAMERA_FIRST_PERSON;
        else if (cammode == "Third Person Follow")
            currCamMode = psCamera::CAMERA_THIRD_PERSON;
        else if (cammode == "Third Person M64")
            currCamMode = psCamera::CAMERA_M64_THIRD_PERSON;
        else if (cammode == "Third Person Lara")
            currCamMode = psCamera::CAMERA_LARA_THIRD_PERSON;
        else if (cammode == "Free Rotation")
            currCamMode = psCamera::CAMERA_FREE;

        if (!strcmp(subtreeRoot->GetAttr("type"), "onoff"))
        {
            label = new pawsTextBox();
            label->SetText(subtreeRoot->GetName());
            label->SetColour(0xffffff);
            label->Show();
            label->SetRelativeFrame( 0, 0, (int)labelWidth, 20 );
            rootAsSeq->AddSeqWidget(label, (int)labelWidth + 5);

            button = new pawsButton();
            button->SetNotify(this);
            button->SetUpImage("Checkbox Off");
            button->SetDownImage("Checkbox On");
            button->SetToggle(true);
            button->Show();
            button->SetState(GetCameraSetting(subtreeRoot->GetName(), currCamMode) > 0.0f);
            button->SetRelativeFrame( (int)labelWidth+5, 0, 16, 16 );
            rootAsSeq->AddSeqWidget(button);
        }
        else if (!strcmp(subtreeRoot->GetAttr("type"), "real"))
        {
            label = new pawsTextBox();
            label->SetText(subtreeRoot->GetName());
            label->SetColour(0xffffff);
            label->Show();
            label->SetRelativeFrame( 0, 0, (int)labelWidth, 20 );
            rootAsSeq->AddSeqWidget(label, (int)labelWidth + 5);

            pawsSpinBox* spin = new pawsSpinBox();
            spin->SetRelativeFrame( (int)labelWidth+5, 0, (int)inputWidth, 20 );
            csString val("");
            val += GetCameraSetting(subtreeRoot->GetName(), currCamMode);
            csString pos("right");
            spin->ManualSetup(val, atof(subtreeRoot->GetAttr("min")), atof(subtreeRoot->GetAttr("max")), atof(subtreeRoot->GetAttr("inc")), pos);
            spin->Show();
            rootAsSeq->AddSeqWidget(spin, (int)inputWidth);
        }
    }

    child = subtreeRoot->GetFirstChild();
    while (child != NULL)
    {
        CreateTreeWidgets(child);
        child = child->GetNextSibling();
    }
}

void pawsConfigCamera::SetLabels(pawsTreeNode * subtreeRoot)
{
    pawsSeqTreeNode * rootAsSeq;
    pawsTreeNode * child;
    csString boundAction;

    rootAsSeq = dynamic_cast<pawsSeqTreeNode*> (subtreeRoot);
    if (rootAsSeq != NULL)
    {
        if (!strcmp(subtreeRoot->GetAttr("type"), "onoff"))
        {
            pawsButton* actionButton = dynamic_cast<pawsButton*> (rootAsSeq->GetSeqWidget(1));
            assert(actionButton);
            actionButton->SetState(true);
        }
        else if (!strcmp(subtreeRoot->GetAttr("type"), "real"))
        {
            pawsSpinBox* spin = dynamic_cast<pawsSpinBox*> (rootAsSeq->GetSeqWidget(1));
            spin->SetValue(0);
        }
    }

    child = subtreeRoot->GetFirstChild();
    while (child != NULL)
    {
        SetLabels(child);
        child = child->GetNextSibling();
    }
}

void pawsConfigCamera::SetCameraValues(pawsTreeNode * subtreeRoot)
{
    pawsButton * button;
    pawsSeqTreeNode * rootAsSeq;
    pawsTreeNode * child;

    rootAsSeq = dynamic_cast<pawsSeqTreeNode*> (subtreeRoot);
    if (rootAsSeq != NULL)
    {
        csString cammode = subtreeRoot->GetAttr("cammode");
        int currCamMode = -1;
        if (cammode == "First Person")
        {
            currCamMode = psCamera::CAMERA_FIRST_PERSON;
        }
        else if (cammode == "Third Person Follow")
        {
            currCamMode = psCamera::CAMERA_THIRD_PERSON;
        }
        else if (cammode == "Third Person M64")
        {
            currCamMode = psCamera::CAMERA_M64_THIRD_PERSON;
        }
        else if (cammode == "Third Person Lara")
        {
            currCamMode = psCamera::CAMERA_LARA_THIRD_PERSON;
        }
        else if (cammode == "Free Rotation")
        {
            currCamMode = psCamera::CAMERA_FREE;
        }

        if (!strcmp(subtreeRoot->GetAttr("type"), "onoff"))
        {
            button = dynamic_cast<pawsButton*> (rootAsSeq->GetSeqWidget(1));
            button->SetState(GetCameraSetting(subtreeRoot->GetName(), currCamMode) > 0.0f);
        }
        else if (!strcmp(subtreeRoot->GetAttr("type"), "real"))
        {
            pawsSpinBox* spin = dynamic_cast<pawsSpinBox*> (rootAsSeq->GetSeqWidget(1));
            spin->SetValue(GetCameraSetting(subtreeRoot->GetName(), currCamMode));
        }
    }

    child = subtreeRoot->GetFirstChild();
    while (child != NULL)
    {
        SetCameraValues(child);
        child = child->GetNextSibling();
    }
}


bool pawsConfigCamera::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* /*widget*/)
{
    dirty = true;
    return true;
}

float pawsConfigCamera::GetCameraSetting(const csString& settingName, int mode) const
{
    if (settingName == "Use Collision Detection")
        return (psengine->GetPSCamera()->CheckCameraCD() ? 1 : -1);
    else if(settingName == "Use NPC Chat Camera")
        return psengine->GetPSCamera()->GetUseNPCCam();
    else if (settingName == "Transition Threshold")
        return psengine->GetPSCamera()->GetTransitionThreshold();
    else if (settingName == "Transition Spring Length")
        return psengine->GetPSCamera()->GetSpringLength(psCamera::CAMERA_TRANSITION);
    else if (settingName == "Transition Spring Coefficient")
        return psengine->GetPSCamera()->GetSpringCoef(psCamera::CAMERA_TRANSITION);
    else if (settingName == "Transition Dampening Coefficient")
        return psengine->GetPSCamera()->GetDampeningCoef(psCamera::CAMERA_TRANSITION);
    else if (settingName == "Collision Spring Length")
        return psengine->GetPSCamera()->GetSpringLength(psCamera::CAMERA_COLLISION);
    else if (settingName == "Collision Spring Coefficient")
        return psengine->GetPSCamera()->GetSpringCoef(psCamera::CAMERA_COLLISION);
    else if (settingName == "Collision Dampening Coefficient")
        return psengine->GetPSCamera()->GetDampeningCoef(psCamera::CAMERA_COLLISION);
    else if (settingName == "Spring Length")
        return psengine->GetPSCamera()->GetSpringLength(mode);
    else if (settingName == "Spring Coefficient")
        return psengine->GetPSCamera()->GetSpringCoef(mode);
    else if (settingName == "Dampening Coefficient")
        return psengine->GetPSCamera()->GetDampeningCoef(mode);
    else if (settingName == "Starting Pitch")
        return psengine->GetPSCamera()->GetDefaultPitch(mode);
    else if (settingName == "Starting Yaw")
        return psengine->GetPSCamera()->GetDefaultYaw(mode);
    else if (settingName == "Camera Distance")
        return 0.1*(int)(10.0*psengine->GetPSCamera()->GetDistance(mode));
    else if (settingName == "Min Camera Distance")
        return psengine->GetPSCamera()->GetMinDistance(mode);
    else if (settingName == "Max Camera Distance")
        return psengine->GetPSCamera()->GetMaxDistance(mode);
    else if (settingName == "Turning Speed")
        return psengine->GetPSCamera()->GetTurnSpeed(mode);
    else if (settingName == "Swing Coefficient")
        return psengine->GetPSCamera()->GetSwingCoef(mode);

    // unrecognized setting
    return -1.0f;
}

void pawsConfigCamera::SetCameraSetting(const csString& settingName, float value, int mode) const
{
    if (settingName == "Use Collision Detection")
        psengine->GetPSCamera()->SetCameraCD(value > 0.0f);
    else if(settingName == "Use NPC Chat Camera")
        psengine->GetPSCamera()->SetUseNPCCam(value > 0.0f);
    else if (settingName == "Transition Threshold")
        psengine->GetPSCamera()->SetTransitionThreshold(value);
    else if (settingName == "Transition Spring Length")
        psengine->GetPSCamera()->SetSpringLength(value, psCamera::CAMERA_TRANSITION);
    else if (settingName == "Transition Spring Coefficient")
        psengine->GetPSCamera()->SetSpringCoef(value, psCamera::CAMERA_TRANSITION);
    else if (settingName == "Transition Dampening Coefficient")
        psengine->GetPSCamera()->SetDampeningCoef(value, psCamera::CAMERA_TRANSITION);
    else if (settingName == "Collision Spring Length")
        psengine->GetPSCamera()->SetSpringLength(value, psCamera::CAMERA_COLLISION);
    else if (settingName == "Collision Spring Coefficient")
        psengine->GetPSCamera()->SetSpringCoef(value, psCamera::CAMERA_COLLISION);
    else if (settingName == "Collision Dampening Coefficient")
        psengine->GetPSCamera()->SetDampeningCoef(value, psCamera::CAMERA_COLLISION);
    else if (settingName == "Spring Length")
        psengine->GetPSCamera()->SetSpringLength(value, mode);
    else if (settingName == "Spring Coefficient")
        psengine->GetPSCamera()->SetSpringCoef(value, mode);
    else if (settingName == "Dampening Coefficient")
        psengine->GetPSCamera()->SetDampeningCoef(value, mode);
    else if (settingName == "Starting Pitch")
        psengine->GetPSCamera()->SetDefaultPitch(value, mode);
    else if (settingName == "Starting Yaw")
        psengine->GetPSCamera()->SetDefaultYaw(value, mode);
    else if (settingName == "Camera Distance")
        psengine->GetPSCamera()->SetDistance(0.1*(int)(10.0*value), mode);
    else if (settingName == "Min Camera Distance")
        psengine->GetPSCamera()->SetMinDistance(value, mode);
    else if (settingName == "Max Camera Distance")
        psengine->GetPSCamera()->SetMaxDistance(value, mode);
    else if (settingName == "Turning Speed")
        psengine->GetPSCamera()->SetTurnSpeed(value, mode);
    else if (settingName == "Swing Coefficient")
        psengine->GetPSCamera()->SetSwingCoef(value, mode);
}

void pawsConfigCamera::SaveConfigByTreeNodes(pawsTreeNode * subtreeRoot)
{
    pawsSeqTreeNode * rootAsSeq;

    rootAsSeq = dynamic_cast<pawsSeqTreeNode*> (subtreeRoot);
    if (rootAsSeq != NULL)
    {
        csString cammode = subtreeRoot->GetAttr("cammode");
        int currCamMode = -1;
        if (cammode == "First Person")
            currCamMode = psCamera::CAMERA_FIRST_PERSON;
        else if (cammode == "Third Person Follow")
            currCamMode = psCamera::CAMERA_THIRD_PERSON;
        else if (cammode == "Third Person M64")
            currCamMode = psCamera::CAMERA_M64_THIRD_PERSON;
        else if (cammode == "Third Person Lara")
            currCamMode = psCamera::CAMERA_LARA_THIRD_PERSON;
        else if (cammode == "Free Rotation")
            currCamMode = psCamera::CAMERA_FREE;

        if (!strcmp(subtreeRoot->GetAttr("type"), "onoff"))
        {
            pawsButton* button = dynamic_cast <pawsButton*> (rootAsSeq->GetSeqWidget(1));
            assert(button);

            SetCameraSetting(subtreeRoot->GetName(), (button->GetState() ? 1 : -1), currCamMode);
        }
        else if (!strcmp(subtreeRoot->GetAttr("type"), "real"))
        {
            pawsSpinBox* spin = dynamic_cast <pawsSpinBox*> (rootAsSeq->GetSeqWidget(1));
            assert(spin);

            SetCameraSetting(subtreeRoot->GetName(), spin->GetValue(), currCamMode);
        }
    }

    pawsTreeNode *child = subtreeRoot->GetFirstChild();
    while (child != NULL)
    {
        SaveConfigByTreeNodes(child);
        child = child->GetNextSibling();
    }
}
