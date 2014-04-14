/*
 * pawsLoadWindow.h - Author: Andrew Craig
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

#include <iutil/objreg.h>

#include "../globals.h"

#include "paws/pawsmanager.h"
#include "pawsloading.h"
#include "paws/pawstextbox.h"

#include "net/cmdhandler.h"
#include "net/messages.h"
#include "net/clientmsghandler.h"
#include "pscharcontrol.h"

pawsLoadWindow::~pawsLoadWindow()
{
    psengine->GetMsgHandler()->Unsubscribe(this,MSGTYPE_MOTD);
}

void pawsLoadWindow::AddText( const char* newText )
{
    if(loadingText)
        loadingText->ReplaceLastMessage( newText );
}

void pawsLoadWindow::Clear()
{
    if(loadingText)
        loadingText->Clear();
}

bool pawsLoadWindow::PostSetup()
{
    psengine->GetMsgHandler()->Subscribe(this,MSGTYPE_MOTD);

    loadingText = (pawsMessageTextBox*)FindWidget("loadtext");

    dot = PawsManager::GetSingleton().GetTextureManager()->GetPawsImage("MapDot");

    pawsMultiLineTextBox* tipBox = (pawsMultiLineTextBox*)FindWidget( "tip" );
    tipDefaultRect = tipBox->GetScreenFrame();

    return true;
}

void pawsLoadWindow::HandleMessage(MsgEntry *me)
{
    if ( me->GetType() == MSGTYPE_MOTD )
    {
        psMOTDMessage tipmsg(me);

        pawsMultiLineTextBox* tipBox = (pawsMultiLineTextBox*)FindWidget( "tip" );
        pawsMultiLineTextBox* motdBox = (pawsMultiLineTextBox*)FindWidget( "motd" );
        pawsMultiLineTextBox* guildmotdBox = (pawsMultiLineTextBox*)FindWidget( "guildmotd" );

        //Format the guild motd
        csString guildmotdMsg;

        if(tipmsg.guild.Length())
            guildName = tipmsg.guild;

        if(tipmsg.guildmotd.Length())
            guildMOTD = tipmsg.guildmotd;
        
        if (!guildMOTD.IsEmpty())
            guildmotdMsg.Format("%s's info: %s", guildName.GetData(), guildMOTD.GetData());

        //Set the text
        if(tipBox && tipmsg.tip.Length())
            tipBox->SetText(tipmsg.tip.GetData());
        if(motdBox && tipmsg.motd.Length())
            motdBox->SetText(tipmsg.motd.GetData());
        if(guildmotdBox && guildmotdMsg.Length())
            guildmotdBox->SetText(guildmotdMsg.GetData());

    }
}

void pawsLoadWindow::PublishMOTD()
{
    pawsMultiLineTextBox* tipBox = (pawsMultiLineTextBox*)FindWidget( "tip" );
    pawsMultiLineTextBox* motdBox = (pawsMultiLineTextBox*)FindWidget( "motd" );

    psMOTDMessage motd(0, tipBox->GetText(), motdBox->GetText(), guildMOTD, guildName);
    motd.FireEvent();
}

void pawsLoadWindow::Show()
{
    PawsManager::GetSingleton().GetMouse()->Hide(true);
    pawsWidget::Show();
}

void pawsLoadWindow::Hide()
{
    if (!psengine->GetCharControl()->GetMovementManager()->MouseLook()) // do not show the mouse if it was hidden
    PawsManager::GetSingleton().GetMouse()->Hide(false);
    pawsWidget::Hide();
    renderAnim = false;
}

void pawsLoadWindow::Draw()
{
    if(DrawWindow())
    {
        if(renderAnim)
            DrawAnim();

        DrawForeground();
    }
}

void pawsLoadWindow::InitAnim(csVector2 start, csVector2 dest, csTicks delay)
{
    //if we lack the picture for the anim we don't render it
    if(!dot)
    {
        Error1("Couldn't find the picture to be used for the movement anim. Animation Aborted.");
        return;
    }

    float length;
    renderAnim = true;
    startFrom = 0;
    lastPos = start;
    destination = dest;

    //adjust the anim to resolution
    lastPos.x = GetActualWidth((int)lastPos.x);
    lastPos.y = GetActualHeight((int)lastPos.y);
    destination.x = GetActualWidth((int)destination.x);
    destination.y = GetActualHeight((int)destination.y);

    positions.DeleteAll();
    
    csVector2 direction = destination - lastPos;
    length = direction.Norm();

    numberDot = (int)ceil(length / 40);

    //we make the dots complete a bit before the end of the delay so it's
    //possible to see the last dot
    delayBetDot = ((delay * 1000)*0.9) / numberDot;
}

void pawsLoadWindow::DrawAnim()
{       
    if(startFrom + delayBetDot <= csGetTicks() && positions.GetSize() <= numberDot)
    {
        if(positions.GetSize() == 0 || positions.GetSize() == numberDot) //First and last dot shall not have noise
        {
            lastPos = (positions.GetSize() == 0) ? lastPos : destination; //lastPos has the start position of the first dot
            positions.Push(lastPos);
            startFrom = csGetTicks();
        }
        else
        {
            diffVector.x = psengine->GetRandom() - 0.5f;
            diffVector.y = psengine->GetRandom() - 0.5f;

            csVector2 direction = destination - lastPos;
            direction.Normalize();
            direction += diffVector;
            vel = direction.Unit() * 40;    

            lastPos += vel;
            positions.Push(lastPos);

            startFrom = csGetTicks();
        }
    } 
    
    for(size_t i = 0; i < positions.GetSize(); ++i)
    { 
        dot->Draw(positions[i].x, positions[i].y);
    } 
}
