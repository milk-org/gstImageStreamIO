#!/bin/bash

GST_DEBUG=gstImageStreamsrc:4 LD_LIBRARY_PATH=/home/brule/cacao/ImageStreamIO/build:/home/brule/gst/lib  gst-launch-1.0 --gst-plugin-path=/home/brule/gst/lib/gstreamer-1.0/  gstImageStreamsrc !  videoconvert !  videorate ! video/x-raw,framerate=25/1 !  timeoverlay time-mode=1 valignment=top halignment=right  ! timeoverlay valignment=top time-mode=0  ! fpsdisplaysink
