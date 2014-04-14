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
#include "eeditreporter.h"
#include "eediterrortoolbox.h"


EEditReporter::EEditReporter(iObjectRegistry* obj_reg) : scfImplementationType (this)
{
    object_reg = obj_reg;
    errorToolbox = 0;
}

EEditReporter::~EEditReporter()
{
 
}

THREADED_CALLABLE_IMPL4(EEditReporter, Report, iReporter* reporter, int severity, const char * msgId, const char * description)
{
    if (errorToolbox == 0)
        printf("%s[SEVERITY=%d]: %s\n", msgId, severity, description);
    else
    {
        errorToolbox->Show();
        errorToolbox->AddError(severity, msgId, description);
    }
    
    return true;
}

void EEditReporter::SetErrorToolbox(EEditErrorToolbox * toolbox)
{
    errorToolbox = toolbox;
}
