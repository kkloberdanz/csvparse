#include <stdio.h>
#include <stdlib.h>

char **parse_line(const char *line);

int main() {
    const char *buf = "1asdf,rqewrwq,rewre";
    char **fields;
    char **fields_cursor;
    fields = parse_line(buf);

    for (fields_cursor = fields; *fields_cursor; fields_cursor++) {
        puts(*fields_cursor);
        free(*fields_cursor);
    }
    free(fields);
    return 0;
}
