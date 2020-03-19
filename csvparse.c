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

static char *tok_not_found = "token not found";

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

static char *getfield(char *line, size_t num) {
    char *tok;
    for (tok = strtok(line, ","); tok && *tok; tok = strtok(NULL, ",\n")) {
        if (!--num) {
            return strdup(tok);
        }
    }
    return tok_not_found;
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
    char header_buf[CSV_MAX_LINE];
    char line[CSV_MAX_LINE];
    size_t curr_line = 0;
    size_t i;
    long tell;
    enum csv_ErrorCode exit_code = csv_NO_ERROR;

    tell = ftell(fp);
    csv->nrows = countlines(fp) - 1;
    fseek(fp, tell, SEEK_SET);

    if (!fgets(header_buf, CSV_MAX_LINE, fp)) {
        return csv_EMPTY_FILE;
    }

    strip(header_buf);

    csv->nfields = charcount(header_buf, ',') + 1;

    if ((csv->header = calloc(csv->nfields, sizeof(char *))) == NULL) {
        exit_code = csv_OUT_OF_MEMORY;
        goto cleanup_header;
    }

    for (i = 0; i < csv->nfields; i++) {
        char tmp[CSV_MAX_LINE];
        char *col_name;
        strcpy(tmp, header_buf);
        if ((col_name = getfield(tmp, i + 1)) == NULL) {
            exit_code = csv_OUT_OF_MEMORY;
            goto cleanup_header;
        } else if (col_name == tok_not_found) {
            exit_code = csv_PARSE_ERROR;
            goto cleanup_header;
        }
        csv->header[i] = col_name;
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
        char tmp[CSV_MAX_LINE];
        char *field;

        strip(line);
        if (!*line) {
            continue;
        }

        if (charcount(line, ',') != (csv->nfields - 1)) {
            csv_free(csv);
            return csv_PARSE_ERROR;
        }

        for (i = 0; i < csv->nfields; i++) {
            strcpy(tmp, line);
            if ((field = getfield(tmp, i + 1)) == NULL) {
                csv_free(csv);
                return csv_OUT_OF_MEMORY;
            } else if (field == tok_not_found) {
                csv_free(csv);
                return csv_PARSE_ERROR;
            }
            csv->data[i][curr_line] = field;
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
