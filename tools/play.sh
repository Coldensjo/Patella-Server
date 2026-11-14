#!/bin/bash
# Starts everything a 7.72 client needs to play on this tree's game server, in
# Linux or WSL: the query manager, the login server (port 7171) and the game
# (port 7172). Ctrl-C stops all three cleanly, the game first, so that it can
# save its players and log them out.
#
# usage:
#   tools/play.sh               start; Ctrl-C stops
#   tools/play.sh seed ARGS...  run querymanager-seed on the play database, e.g.
#                                 tools/play.sh seed --add-account 1234567 --password secret \
#                                   --new-character "Name" --world Zanera --sex 1
#   tools/play.sh modulus       print the RsaModulus= line clients need: the play copy's key
#   tools/play.sh reset         delete the play copy and its database
#
# The play copy is a runtime copy of the original installation (PLAY_INSTALL, a tar
# or a folder) in PLAY_ROOT, laid out for this tree: data/config.toml becomes its
# config.toml (with QM_PORT and SHM_KEY set), the other data/*.toml files go into
# its data folder ([paths] data), and data/origmap.zip fills its origmap folder
# when the installation has none (map/ then starts as a copy of it). The rest
# (monsters, NPCs, owners.dat, monster.db) comes from the installation. The RSA
# key is the copy's key.pem (the file rsa_key in its config.toml names, key.pem
# by default), which the game and the login server (--key) both read; a copy
# without one gets PLAY_KEY, or else a new key from the build's rsakeygen. Its
# player files are imported into the database (PLAY_RUN/querymanager.sqlite) by
# querymanager-seed when the database is made; the game itself never reads them.
# Copy and database are made on first use and kept, so characters keep their
# progress; the installation and the repository are not touched.
#
# Environment (defaults in brackets):
#   PLAY_BUILD      the build directory [$HOME/build/patella-server]
#   PLAY_INSTALL    the original installation, a tar or a folder [$HOME/patella-cmp/pristine.tar]
#   PLAY_ROOT       the play copy [$HOME/game-play-main]
#   PLAY_RUN        the database, logs and consoles [$HOME/patella-play-main]
#   LOGIN_PASSWORD  the login server's query manager password, stored when the database is made [patella-login]
#   MOTD            a message of the day for the login server [none]
#   LOGIN_PORT      the login server's port [7171]
#   GAME_PORT       the game's port, stored when the database is made [7172]
#   QM_PORT, SHM_KEY  the query manager's port and the game's SHM key, written into the copy's
#                   config.toml when the copy is made [7173, 10011]; later runs read them from it
#   PLAY_KEY        an existing key file (PEM) copied into the play copy when it has no key yet
#                   [none: rsakeygen makes a new one]
#
# The client needs the modulus of the copy's key (tools/play.sh modulus) as
# RsaModulus= in its config. Keep key.pem private; reset deletes it with the copy.
set -u
REPO=$(cd "$(dirname "$0")/.." && pwd)
BUILD=${PLAY_BUILD:-$HOME/build/patella-server}
INSTALL=${PLAY_INSTALL:-$HOME/patella-cmp/pristine.tar}
ROOT=${PLAY_ROOT:-$HOME/game-play-main}
RUN=${PLAY_RUN:-$HOME/patella-play-main}
QM=$BUILD/querymanager/querymanager
SEED=$BUILD/querymanager/querymanager-seed
LS=$BUILD/loginserver/loginserver
GAME=$BUILD/src/game
KEYGEN=$BUILD/keygen/rsakeygen
DB=$RUN/querymanager.sqlite
LOGIN_PASSWORD=${LOGIN_PASSWORD:-patella-login}
MOTD=${MOTD:-}
LOGIN_PORT=${LOGIN_PORT:-7171}
GAME_PORT=${GAME_PORT:-7172}
NEW_QM_PORT=${QM_PORT:-7173}
NEW_SHM_KEY=${SHM_KEY:-10011}

die() {
	echo "play.sh: $*" >&2
	exit 1
}

# Prints a value of the copy's config.toml: a key of the top level, of
# [paths], or the port of the first [[query_manager]].
config_value() {
	python3 -I - "$ROOT/config.toml" "$1" <<'EOF'
import sys, tomllib
with open(sys.argv[1], "rb") as f:
	config = tomllib.load(f)
key = sys.argv[2]
if key == "qm_port":
	value = config.get("query_manager", [{}])[0].get("port")
elif key.startswith("paths."):
	value = config.get("paths", {}).get(key[6:])
else:
	value = config.get(key)
if value is None:
	sys.exit(1)
print(value)
EOF
}

# Writes data/config.toml as the copy's config.toml with the SHM key and the
# query manager ports (QM_PORT and the ports after it) set. Its paths must be
# relative, so that they point into the copy.
write_config() {
	python3 -I - "$REPO/data/config.toml" "$ROOT/config.toml" "$NEW_SHM_KEY" "$NEW_QM_PORT" <<'EOF'
import re, sys, tomllib
source, target, shm, port = sys.argv[1], sys.argv[2], int(sys.argv[3]), int(sys.argv[4])
with open(source, encoding="utf-8") as f:
	text = f.read()
config = tomllib.loads(text)
for key, value in config.get("paths", {}).items():
	if not isinstance(value, str) or value.startswith("/"):
		sys.exit("data/config.toml: [paths] %s = %r is not a relative path" % (key, value))
text, count = re.subn(r"(?m)^(\s*shm_key\s*=\s*)-?\d+", lambda m: "%s%d" % (m.group(1), shm), text)
if count != 1:
	sys.exit("data/config.toml: no shm_key")
entries = []
# port is a key of [[query_manager]] only.
def renumber(m):
	entries.append(m)
	return "%s%d" % (m.group(1), port + len(entries) - 1)
text = re.sub(r"(?m)^(\s*port\s*=\s*)\d+", renumber, text)
patched = tomllib.loads(text)
if patched["shm_key"] != shm or [q["port"] for q in patched.get("query_manager", [])] != [port + i
		for i in range(len(entries))] or not entries:
	sys.exit("data/config.toml: the SHM key or the query manager ports could not be set")
with open(target, "w", encoding="utf-8", newline="\n") as f:
	f.write(text)
EOF
}

# The copy's key file: rsa_key of its config.toml, relative to the copy, or
# key.pem in the copy.
key_file() {
	local name
	name=$(config_value rsa_key 2> /dev/null) || name=key.pem
	case $name in
	/*) echo "$name" ;;
	*) echo "$ROOT/$name" ;;
	esac
}

# Prints the RsaModulus= line of the copy's key (rsakeygen --modulus; the
# primes are never printed).
modulus() {
	local key
	[ -x "$KEYGEN" ] || die "$KEYGEN is missing; build with: cmake --build $BUILD -j8"
	[ -f "$ROOT/config.toml" ] || die "no play copy in $ROOT yet; start tools/play.sh once to make it"
	key=$(key_file)
	[ -f "$key" ] || die "the play copy has no key file $key yet; start tools/play.sh once to make it"
	"$KEYGEN" --modulus "$key" || die "$key is not a usable key file"
}

# Gives the copy its key file when it has none: PLAY_KEY, or a new key. A key
# file it has is checked, so that a bad one stops the script before any
# server is started.
make_key() {
	local key
	key=$(key_file)
	if [ -f "$key" ]; then
		"$KEYGEN" --modulus "$key" > /dev/null \
			|| die "$key is not a usable key file; replace it, or delete it to get a new key"
		return 0
	fi
	if [ -n "${PLAY_KEY:-}" ]; then
		"$KEYGEN" --modulus "$PLAY_KEY" > /dev/null || die "PLAY_KEY=$PLAY_KEY is not a usable key file"
		echo "copying the key file $PLAY_KEY to $key"
		mkdir -p "$(dirname "$key")" && (umask 077 && cp "$PLAY_KEY" "$key") && chmod 600 "$key" \
			|| die "cannot copy $PLAY_KEY to $key"
	else
		echo "making a new RSA key in $key"
		mkdir -p "$(dirname "$key")" && "$KEYGEN" "$key" > /dev/null || die "cannot make the key file $key"
	fi
}

setup() {
	local program data origmap map scratch name
	for program in "$QM" "$SEED" "$LS" "$GAME" "$KEYGEN"; do
		[ -x "$program" ] || die "$program is missing; build with: cmake --build $BUILD -j8"
	done
	if [ ! -f "$ROOT/config.toml" ]; then
		[ -e "$INSTALL" ] || die "$INSTALL is missing; set PLAY_INSTALL to the original installation (a tar or a folder)"
		[ -e "$ROOT" ] && die "$ROOT exists but has no config.toml; remove it or run: tools/play.sh reset"
		echo "making the play copy in $ROOT"
		mkdir -p "$ROOT" || exit 1
		if [ -d "$INSTALL" ]; then
			cp -a "$INSTALL/." "$ROOT/" || die "cannot copy $INSTALL"
		else
			tar -x -C "$ROOT" -f "$INSTALL" || die "cannot unpack $INSTALL"
		fi
		write_config || die "cannot write $ROOT/config.toml"
		data=$ROOT/$(config_value paths.data) || die "config.toml has no [paths] data"
		mkdir -p "$data" || exit 1
		for name in "$REPO"/data/*.toml; do
			[ "$(basename "$name")" = config.toml ] && continue
			cp "$name" "$data/" || die "cannot copy $name"
		done
		origmap=$ROOT/$(config_value paths.origmap) || die "config.toml has no [paths] origmap"
		map=$ROOT/$(config_value paths.map) || die "config.toml has no [paths] map"
		if [ ! -d "$origmap" ]; then
			echo "unpacking data/origmap.zip into $origmap"
			scratch=$ROOT/.origmap-unzip
			rm -rf "$scratch"
			unzip -q "$REPO/data/origmap.zip" -d "$scratch" || die "cannot unpack data/origmap.zip"
			mv "$scratch/origmap" "$origmap" && rmdir "$scratch" || die "cannot move the unpacked origmap"
		fi
		if [ ! -d "$map" ]; then
			cp -a "$origmap" "$map" || die "cannot copy $origmap to $map"
		fi
		mkdir -p "$ROOT/$(config_value paths.log)" "$ROOT/$(config_value paths.save)" || exit 1
		# The installation's .tibia stays for its operator tools, pointed at the copy.
		[ -f "$ROOT/.tibia" ] && sed -i "s#\"/game/#\"$ROOT/#" "$ROOT/.tibia"
	fi
	make_key
	if [ ! -f "$DB" ]; then
		echo "seeding $DB (game on 127.0.0.1:$GAME_PORT)"
		mkdir -p "$RUN" || exit 1
		"$SEED" --db "$DB" --game-root "$ROOT" --address 127.0.0.1 --port "$GAME_PORT" \
			--login-password "$LOGIN_PASSWORD" || { rm -f "$DB" "$DB-wal" "$DB-shm"; die "seeding failed"; }
	fi
}

case "${1:-}" in
seed)
	shift
	setup
	exec "$SEED" --db "$DB" "$@"
	;;
modulus)
	modulus
	exit 0
	;;
reset)
	[ -f "$RUN/play.pid" ] && kill -0 "$(cat "$RUN/play.pid")" 2> /dev/null && die "play.sh is running"
	rm -rf "$ROOT" "$RUN"
	echo "play copy and database deleted"
	exit 0
	;;
"")
	;;
*)
	sed -n '7,13p' "$0" | sed 's/^# \{0,1\}//'
	exit 2
	;;
esac

setup
QM_PORT=$(config_value qm_port) && SHM_KEY=$(config_value shm_key) \
	|| die "cannot read the query manager port or shm_key from $ROOT/config.toml"
for port in $LOGIN_PORT $GAME_PORT $QM_PORT; do
	ss -ltn | grep -q ":$port " && die "port $port is in use (another server running?)"
done
if ipcs -m | grep -q "^$(printf '0x%08x' "$SHM_KEY") "; then
	die "SHM key $SHM_KEY is in use: another game runs, or a leftover segment (ipcrm -M $SHM_KEY)"
fi
modulus
echo $$ > "$RUN/play.pid"

STAMP=$(date +%Y%m%d-%H%M%S)
QMLOG=$RUN/qm-$STAMP.log
LSLOG=$RUN/login-$STAMP.log
CONSOLE=$RUN/game-$STAMP.console
LSARGS=(--port "$LOGIN_PORT" --qm-port "$QM_PORT" --qm-password "$LOGIN_PASSWORD" --key "$(key_file)" --log "$LSLOG")
[ -n "$MOTD" ] && LSARGS+=(--motd "$MOTD")

# Each in a session of its own: the terminal's Ctrl-C reaches only this
# script, which then stops them in order.
setsid "$QM" --db "$DB" --port "$QM_PORT" --log "$QMLOG" < /dev/null &
QMPID=$!
sleep 1
kill -0 $QMPID 2> /dev/null || { rm -f "$RUN/play.pid"; die "the query manager did not start; see $QMLOG"; }
setsid "$LS" "${LSARGS[@]}" < /dev/null &
LSPID=$!
(cd "$ROOT" && HOME=$ROOT exec setsid stdbuf -oL -eL "$GAME" nofork) > "$CONSOLE" 2>&1 < /dev/null &
GAMEPID=$!

stopping=0
stop() {
	[ $stopping = 1 ] && return
	stopping=1
	echo
	if kill -0 $GAMEPID 2> /dev/null; then
		echo "stopping the game (pid $GAMEPID)"
		kill -INT $GAMEPID
		for i in $(seq 1 120); do kill -0 $GAMEPID 2> /dev/null || break; sleep 1; done
		if kill -0 $GAMEPID 2> /dev/null; then
			echo "the game did not stop within 120 s; killing it"
			kill -KILL $GAMEPID
		fi
	fi
	echo "stopping the login server and the query manager"
	kill -TERM $LSPID $QMPID 2> /dev/null
	wait $LSPID $QMPID 2> /dev/null
	echo "--- game console tail ($CONSOLE)"
	tail -5 "$CONSOLE"
	echo "logs: $QMLOG $LSLOG"
	for pid in $GAMEPID $LSPID $QMPID; do
		kill -0 $pid 2> /dev/null && echo "pid $pid is still running"
	done
	ipcs -m | grep -q "^$(printf '0x%08x' "$SHM_KEY") " && echo "SHM key $SHM_KEY is still there"
	rm -f "$RUN/play.pid"
}
trap 'stop; exit 0' INT TERM

echo "query manager pid $QMPID, login server pid $LSPID, game pid $GAMEPID"
echo "console $CONSOLE"
# The game opens its port before it loads the map and refuses logins until
# LaunchGame reports that it is ready.
for i in $(seq 1 600); do
	kill -0 $GAMEPID 2> /dev/null || { echo "the game exited during boot"; stop; exit 1; }
	if grep -q "Game server is ready" "$CONSOLE"; then
		echo "game ready after $i s: log in with the client (127.0.0.1:$LOGIN_PORT). Ctrl-C stops."
		break
	fi
	sleep 1
done

while kill -0 $GAMEPID 2> /dev/null; do
	wait $GAMEPID 2> /dev/null
done
echo "the game exited"
stop
