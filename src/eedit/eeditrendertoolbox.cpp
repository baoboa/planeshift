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
#include "eeditrendertoolbox.h"
#include "eeditglobals.h"

#include "paws/pawsmanager.h"
#include "paws/pawsbutton.h"
#include "paws/pawstextbox.h"
#include "paws/pawslistbox.h"

#include "effects/pseffectanchor.h"
#include "effects/pseffectobj.h"

//---------------------------------------------------------------------------------------

// @@@ Duplicated in eeditpartlisttoolbox.cpp, refactor to use some lib.
static pawsListBoxRow* NewRow (size_t& a, pawsListBox* box, pawsTextBox** col1, pawsTextBox** col2 = 0, pawsTextBox** col3 = 0)
{
    box->NewRow(a);
    pawsListBoxRow * row = box->GetRow(a);
    if (!row) return 0;

    *col1 = (pawsTextBox *)row->GetColumn(0);
    if (!*col1) return 0;

    if (col2)
    {
        *col2 = (pawsTextBox *)row->GetColumn(1);
        if (!*col2) return 0;
    }

    if (col3)
    {
        *col3 = (pawsTextBox *)row->GetColumn(2);
        if (!*col3) return 0;
    }

    ++a;
    return row;
}

//---------------------------------------------------------------------------------------

EEditRenderToolbox::EEditRenderToolbox() : scfImplementationType(this)
{
    effectManager = 0;
}

EEditRenderToolbox::~EEditRenderToolbox()
{
}

void EEditRenderToolbox::Update(unsigned int elapsed)
{
}

size_t EEditRenderToolbox::GetType() const
{
    return T_RENDER;
}

const char * EEditRenderToolbox::GetName() const
{
    return "Render";
}

psEffect* EEditRenderToolbox::GetCurrentEffect()
{
    if (!effectManager) return 0;
    return effectManager->FindEffect(effectName);
}

void EEditRenderToolbox::LoadEffect(psEffectManager* mgr, const char* effectName)
{
    EEditRenderToolbox::effectManager = mgr;
    EEditRenderToolbox::effectName = effectName;
    psEffect* eff = GetCurrentEffect();
    if (!eff) return;

    list->Clear();
    list->Select(0);
    list2->Clear();
    list2->Select(0);

    size_t a = 0;
    pawsTextBox* col1, * col2;

    for (size_t i = 0 ; i < eff->GetAnchorCount() ; i++)
    {
        psEffectAnchor* anchor = eff->GetAnchor(i);
	NewRow(a, list, &col1, &col2);
	col1->SetText("Anchor");
	col2->SetText(anchor->GetName());
    }
    for (size_t i = 0 ; i < eff->GetObjCount() ; i++)
    {
        psEffectObj* obj = eff->GetObj(i);
	NewRow(a, list, &col1, &col2);
	col1->SetText("Obj");
	col2->SetText(obj->GetName());
    }
}

void EEditRenderToolbox::FillAnchor(const char* anchorName)
{
    list2->Clear();
    list2->Select(0);

    psEffect* eff = GetCurrentEffect();
    if (!eff) return;
    psEffectAnchor* anchor = eff->FindAnchor(anchorName);
    if (!anchor) return;
    pawsTextBox* col1, * col2;

    csString fmt;

    size_t a = 0;
    NewRow(a, list2, &col1, &col2);
    col1->SetText("Dir");
    col2->SetText(anchor->GetDirectionType());
    //NewRow(a, list2, &col1, &col2);
    //col1->SetText("ASize");
    //fmt.Format("%g", anchor->GetAnimGetSize());
    //col2->SetText(fmt);

    size_t kfcount = anchor->GetKeyFrameCount ();
    for (size_t i = 0 ; i < kfcount ; i++)
    {
        psEffectAnchorKeyFrame* kf = anchor->GetKeyFrame (i);
        NewRow(a, list2, &col1, &col2);
        fmt.Format("KF%zu", i);
        col1->SetText(fmt);
        fmt = "";
        bool px, py, pz, ttx, tty, ttz;
        px = kf->IsActionSet (psEffectAnchorKeyFrame::KA_POS_X);
        py = kf->IsActionSet (psEffectAnchorKeyFrame::KA_POS_Y);
        pz = kf->IsActionSet (psEffectAnchorKeyFrame::KA_POS_Z);
        ttx = kf->IsActionSet (psEffectAnchorKeyFrame::KA_TOTARGET_X);
        tty = kf->IsActionSet (psEffectAnchorKeyFrame::KA_TOTARGET_Y);
        ttz = kf->IsActionSet (psEffectAnchorKeyFrame::KA_TOTARGET_Z);
        if (px || py || pz)
        {
            fmt = "pos(";
            if (px) fmt += "X"; else fmt += "_";
            if (py) fmt += "Y"; else fmt += "_";
            if (pz) fmt += "Z"; else fmt += "_";
            fmt += ")";
        }
        if (ttx || tty || ttz)
        {
            if (px || py || pz) fmt += ' ';
            fmt = "tgt(";
            if (ttx) fmt += "X"; else fmt += "_";
            if (tty) fmt += "Y"; else fmt += "_";
            if (ttz) fmt += "Z"; else fmt += "_";
            fmt += ")";
        }
            col2->SetText(fmt);
    }
}

void EEditRenderToolbox::FillObj(const char* objName)
{
    list2->Clear();
    list2->Select(0);

    psEffect* eff = GetCurrentEffect();
    if (!eff) return;
    psEffectObj* obj = eff->FindObj(objName);
    if (!obj) return;
    pawsTextBox* col1, * col2;

    csString fmt;
    size_t a = 0;

    NewRow(a, list2, &col1, &col2);
    col1->SetText("Birth");
    fmt.Format("%g", obj->GetBirth ());
    col2->SetText(fmt);

    NewRow(a, list2, &col1, &col2);
    col1->SetText("Death");
    fmt.Format("%d", obj->GetKillTime ());
    col2->SetText(fmt);

    NewRow(a, list2, &col1, &col2);
    col1->SetText("Attach");
    col2->SetText(obj->GetAnchorName ().GetData ());

    NewRow(a, list2, &col1, &col2);
    col1->SetText("Attach");
    csString priority = engine->GetRenderPriorityName (obj->GetRenderPriority ());
    col2->SetText(priority.GetData ());

    csZBufMode zmode = obj->GetZBufMode ();
    NewRow(a, list2, &col1, &col2);
    col1->SetText("ZBufMode");
    if (zmode == CS_ZBUF_NONE) col2->SetText("znone");
    else if (zmode == CS_ZBUF_FILL) col2->SetText("zfill");
    else if (zmode == CS_ZBUF_USE) col2->SetText("zuse");
    else if (zmode == CS_ZBUF_TEST) col2->SetText("ztest");
    else col2->SetText("?");

    unsigned int mixmode = obj->GetMixMode ();
    NewRow(a, list2, &col1, &col2);
    col1->SetText("MixMode");
    if (mixmode == CS_FX_COPY) col2->SetText ("copy");
    else if (mixmode == CS_FX_MULTIPLY) col2->SetText ("multiply");
    else if (mixmode == CS_FX_MULTIPLY2) col2->SetText ("multiply2");
    else if (mixmode == CS_FX_ALPHA) col2->SetText ("alpha");
    else if (mixmode == CS_FX_TRANSPARENT) col2->SetText ("transparent");
    else if (mixmode == CS_FX_DESTALPHAADD) col2->SetText ("destalphaadd");
    else if (mixmode == CS_FX_SRCALPHAADD) col2->SetText ("srcalphaadd");
    else if (mixmode == CS_FX_PREMULTALPHA) col2->SetText ("premultalpha");
    else if (mixmode == CS_FX_ADD) col2->SetText ("add");
    else col2->SetText ("?");

    NewRow(a, list2, &col1, &col2);
    col1->SetText("Dir");
    int dir = obj->GetDirection ();
    if (dir == psEffectObj::DT_TARGET) col2->SetText ("target");
    else if (dir == psEffectObj::DT_ORIGIN) col2->SetText ("origin");
    else if (dir == psEffectObj::DT_TO_TARGET) col2->SetText ("totarget");
    else if (dir == psEffectObj::DT_CAMERA) col2->SetText ("camera");
    else if (dir == psEffectObj::DT_BILLBOARD) col2->SetText ("billboard");
    else col2->SetText ("none");

    // Keyframes...
    /*size_t kfcount = obj->GetKeyFrameCount ();
    for (size_t i = 0 ; i < kfcount ; i++)
    {
        psEffectObjKeyFrame* kf = obj->GetKeyFrame (i);
	// @@@ TODO
    }*/
}

void EEditRenderToolbox::OnListAction(pawsListBox* selected, int status)
{
    if (selected == list)
    {
        size_t num = list->GetSelectedRowNum();
	pawsListBoxRow * row = list->GetRow(num);
	pawsTextBox* col1 = (pawsTextBox *)row->GetColumn(0);
	pawsTextBox* col2 = (pawsTextBox *)row->GetColumn(1);
	csString anchor = "Anchor";
	if (anchor == col1->GetText())
	    FillAnchor(col2->GetText());
	else
	    FillObj(col2->GetText());
    }
}

bool EEditRenderToolbox::PostSetup()
{
    renderButton = (pawsButton *) FindWidget("render");     CS_ASSERT(renderButton);
    loadButton   = (pawsButton *) FindWidget("load");       CS_ASSERT(loadButton);
    pauseButton  = (pawsButton *) FindWidget("pause");      CS_ASSERT(pauseButton);
    cancelButton = (pawsButton *) FindWidget("stop");       CS_ASSERT(cancelButton);
    list         = (pawsListBox *) FindWidget("list");      CS_ASSERT(list);
    list2        = (pawsListBox *) FindWidget("list2");     CS_ASSERT(list2);
    return true;
}

bool EEditRenderToolbox::OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget)
{
    if (widget == renderButton)
    {
        editApp->RenderCurrentEffect();
        return true;
    }
    else if (widget == loadButton)
    {
        editApp->ReloadCurrentEffect();
        return true;
    }
    else if (widget == pauseButton)
    {
        if (editApp->TogglePauseEffect())
            pauseButton->SetText("Resume");
        else
            pauseButton->SetText("Pause");
    }
    else if (widget == cancelButton)
    {
        editApp->CancelEffect();
        return true;
    }
    return false;
}

