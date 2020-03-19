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

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

typedef char BOOL;

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

static size_t charcount(const char *src, char c) {
    size_t count = 0;
    for (; *src; src++) {
        if (*src == c) {
            count++;
        }
    }
    return count;
}

char **parse_line(const char *line) {
    char c;
    BOOL in_quote = FALSE;
    char tok[255] = {'\0'};
    char **fields;
    char **fields_cursor;
    size_t tok_index = 0;
    size_t nfields = charcount(line, ',') + 2;

    if ((fields = calloc(nfields, sizeof(char *))) == NULL) {
        return NULL;
    }

    fields_cursor = fields;

    for (; *line; line++) {
        c = *line;
        if (c == '"') {
            in_quote = !in_quote;
        }

        if ((c == ',') && (!in_quote)) {
            if ((*fields_cursor = strdup(tok)) == NULL) {
                free(fields);
                return NULL;
            }
            fields_cursor++;
            tok_index = 0;
            *tok = '\0';
        } else {
            tok[tok_index++] = c;
        }
    }

    if (*tok) {
        if ((*fields_cursor = strdup(tok)) == NULL) {
            for (fields_cursor = fields; *fields_cursor; fields_cursor++) {
                free(*fields_cursor);
            }
            free(fields);
            return NULL;
        }
        fields_cursor++;
    }
    *fields_cursor = NULL;

    return fields;
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
    enum csv_ErrorCode exit_code = csv_NO_ERROR;

    tell = ftell(fp);
    csv->nrows = countlines(fp) - 1;
    fseek(fp, tell, SEEK_SET);

    if (!fgets(line, CSV_MAX_LINE, fp)) {
        return csv_EMPTY_FILE;
    }

    strip(line);

    csv->nfields = charcount(line, ',') + 1;

    if ((csv->header = parse_line(line)) == NULL) {
        exit_code = csv_OUT_OF_MEMORY;
        goto cleanup_header;
    }

    if ((csv->data = calloc(csv->nfields, sizeof(char *))) == NULL) {
        exit_code = csv_OUT_OF_MEMORY;
        goto cleanup_cols;
    }

    for (i = 0; i < csv->nfields; i++) {
        if ((csv->data[i] = calloc(csv->nrows, sizeof(char *))) == NULL) {
            exit_code = csv_OUT_OF_MEMORY;
            goto cleanup_rows;
        }
    }

    while (fgets(line, CSV_MAX_LINE, fp)) {
        char **fields;

        strip(line);
        if (!*line) {
            continue;
        }

        if (charcount(line, ',') != (csv->nfields - 1)) {
            csv_free(csv);
            return csv_PARSE_ERROR;
        }

        fields = parse_line(line);

        for (i = 0; i < csv->nfields; i++) {
            csv->data[i][curr_line] = fields[i];
        }
        curr_line++;
    }

    return csv_NO_ERROR;

cleanup_rows:
    for (i = 0; csv->data[i] != NULL && i < csv->nfields; i++) {
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
    return exit_code;
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
