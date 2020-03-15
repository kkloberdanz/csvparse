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

#include "csvparse.h"

int main(int argc, char **argv) {
    FILE *fp;
    char *filename;
    struct CSV csv;
    enum csv_ErrorCode parse_code;

    if (argc > 1) {
        filename = argv[1];
    } else {
        fprintf(stderr, "supply an input file\n");
        return 255;
    }

    fp = fopen(filename, "r");
    parse_code = csv_parse(&csv, fp);
    switch (parse_code) {
        case csv_NO_ERROR:
            csv_print(&csv);
            csv_free(&csv);
            break;

        case csv_PARSE_ERROR:
            fprintf(stderr, "failed to parse input file\n");
            break;

        case csv_OUT_OF_MEMORY:
            fprintf(stderr, "failed to allocate memory\n");
            break;

        case csv_EMPTY_FILE:
            fprintf(stderr, "empty file\n");
            break;
    }
    return parse_code;
}
