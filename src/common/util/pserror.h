/* We have to find a better solution later (ie exceptions?)
 * just implement errorhalt (which should stop the program)
 * and errormsg (which should just print an error msg and continue
 */

#ifndef __ERROR_H__
#define __ERROR_H__

/**
 * \addtogroup common_util
 * @{ */

void psprintf(const char *arg, ...);

void errorhalt(const char* function, const char* file, int line,
    const char *msg);
void errormsg(const char* function, const char* file, int line,
    const char *msg);

#define ERRORHALT(msg)    errorhalt (__PRETTY_FUNCTION__,__FILE__, __LINE__, msg)
#define ERRORMSG(msg)    errormsg (__PRETTY_FUNCTION__,__FILE__, __LINE__, msg)

/** @} */

#endif
