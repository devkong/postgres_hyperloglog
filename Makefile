MODULE_big = hyperloglog_counter
OBJS = src/hyperloglog_counter.o src/hyperloglog.o src/legacy.o

EXTENSION = hyperloglog_counter
DATA = sql/hyperloglog_counter--1.1.0--1.2.0.sql  sql/hyperloglog_counter--1.2.0--1.2.3.sql  sql/hyperloglog_counter--1.2.3.sql
MODULES = hyperloglog_counter

OUT_DIR = test/expected
SQL_DIR = test/sql
TESTS        = $(wildcard $(SQL_DIR)*.sql)
REGRESS      = $(patsubst $(OUT_DIR)%,%,$(TESTS))
REGRESS_OPTS = -X --echo-all -P null=NULL
PSQL = psql

TEST = test/sql/base.out

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

all: hyperloglog_counter.so

hyperloglog_counter.so: $(OBJS)

%.o : src/%.c

tests: clean_test $(TEST)
	@find . -maxdepth 1 -name '*.diff' -print -quit > failures
	@if test -s failures; then \
        echo ERROR: `ls -1 $(SQL_DIR)/*.diff | wc -l` / `ls -1 $(SQL_DIR)/*.sql | wc -l` tests failed; \
        echo; \
        cat *.diff; \
        exit 1; \
    else \
        rm failures; \
        echo `ls -1 $(SQL_DIR)/*.out | wc -l` / `ls -1 $(SQL_DIR)/*.sql | wc -l` tests passed; \
    fi

%.out:
	@echo $*
	@if test -f ../testdata/$*.csv; then \
      PGOPTIONS=$(PGOPTIONS) $(PSQL) $(PSQLOPTS) -f $*.sql < ../testdata/$*.csv > $*.out 2>&1; \
    else \
      PGOPTIONS=$(PGOPTIONS) $(PSQL) $(PSQLOPTS) -f $*.sql >> $*.out 2>&1; \
    fi
	@diff -u $*.ref $*.out >> $*.diff || status=1
	@if test -s $*.diff; then \
        echo " .. FAIL"; \
    else \
        echo " .. PASS"; \
        rm -f $*.diff; \
    fi

clean_test:
	rm -f $(SQL_DIR)/*.out $(SQL_DIR)/*.diff
