#!/bin/ash

g2d_mixer_task -src_rgb_file /usr/lib/tt-data/c1080_good.rgb -dst_rgb_file /tmp -src_y8_file /usr/lib/tt-data/en_dmabuf_bike_1280x720_220_Y8.bin -dst_y8_file /mnt/UDISK -src_nv12_file /usr/lib/tt-data/bike_1280x720_220.bin -dst_nv12_file /mnt/UDISK

ttrue "Is the result picture as expected?"
if [ $? = 0 ]
then
echo "GOOD, g2d_mixer_task test success."
return 0
else
echo "ERROR, g2d_mixer_task test failed."
return 1
fi
