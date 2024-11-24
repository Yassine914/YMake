#include "core/logger.h"
#include "core/error.h"

#include <cstdlib>

#include "../include/ymakeparser/parser.h"

const char *ymake_read(const char *filepath)
{
    FILE *file;
    fopen_s(&file, filepath, "r");
    if(file == nullptr)
    {
        LERROR("could not open file: ", filepath);
        return nullptr;
    }

    fseek(file, 0, SEEK_END);
    usize size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buff = (char *)malloc(size + 1);
    fread(buff, 1, size, file);
    buff[size] = '\0';
    return buff;
}

YmakeExpr ymake_eval_expr(StringView sv)
{
    LINFO(true, "evaluating expression: ", sv.data(), "\n");

    // rm leading and trailing spaces
    sv.trim();

    // check if line is empty
    if(sv.empty())
        return YmakeExpr();

    // check if line is a comment
    if(sv[0] == '#')
        return YmakeExpr();

    usize count;
    auto tokens_sv = sv.split(' ', count);
    Array<StringView> tokens(tokens_sv, count);

    for(auto &token : tokens)
    {
        token.trim();
        LTRACE(true, "TOKEN: ", token.data(), "\n");
    }

    return YmakeExpr();

    // check if line is a project
    if(tokens[0].starts_with("project"))
    {
        LINFO(true, "project: ", tokens[1].data(), "\n");
        return YmakeExpr{tokens[0].data(), tokens[1].data()};
    }
    else if(tokens[0].starts_with("if"))
    {
        LINFO(true, "if: ", tokens[1].data(), "\n");
        // analyze expression.
    }
    else
    {
        // variable/value pair
        LINFO(true, "variable: ", tokens[0].data(), " value: ", tokens[1].data(), "\n");
        return YmakeExpr{tokens[0].data(), tokens[1].data()};
    }

    return YmakeExpr();
}

void ymake_parse(const char *content)
{
    LINFO(true, "parsing content =>\n", content, "\n------------------\n");

    while(*content != '\0')
    {
        // get line
        char *line = (char *)malloc(256);

        int i = 0;
        while(content[i] != '\n' && content[i] != '\0')
        {
            line[i] = content[i];
            i++;
        }
        line[i] = '\0';

        ymake_eval_expr(StringView(line));
        free(line);

        content += i;
        if(*content == '\n')
            content++;
    }
}