#!/bin/bash

GST_DEBUG=gstImageStreamsrc:6 LD_LIBRARY_PATH=/home/brule/cacao/ImageStreamIO/build:/home/brule/gst/lib  gst-launch-1.0 --gst-plugin-path=/home/brule/gst/lib/gstreamer-1.0/  gstImageStreamsrc !  videoconvert ! timeoverlay time-mode=1 valignment=top halignment=right  ! timeoverlay valignment=top time-mode=0  ! fpsdisplaysink
