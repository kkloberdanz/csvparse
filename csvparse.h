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

#ifndef CSVPARSE_H
#define CSVPARSE_H

#ifndef TARGET_EXPORT
#    ifdef WIN32
#        ifdef EXPORTING
#            define TARGET_EXPORT __declspec(dllexport)
#        else
#            define TARGET_EXPORT __declspec(dllimport)
#        endif
#    else
#        define TARGET_EXPORT __attribute__ ((visibility ("default")))
#    endif
#endif

#include <stdio.h>

struct CSV {
    char ***data;
    char **header;
    size_t nfields;
    size_t nrows;
};

enum csv_ErrorCode {
    csv_NO_ERROR = 0,
    csv_PARSE_ERROR = 1,
    csv_OUT_OF_MEMORY = 2,
    csv_EMPTY_FILE = 4
};

TARGET_EXPORT void csv_print(struct CSV *csv);
TARGET_EXPORT void csv_write(struct CSV *csv, FILE *fp);
TARGET_EXPORT enum csv_ErrorCode csv_parse(struct CSV *csv, FILE *fp);
TARGET_EXPORT void csv_free(struct CSV *csv);

#endif /* CSVPARSE_H */
