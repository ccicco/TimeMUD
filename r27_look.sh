#!/data/data/com.termux/files/usr/bin/bash
cd ~/TimeMUD
./build/timemud 200 > server.r27.log 2>&1 &
SRV=$!
sleep 1
exec 3<>/dev/tcp/127.0.0.1/2600
cat <&3 | tee r27_transcript.txt &
send() { printf '%s\n' "$1" >&3; sleep 0.9; }
send "Curtiss"
send "look"
wait $SRV
