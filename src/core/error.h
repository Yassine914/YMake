#pragma once

#include "defines.h"

namespace Y {

#define YERR_FILE_COULDNT_OPEN 32

class Error
{
    private:
    i32 errnum;
    const char *errtxt;

    public:
    Error() {}
    Error(const char *txt) : errtxt{txt} {}
    Error(i32 no, const char *txt) : errnum{no}, errtxt{txt} {}

    inline const char *what() { return errtxt; }
    inline i32 GetErrNum() { return errnum; }
};

} // namespace Y