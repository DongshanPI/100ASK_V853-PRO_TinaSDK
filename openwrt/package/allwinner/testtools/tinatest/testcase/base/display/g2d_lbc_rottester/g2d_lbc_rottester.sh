#!/bin/ash

g2d_lbc_rot -flag 256 -in_fb 0 800 480 -src_rect 0 0 800 480 -out_fb 0 480 800 -dst_rect 0 0 480 800 -src_file /usr/lib/tt-data/src_800x480_rgb.bin -dst_file /tmp/en_src_800x480_rgb_90.bin

ttrue "Is the result picture as expected?"
if [ $? = 0 ]
then
echo "GOOD, g2d_lbc_rot test success."
return 0
else
echo "ERROR, g2d_lbc_rot test failed."
return 1
fi
