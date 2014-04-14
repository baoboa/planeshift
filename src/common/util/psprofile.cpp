/*
 * psprofile.cpp by Ondrej Hurt
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
#include <csutil/sysfunc.h>
#include "psprofile.h"
#include <math.h>

void psStopWatch::Start()
{
    start = csGetTicks();
}

unsigned psStopWatch::Stop()
{
    return csGetTicks() - start;
}

/***************************************************************
*
*                   psOperProfile
*
****************************************************************/

psOperProfile::psOperProfile(const csString & desc)
{
    this->desc  =  desc;
    Reset();
}

void psOperProfile::AddConsumption(double cons)
{
    count ++;
    consumption += cons;
    if (cons > maxCons)
        maxCons = cons;
}

csString psOperProfile::Dump(double totalConsumption, const csString & unitName)
{
    double perc = consumption/totalConsumption*100;
    if (perc > 0)
        return csString().Format(
                "count=%-5u perc=%.1lf %s=%-2i avg-%s=%.3f max=%.3f Name=%s\n",
                unsigned(count), perc, unitName.GetData(), int(consumption),
                unitName.GetData(), consumption/count, maxCons, desc.GetData());
    else
        return "";
}

void psOperProfile::Reset()
{
    count       = 0;
    consumption = 0;
    maxCons  =  0;
}

double psOperProfile::GetConsumption()
{
    return consumption;
}

int psOperProfile::cmpProfs(const void * a, const void * b)
{
    psOperProfile * pA = *((psOperProfile**)a);
    psOperProfile * pB = *((psOperProfile**)b);
    if (pA->GetConsumption() > pB->GetConsumption())
        return -1;
    if (pA->GetConsumption() == pB->GetConsumption())
        return 0;
    else
        return 1;
}

/***************************************************************
*
*                   psOperProfiles
*
****************************************************************/

psOperProfileSet::psOperProfileSet()
{
    profStart = csGetTicks();
}

void psOperProfileSet::Dump(const csStringFast<50> & unitName, csStringFast<50> & header, csStringFast<50> & list)
{
    double totalCons=0;
    psOperProfile **sortedProfs;
    float time;
    size_t i;
    
    for (i=0; i < profs.GetSize(); i++)
        totalCons += profs[i]->GetConsumption();
    time = csGetTicks() - profStart;

    header += csString().Format("Total time: %i seconds\n", int(time/1000));
    header += csString().Format("Total %s: %i \n", unitName.GetData(), int(totalCons));
    // NOTE: totalcons might NOT be in kilo units as assumed here
    //header += csString().Format("Average consumption: %f %s/sec\n", totalCons*8/1024/time*1000, unitName.GetData());
    
    // sort the profiles by bandwidth and dump them out
    sortedProfs = new psOperProfile*[profs.GetSize()];
    for (i=0; i < profs.GetSize(); i++)
        sortedProfs[i] = profs[i];
    qsort(sortedProfs, profs.GetSize(), sizeof(psOperProfile*), psOperProfile::cmpProfs);
    for (i=0; i < profs.GetSize(); i++)
        list += sortedProfs[i]->Dump(totalCons, unitName);
    delete [] sortedProfs;
}

void psOperProfileSet::Reset()
{
    profStart = csGetTicks();
    profs.DeleteAll();
}

/***************************************************************
*
*                   psNamedProfiles
*
****************************************************************/

void psNamedProfiles::AddCons(const csString & operName, csTicks time)
{
    if (namedProfs.In(operName))
    {
        psOperProfile ** prof;
        
        prof = namedProfs.GetElementPointer(operName);
        assert(prof);
        (*prof)->AddConsumption(time);
    }
    else
    {
        psOperProfile * newProf = new psOperProfile(operName);
        namedProfs.Put(operName, newProf);
        profs.Push(newProf);
        newProf->AddConsumption(time);
    }
}

csString psNamedProfiles::Dump(const csString & unitName, const csString & label)
{
    csStringFast<50> header, list;
    
    psOperProfileSet::Dump(unitName, header, list);
    return "=================\n" + label + "\n=================\n" + header + list;
}

void psNamedProfiles::Reset()
{
    namedProfs.DeleteAll();
    psOperProfileSet::Reset();
}

