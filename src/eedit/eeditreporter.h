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

#ifndef EEDIT_REPORTER_HEADER
#define EEDIT_REPORTER_HEADER

#include <csutil/ref.h>
#include <csutil/refcount.h>
#include <csutil/scf_implementation.h>
#include <csutil/scf.h>
#include <csutil/threadmanager.h>
#include <csutil/weakref.h>
#include <ivaria/reporter.h>

class EEditErrorToolbox;

/**
 * \addtogroup eedit
 * @{ */

/** A csReporterListener for eedit
 */
class EEditReporter : public scfImplementation1<EEditReporter, iReporterListener>,
  public ThreadedCallable<EEditReporter>
{
public:
    EEditReporter(iObjectRegistry* obj_reg);
    virtual ~EEditReporter();

    THREADED_CALLABLE_DECL4(EEditReporter, Report, csThreadReturn, iReporter*, reporter,
      int, severity, const char*, msgId, const char*, description, HIGH, false, false);

    void SetErrorToolbox(EEditErrorToolbox * toolbox);

    iObjectRegistry* GetObjectRegistry() const { return object_reg; }

private:

    csWeakRef<EEditErrorToolbox> errorToolbox;
    iObjectRegistry* object_reg;
};

/** @} */

#endif 
