#!/bin/bash
# Integration test for the webserver security/robustness fixes in src/webserver.cpp:
#  - command injection: no system() anymore (run_execvp), URL sanitation
#  - path traversal in static file serving -> 403
#  - post processor NULL handling (non-form POST must not crash)
#  - make_graph_from_url fields[size-4] OOB (short graph URLs must not crash)
#
# Usage: ./test_webserver.sh [path-to-jDaLog-binary]
# Build first:  mkdir -p build && cd build && cmake -DHAVE_S7=OFF -DENABLE_ASAN=ON ../src && make -j4

set -u
REPO="$(cd "$(dirname "$0")" && pwd)"
BIN="${1:-$REPO/build/jDaLog}"
PORT=8092
RUN=$(mktemp -d /tmp/jdtest.XXXXXX)
FAILS=0

if [ ! -x "$BIN" ]; then echo "binary niet gevonden: $BIN"; exit 1; fi

mkdir -p "$RUN/http/graphs"
cp "$REPO/http/favicon.ico" "$RUN/http/" 2>/dev/null
cat > "$RUN/config.json" <<'EOF'
{
 "scheduler_threads": 2,
 "webin": [ {"id":"test1", "name":"Test", "unit":"u", "decimals":1} ],
 "interface": [],
 "logic": [],
 "in_equation": [],
 "out": {}
}
EOF

cd "$RUN"
ASAN_OPTIONS=detect_leaks=0 "$BIN" > server.log 2>&1 &
PID=$!
sleep 2

check() { # check <beschrijving> <expected> <actual>
	if [ "$2" = "$3" ]; then echo "PASS: $1 ($3)"; else echo "FAIL: $1 (expect $2, got $3)"; FAILS=$((FAILS+1)); fi
}

code() { curl -s --path-as-is -o /dev/null -w "%{http_code}" --max-time 10 "$@"; }

check "GET /"                    200 "$(code http://localhost:$PORT/)"
check "GET /favicon.ico"         200 "$(code http://localhost:$PORT/favicon.ico)"
check "traversal /../../etc/passwd"        403 "$(code http://localhost:$PORT/../../etc/passwd)"
check "traversal /graphs/../config.json"   403 "$(code http://localhost:$PORT/graphs/../config.json)"
check "traversal /%2e%2e/config.json"      403 "$(code "http://localhost:$PORT/%2e%2e/config.json")"
check "POST json (non-form, pp=NULL pad)"  200 "$(code -X POST -H 'Content-Type: application/json' -d '{"a":1}' http://localhost:$PORT/webin/test1)"
check "POST raw zonder content-type"       200 "$(code -X POST -H 'Content-Type:' --data-binary 'raw' http://localhost:$PORT/)"

# form POST: waarde moet geparsed worden
curl -s --max-time 10 -X POST -d 'value=9' "http://localhost:$PORT/webin/test1" | grep -q 'last input: 9.0' \
	&& echo "PASS: POST form value=9 geparsed" || { echo "FAIL: POST form value=9 niet geparsed"; FAILS=$((FAILS+1)); }

# OOB regression: korte graph-URLs mogen niet crashen (voor de fix: SIGSEGV)
check "GET /graphs/a.png (1 veld, was OOB)"     500 "$(code http://localhost:$PORT/graphs/a.png)"
check "GET /graphs/a+b.png (2 velden)"           500 "$(code http://localhost:$PORT/graphs/a+b.png)"
check "GET /graphs/a+b+c+d.png (parse-fail)"    500 "$(code http://localhost:$PORT/graphs/a+b+c+d.png)"
check "server leeft na OOB tests"               200 "$(code http://localhost:$PORT/)"

# legitieme graph: png moet gegenereerd worden
NOW=$(date +%s); FROM=$((NOW-3600))
check "GET geldige graph-url" 200 "$(code "http://localhost:$PORT/graphs/graph+test1+${FROM}+${NOW}+800+300.png")"
ls "$RUN"/http/graphs/*.png >/dev/null 2>&1 \
	&& echo "PASS: png gegenereerd" || { echo "FAIL: geen png gegenereerd"; FAILS=$((FAILS+1)); }

# server moet nog steeds leven + geen ASAN errors
grep -q 'AddressSanitizer' server.log && { echo "FAIL: ASAN errors in server.log"; FAILS=$((FAILS+1)); } || echo "PASS: geen ASAN errors"
check "server leeft op het eind" 200 "$(code http://localhost:$PORT/)"

kill -9 $PID 2>/dev/null
rm -rf "$RUN"
echo "== $([ $FAILS -eq 0 ] && echo 'ALLES GESLAAGD' || echo "$FAILS FAILURES") =="
exit $FAILS
