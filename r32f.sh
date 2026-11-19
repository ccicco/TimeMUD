#!/data/data/com.termux/files/usr/bin/bash
cd ~/TimeMUD
./build/timemud 900 > server.r32f.log 2>&1 &
SRV=$!
sleep 1
send() { echo "SENT $(date +%T): $1"; printf '%s\n' "$1"; sleep 1.1; }
{
send "Curtiss"
send "say A"
send "say B"
send "say C"
send "say D"
send "say E"
send "say F"
send "say G"
send "say H"
send "say I"
send "quit"
sleep 60
} | busybox nc 127.0.0.1 2600 > r32f_transcript.txt 2>/dev/null
wait $SRV
