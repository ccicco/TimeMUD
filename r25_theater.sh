#!/data/data/com.termux/files/usr/bin/bash
cd ~/TimeMUD
./build/timemud > server.r25.log 2>&1 &
SRV=$!
sleep 1
exec 3<>/dev/tcp/127.0.0.1/2600
cat <&3 > r25_transcript.txt &
CAT=$!
send() { printf '%s\n' "$1" >&3; sleep 0.8; }
send "Curtiss"
send "go east"; send "go east"; send "go east"; send "go east"; send "go east"
send "go north"; send "go north"; send "go north"
send "search pool"
send "look"
send "search pool"
exec 3>&-
kill $CAT 2>/dev/null
wait $SRV
echo "=== TRANSCRIPT ==="
cat r25_transcript.txt
echo "=== SERVER (tail) ==="
tail -25 server.r25.log
echo "=== WORLD STATE ==="
cat data/world.state
