#ifndef YMAKE_PARSER_H
#define YMAKE_PARSER_H

//_________ FUNCTION PROTOTYPES __________

// read the file and return the content
const char *ymake_read(const char *filepath);

// eval expression
void ymake_eval_expr(const char *expr);

#endif // YMAKE_PARSER_H