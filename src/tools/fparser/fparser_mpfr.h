/***************************************************************************\
|* Function Parser for C++ v4.5                                            *|
|*-------------------------------------------------------------------------*|
|* Copyright: Juha Nieminen                                                *|
\***************************************************************************/

#ifndef ONCE_FPARSER_MPFR_H_
#define ONCE_FPARSER_MPFR_H_

#include "fparser.h"
#include "mpfr/MpfrFloat.h"

class FunctionParser_mpfr: public FunctionParserBase<MpfrFloat> {};

#endif
