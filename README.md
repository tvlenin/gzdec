# Zlib decompress GStreamer plugin

This project provides a GStreamer plugin to decompress files in with .gz and .bz2 compression

## Prerequisites

Install GStreamer following the oficial site https://gstreamer.freedesktop.org/documentation/installing/on-linux.html?gi-language=c
#### Zlib:
sudo apt install -y zlib1g-dev libbz2-dev

## Installing

Clone the git repository:
```
git clone https://github.com/tvlenin/gzdec.git
```
To select the GStreamer version to build the plugin use the ``--with-gst-version=x.x`` with the ``./configure`` as in the next example.


Build and install the plugins:
```
cd gzdec
./autogen.sh
./configure --with-gst-version=1.0
make
# Optional to install the plugins
sudo make install
```
If you want to use the plugins without installing it use the following ENV variable:

```
GST_PLUGIN_PATH=src/.libs/ gst-launch-1.0 ...

```

## Test the plugin

### Create the text file and compress it with both compressors

```
echo "Test file to be decompressed with gzdec" > test_file.txt 
bzip2 -k test_file.txt
gzip -k test_file.txt
 ```
 
 ### Decompress the files using gzdec with gstreamer-1.0

 ```

# Zlib
GST_PLUGIN_PATH=src/.libs/ gst-launch-1.0 filesrc location=path/to/file.gz ! gzdec method=zlib ! filesink location=decompressed_zlib.txt 

#Bzlib
GST_PLUGIN_PATH=src/.libs/ gst-launch-1.0 filesrc location=path/to/file.bz2 ! gzdec method=bzlib ! filesink location=decompressed_bzlib.txt 

```

### Decompress the files using gzdec with gstreamer-0.10
```
# Zlib
GST_PLUGIN_PATH=src/.libs/ gst-launch-0.10 filesrc location=path/to/file.gz ! gzdec method=zlib ! filesink location=decompressed_zlib.txt 

#Bzlib
GST_PLUGIN_PATH=src/.libs/ gst-launch-0.10 filesrc location=path/to/file.bz2 ! gzdec method=bzlib ! filesink location=decompressed_bzlib.txt 

```

## Properties
The gzdec plugin have the method property to select which decompress method use, by default it use zlib.
```
Element Properties:
  method              : Decompress method
                        flags: readable, writable, changeable only in NULL or READY state
                        Enum "GstDecMethod" Default: 0, "zlib"
                           (0): zlib             - ZLIB method
                           (1): bzlib            - BZLIB method
```