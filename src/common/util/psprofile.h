/*
 * psprofile.h by Ondrej Hurt
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

#ifndef __PSPROFILE_H__
#define __PSPROFILE_H__

#include <csutil/parray.h>
#include <csutil/csstring.h>
#include <csutil/hash.h>

/**
 * \addtogroup common_util
 * @{ */

/****************************************************************************************
* General profilling library
*
* It is used to measure consumption of certain hardware resources (time, bandwidth etc) 
* by different operations. "Operation" is some action that consumes resources.
* Profiler keeps aggregate statistics for each operation. When the operation
* runs more than once, the consumption is added.
*
* Statistics of one operation are kept in psOperProfile.
* All psOperProfiles are kept in some subclass of psOperProfileSet.
* Abstract class psOperProfileSet offers some common actions that can be done with
* the statistics, regardless of their nature. But other actions must be implemented
* in its subclasses. The most important subclass is psNamedProfiles that can be used
* for most purposes.
*****************************************************************************************/

/**  Statistics for one operation */
class psOperProfile
{
public:
    psOperProfile(const csString & desc);
    
    /** Use this to notify about resource consumption */
    void AddConsumption(double cons);
    
    /** Reset consumption counters */
    void Reset();
    
    /** Return textual description of consumption statistics 
      * where 'totalConsumption' is total consumption of resources by all kinds of operations 
      * and   'unitName' contains name of consumption unit (e.g. "millisecond") */
    csString Dump(double totalConsumption, const csString & unitName);
    
    double GetConsumption();
    
    /** Sorting */
    static int cmpProfs(const void * a, const void * b);

protected:
    csStringFast<100> desc;         /// textual description
    double count;          /// number of operations of this kind that took place
    double consumption ;   /// total resource consumption by this kind of operation
    double maxCons; // the maximum resource consumption per operation 
                    //     that we witnessed
};

/**  Statistics for all kinds of operations 
  * This class is abstract, you have to inherit from it to use it,
  * for example add a method that will be called to collect statistics.
  */

class psOperProfileSet
{
public:
    psOperProfileSet();
    
    /** Builds textual description of all profilling statistics and returns it in two 
      * separate parts */
    void Dump(const csStringFast<50> & unitName, csStringFast<50> & header, csStringFast<50> & list);
    
    void Reset();
    
protected:
    csPDelArray<psOperProfile> profs; /** keeps statistics of all operations */
    
    /** The time when we began collecting the stats */
    csTicks profStart;
};


/**  Statistics of consumption by operations that are identified by names (strings)
  *  This is usable for most profilling purposes - some operations are identified
  *  using other means (e.g. integer constants) */
class psNamedProfiles : public psOperProfileSet
{
public:
    /// needed to clear MSVC warning
    virtual ~psNamedProfiles() { }  

    /** Notify about resource consumption done by operation identified by 'operName'. */
    virtual void AddCons(const csString & operName, csTicks time);
    
    /** Builds textual description of all profilling statistics.
      * 'unitName' contains name of consumption unit (e.g. "millisecond") */
    csString Dump(const csString & unitName, const csString & header);
    
    void Reset();

protected:
    /** Maps strings IDs to their operations */
    csHash<psOperProfile*, csString> namedProfs;
};


/** Used to measure time intervals */
class psStopWatch
{
public:
    /** Mark the start of a time interval */
    void Start();
    /** Measure the time interval from marked start (in msec) */
    csTicks Stop();
protected:
    csTicks start;
};

/** @} */

#endif
