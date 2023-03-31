#!/bin/ash

g2d_fill_dmabuf -out_fb 0 800 480 -dst_rect 0 0 800 480 -out_file /tmp/fill_dmabuf_0.bin -color 4278190335 -alpha_mode 1 -alpha 255

ttrue "Is the result picture as expected?"
if [ $? = 0 ]
then
echo "GOOD, g2d_fill_dmabuf test success."
return 0
else
echo "ERROR, g2d_fill_dmabuf test failed."
return 1
fi
