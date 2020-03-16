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

int parse(const char *filename, bool print, bool stats) {
    FILE *fp;
    struct CSV csv;
    size_t i;
    enum csv_ErrorCode parse_code;

    fp = fopen(filename, "r");
    parse_code = csv_parse(&csv, fp);
    fclose(fp);

    switch (parse_code) {
        case csv_NO_ERROR:
            break;

        case csv_PARSE_ERROR:
            fprintf(stderr, "failed to parse input file\n");
            return parse_code;
            break;

        case csv_OUT_OF_MEMORY:
            fprintf(stderr, "failed to allocate memory\n");
            return parse_code;
            break;

        case csv_EMPTY_FILE:
            fprintf(stderr, "empty file\n");
            return parse_code;
            break;
    }

    if (stats) {
        printf("%s", "headers: ");

        for (i = 0; i < csv.nfields; i++) {
            printf("%s,", csv.header[i]);
        }

        printf(
            " -- lines: %ld -- nfields: %ld\n",
            csv.nrows,
            csv.nfields
        );
    }

    if (print) {
        csv_print(&csv);
    }
    csv_free(&csv);
    return 0;
}

int main(int argc, char **argv) {
    char *filename;
    int i;
    bool print_flag = 0;
    bool stats_flag = 0;
    int c;
    int status_code = 0;

    while ((c = getopt(argc, argv, "ps")) != -1) {
        switch (c) {
            case 'p':
                print_flag = 1;
                break;

            case 's':
                stats_flag = 1;
                break;

            case '?':
                fprintf(stderr, "unknown option: %c\n", optopt);
                exit(254);

            default:
                fprintf(stderr, "default: %c\n", c);
                exit(255);
        }
    }

    for (i = optind; i < argc; i++) {
        filename = argv[i];
        status_code &= parse(filename, print_flag, stats_flag);
    }

    return status_code;
}
