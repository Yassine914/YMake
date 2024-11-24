#include "core/defines.h"
#include "core/logger.h"
#include "core/error.h"

#include <ymakeparser/parser.h>

int main(int argc, char **argv)
{
    LINFO(true, GREEN_TEXT("YMake Config Parser v0.0.1"), "\n");

    if(argc < 2)
    {
        LERROR(true, "Usage: ymakep <file>");
        return 1;
    }

    const char *file = argv[1];
}