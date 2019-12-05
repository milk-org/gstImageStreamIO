#!/bin/bash

cp libgstImageStreamIOsrc.so /home/sevin/Packages/miniconda3/lib/gstreamer-1.0/
GST_DEBUG=imageStreamsrc:4 gst-launch-1.0 imageStreamIOsrc shm-name=imtest00 ! videoconvert ! ximagesink
