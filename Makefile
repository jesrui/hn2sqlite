SQLITE3INC = $(shell pkg-config --cflags sqlite3)
SQLITE3LIB = $(shell pkg-config --libs sqlite3)

CXXFLAGS = ${SQLITE3INC} -std=c++11 -DNDEBUG -I. -Wall -Wno-unused-local-typedefs -O2 -msse2
#CXXFLAGS = ${SQLITE3INC} -std=c++11 -DNDEBUG -I. -Wall -Wno-unused-local-typedefs -O0 -g -msse2
LDLIBS = ${SQLITE3LIB}
LDFLAGS += -s

hn2sqlite: hn2sqlite.cpp


test_sqlite: test_sqlite.cpp


.PHONY: clean
clean:
	${RM} hn2sqlite

