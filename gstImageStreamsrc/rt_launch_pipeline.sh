#!/bin/bash
LD_LIBRARY_PATH=/home/brule/cacao/ImageStreamIO/build:/home/brule/gst/lib  gst-launch-1.0 --gst-plugin-path=/home/brule/gst/lib/gstreamer-1.0/  gstImageStreamsrc ! videoconvert ! autovideosink
