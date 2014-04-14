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
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iengine/movable.h>
#include <iengine/mesh.h>
#include <imesh/objmodel.h>
#include <imesh/object.h>
#include <ivideo/graph2d.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/clientmsghandler.h"
#include "gui/chatwindow.h"
#include "gui/pawsnpcdialog.h"
#include "gui/psmainwidget.h"

#include "effects/pseffectmanager.h"
#include "effects/pseffect.h"
#include "util/strutil.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "chatbubbles.h"
#include "pscelclient.h"

//#define DISABLE_CHAT_BUBBLES

// Used to set default config files if the userdata ones are not found.
#define DEFAULT_FILE  "/planeshift/data/options/chatbubbles_def.xml"
#define USER_FILE     "/planeshift/userdata/options/chatbubbles.xml"

psChatBubbles::psChatBubbles()
             : psengine(NULL)
{
}

psChatBubbles::~psChatBubbles()
{
    psengine->GetMsgHandler()->Unsubscribe(this, MSGTYPE_CHAT);
}

bool psChatBubbles::Initialize(psEngine * psengine)
{
    this->psengine = psengine;
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_CHAT);
    return (Load(USER_FILE) || Load(DEFAULT_FILE, true));
}

bool psChatBubbles::Load(const char * filename, bool saveAgain)
{
    csRef<iDocument> doc;
    csRef<iDocumentNode> root;
    iVFS* vfs;
    csRef<iDocumentSystem>  xml;
    const char* error;

    chatTypes.DeleteAll();

    vfs = psengine->GetVFS();
    assert(vfs);
    csRef<iDataBuffer> buff = vfs->ReadFile(filename);
      
    if (buff == NULL)
    {
        Error2("Could not find file: %s", filename);
        return false;
    }
    
    if ( saveAgain )
    {
        vfs->WriteFile(USER_FILE, buff->GetData(), buff->GetSize());
    }
    
    xml = psengine->GetXMLParser ();
    doc = xml->CreateDocument();
    assert(doc);
    error = doc->Parse( buff );
    if ( error )
    {
        Error3("Parse error in %s: %s", filename, error);
        return false;
    }
    if (doc == NULL)
        return false;

    root = doc->GetRoot();
    if (root == NULL)
    {
        Error2("No root in XML %s", filename);
        return false;
    }

    csRef<iDocumentNode> chatBubblesNode = root->GetNode("chat_bubbles");
    if (!chatBubblesNode)
    {
        Error2("No chat_bubbles node in %s", filename);
        return false;
    }

    bubbleMaxLineLen = chatBubblesNode->GetAttributeValueAsInt("maxLineLen");
    bubbleShortPhraseCharCount = chatBubblesNode->GetAttributeValueAsInt("shortPhraseCharCount");
    bubbleLongPhraseLineCount = chatBubblesNode->GetAttributeValueAsInt("longPhraseLineCount");
    
	mixActionColours = chatBubblesNode->GetAttributeValueAsBool("mixActionColours",true);
	
	bubblesEnabled = chatBubblesNode->GetAttributeValueAsBool("enabled", true);
		
    

    csRef<iDocumentNodeIterator> nodes = chatBubblesNode->GetNodes("chat");
    while (nodes->HasNext())
    {
        csRef<iDocumentNode> chatNode = nodes->Next();

        int r, g, b;
        BubbleChatType chat = { CHAT_SAY, {"", 0, false, 0, false, 0, ETA_LEFT}, "chatbubble_", true };

        csString type = chatNode->GetAttributeValue("type");
        type.Downcase();
        if (type == "say")
            chat.chatType = CHAT_SAY;
        else if (type == "tell")
            chat.chatType = CHAT_TELL;
        else if (type == "group")
            chat.chatType = CHAT_GROUP;
        else if (type == "guild")
            chat.chatType = CHAT_GUILD;
        else if (type == "alliance")
            chat.chatType = CHAT_ALLIANCE;
        else if (type == "auction")
            chat.chatType = CHAT_AUCTION;
        else if (type == "shout")
            chat.chatType = CHAT_SHOUT;
        else if (type == "me")
            chat.chatType = CHATBUBBLE_ME;
        else if (type == "tellself")
            chat.chatType = CHAT_TELLSELF;
        else if (type == "my")
            chat.chatType = CHATBUBBLE_MY;
        else if (type == "npc")
            chat.chatType = CHAT_NPC;
        else if (type == "npcinternal")
            chat.chatType = CHAT_NPCINTERNAL; // /tellnpc messages are now ignored and only /tellnpcinternal messages are displayed.  These are normally sent in pairs.
        else if (type == "npc_me")
            chat.chatType = CHAT_NPC_ME;
        else if (type == "npc_my")
            chat.chatType = CHAT_NPC_MY;
        else if (type == "npc_narrate")
            chat.chatType = CHAT_NPC_NARRATE;

        // colour
        if (chatNode->GetAttribute("colorR") != 0)
        {
            r = chatNode->GetAttributeValueAsInt("colorR");
        }
        else
        {
            r = chatNode->GetAttributeValueAsInt("colourR");
        }
        if (chatNode->GetAttribute("colorG") != 0)
        {
            g = chatNode->GetAttributeValueAsInt("colorG");
        }
        else
        {
            g = chatNode->GetAttributeValueAsInt("colourG");
        }
        if (chatNode->GetAttribute("colorB") != 0)
        {
            b = chatNode->GetAttributeValueAsInt("colorB");
        }
        else
        {
            b = chatNode->GetAttributeValueAsInt("colourB");
        }
        chat.textSettings.colour = psengine->GetG2D()->FindRGB(r, g, b);

        // shadow
        chat.textSettings.hasShadow = (chatNode->GetAttribute("shadowR") != 0);
        r = chatNode->GetAttributeValueAsInt("shadowR");
        g = chatNode->GetAttributeValueAsInt("shadowG");
        b = chatNode->GetAttributeValueAsInt("shadowB");
        chat.textSettings.shadowColour = psengine->GetG2D()->FindRGB(r, g, b);

        // outline
        chat.textSettings.hasOutline = (chatNode->GetAttribute("outlineR") != 0);
        r = chatNode->GetAttributeValueAsInt("outlineR");
        g = chatNode->GetAttributeValueAsInt("outlineG");
        b = chatNode->GetAttributeValueAsInt("outlineB");
        chat.textSettings.outlineColour = psengine->GetG2D()->FindRGB(r, g, b);

        // align
        csString align = chatNode->GetAttributeValue("align");
        align.Downcase();
        if (align == "right")
            chat.textSettings.align = ETA_RIGHT;
        else if (align == "center")
            chat.textSettings.align = ETA_CENTER;
        else
            chat.textSettings.align = ETA_LEFT;

        // prefix
        strcpy(chat.effectPrefix, chatNode->GetAttributeValue("effectPrefix"));
		
		//enabled
		if ((csString) chatNode->GetAttributeValue("enabled") == "no")
			chat.enabled = false;
		else
			chat.enabled = true;

        chatTypes.Push(chat);
    }
    return true;
}

bool psChatBubbles::Verify(MsgEntry* /*msg*/, unsigned int /*flags*/, Client*& /*client*/)
{
    return true;
}

//returns the template of the chat bubble. If not found returns NULL
BubbleChatType* psChatBubbles::GetTemplate(int iChatType)
{
    // get the template for the chat type
    size_t len = chatTypes.GetSize();
    for (size_t a=0; a<len; ++a)
    {
        if (iChatType == chatTypes[a].chatType)
        {
            return &chatTypes[a];
        }
    }
    return NULL;
}

void psChatBubbles::HandleMessage(MsgEntry* msg, Client* /*client*/)
{
#ifdef DISABLE_CHAT_BUBBLES
    return;
#endif

    if (!bubblesEnabled)
		return;
	
    size_t a;
    if (msg->GetType() != MSGTYPE_CHAT)
        return;

    psChatMessage chatMsg(msg);
    
    // Check to see if the person talking is on the ignore list. 
    pawsChatWindow* chatWindow = dynamic_cast<pawsChatWindow*>(PawsManager::GetSingleton().FindWidget("ChatWindow"));
    if (!chatWindow)
    {
        // Chat window isn't loaded yet, GUI is still loading
        return;
    }
    if (chatWindow->GetIgnoredList()->IsIgnored(chatMsg.sPerson) && chatMsg.iChatType != CHAT_TELLSELF && chatMsg.iChatType != CHAT_ADVISOR && chatMsg.iChatType != CHAT_ADVICE)
    {
        return;
    }
    
    //we have to manage this separately as sPerson in this case holds the destination in place of the origin
    if (chatMsg.iChatType == CHAT_TELLSELF || chatMsg.iChatType == CHAT_NPCINTERNAL)
    {
        chatMsg.sPerson = psengine->GetMainPlayerName();
        chatMsg.actor = psengine->GetCelClient()->GetMainPlayer()->GetEID();
    }

    // Get the first name of the person (needed for NPCs with both the first and the last name)
    csString firstName;
    if ((a = chatMsg.sPerson.FindFirst(' ')) != (size_t)-1)
        firstName = chatMsg.sPerson.Slice(0, a);
    else
        firstName = chatMsg.sPerson;

    if(!chatMsg.actor.IsValid())
        return;
    
    GEMClientActor* actor = dynamic_cast<GEMClientActor*> (psengine->GetCelClient()->FindObject(chatMsg.actor));
    if (!actor)
        return;

    // get the template for the chat type
    BubbleChatType mixType;
    const BubbleChatType* type = 0;
    type = GetTemplate(chatMsg.iChatType);

    if (!type)
        return;
		
    if (!(type->enabled))
        return;

    //We don't want /me or /my messages in the chat box, change them to something nice
    if (chatMsg.sText.StartsWith("/me") || chatMsg.sText.StartsWith("/my"))
    {
        const BubbleChatType* subType = 0;
        if(chatMsg.sText.StartsWith("/my")) //we have to add an 's
        {
            size_t len = firstName.Length() - 1;
            firstName.Append(firstName.GetAt(len) == 's' ? "'" : "'s");
            subType = GetTemplate(CHATBUBBLE_MY);
        }
        else
        {
            subType = GetTemplate(CHATBUBBLE_ME);
        }
        chatMsg.sText.DeleteAt(0, 3);
        chatMsg.sText.Insert(0, firstName);
        chatMsg.sText.Insert(0, "* ");
        chatMsg.sText.Append(" *");

        //check if the subtype is enabled if so mix the main type to the subtype and generated a mixed type
        if(!subType)
            return;

        if(!(subType->enabled))
            return;

        if(mixActionColours)
        {
            //generate the mixed type:
            //mix the colours
            mixType.textSettings.colour = chatWindow->mixColours(type->textSettings.colour, subType->textSettings.colour);
            mixType.textSettings.shadowColour = chatWindow->mixColours(type->textSettings.shadowColour, subType->textSettings.shadowColour);
            mixType.textSettings.outlineColour = chatWindow->mixColours(type->textSettings.outlineColour, subType->textSettings.outlineColour);
            
            //copy the remaining data from the subtype
            mixType.textSettings.hasShadow = subType->textSettings.hasShadow;
            mixType.textSettings.hasOutline = subType->textSettings.hasOutline;
            mixType.textSettings.align = subType->textSettings.align;
            mixType.enabled = subType->enabled; //not really usefull but just to be safe
            mixType.chatType = subType->chatType; //not really usefull but just to be safe
            strcpy(mixType.effectPrefix, subType->effectPrefix);
            type = &mixType; //put our mixed type in place of the type for use later
        }
        else //if we don't mix colours just get the settings for me/my directly
        {
            type = subType;
        }
    }

    // build the text rows
    csString inText = chatMsg.sText;
    if (chatWindow->GetSettings().enableBadWordsFilterIncoming) //check for badwords filtering
        chatWindow->BadWordsFilter(inText); //if enabled apply it

    // split text into lines with preferred max size
    int maxRowLen = 0;
    csArray<csString> lines = splitTextInLines(inText, bubbleMaxLineLen, maxRowLen);

    // populates rowBuffer effect with lines
    static csArray<psEffectTextRow> rowBuffer;
    rowBuffer.Empty();
    psEffectTextRow chat = type->textSettings;
    for (size_t i = 0; i < lines.GetSize(); i++) {
        chat.text = lines[i];
        rowBuffer.Push(chat);
    }

    //checks if the NPC Dialogue is displayed, in this case don't show the normal overhead bubble
    pawsNpcDialogWindow *npcdialog = dynamic_cast<pawsNpcDialogWindow *>(psengine->GetMainWidget()->FindWidget("NPCDialogWindow"));
    if (npcdialog->IsVisible())
        return;

    // decide on good effect size
    csString effectName = type->effectPrefix;
    if (rowBuffer.GetSize() == 1 && maxRowLen <= (int)bubbleShortPhraseCharCount)
        effectName += "shortphrase";
    else if (rowBuffer.GetSize() > bubbleLongPhraseLineCount)
        effectName += "longphrase";
    else
        effectName += "normal";

    // create the effect
    psEffectManager * effectManager = psengine->GetEffectManager();
    csRef<iMeshWrapper> mesh = actor->GetMesh();

    // Mesh might not be loaded yet.
    if(!mesh.IsValid())
    {
        return;
    }

    const csBox3& boundBox = actor->GetBBox();
    unsigned int bubbleID = effectManager->RenderEffect("chatbubble", //effectName, 
          csVector3(0, boundBox.Max(1) + 0.5f, 0), mesh);
  
    // delete existing bubble if it exists
    psEffect * bubble = effectManager->FindEffect(actor->GetChatBubbleID());
    if (bubble)
        effectManager->DeleteEffect(actor->GetChatBubbleID());
  
    bubble = psengine->GetEffectManager()->FindEffect(bubbleID);
    if (!bubble)
    {
        Error2("Couldn't create effect '%s'", effectName.GetData());
        return;
    }

    actor->SetChatBubbleID(bubbleID);

    psEffectObjTextable * txt = bubble->GetMainTextObj();
    if (!txt)
    {
        Error2("Effect '%s' has no text object", effectName.GetData());
        return;
    }
    txt->SetText(rowBuffer);
}
