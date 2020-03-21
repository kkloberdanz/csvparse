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
    char** tok,
    size_t *tok_len,
    const char *line,
    char **fields,
    size_t nfields
) {
    char c;
    BOOL in_quote = FALSE;
    size_t tok_index = 0;
    size_t fields_index = 0;
    size_t i = 0;

    for (; *line; line++) {
        c = *line;
        if (c == '"') {
            in_quote = !in_quote;
        }

        if ((c == ',') && (!in_quote)) {
            if ((fields[fields_index++] = strdup(*tok)) == NULL) {
                return csv_OUT_OF_MEMORY;
            }
            memset(*tok, 0, *tok_len);
            tok_index = 0;
        } else {
            if (tok_index >= (*tok_len - 1)) {
                *tok_len *= 2;
                if ((*tok = realloc(*tok, (1 + *tok_len))) == NULL) {
                    return csv_OUT_OF_MEMORY;
                }
                memset(*tok + tok_index, 0, *tok_len - tok_index);
                fprintf(stderr, "reallocated to %lu\n", *tok_len);
            }
            (*tok)[tok_index++] = c;
        }
    }

    if (tok && *tok && **tok) {
        if ((fields[fields_index++] = strdup(*tok)) == NULL) {
            for (i = 0; i < fields_index; i++) {
                free(fields[i]);
            }
            return csv_OUT_OF_MEMORY;
        }
    }

    if (fields_index != nfields) {
        for (i = 0; i < fields_index; i++) {
            free(fields[i]);
        }
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
    char *line = NULL;
    size_t curr_line = 0;
    size_t i;
    size_t n = 0;
    long tell;
    char **fields;
    char **headers;
    enum csv_ErrorCode parse_code;
    char *tok = NULL;
    size_t tok_len = 128;

    tell = ftell(fp);
    csv->nrows = countlines(fp) - 1;
    fseek(fp, tell, SEEK_SET);

    if ((getline(&line, &n, fp)) == -1) {
        free(line);
        return csv_EMPTY_FILE;
    }

    strip(line);

    csv->nfields = get_nfields(line) + 1;

    if ((fields = calloc(csv->nfields, sizeof(char *))) == NULL) {
        free(line);
        return csv_OUT_OF_MEMORY;
    }

    if ((headers = calloc(csv->nfields, sizeof(char *))) == NULL) {
        parse_code = csv_OUT_OF_MEMORY;
        goto cleanup_fields;
    }

    if ((tok = calloc(tok_len, sizeof(char))) == NULL) {
        parse_code = csv_OUT_OF_MEMORY;
        goto cleanup_fields;
    }

    parse_code = parse_line(&tok, &tok_len, line, headers, csv->nfields);
    switch (parse_code) {
        case csv_NO_ERROR:
            csv->header = headers;
            break;

        case csv_OUT_OF_MEMORY:
            goto cleanup_header;

        case csv_PARSE_ERROR:
            free(headers);
            free(line);
            free(tok);
            return csv_PARSE_ERROR;

        case csv_EMPTY_FILE:
            /* N/A */
            break;
    }

    if ((csv->data = calloc(csv->nfields, sizeof(char *))) == NULL) {
        parse_code = csv_OUT_OF_MEMORY;
        goto cleanup_cols;
    }

    for (i = 0; i < csv->nfields; i++) {
        if ((csv->data[i] = calloc(csv->nrows, sizeof(char *))) == NULL) {
            parse_code = csv_OUT_OF_MEMORY;
            goto cleanup_rows;
        }
    }

    while ((getline(&line, &n, fp)) != -1) {

        strip(line);
        if (!*line) {
            continue;
        }

        parse_code = parse_line(&tok, &tok_len, line, fields, csv->nfields);
        switch (parse_code) {
            case csv_EMPTY_FILE:
            case csv_OUT_OF_MEMORY:
            case csv_PARSE_ERROR:
                free(fields);
                csv_free(csv);
                free(line);
                free(tok);
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
    free(line);
    free(tok);
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
    free(line);
    return parse_code;
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
