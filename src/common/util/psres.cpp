#ifndef __PSRES_CPP__
#define __PSRES_CPP__

#include <psconfig.h>

#include "psres.h"
#include "psresmngr.h"

psTemplateRes::psTemplateRes():
    mngr(NULL)
{
}

void psTemplateRes::Init (psTemplateResMngr* mngr, const char* name)
{
    this->mngr = mngr;
    this->name = name;
}

psTemplateRes::~psTemplateRes()
{
}

#endif

