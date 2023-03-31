#!/bin/ash

resolution=`mjson_fetch /base/display/tplayertester/resolution`

path="/sys/kernel/debug/dispdbg"

if [[ $resolution -eq 0 ]];
then
echo disp0 > ${path}/name; echo switch1 > ${path}/command; echo 4 10 0 0 0x4 0x101 0 0 0 8 > ${path}/param; echo 1 > ${path}/start;
sleep 2
(tplayerdemo /usr/lib/tt-data/01-1080P-HEVC-AAC-60F.mkv) &
elif [[ $resolution -eq 1 ]];
then
echo disp0 > ${path}/name; echo switch1 > ${path}/command; echo 4 28 0 0 0x4 0x101 0 0 0 8 > ${path}/param; echo 1 > ${path}/start;
sleep 2
(tplayerdemo /usr/lib/tt-data/H264_4K.mp4) &
fi

sleep 9
kill -9 $(ps | grep tplayerdemo | head -n 1 | awk -F " " '{print $1}')

ttrue "Whether the video is playing normally?"
if [ $? = 0 ]
then
echo "GOOD, video player test success."
return 0
else
echo "ERROR, video player test failed."
return 1
fi
