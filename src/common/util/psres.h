#ifndef __PSRES_H__
#define __PSRES_H__

#include <csutil/refcount.h>
#include <csutil/scf.h>
#include <csutil/csstring.h>

class psTemplateResMngr;

/**
 * \addtogroup common_util
 * @{ */

class psTemplateRes : public csRefCount
{
public:
    psTemplateRes();
    virtual ~psTemplateRes();

    void Init(psTemplateResMngr* mngr, const char* name);

    const char* GetName() { return name.GetData(); }
    
protected:
    psTemplateResMngr* mngr;
    csString name;
};

/** @} */

#endif

