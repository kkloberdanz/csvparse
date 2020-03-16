# 
#      Copyright (C) 2020 Kyle Kloberdanz
# 
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU Affero General Public License as
#   published by the Free Software Foundation, either version 3 of the
#   License, or (at your option) any later version.
# 
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU Affero General Public License for more details.
# 
#   You should have received a copy of the GNU Affero General Public License
#   along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

CC=cc
AR=ar rcs
OPTIM=-Os
WARN_FLAGS=-Wall -Wextra -Wpedantic -Wswitch
STD=-std=gnu89
VISIBILITY=-fvisibility=hidden
CFLAGS=$(OPTIM) $(WARN_FLAGS) $(STD) $(VISIBILITY) -fPIC

.PHONY: all
all: csvparse libcsvparse.so libcsvparse.a

.PHONY: test
test: all
	./csvparse -s testdata/voo_historical.csv -p

.PHONY: debug
debug: OPTIM := -ggdb3 -O0 -Werror
debug: all

.PHONY: clang-everything
clang-everything: WARN_FLAGS := $(WARN_FLAGS) -Weverything
clang-everything: CC := clang
clang-everything: all

.PHONY: sanitize
sanitize: OPTIM := -ggdb3 -O0 -Werror \
	-fsanitize=address \
	-fsanitize=leak \
	-fsanitize=undefined
sanitize: all
	./csvparse -s -p testdata/voo_historical.csv

.PHONY: valgrind
valgrind: VALGRIND_FLAGS := \
	--leak-check=full \
	--track-origins=yes \
	--show-reachable=yes
valgrind: debug
	valgrind $(VALGRIND_FLAGS) ./csvparse -s -p testdata/voo_historical.csv

csvparse: main.o libcsvparse.a
	$(CC) -o csvparse main.o libcsvparse.a $(CFLAGS)

main.o: main.c csvparse.h
	$(CC) -c main.c $(CFLAGS)

libcsvparse.a: csvparse.o
	$(AR) libcsvparse.a csvparse.o

csvparse.o: csvparse.c csvparse.h 
	$(CC) -c csvparse.c $(CFLAGS)

libcsvparse.so: csvparse.o
	$(CC) -shared -o libcsvparse.so csvparse.o $(CFLAGS)

.PHONY: clean
clean:
	rm -f *.o
	rm -f *.a
	rm -f *.so
	rm -f csvparse
