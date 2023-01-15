#include <stdio.h>

#include "compiler.h"
#include "scanner.h"

void compile(const char *source)
{
    init_scanner(source);

    int line = -1;

    for (;;)
    {
        Token token = scan_token();
        if (token.line != line)
        {
            // token on a new line
            printf("%d ", token.line);
            line = token.line;
        }
        else
        {
            printf("   | ");
        }

        // when printing a string, the resolution is specified as, e.g., %6s
        // if we use the %.*s formatter, we make it an argument, which we pass
        // as token.length. This is necessary because token.start is simply a pointer
        // into the source, with no null pointer at the end of the token
        printf("%2d '%.*s'\n", token.type, token.length, token.start);
        if (token.type == TOKEN_EOF)
            break;
    }
}
