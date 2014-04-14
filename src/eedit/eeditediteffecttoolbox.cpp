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
#include "eeditediteffecttoolbox.h"
#include "eeditglobals.h"

#include "paws/pawsmanager.h"
#include "paws/pawstree.h"
#include "paws/pawstextbox.h"
#include "paws/pawsspinbox.h"

#include "effects/pseffect.h"

#include "effects/pseffectanchor.h"
#include "effects/pseffectanchorbasic.h"
#include "effects/pseffectanchorspline.h"
#include "effects/pseffectanchorsocket.h"

#include "effects/pseffectmanager.h"
#include "effects/pseffectobj.h"

#include "eeditinputboxmanager.h"


// tree node colours
#define NODE_ANCHOR_ROOT_COLOUR                 0xff00ff
#define NODE_ANCHOR_COLOUR                      0xff33cc
#define NODE_ANCHOR_ATTR_COLOUR                 0xff6699
#define NODE_ANCHOR_KEYFRAME_ROOT_COLOUR        0xff9966 
#define NODE_ANCHOR_KEYFRAME_COLOUR             0xffcc33
#define NODE_ANCHOR_KEYFRAME_ATTR_COLOUR        0xffff00


#define NODE_OBJ_ROOT_COLOUR                    0x00ff00
#define NODE_OBJ_COLOUR                         0x33cc33
#define NODE_OBJ_ATTR_COLOUR                    0x669966
#define NODE_OBJ_KEYFRAME_ROOT_COLOUR           0x996699
#define NODE_OBJ_KEYFRAME_COLOUR                0xcc33cc
#define NODE_OBJ_ANCHOR_KEYFRAME_ATTR_COLOUR    0xff00ff

#define LABEL_WIDTH     75
#define FIELD1_WIDTH    130
#define FIELD2_WIDTH    110
#define FIELD3_WIDTH    90


const char OWNER_NONE = ' ';
const char OWNER_ANCHOR = 'a';
const char OWNER_OBJECT = 'o';

const char ATTR_NONE = ' ';
const char ATTR_DIR = 'd';
const char ATTR_DIST_SCALE = 't';
const char ATTR_SOCKET = 's';
const char ATTR_KEYTIME = 'y';
const char ATTR_KEYFRAME = 'k';


EEditEditEffectToolbox::EEditEditEffectToolbox() : scfImplementationType(this)
{
    currEffect = 0;
}

EEditEditEffectToolbox::~EEditEditEffectToolbox()
{
    PawsManager::GetSingleton().UnSubscribe(this);
}

void EEditEditEffectToolbox::LoadEffect(psEffect * effect)
{
    return; // TODO: FIX THIS
    currEffect = effect;
    currEffectName = effect->GetName();
    if (effect)
    {
        RebuildTreeRoot();

        for (size_t a=0; a<effect->GetAnchorCount(); ++a)
        {
            psEffectAnchor * anchor = effect->GetAnchor(a);
            if (anchor)
                LoadEffectAnchor(anchor, a);
        }
        effectAnchorsRoot->CollapseAll();
        
        for (size_t a=0; a<effect->GetObjCount(); ++a)
        {
            psEffectObj * obj = effect->GetObj(a);
            if (obj)
                LoadEffectObject(obj, a);
        }
        effectObjsRoot->CollapseAll();
    }
}

void EEditEditEffectToolbox::Update(unsigned int elapsed)
{
}

size_t EEditEditEffectToolbox::GetType() const
{
    return T_EDIT_EFFECT;
}

const char * EEditEditEffectToolbox::GetName() const
{
    return "Edit Effect";
}

csRef<iDocumentNode> ParseString(const csString & str, const csString & topNodeName);

bool EEditEditEffectToolbox::PostSetup()
{
    effectItemsTree = (pawsTree *)FindWidget("EffectItemsTree");    CS_ASSERT(effectItemsTree);
    if (!effectItemsTree)
    {
        printf("Warning: Edit Effect toolbox does not have an EffectItemsTree defined!\n");
        return false;
    }
    effectItemsTree->SetNotify(this);
    effectItemsTree->SetScrollBars(false, true);

    return true;
}

bool EEditEditEffectToolbox::OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget)
{
    return false;
}

bool EEditEditEffectToolbox::OnSelected( pawsWidget* widget)
{
    effectItemsTree->Deselect();
    return false;
}

void EEditEditEffectToolbox::UpdateEffectData(psEffect * effect, char owner, char attr, int index, int index2, int index3, PAWSData & data)
{
    if (owner == OWNER_ANCHOR && attr == ATTR_DIR)
    {
        psEffectAnchor * anchor = effect->GetAnchor(index);
        anchor->SetDirectionType(data.GetStr());
    }

    if (owner == OWNER_ANCHOR && attr == ATTR_SOCKET)
    {
        psEffectAnchorSocket * sockAnchor = dynamic_cast<psEffectAnchorSocket *>(effect->GetAnchor(index));
        sockAnchor->SetSocket(csString(data.GetStr()));
        return;
    }

    if (owner == OWNER_ANCHOR && attr == ATTR_KEYFRAME)
    {
        psEffectAnchor * anchor = effect->GetAnchor(index);
        anchor->GetKeyFrame(index2)->actions[index3] = data.GetFloat();
        anchor->FillInLerps();

        psEffectAnchorSpline * splineAnchor = dynamic_cast<psEffectAnchorSpline *>(anchor);
        if (splineAnchor)
            splineAnchor->PostSetup();
    }    

    if (owner == OWNER_ANCHOR && attr == ATTR_KEYTIME)
    {
        int time = data.GetInt();
        psEffectAnchor * anchor = effect->GetAnchor(index);
        anchor->GetKeyFrame(index2)->time = time;

        if (index2 == int(anchor->GetKeyFrameCount()-1))
        {
            if (anchor->GetKeyFrameCount() <= 1)
                anchor->SetAnimLength(10);
            if (time >= (int)anchor->GetKeyFrame(index2-1)->time)
                anchor->SetAnimLength(time);
        }

        anchor->FillInLerps();

        psEffectAnchorSpline * splineAnchor = dynamic_cast<psEffectAnchorSpline *>(anchor);
        if (splineAnchor)
            splineAnchor->PostSetup();
    }
}

void EEditEditEffectToolbox::OnUpdateData(const char *dataname, PAWSData & data)
{
    char owner = dataname[0];
    char attr = dataname[1];

    int index = atoi(&dataname[3]);
    int index2 = -1;
    int index3 = -1;
    size_t len = strlen(dataname);

    for (size_t a=3; a<len; ++a)
    {
        if (dataname[a] == '_')
        {
            if (index2 == -1)
                index2 = atoi(&dataname[a+1]);
            else
                index3 = atoi(&dataname[a+1]);
        }
    }

    UpdateEffectData(currEffect, owner, attr, index, index2, index3, data);
    psEffect * effect = editApp->GetEffectManager()->FindEffect(editApp->GetCurrEffectID());
    if (effect)
        UpdateEffectData(effect, owner, attr, index, index2, index3, data);
}

void EEditEditEffectToolbox::RebuildTreeRoot()
{
    effectItemsTree->Clear();
    
    effectAnchorsRoot = new pawsSimpleTreeNode();
    effectItemsTree->InsertChild("", effectAnchorsRoot, "");
    effectAnchorsRoot->SetColour(NODE_ANCHOR_ROOT_COLOUR);
    effectAnchorsRoot->Set(showLabel, false, "", "Anchors");
    effectAnchorsRoot->SetName("AnchorsRoot");
    
    effectObjsRoot = new pawsSimpleTreeNode();
    effectItemsTree->InsertChild("", effectObjsRoot, "");
    effectObjsRoot->SetColour(NODE_OBJ_ROOT_COLOUR);
    effectObjsRoot->Set(showLabel, false, "", "Objects");
    effectObjsRoot->SetName("ObjectsRoot");
}

void EEditEditEffectToolbox::BuildAnchorNode(pawsSimpleTreeNode * node, psEffectAnchor * anchor, 
                                              unsigned char anchorType)
{
    csString anchorLabel = anchor->GetName();
    switch (anchorType)
    {
    case AT_BASIC:
        anchorLabel += " [basic]";
        break;
    case AT_SPLINE:
        anchorLabel += " [spline]";
        break;
    case AT_SOCKET:
        anchorLabel += " [socket]";
        break;
    }
    node->Set(showLabel, false, "", anchorLabel);
    node->SetColour(NODE_ANCHOR_COLOUR);
    node->SetName(anchor->GetName());  
} 

void EEditEditEffectToolbox::BuildDirectionNode(pawsSeqTreeNode * node, psEffectAnchor * anchor, size_t index)
{
    pawsTextBox * label;
    label = new pawsTextBox();
    label->SetSize(LABEL_WIDTH, 24);
    label->SetText("Direction:");
    label->SetColour(NODE_ANCHOR_ATTR_COLOUR);
    label->Show();
    label->SetParent(node);
    label->SetRelativeFrame(0, 5, LABEL_WIDTH, 24);
    node->AddSeqWidget(label, LABEL_WIDTH + 10);

    pawsEditTextBox * field;
    field = new pawsEditTextBox();
    field->SetSize(FIELD1_WIDTH, 20);
    field->SetBackground("Shaded");
    field->SetBackgroundAlpha(75);
    field->SetColour(0xffffff);
    field->Show();
    field->SetParent(node);
    field->SetRelativeFrame(LABEL_WIDTH + 10, 2, FIELD1_WIDTH, 20);
    node->AddSeqWidget(field, FIELD1_WIDTH);

    char header[3];
    header[0] = OWNER_ANCHOR;
    header[1] = ATTR_DIR;
    header[2] = '\0';
    csString varName; varName.Format("%s_%zu", header, index);

    PawsManager::GetSingleton().Subscribe(varName, field);
    PawsManager::GetSingleton().Subscribe(varName, this);
    PawsManager::GetSingleton().Publish(varName, anchor->GetDirectionType());

    node->SetColour(NODE_ANCHOR_ATTR_COLOUR);
    node->SetName(anchor->GetName() + csString("_direction"));
}

void EEditEditEffectToolbox::BuildSocketNode(pawsSeqTreeNode * node, psEffectAnchorSocket * anchor, size_t index)
{
    pawsTextBox * label;
    label = new pawsTextBox();
    label->SetSize(LABEL_WIDTH, 24);
    label->SetText("Socket:");
    label->SetColour(NODE_ANCHOR_ATTR_COLOUR);
    label->Show();
    label->SetParent(node);
    label->SetRelativeFrame(0, 5, LABEL_WIDTH, 24);
    node->AddSeqWidget(label, LABEL_WIDTH + 10);

    pawsEditTextBox * field;
    field = new pawsEditTextBox();
    field->SetSize(FIELD1_WIDTH, 20);
    field->SetBackground("Shaded");
    field->SetBackgroundAlpha(75);
    field->SetColour(0xffffff);
    field->Show();
    field->SetParent(node);
    field->SetRelativeFrame(LABEL_WIDTH + 10, 2, FIELD1_WIDTH, 20);
    node->AddSeqWidget(field, FIELD1_WIDTH);

    char header[3];
    header[0] = OWNER_ANCHOR;
    header[1] = ATTR_SOCKET;
    header[2] = '\0';
    csString varName; varName.Format("%s_%zu", header, index);

    PawsManager::GetSingleton().Subscribe(varName, field);
    PawsManager::GetSingleton().Subscribe(varName, this);
    PawsManager::GetSingleton().Publish(varName, anchor->GetSocketName());

    node->SetColour(NODE_ANCHOR_ATTR_COLOUR);
    node->SetName(anchor->GetName() + csString("_socket"));
}

void EEditEditEffectToolbox::BuildAnchorKeyFrameNode(pawsSeqTreeNode * node, psEffectAnchor * anchor, size_t index, 
                                                     size_t keyIndex)
{
    psEffectAnchorKeyFrame * key = anchor->GetKeyFrame(keyIndex);
    
    pawsTextBox * label;
    label = new pawsTextBox();
    label->SetSize(LABEL_WIDTH, 24);
    label->SetText("Time:");
    label->SetColour(NODE_ANCHOR_KEYFRAME_COLOUR);
    label->Show();
    label->SetParent(node);
    label->SetRelativeFrame(0, 5, LABEL_WIDTH, 24);
    node->AddSeqWidget(label, LABEL_WIDTH + 10);

    pawsEditTextBox * field;
    field = new pawsEditTextBox();
    field->SetSize(FIELD2_WIDTH, 20);
    field->SetBackground("Shaded");
    field->SetBackgroundAlpha(75);
    field->SetColour(0xffffff);
    field->Show();
    field->SetParent(node);
    field->SetRelativeFrame(LABEL_WIDTH + 10, 2, FIELD2_WIDTH, 20);
    node->AddSeqWidget(field, FIELD2_WIDTH);

    char header[3];
    header[0] = OWNER_ANCHOR;
    header[1] = ATTR_KEYTIME;
    header[2] = '\0';
    csString varName; varName.Format("%s_%zu_%zu", header, index, keyIndex);

    PawsManager::GetSingleton().Subscribe(varName, field);
    PawsManager::GetSingleton().Subscribe(varName, this);
    PawsManager::GetSingleton().Publish(varName, (int)key->time);

    node->SetColour(NODE_ANCHOR_KEYFRAME_COLOUR);
    csString nam; nam.Format("%s_key%zu", anchor->GetName().GetData(), keyIndex);
    node->SetName(nam);

    for (size_t a=1; a<psEffectAnchorKeyFrame::KA_COUNT; ++a)
    {
        if (key->IsActionSet(a))
        {
            pawsSeqTreeNode * actionNode = new pawsSeqTreeNode();
            node->InsertChild(actionNode);

            csString lbl; lbl.Format("%s:", key->GetActionName(a));

            pawsTextBox * label;
            label = new pawsTextBox();
            label->SetSize(LABEL_WIDTH, 24);
            label->SetText(lbl);
            label->SetColour(NODE_ANCHOR_KEYFRAME_COLOUR);
            label->Show();
            label->SetParent(node);
            label->SetRelativeFrame(0, 5, LABEL_WIDTH, 24);
            actionNode->AddSeqWidget(label, LABEL_WIDTH + 10);

            pawsEditTextBox * field;
            field = new pawsEditTextBox();
            field->SetSize(FIELD3_WIDTH, 20);
            field->SetBackground("Shaded");
            field->SetBackgroundAlpha(75);
            field->SetColour(0xffffff);
            field->Show();
            field->SetParent(node);
            field->SetRelativeFrame(LABEL_WIDTH + 10, 2, FIELD3_WIDTH, 20);
            actionNode->AddSeqWidget(field, FIELD3_WIDTH);

            char header[3];
            header[0] = OWNER_ANCHOR;
            header[1] = ATTR_KEYFRAME;
            header[2] = '\0';
            csString varName; varName.Format("%s_%zu_%zu_%zu", header, index, keyIndex, a);

            PawsManager::GetSingleton().Subscribe(varName, field);
            PawsManager::GetSingleton().Subscribe(varName, this);
            PawsManager::GetSingleton().Publish(varName, key->actions[a]);

            actionNode->SetColour(NODE_ANCHOR_KEYFRAME_ATTR_COLOUR);
            actionNode->SetName(nam);    
        }
    }
}

void EEditEditEffectToolbox::LoadEffectAnchor(psEffectAnchor * anchor, size_t index)
{
    unsigned char anchorType = AT_NONE;
    if (dynamic_cast <psEffectAnchorBasic *> (anchor))
        anchorType = AT_BASIC;
    else if (dynamic_cast <psEffectAnchorSpline *> (anchor))
        anchorType = AT_SPLINE;
    else if (dynamic_cast <psEffectAnchorSocket *> (anchor))
        anchorType = AT_SOCKET;
    
    // main anchor node
    pawsSimpleTreeNode * node = new pawsSimpleTreeNode();
    effectAnchorsRoot->InsertChild(node);
    BuildAnchorNode(node, anchor, anchorType);

    // abs dir
    pawsSeqTreeNode * dirNode = new pawsSeqTreeNode();
    node->InsertChild(dirNode);
    BuildDirectionNode(dirNode, anchor, index);
    
    // socket specific attributes
    if (anchorType == AT_SOCKET)
    {
        psEffectAnchorSocket * socketAnchor = dynamic_cast <psEffectAnchorSocket *> (anchor);
        
        // socket name
        pawsSeqTreeNode * socketNode = new pawsSeqTreeNode();
        node->InsertChild(socketNode);
        BuildSocketNode(socketNode, socketAnchor, index);
    }
    
    // keyframes
    pawsSimpleTreeNode * keyFramesRoot = new pawsSimpleTreeNode();
    node->InsertChild(keyFramesRoot);
    keyFramesRoot->Set(showLabel, false, "", "KeyFrames");
    keyFramesRoot->SetColour(NODE_ANCHOR_KEYFRAME_ROOT_COLOUR);
    csString keyFrameName = anchor->GetName() + "_keyframes";
    keyFramesRoot->SetName(keyFrameName);
    
    for (size_t b=0; b<anchor->GetKeyFrameCount(); ++b)
    {
        pawsSeqTreeNode * keyNode = new pawsSeqTreeNode();
        keyFramesRoot->InsertChild(keyNode);
        BuildAnchorKeyFrameNode(keyNode, anchor, index, b);
    }
}

void EEditEditEffectToolbox::LoadEffectObject(psEffectObj * obj, size_t index)
{
}       
