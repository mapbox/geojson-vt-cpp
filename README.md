# geojson-vt-cpp

Port to C++ of [JS GeoJSON-VT](https://github.com/mapbox/geojson-vt) for slicing GeoJSON into vector tiles on the fly.

### JNI Bindings 
This fork Contains JNI bindings for the main functionalities of the geojson-vt-cpp package. It is [jni.hpp](https://github.com/mapbox/jni.hpp) to easily register native methods into a Java class of your choice. The Java class needs to define the following methods :

```java
   package org.something

   public class Tiler {
     public native void load(String key, String path);
  
     public native void unload(String key);
  
     public synchronized native byte[] getTile(String key, int z, int x, int y);   
   }
```
Note that getTile is synchronized since it has the side effect of changing internal data structures in native code.

### Compilation using make

To create a dynamic lib execute make with target "jni" and specify the class name which you

```
make TILER_KLASS=org/something/Tiler clean jni
```

[![Build Status](https://travis-ci.org/mapbox/geojson-vt-cpp.svg?branch=master)](https://travis-ci.org/mapbox/geojson-vt-cpp)
