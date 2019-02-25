# gstImageStreamIO plugin

## installation

```bash
git clone git@gitlab.obspm.fr:greenflash/gstImageStreamIOsrc.git
cd gstImageStreamIOsrc
mkdir build
cd build
cmake ..
make
sudo cp libgstimageStreamIO.so /usr/lib/gstreamer-1.0/
```

NB: /usr/lib/gstreamer-1.0/ can be changed where your gstreamer plugins are installed.
You can also use *--gst-plugin-path=* flag to avoid the copy in your system

## gstImageStreamIOsrc element

standalone application: 

```bash
./gstimageStreamIOsrc_main imtest00
```

```bash
ImageStreamIO producer: ~/workspace/greenflash/octopus/tplib/ImageStreamIO/a.out
```

### using 264 needs gst-plugins-ugly

UDP server:  

```bash
gst-launch-1.0 imageStreamIOsrc shm-name=imtest00 ! videoconvert ! x264enc tune=zerolatency bitrate=500 speed-preset=superfast ! rtph264pay ! udpsink host=127.0.0.1 port=5000  sync=false 
```

UDP client:

```bash
gst-launch-1.0 udpsrc address=127.0.0.1 port=5000 caps="application/x-rtp" ! rtph264depay ! decodebin ! videoconvert ! autovideosink 
```

### using 263 needs gst-plugins-good (low quality)

UDP server: 

```bash
gst-launch-1.0 imageStreamIOsrc shm-name=imtest00 ! videoconvert ! avenc_h263p ! rtph263ppay ! udpsink host=127.0.0.1 port=5000 
```

UDP client: 

```bash
gst-launch-1.0 udpsrc address=127.0.0.1 port=5000 caps="application/x-rtp" ! rtph264depay ! decodebin ! videoconvert ! autovideosink 
```

### using vp8 needs gst-plugins-good (high CPU consumption)

UDP server: 

```bash
gst-launch-1.0 imageStreamIOsrc shm-name=imtest00 ! videoconvert ! vp8enc ! rtpvp8pay ! udpsink host=127.0.0.1 port=5000
```

UDP client: 

```bash
gst-launch-1.0 udpsrc address=127.0.0.1 port=5000 caps="application/x-rtp" ! rtpvp8depay ! decodebin ! videoconvert ! autovideosink
```

## Applications

### server

```bash
./gstimageStreamIOsrc_server imtest00 127.0.0.1 5000
```

### client (not working)

```bash
./gstimageStreamIOsrc_client 127.0.0.1 5000
```