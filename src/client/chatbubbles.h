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

#ifndef CHAT_BUBBLES_HEADER
#define CHAT_BUBBLES_HEADER
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <cstypes.h>
#include <csutil/ref.h>
#include <csutil/array.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/subscriber.h"

#include "effects/pseffectobjtextable.h"

//=============================================================================
// Local Includes
//=============================================================================

//these are special cases used to manage /me /my configurations in chat bubbles
#define CHATBUBBLE_ME -2
#define CHATBUBBLE_MY -3

struct BubbleChatType
{
    int                 chatType;           // the chat type this settings will apply to
    psEffectTextRow     textSettings;       // the settings
    char                effectPrefix[64];   // the prefix of the effect name to apply, effects of name <prefix>longphrase, <prefix>normal, and <prefix>shortphrase should exist
	bool				enabled;
};


class psChatBubbles : public iNetSubscriber
{
private:
    psEngine *      psengine;

    csArray<BubbleChatType> chatTypes;

    size_t bubbleMaxLineLen;            // maximum number of characters per line
    size_t bubbleShortPhraseCharCount;  // messages with fewer than this many characters get small bubble
    size_t bubbleLongPhraseLineCount;   // messages with more than this many lines get large bubble

	bool bubblesEnabled;						// enable all chat bubbles
	bool mixActionColours;
	BubbleChatType* GetTemplate(int iChatType);

public:
    psChatBubbles();
    virtual ~psChatBubbles();

    bool Initialize(psEngine * engine);

    /** Load the chat bubble config file.
    *   @param filename The name of the file to load.
    *   @param saveUserData This is used to save the file loaded in the /userdata/ folder.
    *                       This is normally done when the userdata config file has not 
    *                       been created yet.
    *
    *   @return true if the file loaded correctly. false if the file was not found or could 
    *           not be loaded.
    */
    bool Load(const char * filename, bool saveUserData = false);
	
	csArray<BubbleChatType> GetBubbleChatTypes() { return chatTypes; }
	void SetBubbleChatTypes(csArray<BubbleChatType> chatTypes) { this->chatTypes = chatTypes; }
	
	bool isEnabled() { return bubblesEnabled; }
	bool isMixingActionColours() { return mixActionColours; }
	void setEnabled(bool enable) { bubblesEnabled = enable; }

    size_t GetBubbleMaxLineLen() { return bubbleMaxLineLen; }
    size_t GetBubbleShortPhraseCharCount() { return bubbleShortPhraseCharCount; }
    size_t GetBubbleLongPhraseLineCount() { return bubbleLongPhraseLineCount; }
	
    // implemented iNetSubscriber messages
    virtual bool Verify(MsgEntry * msg, unsigned int flags, Client *& client);
    virtual void HandleMessage(MsgEntry * msg, Client * client);

};

#endif // CHAT_BUBBLES_HEADER
