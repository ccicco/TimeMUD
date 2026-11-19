#!/data/data/com.termux/files/usr/bin/bash
cd ~/TimeMUD
./build/timemud > server.r25b.log 2>&1 &
SRV=$!
sleep 1
exec 3<>/dev/tcp/127.0.0.1/2600
cat <&3 | tee r25b_transcript.txt &
send() { printf '[%s] -> %s\n' "$(date +%M:%S)" "$1" >&2; printf '%s\n' "$1" >&3; sleep 0.7; }
send "Curtiss"
send "go east"; send "go east"; send "go east"; send "go east"; send "go east"
send "go north"; send "go north"
send "search pool"
wait $SRV
echo "=== WORLD STATE ==="
cat data/world.state
