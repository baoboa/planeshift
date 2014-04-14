/*
 * ClientMsgHandler.cpp by Keith Fulton <keith@paqrat.com>
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

#include <iutil/objreg.h>
#include <iutil/eventq.h>
#include <csutil/event.h>
#include <csutil/eventnames.h>

#include "net/netbase.h"
#include "net/clientmsghandler.h"

ClientMsgHandler::ClientMsgHandler()
{
    // no special code needed right now
    scfiEventHandler = NULL;
    object_reg = NULL;
}

ClientMsgHandler::~ClientMsgHandler()
{
    if (scfiEventHandler)
    {
        csRef<iEventQueue> queue = csQueryRegistry<iEventQueue> (object_reg);
        if (queue)
        {
            queue->RemoveListener(scfiEventHandler);
        }
    }    
}

bool ClientMsgHandler::Initialize(NetBase* nb, iObjectRegistry* object_reg)
{
    // Create and register inbound queue
    if (!MsgHandler::Initialize(nb))
        return false;

    ClientMsgHandler::object_reg = object_reg;

    // This hooks our HandleEvent function into main application event loop
    csRef<iEventQueue> queue =  csQueryRegistry<iEventQueue> (object_reg);
    if (!queue)
        return false;
    scfiEventHandler = new EventHandler(this);

    csEventID esub[] = { 
      csevFrame (object_reg),
      CS_EVENTLIST_END 
    };
    queue->RegisterListener(scfiEventHandler, esub);

    return true;
}

bool ClientMsgHandler::HandleEvent(iEvent& /*ev*/)
{
    if (!netbase->IsReady())
        return false;
    
    return DispatchQueue();
}

bool ClientMsgHandler::DispatchQueue()
{
    /* 
     * If called, it publishes (and therefore handles)
     * all the messages in the inbound queue before returning.
     */
    csRef<MsgEntry> msg;

    while((msg = queue->Get()))
    {
        // printf("Got a message from the client msg queue.\n");
		// Check for out of sequence messages.  Handle normally if not sequenced.
		if (msg->GetSequenceNumber() == 0)
		{
		    Publish(msg);
			/* Destroy this message.  Note that Msghandler normally does this in the 
	         * Run() loop for the server.
		     */
			// don't forget to release the packet
	        msg = NULL;
		}
		else // sequenced message
		{
			int seqnum = msg->GetSequenceNumber();
			OrderedMessageChannel *channel = orderedMessages.Get(msg->GetType(),NULL);
			if (!channel) // new type of sequence to track
			{
				// printf("Adding new sequence channel for msgtype %d.\n", msg->GetType());
				channel = new OrderedMessageChannel;
                channel->IncrementSequenceNumber(); // prime the pump
				orderedMessages.Put(msg->GetType(), channel);
			}
			int nextSequenceExpected  = channel->GetCurrentSequenceNumber();
			// printf("Expecting sequence number %d, got %d.\n", nextSequenceExpected, seqnum);

			if (seqnum < nextSequenceExpected)
			{
				Error1("Cannot have a sequence number lower than expected!");
				seqnum = nextSequenceExpected;
			}

			channel->pendingMessages.Put(seqnum - nextSequenceExpected, msg);
			// printf("Added as element %u to pending queue.\n", seqnum - nextSequenceExpected);
			
			if (seqnum == nextSequenceExpected) // have something to publish
			{
                printf("Ok we have at least one message to publish.\n");
				while (channel->pendingMessages.GetSize() && channel->pendingMessages[0] != NULL)
				{
					printf("Publishing sequence number %d.\n", channel->GetCurrentSequenceNumber());
					Publish(channel->pendingMessages[0]);
					channel->pendingMessages[0] = NULL;  // release the ref
					channel->pendingMessages.DeleteIndex(0);
					channel->IncrementSequenceNumber();
				}
			}
            else
            {
               // printf("Nothing to publish yet from this channel.\n");
            }
		}
	}
    return false;  // this function should not eat the event
}

int ClientMsgHandler::GetNextSequenceNumber(msgtype mtype)
{
	OrderedMessageChannel *channel = orderedMessages.Get(mtype,NULL);

	if (!channel)
	{
		channel = new OrderedMessageChannel;
		orderedMessages.Put(mtype, channel);
	}
	return channel->IncrementSequenceNumber();
}
