/*
 * Author: Andrew Robberts
 *
 * Copyright (C) 2007 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef PS_OPTIONS_H
#define PS_OPTIONS_H

#include <csutil/hash.h>
#include <csgeom/vector2.h>
#include <csgeom/vector3.h>
#include <iutil/cfgfile.h>
#include <csutil/csstring.h>

#include "paws/pawsmanager.h"
#include "util/scriptvar.h"

struct iOptionsClass
{
    virtual ~iOptionsClass() {}

    /**
     * This function is called when the options system has been issued a Save() call.
     * Options classes will be expected to load all associated options and apply them
     * as appropriate.
     */
    virtual void LoadOptions() = 0;

    /**
     * This function is called whenever the options system needs the options class to
     * give it its options values.  The options class is expected to call SetOption
     * for every option it uses.
     */
    virtual void SaveOptions() = 0;
};

class psOptions : public iPAWSSubscriber, public iScriptableVar
{
private:
    csHash<iOptionsClass *> optionsClasses;
    csRef<iConfigFile> configFile;
    csHash<csString, csString> subscriptions;
    csString filename;

    void BuildKey(csString & result, const char * className, const char * optionName) const;
    void EnsureSubscription(const char * name);

public:
    psOptions(const char * filename, iVFS* vfs);
    ~psOptions();

    // inherited from iPAWSSubscriber
    void OnUpdateData(const char *name, PAWSData& data);
    void NewSubscription(const char *name);

    // inherited from iScriptableVar
    double GetProperty(MathEnvironment* env, const char* ptr);
    double CalcFunction(MathEnvironment* env, const char* functionName, const double* params);
    const char * ToString() { return "options"; }

    /**
     * Registers a new options class with the options system.
     */
    void RegisterOptionsClass(const char * className, iOptionsClass * optionsClass);

    /**
     * Sets the value of a specific option.
     */
    void SetOption(const char * className, const char * name, const char * value);
    void SetOption(const char * className, const char * name, float value);
    void SetOption(const char * className, const char * name, int value);
    void SetOption(const char * className, const char * name, bool value);

    /**
     * Gets the value of a specific option
     */
    const char *      GetOption(const char * className, const char * name, const char * defaultValue);
    float             GetOption(const char * className, const char * name, float defaultValue);
    int               GetOption(const char * className, const char * name, int defaultValue);
    bool              GetOption(const char * className, const char * name, bool defaultValue);

    /**
     * Save will apply the values of all current options to the options classes.
     */
    void Save();

    /**
     * This will load/reload all options.
     */
    void Load();
};

#endif // PS_OPTIONS_H
