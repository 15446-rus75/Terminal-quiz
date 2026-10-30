#!/bin/sh
set -eu

if [ "$#" -lt 1 ]; then
  echo 'Usage: run_tests.sh /path/to/run_tests [Boost.Test arguments]' >&2
  exit 2
fi
if [ "$(id -u)" -eq 0 ]; then
  echo 'PostgreSQL initdb requires a regular user. Run make test without sudo.' >&2
  exit 2
fi
pg_bin=$(${PG_CONFIG:-pg_config} --bindir 2>/dev/null || true)
for tool in initdb pg_ctl; do
  if [ -n "$pg_bin" ] && [ -x "$pg_bin/$tool" ]; then
    continue
  fi
  if ! command -v "$tool" >/dev/null 2>&1; then
    echo "Missing PostgreSQL server tools ($tool). Install PostgreSQL and pg_trgm, or set PG_CONFIG." >&2
    exit 2
  fi
done
if [ -n "$pg_bin" ]; then
  PATH="$pg_bin:$PATH"
  export PATH
fi

task_cluster=$(mktemp -d /tmp/quiz-db-tests.XXXXXXXX)
cleanup()
{
  status=$?
  trap - EXIT
  if [ -f "$task_cluster/data/postmaster.pid" ]; then
    if ! pg_ctl -D "$task_cluster/data" -m immediate -w stop >/dev/null 2>&1; then
      echo "Could not stop test PostgreSQL; retained $task_cluster" >&2
      exit 1
    fi
  fi
  if [ "$status" -ne 0 ] && [ -f "$task_cluster/server.log" ]; then
    tail -n 30 "$task_cluster/server.log" >&2
  fi
  rm -rf -- "$task_cluster"
  exit "$status"
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
mkdir "$task_cluster/socket"
initdb -D "$task_cluster/data" -A trust --locale=C -E UTF8 > "$task_cluster/initdb.log" 2>&1 || {
  cat "$task_cluster/initdb.log" >&2
  exit 1
}
pg_ctl -D "$task_cluster/data" -l "$task_cluster/server.log" \
  -o "-h '' -k $task_cluster/socket -p 5432 -F" -w start
QUIZ_TEST_ISOLATED=1
QUIZ_TEST_SOCKET="$task_cluster/socket"
QUIZ_TEST_USER=$(id -un)
export QUIZ_TEST_ISOLATED QUIZ_TEST_SOCKET QUIZ_TEST_USER
"$@"
