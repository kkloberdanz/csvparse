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

enum {
    BUF_SIZE = 1024
};

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
    return NULL;
}

static size_t charcount(const char *src, char c) {
    size_t count = 0;
    while (*src++) {
        if (*src == c) {
            count++;
        }
    }
    return count;
}

static char *replace(char *dst, char from_this, char to_that) {
    size_t i;
    for (i = 0; dst[i] != '\0'; i++) {
        if (dst[i] == from_this) {
            dst[i] = to_that;
        }
    }
    return dst;
}

static char *strip(char *dst) {
    return replace(dst, '\n', '\0');
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
    char header_buf[BUF_SIZE];
    char line[BUF_SIZE];
    size_t curr_line = 0;
    size_t i;

    csv->nrows = countlines(fp) - 1;
    fseek(fp, 0, SEEK_SET);

    if (!fgets(header_buf, BUF_SIZE, fp)) {
        return csv_EMPTY_FILE;
    }

    strip(header_buf);

    csv->nfields = charcount(header_buf, ',') + 1;

    if ((csv->header = malloc(csv->nfields * sizeof(char *))) == NULL) {
        goto cleanup_header;
    }

    for (i = 0; i < csv->nfields; i++) {
        char tmp[BUF_SIZE];
        char *col_name;
        strcpy(tmp, header_buf);
        if ((col_name = getfield(tmp, i + 1)) == NULL) {
            csv_free(csv);
            return csv_OUT_OF_MEMORY;
        }
        csv->header[i] = col_name;
    }

    if ((csv->data = calloc(csv->nfields, sizeof(char *))) == NULL) {
        goto cleanup_cols;
    }

    for (i = 0; i < csv->nfields; i++) {
        if ((csv->data[i] = calloc(csv->nrows, sizeof(char *))) == NULL) {
            goto cleanup_rows;
        }
    }

    while (fgets(line, BUF_SIZE, fp)) {
        char tmp[BUF_SIZE];
        char *field;
        strip(line);
        for (i = 0; i < csv->nfields; i++) {
            strcpy(tmp, line);
            if ((field = getfield(tmp, i + 1)) == NULL) {
                csv_free(csv);
                return csv_OUT_OF_MEMORY;
            }
            csv->data[i][curr_line] = field;
        }
        curr_line++;
    }

    return csv_NO_ERROR;

cleanup_rows:
    for (i = 0; csv->data[i] != NULL; i++) {
        free(csv->data[i]);
    }

cleanup_cols:
    free(csv->data);

cleanup_header:
    free(csv->header);
    return csv_OUT_OF_MEMORY;
}

void csv_print(struct CSV *csv) {
    size_t i, j;
    for (i = 0; i < csv->nfields; i++) {
        char *col_name = csv->header[i];
        if (i >= csv->nfields - 1) {
            printf("%s\n", col_name);
        } else {
            printf("%s,", col_name);
        }
    }

    for (j = 0; j < csv->nrows; j++) {
        for (i = 0; i < csv->nfields; i++) {
            if (i >= csv->nfields - 1) {
                printf("%s\n", csv->data[i][j]);
            } else {
                printf("%s,", csv->data[i][j]);
            }
        }
    }
}
