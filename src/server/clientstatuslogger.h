/*
 * clientstatuslogger.h - Author: Dewitt Colclough <dewitt@twcny.rr.com>
 *
 * Copyright (C) 2004 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
 */
/** @file clientstatuslogger.h */
/** @author Dewitt Colclough */

#ifndef __CLIENTSTATUSLOGGER_H__
#define __CLIENTSTATUSLOGGER_H__

#include <iutil/document.h>

class Client;

/**
* Logs client status to document
*/
class ClientStatusLogger
{
public:
    void LogClientInfo(Client* client); ///< write client status info to doc

    ClientStatusLogger(iDocumentNode* statusNode);
private:
    csRef<iDocumentNode> statusRootNode; ///< top node for all status entries
    csTicks startTime;

    void AddBasicNode(iDocumentNode* parent, const char* fieldName, const char* text);
    void AddBasicNode(iDocumentNode* parent, const char* fieldName, int data);
    csPtr<iDocumentNode> AddContainerNode(iDocumentNode* parent, const char* fieldName);

    void LogBasicInfo(Client* client, iDocumentNode* node);
    void LogConnectionInfo(Client* client, iDocumentNode* node);
    void LogGuildInfo(Client* client, iDocumentNode* node);

    ClientStatusLogger() {}
};

#endif

