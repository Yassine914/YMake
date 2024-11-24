// #include "core/defines.h"
#include "core/logger.h"
// #include "core/error.h"

#include "../include/ymakeparser/parser.h"

int main(int argc, char **argv)
{
    LINFO(true, GREEN_TEXT("YMake Config Parser v0.0.1"), "\n");

    if(argc < 2)
    {
        LERROR("Usage: ymakep <file>\n");
        return 1;
    }

    const char *file = argv[1];

    const char *content = ymake_read(file);

    ymake_parse(content);
}