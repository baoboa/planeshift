#ifndef __PSRESMNGR_CPP__
#define __PSRESMNGR_CPP__

#include <psconfig.h>

#include "psresmngr.h"
#include "util/consoleout.h"

psTemplateResMngr::psTemplateResMngr()
{
    p_resources = new psTemplateResourceHash;
}

psTemplateResMngr::~psTemplateResMngr()
{
    delete p_resources;
    p_resources = NULL;
}

csPtr<psTemplateRes> psTemplateResMngr::CreateResource(const char* name)
{
    if (!p_resources)
        return NULL;

    // Is there already a resource with that name there?
    psTemplateResourceHash::Iterator i = p_resources->GetIterator(name);
    csRef<psTemplateRes> res;
    while (i.HasNext())
    {
        res = i.Next();
        if(!strcmp(res->GetName(), name))
        {
            return csPtr<psTemplateRes>(res);
        }
    }

    // Else we have to create it...
    csRef<psTemplateRes> newres = LoadResource(name);
    if (!newres)
    {
        CPrintf (CON_ERROR, "Couldn't create Resource '%s'\n", name);
        return NULL;
    }

    newres->Init(this, name);
    p_resources->Put(name, newres);

    return csPtr<psTemplateRes>(newres);
}

void psTemplateResMngr::UnregisterResource (psTemplateRes* )
{
    // just do nothing at the moment
}

/*  Since it's not safe to perform a delete while an iterator exists
*  we create a new hash, copy the resources that stay into it,
*  decref the resources that dont.  Destroy the old hashmap and keep
*  the new one.
*
*/

void psTemplateResMngr::Clean()
{
    if (p_resources)
    {
        psTemplateResourceHash::GlobalIterator i = p_resources->GetIterator();
        psTemplateResourceHash *p_newresources = new psTemplateResourceHash;

        csRef<psTemplateRes> res;
        while (i.HasNext())
        {
            res = i.Next();
            // Add to the new hash map
            p_newresources->Put(res->GetName(), res);
        }
        delete p_resources;
        p_resources = p_newresources;
    }
}

#endif

