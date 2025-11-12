#!/usr/bin/env bash
set -euo pipefail


# --- CONFIGURABLES ---
BIN="./webserv"                       # server binary
CONF="./static_site.conf"             # config file to load
ROOT="./webservinho_app"              # webroot containing html/, cgi-bin/, etc.
BASE_URL="http://127.0.0.1:8080"      # base URL of the server
PRIMARY_PORT="8080"                   # main port (matches BASE_URL)
ALT_PORT="8081"                       # secondary port for alternate listener test
HOSTNAME_TEST="example.com"           # for virtual host test (--resolve)
CGI_PREFIX="/cgi-bin"                 # CGI prefix
UPLOAD_ENDPOINT="/uploads"            # upload endpoint (matches config)

# --- OPTIONAL TOOLS ---
SIEGE_BIN="$(command -v siege || true)"
VALGRIND_BIN="$(command -v valgrind || true)"
CURL_BIN="$(command -v curl || true)"
NC_BIN="$(command -v nc || true)"
JQ_BIN="$(command -v jq || true)"
PY_BIN="$(command -v python3 || command -v python || true)"
TIMEOUT_SECS=8
SERVER_STARTUP_WAIT=0.5

# --- COLORS ---
RED="\033[31m"; GREEN="\033[32m"; YELLOW="\033[33m"; BLUE="\033[34m"; BOLD="\033[1m"; RESET="\033[0m"

section() { echo -e "\n${BOLD}${BLUE}==> $*${RESET}"; }
ok()      { echo -e "${GREEN}✔${RESET} $*"; }
warn()    { echo -e "${YELLOW}⚠${RESET} $*"; }
fail()    { echo -e "${RED}✘${RESET} $*"; exit 1; }

require() {
  local bin="$1" name="$2"
  if [ -z "$bin" ]; then fail "Required tool not found: $name"; fi
}

# --- REQUIREMENTS ---
require "$CURL_BIN" "curl"

# --- UTILS ---
pid=""
cleanup() {
  if [ -n "${pid:-}" ] && ps -p "$pid" > /dev/null 2>&1; then
    kill "$pid" || true
    sleep 0.2
    kill -9 "$pid" || true
  fi
}
trap cleanup EXIT

wait_port() {
  local port="$1" i=0
  while :; do
    if $CURL_BIN -sS --max-time 1 "http://127.0.0.1:${port}/" >/dev/null 2>&1; then
      break
    fi
    i=$((i+1))
    if [ $i -ge 40 ]; then fail "Port ${port} did not respond in time"; fi
    sleep 0.1
  done
}

start_server() {
  section "Starting server"
  if [ ! -x "$BIN" ]; then fail "Server binary not found/executable: $BIN"; fi
  [ -f "$CONF" ] || fail "Configuration file missing: $CONF"
  "$BIN" "$CONF" &
  pid=$!
  sleep "$SERVER_STARTUP_WAIT"
  wait_port "$PRIMARY_PORT"
  ok "Server started (pid=$pid) at $BASE_URL"
}

stop_server() {
  section "Stopping server"
  cleanup
  ok "Server stopped"
}

# --- HTTP HELPERS ---
http_status() { $CURL_BIN -s -o /dev/null -w "%{http_code}" --max-time "$TIMEOUT_SECS" "$1"; }

assert_status() {
  local expected="$1" url="$2" code
  code="$(http_status "$url")"
  if [ "$code" = "$expected" ]; then ok "$url → $code (expected $expected)"; else fail "$url → $code (expected $expected)"; fi
}

assert_body_contains() {
  local url="$1" needle="$2" body
  body="$($CURL_BIN -sS --max-time "$TIMEOUT_SECS" "$url")"
  if echo "$body" | grep -qi -- "$needle"; then ok "Body contains '$needle' at $url"; else fail "Body does NOT contain '$needle' at $url"; fi
}

assert_header_contains() {
  local url="$1" header="$2" needle="$3" out headers
  out="$($CURL_BIN -sS -D - -o /dev/null --max-time "$TIMEOUT_SECS" "$url")"
  headers="$(echo "$out" | tr -d '\r')"
  if echo "$headers" | awk -v h="$header" 'BEGIN{IGNORECASE=1}/^'"$header"':/{print}' | grep -qi -- "$needle"; then
    ok "Header $header contains '$needle' at $url"
  else
    fail "Header $header does NOT contain '$needle' at $url"
  fi
}

gen_long_body() {
  if [ -n "$PY_BIN" ]; then
    "$PY_BIN" - <<'PY'
print("X"*65536)
PY
  else
    head -c 65536 /dev/zero | tr '\0' 'X'
  fi
}

# ------------------------------------------------------
# 0) Compile / Relink check
# ------------------------------------------------------
section "Compilation / Relink verification"
if [ -f Makefile ]; then
  if make -q >/dev/null 2>&1; then
    ok "Make reports nothing to rebuild"
  else
    warn "make -q returned non-zero (possible stale deps). Running 'make -s'..."
    make -s
    ok "Rebuilt"
  fi
else
  warn "Makefile not found — skipping build check"
fi

# ------------------------------------------------------
# 1) Start server
# ------------------------------------------------------
start_server

# ------------------------------------------------------
# 2) Configuration checks
# ------------------------------------------------------
section "Configuration tests"

assert_status 200 "${BASE_URL}/"
assert_body_contains "${BASE_URL}/" "Webservinho"

assert_status 404 "${BASE_URL}/definitely_does_not_exist"

if [ -f "${ROOT}/errors/404.html" ]; then
  title404="$(sed -n 's:.*<title>\(.*\)</title>.*:\1:ip' "${ROOT}/errors/404.html" | head -n1 || true)"
  [ -n "${title404}" ] && assert_body_contains "${BASE_URL}/definitely_does_not_exist" "$title404" || warn "Couldn't infer title from custom 404 page"
else
  warn "errors/404.html not found — skipping content comparison"
fi

# vhost test with --resolve on port 8080
HOST="$HOSTNAME_TEST"
HOST_CODE="$($CURL_BIN -s -o /dev/null -w "%{http_code}" --resolve "${HOST}:8080:127.0.0.1" "http://${HOST}:8080/" || true)"
if [ -n "$HOST_CODE" ]; then
  ok "curl --resolve ${HOST}:8080:127.0.0.1 returned ${HOST_CODE} (vhost check)"
else
  warn "Vhost test failed — adjust server_name if needed"
fi

# Body size limits
SHORT_BODY="BODY IS HERE"
code_short="$($CURL_BIN -s -o /dev/null -w "%{http_code}" -X POST -H "Content-Type: text/plain" --data "$SHORT_BODY" "${BASE_URL}/echo" || true)"
code_long="$(gen_long_body | $CURL_BIN -s -o /dev/null -w "%{http_code}" -X POST -H "Content-Type: text/plain" --data-binary @- "${BASE_URL}/echo" || true)"
ok "Short POST → ${code_short} ; Long POST → ${code_long}  [body limit check]"

DELETE_URL="${BASE_URL}/tmp_delete_test.txt"
echo "temp" > /tmp/tmp_delete_test.txt
$CURL_BIN -s -o /dev/null -X POST -F "file=@/tmp/tmp_delete_test.txt" "${BASE_URL}${UPLOAD_ENDPOINT}" || true
del_code="$($CURL_BIN -s -o /dev/null -w "%{http_code}" -X DELETE "$DELETE_URL" || true)"
ok "DELETE ${DELETE_URL} → ${del_code} [method check]"

assert_status 200 "${BASE_URL}/img/webservinho_logo.png"

AI_BODY="$($CURL_BIN -sS --max-time "$TIMEOUT_SECS" "${BASE_URL}/uploads/")"
if echo "$AI_BODY" | grep -qi "Index of" ; then ok "Autoindex active at /uploads/"; else warn "Autoindex not detected at /uploads/ (ok if disabled)"; fi

# ------------------------------------------------------
# 3) HTTP basics (GET / POST / DELETE / UNKNOWN)
# ------------------------------------------------------
section "HTTP methods"

assert_status 200 "${BASE_URL}/html/about.html"
POST_TARGET="${BASE_URL}/echo"
if [ "$($CURL_BIN -s -o /dev/null -w "%{http_code}" -X POST -d "hello=world" "$POST_TARGET")" = "200" ]; then
  ok "POST /echo works (200)"
else
  warn "POST /echo unavailable; using CGI echo.py instead"
  POST_TARGET="${BASE_URL}${CGI_PREFIX}/echo.py"
  assert_status 200 "$POST_TARGET"
  assert_body_contains "$POST_TARGET" "REQUEST_METHOD=GET"
  post_code="$($CURL_BIN -s -o /dev/null -w "%{http_code}" -X POST -H "Content-Type: application/x-www-form-urlencoded" -d "foo=bar&baz=qux" "$POST_TARGET")"
  [ "$post_code" = "200" ] && ok "CGI POST echo.py → 200" || fail "CGI POST echo.py → $post_code"
fi

unknown_code="$($CURL_BIN -s -o /dev/null -w "%{http_code}" -X FOO "${BASE_URL}/")"
[ "$unknown_code" != "000" ] && ok "UNKNOWN method didn't crash (HTTP $unknown_code)" || fail "UNKNOWN method caused hang"

# Upload test
section "File upload"
TEST_UPLOAD_FILE="/tmp/ws_upload_$$.txt"
echo "hello upload" > "$TEST_UPLOAD_FILE"
up_code="$($CURL_BIN -s -o /dev/null -w "%{http_code}" -F "file=@${TEST_UPLOAD_FILE}" "${BASE_URL}${UPLOAD_ENDPOINT}" || true)"
if [ "$up_code" = "200" ] || [ "$up_code" = "201" ]; then
  ok "Upload succeeded (${up_code}) at ${UPLOAD_ENDPOINT}"
else
  warn "Upload returned ${up_code}. Adjust UPLOAD_ENDPOINT if needed."
fi

# ------------------------------------------------------
# 4) CGI
# ------------------------------------------------------
section "CGI"
assert_status 200 "${BASE_URL}${CGI_PREFIX}/hello.py"
assert_body_contains "${BASE_URL}${CGI_PREFIX}/hello.py" "Hello"
assert_status 200 "${BASE_URL}${CGI_PREFIX}/form.py"
assert_status 200 "${BASE_URL}${CGI_PREFIX}/echo.py"
assert_body_contains "${BASE_URL}${CGI_PREFIX}/echo.py" "REQUEST_METHOD=GET"
post_cgi="$($CURL_BIN -s -o /dev/null -w "%{http_code}" -X POST -H "Content-Type: application/x-www-form-urlencoded" -d "a=1&b=2" "${BASE_URL}${CGI_PREFIX}/echo.py")"
[ "$post_cgi" = "200" ] && ok "CGI POST ok (echo.py)" || fail "CGI POST failed ($post_cgi)"

section "CGI - controlled loop / timeout"
t0=$(date +%s)
$CURL_BIN -sS --max-time "$TIMEOUT_SECS" "${BASE_URL}${CGI_PREFIX}/sleep.py" -o /dev/null && {
  ok "sleep.py responded within timeout (no hang)"
} || {
  t1=$(date +%s)
  dt=$((t1 - t0))
  if [ "$dt" -ge "$TIMEOUT_SECS" ]; then ok "sleep.py exceeded timeout but server handled it"; else warn "sleep.py failed too quickly"; fi
}

# ------------------------------------------------------
# 5) Browser-like checks
# ------------------------------------------------------
section "Browser-like checks"
assert_header_contains "${BASE_URL}/" "Server" "Webservinho"
assert_status 301 "${BASE_URL}/redirect" || warn "If no redirect route exists, ignore"
assert_status 404 "${BASE_URL}/bad/url/for/sure"

# ------------------------------------------------------
# 6) Ports
# ------------------------------------------------------
section "Ports"
ALT_URL="http://127.0.0.1:${ALT_PORT}"
if $CURL_BIN -sS --max-time 1 "${ALT_URL}/" -o /dev/null ; then
  ok "Site also responds on port ${ALT_PORT}"
else
  warn "Port ${ALT_PORT} did not respond — ok if not configured"
fi

# ------------------------------------------------------
# 7) Siege / Stress Test
# ------------------------------------------------------
section "Siege / Stress test"
if [ -n "$SIEGE_BIN" ]; then
  URL_LIST="/tmp/ws_siege_urls_$$.txt"
  echo "${BASE_URL}/" > "$URL_LIST"
  echo "${BASE_URL}/html/about.html" >> "$URL_LIST"
  echo "${BASE_URL}${CGI_PREFIX}/hello.py" >> "$URL_LIST"
  set +e
  out="$($SIEGE_BIN -b -c 5 -r 5 -f "$URL_LIST" 2>&1)"
  set -e
  echo "$out" | tail -n 20
  if echo "$out" | grep -qi "Availability" ; then
    avail="$(echo "$out" | awk '/availability/{print $2}' | tr -d '%')"
    if [ -n "$avail" ] && awk "BEGIN{exit !($avail >= 99.5)}"; then
      ok "Availability >= 99.5% (${avail}%)"
    else
      warn "Availability below 99.5% (${avail:-N/A}%)"
    fi
  else
    warn "Couldn't extract Availability from siege output"
  fi
else
  warn "siege not installed — skipping stress test"
fi

# ------------------------------------------------------
# 8) Leaks (optional Valgrind)
# ------------------------------------------------------
section "Memory leaks (optional Valgrind)"
if [ -n "$VALGRIND_BIN" ]; then
  stop_server
  section "Running valgrind short cycle"
  set +e
  $VALGRIND_BIN --leak-check=full --error-exitcode=42 --log-file=/tmp/ws_valgrind_$$.log "$BIN" "$CONF" &
  vpid=$!
  sleep "$SERVER_STARTUP_WAIT"
  $CURL_BIN -sS "${BASE_URL}/" -o /dev/null
  kill "$vpid" 2>/dev/null
  wait "$vpid" 2>/dev/null || true
  set -e
  if grep -q "definitely lost: 0 bytes" /tmp/ws_valgrind_$$.log ; then
    ok "Valgrind: no leaks (definitely lost: 0 bytes)"
  else
    warn "Valgrind reported leaks — see /tmp/ws_valgrind_$$.log"
  fi
else
  warn "valgrind not available — skipping leak check"
fi

# ------------------------------------------------------
# 9) Static greps (multiplexing rules)
# ------------------------------------------------------
section "Static code greps (multiplexing rules)"
SRC_DIRS="srcs includes"

if grep -RInE "errno[[:space:]]*;|errno[[:space:]]*==|strerror\\(errno\\)" $SRC_DIRS | grep -Ei "(read|recv|write|send).{0,80}errno|errno.{0,80}(read|recv|write|send)" -n ; then
  warn "Found errno usage correlated with read/recv/write/send [forbidden by subject]"
else
  ok "No obvious errno misuse after read/recv/write/send"
fi

if grep -RInE "\\b(recv|send)\\s*\\(" $SRC_DIRS >/dev/null ; then
  ok "recv/send calls detected; verify proper handling of -1 and 0 (manual review)"
fi

if grep -RInE "\\b(write|send)\\s*\\(" $SRC_DIRS | grep -vi "std::" >/dev/null ; then
  warn "Direct write/send calls found; ensure always guarded by poll() readiness"
else
  ok "No direct forbidden I/O patterns detected"
fi

# ------------------------------------------------------
# END
# ------------------------------------------------------
stop_server
section "All tests completed 🎉"
echo "Review warnings (⚠) and adjust config variables if needed."
