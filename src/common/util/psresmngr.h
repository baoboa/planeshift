#ifndef __PSRESMNGR_H__
#define __PSRESMNGR_H__

#include "psres.h"

#include <csutil/hash.h>

/**
 * \addtogroup common_util
 * @{ */

typedef csHash<csRef<psTemplateRes>, csString> psTemplateResourceHash;

class psTemplateResMngr
{
public:
    psTemplateResMngr();
    virtual ~psTemplateResMngr();

    csPtr<psTemplateRes> CreateResource (const char* name);
    /** Releases any resource where the resource manager holds the last reference.
     *
     *  This causes a new csHashMap to be created and assigned to p_resources
     *
     */
    void Clean();

    /// Not yet implemented
    virtual void UnregisterResource (psTemplateRes* res);
protected:     
    virtual csPtr<psTemplateRes> LoadResource (const char* name) = 0;

    /** Pointer to the hash that stores pointers to loaded resources.
     *
     *  This can and does change and should not be accessable outside of
     *  objects of this class.
     */
    psTemplateResourceHash *p_resources;
};

/** @} */

#endif

