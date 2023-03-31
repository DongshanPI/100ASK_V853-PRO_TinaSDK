#!/bin/ash

g2d_bld_dmabuf -flag 4 -in_fb 0 800 480 -src_rect 0 0 800 480 -src_coor 0 0 -in_fb2 0 800 480 -src_rect2 0 0 800 480 -src_coor2 0 0 -out_fb 0 800 480 -dst_rect 0 0 800 480 -src_file /usr/lib/tt-data/src_800x480_rgb.bin -src_file2 /usr/lib/tt-data/src2_800x480_rgb.bin -dst_file /tmp/bld_test.bin -alpha_mode 1 -alpha 208

ttrue "Is the result picture as expected?"
if [ $? = 0 ]
then
echo "GOOD, g2d_bld_dmabuf test success."
return 0
else
echo "ERROR, g2d_bld_dmabuf test failed."
return 1
fi
