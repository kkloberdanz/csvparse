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
#include <getopt.h>
#include <stdbool.h>

#include "csvparse.h"

enum csv_Options {
    STATS = 1,
    PRINT = 2,
    OUTPUT = 4
};

static int handle_csv(
    struct CSV *csv,
    enum csv_ErrorCode parse_code,
    int options,
    FILE *output_file
) {
    size_t i;

    switch (parse_code) {
        case csv_NO_ERROR:
            break;

        case csv_PARSE_ERROR:
            fprintf(stderr, "failed to parse input file\n");
            return (int)parse_code;

        case csv_OUT_OF_MEMORY:
            fprintf(stderr, "failed to allocate memory\n");
            return (int)parse_code;

        case csv_EMPTY_FILE:
            fprintf(stderr, "empty file\n");
            return (int)parse_code;
    }

    if (options & STATS) {
        printf("%s", "headers: ");

        for (i = 0; i < csv->nfields - 1; i++) {
            printf("%s,", csv->header[i]);
        }
        printf("%s", csv->header[i]);

        printf(
            " -- lines: %ld -- nfields: %ld\n",
            csv->nrows,
            csv->nfields
        );
    }

    if (options & PRINT) {
        csv_print(csv);
    }

    if (options & OUTPUT) {
        csv_write(csv, output_file);
    }

    return 0;
}

static int parse(const char *filename, int options, FILE *output_file) {
    FILE *fp;
    struct CSV csv;
    enum csv_ErrorCode parse_code;
    int status_code;

    fp = fopen(filename, "r");
    parse_code = csv_parse(&csv, fp);
    fclose(fp);

    status_code = handle_csv(&csv, parse_code, options, output_file);
    if (!status_code) {
        csv_free(&csv);
    }

    return status_code;
}

int main(int argc, char **argv) {
    char *filename;
    char *output_file = NULL;
    int i;
    int options = 0;
    int c;
    int status_code = 0;
    FILE *fp = NULL;

    if (argc <= 1) {
        fprintf(stderr, "csvparse: csvfile [-s] [-p] [-o outfile]\n");
        return 1;
    }

    while ((c = getopt(argc, argv, "o:ps")) != -1) {
        switch (c) {
            case 'p':
                options |= PRINT;
                break;

            case 's':
                options |= STATS;
                break;

            case 'o':
                options |= OUTPUT;
                output_file = optarg;
                break;

            case '?':
                fprintf(stderr, "unknown option: %c\n", optopt);
                return 128;

            default:
                fprintf(stderr, "default: %c\n", c);
                return 64;
        }
    }

    if (options & OUTPUT) {
        fp = fopen(output_file, "w");
    }

    for (i = optind; i < argc; i++) {
        filename = argv[i];
        status_code |= parse(filename, options, fp);
    }

    if (options & OUTPUT) {
        fclose(fp);
    }

    return status_code;
}
