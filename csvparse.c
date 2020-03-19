/*
 *     Copyright (C) 2020 Kyle Kloberdanz
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "csvparse.h"

#ifndef CSV_MAX_LINE
#define CSV_MAX_LINE 2048
#endif

#ifndef CSV_MAX_TOK_SIZE
#define CSV_MAX_TOK_SIZE 255
#endif

#ifndef FALSE
#define FALSE 0
#endif

typedef char BOOL;

/* not officially C89, so forward declare */
char *strdup(const char *s);

static size_t countlines(FILE *fp) {
    int c;
    size_t nlines = 0;
    while ((c = fgetc(fp)) != EOF) {
        if (c == '\n') {
            nlines++;
        }
    }
    return nlines;
}

static size_t get_nfields(const char *src) {
    BOOL in_quote = FALSE;
    size_t count = 0;
    char c;

    for (; *src; src++) {
        c = *src;

        switch (c) {
            case '"':
                in_quote = !in_quote;
                break;

            case ',':
                if (!in_quote) {
                    count++;
                }
                break;
        }
    }
    return count;
}

static enum csv_ErrorCode parse_line(
    const char *line,
    char **fields,
    size_t nfields
) {
    char c;
    BOOL in_quote = FALSE;
    char tok[CSV_MAX_TOK_SIZE] = {'\0'};
    size_t tok_index = 0;
    size_t fields_index = 0;

    for (; *line; line++) {
        c = *line;
        if (c == '"') {
            in_quote = !in_quote;
        }

        if ((c == ',') && (!in_quote)) {
            if ((fields[fields_index++] = strdup(tok)) == NULL) {
                return csv_OUT_OF_MEMORY;
            }
            tok_index = 0;
            memset(tok, 0, CSV_MAX_TOK_SIZE);
        } else {
            tok[tok_index++] = c;
        }
    }

    if (*tok) {
        if ((fields[fields_index++] = strdup(tok)) == NULL) {
            return csv_OUT_OF_MEMORY;
        }
    }

    if (fields_index != nfields) {
        return csv_PARSE_ERROR;
    } else {
        return csv_NO_ERROR;
    }
}

static char *strip(char *dst) {
    char *tmp;
    for (tmp = dst; *tmp; tmp++) {
        if ((*tmp == '\n') || (*tmp == '\r')) {
            *tmp = '\0';
            return dst;
        }
    }
    return dst;
}

void csv_free(struct CSV *csv) {
    size_t i, j;
    for (i = 0; i < csv->nfields; i++) {
        free(csv->header[i]);
        for (j = 0; j < csv->nrows; j++) {
            free(csv->data[i][j]);
        }
        free(csv->data[i]);
    }
    free(csv->data);
    free(csv->header);
    csv->data = NULL;
    csv->header = NULL;
}

enum csv_ErrorCode csv_parse(struct CSV *csv, FILE *fp) {
    char line[CSV_MAX_LINE];
    size_t curr_line = 0;
    size_t i;
    long tell;
    char **fields;
    char **headers;
    enum csv_ErrorCode parse_code;

    tell = ftell(fp);
    csv->nrows = countlines(fp) - 1;
    fseek(fp, tell, SEEK_SET);

    if (!fgets(line, CSV_MAX_LINE, fp)) {
        return csv_EMPTY_FILE;
    }

    strip(line);

    csv->nfields = get_nfields(line) + 1;

    if ((fields = calloc(csv->nfields, sizeof(char *))) == NULL) {
        return csv_OUT_OF_MEMORY;
    }

    if ((headers = calloc(csv->nfields, sizeof(char *))) == NULL) {
        goto cleanup_fields;
    }

    parse_code = parse_line(line, headers, csv->nfields);
    switch (parse_code) {
        case csv_NO_ERROR:
            csv->header = headers;
            break;

        case csv_OUT_OF_MEMORY:
            goto cleanup_header;
            break;

        case csv_PARSE_ERROR:
            free(headers);
            return csv_PARSE_ERROR;
            break;

        case csv_EMPTY_FILE:
            /* N/A */
            break;
    }

    if ((csv->data = calloc(csv->nfields, sizeof(char *))) == NULL) {
        goto cleanup_cols;
    }

    for (i = 0; i < csv->nfields; i++) {
        if ((csv->data[i] = calloc(csv->nrows, sizeof(char *))) == NULL) {
            goto cleanup_rows;
        }
    }

    while (fgets(line, CSV_MAX_LINE, fp)) {

        strip(line);
        if (!*line) {
            continue;
        }

        if (get_nfields(line) != (csv->nfields - 1)) {
            csv_free(csv);
            return csv_PARSE_ERROR;
        }

        parse_code = parse_line(line, fields, csv->nfields);
        switch (parse_code) {
            case csv_EMPTY_FILE:
            case csv_OUT_OF_MEMORY:
            case csv_PARSE_ERROR:
                return parse_code;

            case csv_NO_ERROR:
                break;
        }

        for (i = 0; i < csv->nfields; i++) {
            csv->data[i][curr_line] = fields[i];
        }
        curr_line++;
    }

    free(fields);
    return csv_NO_ERROR;

cleanup_rows:
    for (i = 0; (csv->data[i] != NULL) && (i < csv->nrows); i++) {
        free(csv->data[i]);
    }

cleanup_cols:
    free(csv->data);

cleanup_header:
    if (csv->header) {
        for (i = 0; i < csv->nfields; i++) {
            free(csv->header[i]);
        }
    }
    free(csv->header);

cleanup_fields:
    free(fields);

    return csv_OUT_OF_MEMORY;
}

void csv_write(struct CSV *csv, FILE *fp) {
    size_t i, j;
    for (i = 0; i < csv->nfields; i++) {
        char *col_name = csv->header[i];
        if (i >= csv->nfields - 1) {
            fprintf(fp, "%s\n", col_name);
        } else {
            fprintf(fp, "%s,", col_name);
        }
    }

    for (j = 0; j < csv->nrows; j++) {
        for (i = 0; i < csv->nfields; i++) {
            if (i >= csv->nfields - 1) {
                fprintf(fp, "%s\n", csv->data[i][j]);
            } else {
                fprintf(fp, "%s,", csv->data[i][j]);
            }
        }
    }
}

void csv_print(struct CSV *csv) {
    csv_write(csv, stdout);
}
