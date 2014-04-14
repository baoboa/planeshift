/*
 * cmdutil.h - Author: Keith Fulton
 *
 * Copyright (C) 2001 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iengine/rview.h>
#include <iengine/portal.h>
#include <iengine/portalcontainer.h>
#include <iengine/mesh.h>
#include <imesh/object.h>
#include <iutil/object.h>
#include <iutil/cfgmgr.h>
#include <iutil/objreg.h>
#include <igraphic/imageio.h>
#include <cstool/uberscreenshot.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/cmdhandler.h"
#include "net/clientmsghandler.h"

#include "util/strutil.h"
#include "util/pscssetup.h"

#include "gui/psmainwidget.h"
#include "gui/chatwindow.h"
#include "gui/shortcutwindow.h"

#include "paws/pawsmanager.h"
#include "paws/pawsyesnobox.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "cmdutil.h"
#include "pscelclient.h"
#include "psclientchar.h"
#include "pscamera.h"
#include "entitylabels.h"
#include "pscharcontrol.h"
#include "psengine.h"
#include "globals.h"

psUtilityCommands::psUtilityCommands(ClientMsgHandler *mh,
                                     CmdHandler *ch,
                                     iObjectRegistry* obj)
  : psCmdBase(mh,ch,obj)
{
    cmdsource->Subscribe("/quit",this);
    cmdsource->Subscribe("/echo",this);
    cmdsource->Subscribe("/confirm",this);
    cmdsource->Subscribe("/ping",this);
    cmdsource->Subscribe("/screenshot",this);
    cmdsource->Subscribe("/fps",this);
    cmdsource->Subscribe("/reload",this);
    cmdsource->Subscribe("/graphicbug",this);
    cmdsource->Subscribe("/repaintlabels",this);
    //cmdsource->Subscribe("/dumpmovements",this);
    cmdsource->Subscribe("/testanim",this);
    cmdsource->Subscribe("/version", this);
}

psUtilityCommands::~psUtilityCommands()
{
    cmdsource->Unsubscribe("/quit",this);
    cmdsource->Unsubscribe("/echo",this);
    cmdsource->Unsubscribe("/confirm",this);
    cmdsource->Unsubscribe("/ping",this);
    cmdsource->Unsubscribe("/screenshot",this);
    cmdsource->Unsubscribe("/fps",this);
    cmdsource->Unsubscribe("/reload",this);
    cmdsource->Unsubscribe("/graphicbug",this);
    cmdsource->Unsubscribe("/repaintlabels",this);
    //cmdsource->Unsubscribe("/dumpmovements",this);
    cmdsource->Unsubscribe("/testanim",this);
    cmdsource->Unsubscribe("/version", this);
}

const char *psUtilityCommands::HandleCommand(const char *cmd)
{
    WordArray words(cmd, false);

    if (words.GetCount() == 0)
        return "";

    if (words[0] == "/quit")
    {
        if (words.GetCount() == 2 && words[1] == "now")
            psengine->QuitClient();
        else
            HandleQuit();
        return "Exiting Yliakum...";
    }
    /*else if(words[0] == "/dumpmovements")
    {
        HandleDumpMovements();
        return "";
    }*/
    else if (words[0] == "/echo")
    {
        if ( !HandleEcho(words) )
            return "You must specify a word or phrase to echo.";
        else
            return NULL;
    }
    else if (words[0] == "/confirm")
    {
        static csString errorString;
        HandleConfirm( words, errorString );
        return errorString;
    }
    else if (words[0] == "/ping")
    {
        static csString temp;
        temp.Format("Average Ping time: %i", msgqueue->GetPing());
        return temp;
    }
    else if (words[0] == "/screenshot")
    {
        if ( words.FindStr("nogui",1) != SIZET_NOT_FOUND )
            psengine->GetPSCamera()->Draw();  // Re-draw to hide the GUI for the pic

        // Optionally choose not to compress
        bool compress = words.FindStr("lossless",1) == SIZET_NOT_FOUND;
        
        size_t bigPos = words.FindStr("big",1);
        
        bool bigScreenShot = bigPos != SIZET_NOT_FOUND;
        int width = 0;
        int height = 0;
        
        if(bigScreenShot) //check if the user put arguments: must be next to the command: big width height
        {
            if(words.GetCount() > bigPos+1) //first we have to check if we are under bonduaries
                width = words.GetInt(bigPos+1); //then we get the value as a number, we will get zero if there isn't a number
            if(words.GetCount() > bigPos+2)
                height = words.GetInt(bigPos+2);
        }
            

        HandleScreenShot(compress, bigScreenShot, width, height);

        // Wait for frame (which we might have drawn just above) to finish.
        psengine->GetG3D()->FinishDraw();

        return NULL;
    }
    else if(words[0] == "/graphicbug")
    {
        static csString cameraData;
        cameraData = SaveCamera();
        return cameraData.GetData();
    }
    else if(words[0] =="/fps")
    {
	    static csString fpsStr;
	    fpsStr.Format("%.2f", psengine->getFPS());
	    bool showFPS = psengine->toggleFPS();
	    if(showFPS)
		    fpsStr += " displaying FPS.";
	    else
		    fpsStr += " hiding FPS.";
	return fpsStr;
    }
    else if(words[0] == "/repaintlabels")
    {
        if (words[1] == "force")
        {
            psengine->GetCelClient()->GetEntityLabels()->LoadAllEntityLabels();
            return "Labels recreated";
        }
        else
        {
            psengine->GetCelClient()->GetEntityLabels()->RepaintAllLabels();
            return "Labels repainted";
        }
    }
    else if(words[0] == "/testanim")
    {
        static csString outputstring;
        HandleTestAnim(words, outputstring);
        return outputstring.GetData();
    }
    else if(words[0] == "/reload")
    {
        if (words[1] == "sound")
        {
            psengine->GetSoundManager()->ReloadSectors();
            return "Soundmanager reloaded";
        }
        if(words[1] == "shortcut")
        {
            ((pawsShortcutWindow*)PawsManager::GetSingleton().FindWidget("ShortcutMenu"))->LoadCommandsFile();
            return "Shortcuts reloaded";
        }
        return "try /reload sound";
    }
    else if (words[0] == "/version" )
    {
        return APPNAME;
    }
    return "Unimplemented command received by psUtilityCommands.";
}

//--------------------------------------------------------

void psUtilityCommands::HandleMessage(MsgEntry *)
{
}

void psUtilityCommands::HandleConfirmButton(bool answeredYes, void *thisptr)
{
    psUtilityCommands *This = (psUtilityCommands *)thisptr;

    if (answeredYes)
        psengine->GetCmdHandler()->Execute(This->yescommands);
    else
        psengine->GetCmdHandler()->Execute(This->nocommands);
}

csString psUtilityCommands::SaveCamera()
{
    /* NOTE:
    This function is ported from walktest and should be
    compatible with it
    */

    unsigned long int reportNum=0;
    //Get last saved report and increase it by 1
    reportNum=psengine->GetConfig()->GetInt("PlaneShift.GraphicBugs.LastUsed",1)+1;

    csString filename;
    bool freeSlot=false;
    //Loop until free fileslot
    while(!freeSlot)
    {
        filename = "/planeshift/userdata/reports/graphicbug";
        filename.Append(reportNum);
        filename.Append(".cam");

        //Check if file exist, we wouldn't want to ruin the users old report, would we?
        if(!psengine->GetVFS()->Exists(filename.GetData()))
        {
            freeSlot=true;
        }
        else
        {
            //File exists, increase the number
            reportNum++;
        }
    }

    //Save the number we're using, to improve the rate next time
    //this function is called
    psengine->GetConfig()->SetInt("PlaneShift.GraphicBugs.LastUsed",reportNum);
    //Begin the saving process

    //Get the iCamera stucture
    iCamera *c = psengine->GetPSCamera()->GetICamera()->GetCamera();

    if (!c)
        return "No camera found! (This is fatal!)";

    csOrthoTransform& camtrans = c->GetTransform ();

    const csMatrix3& m_o2t = camtrans.GetO2T ();
    const csVector3& v_o2t = camtrans.GetOrigin ();
    csString s;

    //Format the file, if you call this formated..
    s << v_o2t.x << ' ' << v_o2t.y << ' ' << v_o2t.z << '\n'
        << m_o2t.m11 << ' ' << m_o2t.m12 << ' ' << m_o2t.m13 << '\n'
        << m_o2t.m21 << ' ' << m_o2t.m22 << ' ' << m_o2t.m23 << '\n'
        << m_o2t.m31 << ' ' << m_o2t.m32 << ' ' << m_o2t.m33 << '\n'
        << '"' << c->GetSector ()->QueryObject ()->GetName () << "\"\n"
        << c->IsMirrored () << '\n';

    //Write to the file
    psengine->GetVFS()->WriteFile (filename.GetData(), s.GetData(), s.Length());

	return csString().Format("Saved graphic bug report (graphicbug%lu.cam). "
							"Report it to http://hydlaaplaza.com/flyspray/", reportNum);
}


void psUtilityCommands::HandleQuit( bool answeredYes, void* /*thisPtr*/ )
{
    if (answeredYes)
        psengine->QuitClient();
    else
    {
        PawsManager::GetSingleton().SetModalWidget(0);
    }
}



void psUtilityCommands::HandleQuit()
{
    csString text = "Do you wish to leave Yliakum?";
    pawsYesNoBox* confirm = (pawsYesNoBox*)PawsManager::GetSingleton().FindWidget("YesNoWindow");
    confirm->SetCallBack( psUtilityCommands::HandleQuit,this,text );
    confirm->MoveTo( (PawsManager::GetSingleton().GetGraphics2D()->GetWidth()-512)/2 , (PawsManager::GetSingleton().GetGraphics2D()->GetHeight()-256)/2);
    confirm->Show();
    PawsManager::GetSingleton().SetModalWidget( confirm );
}


/*void psUtilityCommands::HandleDumpMovements()
{

    pawsChatWindow *chat = (pawsChatWindow*)PawsManager::GetSingleton().FindWidget("ChatWindow");
    chat->ChatOutput( "Active movements:" );

    psMovementManager* mgr = psengine->GetCharControl()->GetMovementManager();

    for(size_t i = 0; i < mgr->GetMovementsCount(); i++)
    {
        psMovement* mov = mgr->GetMovement(i);
        if(mov->IsApplied())
            chat->ChatOutput( csString(mov->GetName()) );
    }

} */


bool psUtilityCommands::HandleEcho( WordArray& words )
{
    if (words.GetCount() < 2)
    {
        return false;
    }

    csString buffer;
    buffer = words.GetTail(1);
    psSystemMessage sys(0,MSG_INFO,buffer);
    msgqueue->Publish(sys.msg);
    return true;
}

void psUtilityCommands::HandleTestAnim( WordArray& words, csString& output )
{
    int num = (int)words.GetCount()-1; // GetCount() counts oddly...
    if (num < 3)
    {
        output.Format("Syntax is \"/testanim [name] [fadein] [fadeout]\"");
        return;
    }

    GEMClientActor* actor = psengine->GetCelClient()->GetMainPlayer();
    csRef<iSpriteCal3DState> spstate = actor->cal3dstate;
    if (!spstate)
        output.Format("Error!  Cannot play animations!");

    csString animation = words.GetWords(1,num-1); // Some anim names have spaces
    float fadein = words.GetFloat(num-1);
    float fadeout = words.GetFloat(num);

    if ( strstr(animation,"attack") )
        actor->SetIdleAnimation("combat stand");
    else if ( strstr(animation,"sit") )
        actor->SetIdleAnimation("sit idle");
    else if ( !strstr(animation,"greet") )
        actor->SetIdleAnimation("stand");

    spstate->SetAnimAction(animation.GetData(), fadein, fadeout);
    output.Format("Test playing animation '%s' with fadein of %0.2f and fadeout of %0.2f.", animation.GetData(), fadein, fadeout);
}

void psUtilityCommands::HandleConfirm( WordArray& words, csString& error )
{
    error.Clear();
    text         = words[1];
    yescommands  = words[2];
    nocommands   = words[3];

    if (text.IsEmpty())
    {
        error = "No text for confirm box is specified.";
        return;
    }

    if (yescommands=="" && nocommands=="")
    {
        error = "No commands for confirm box are specified.";
        return;
    }

    pawsYesNoBox* confirm = (pawsYesNoBox*)PawsManager::GetSingleton().FindWidget("YesNoWindow");
    confirm->SetCallBack( psUtilityCommands::HandleConfirmButton,this, text );
    confirm->Show();
    PawsManager::GetSingleton().SetModalWidget( confirm );
}

void psUtilityCommands::HandleScreenShot(bool compress, bool bigScreenShot, int width, int height)
{
    // This is needed for some players who exploit the movement of "laggieness"
    // The following line was commented out check PS#1260 for details.
    //psengine->GetCharControl()->GetMovementManager()->StopAllMovement();

    const char* format = compress ? "jpg" : "png" ;
    unsigned int ssNumber = psengine->GetConfig()->GetInt("PlaneShift.Screenshot.LastUsed",0);
    const char* ssFilename = psengine->GetConfig()->GetStr("PlaneShift.Screenshot.Filename", "shot");

    csString filename, fullpath;
    do  // Loop until free file slot
    {
        ssNumber++;
        filename.Format("%s%02u.%s", ssFilename, ssNumber, format );
        fullpath = "/planeshift/userdata/screenshots/" + filename;
    } while ( psengine->GetVFS()->Exists(fullpath) );

    // Save the number we're using, to improve the next use of this function
    psengine->GetConfig()->SetInt("PlaneShift.Screenshot.LastUsed",ssNumber);

    csString mime;
    mime.Format("image/%s",format);

    // Make the screenshot
    csRef<iImageIO> imageio = csQueryRegistry<iImageIO> (psengine->GetObjectRegistry());
    csRef<iImage> image; //used to store the acquired screenshot before writing to file
    if(bigScreenShot) //we are doing an uberscreenshot (size > window size)
    {
        //some defaults in case the user didn't input them
        if(width == 0) width = 2048;
        if(height == 0) height = 1536;
        CS::UberScreenshotMaker shotMaker (width, height, psengine->GetPSCamera()->GetView()->GetCamera(), 
                                           psengine->GetEngine(), psengine->GetG3D());
        image = shotMaker.Shoot(); //make an "uberscreenshot"
    }
    else
    {
        image = psengine->GetG2D()->ScreenShot();
    }
    csRef<iDataBuffer> buffer = imageio->Save(image,mime);

    // Save the screenshot
    psengine->GetVFS()->WriteFile( fullpath.GetData(), buffer->GetData(), buffer->GetSize() );

    psSystemMessage msg(0, MSG_ACK, "Screenshot saved to " + filename );
    msg.FireEvent();
}
