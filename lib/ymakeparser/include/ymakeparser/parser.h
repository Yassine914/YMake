#ifndef YMAKE_PARSER_H
#define YMAKE_PARSER_H

#include "../../src/core/defines.h"

#include "d_array.h"
#include "string_view.h"

enum class TokenType
{
    UNKOWN = 0,
    NAME,        // x
    VALUE,       // 1
    EQUALS,      // ==
    EXPRESSION,  // {x}
    NEWLINE,     // \n
    COMMENT,     // #
    IF,          // if cond =>
    PROJECT,     // project
    BLOCKSTART,  // =>
    BLOCKEND,    // ;
    INLINEBLOCK, // ->
    APPENDARR,   // +
    ARROPEN,     // [
    ARRCLOSE,    // ]
};

struct YmakeExpr
{
    const char *name;
    const char *value;
};

//_________ FUNCTION PROTOTYPES __________

// read the file and return the content
const char *ymake_read(const char *filepath);

// eval expression
YmakeExpr ymake_eval_expr(StringView sv);

// parse entire file
void ymake_parse(const char *content);

#endif // YMAKE_PARSER_H