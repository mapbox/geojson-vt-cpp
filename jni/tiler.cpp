#include <jni/jni.hpp>
#include <mapbox/geojson.hpp>
#include <mapbox/geojson_impl.hpp>
#include <mapbox/geojsonvt.hpp>
#include <mapbox/geojsonvt/clip.hpp>
#include <mapbox/geojsonvt/convert.hpp>
#include <mapbox/geojsonvt/simplify.hpp>
#include <mapbox/geojsonvt/tile.hpp>
#include <mapbox/geometry.hpp>

#include <vtzero/builder.hpp>
#include <vtzero/index.hpp>

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>

#include "util.hpp"

using namespace mapbox::geojsonvt;
using namespace std;

// Tiler Java Class Name definition //
#ifdef TILER_CLASS_NAME
  struct Tiler     { static constexpr auto Name() { return TILER_CLASS_NAME } };
#else
  #pragma message("Using default Tiler class for java binding")
  #pragma message("Please pass -DTILER_CLASS_NAME=\"xxx/yyy/class/Name\"; to compiler")
  struct Tiler     { static constexpr auto Name() { return "Tiler"; } };
#endif


std::map<std::string, GeoJSONVT> tilesIndexMap;

static void RegisterTilerClass(JavaVM* vm)
{
  jni::JNIEnv& env { jni::GetEnv(*vm) };
 
  /**
   * Load Geojson file into the tile index map internal cache.
   */
  auto load = [] (jni::JNIEnv& env, jni::Object<Tiler>&, jni::String& keyArg, jni::String& pathArg) {
    auto key = jni::Make<std::string>(env, keyArg);
    auto path = jni::Make<std::string>(env, pathArg);
    cout << "Native Tiler::load with Key:" << key << " Path: " << path << endl;
    const auto geojson = mapbox::geojson::parse(loadFile(path));
    GeoJSONVT index{ geojson.get<mapbox::geojson::feature_collection>() };
    tilesIndexMap.insert(std::make_pair(key, index));
  };
  
  /**
   * Unload Geojson file from the tile index map internal cache. Will release all resources taken
   * by this index
   */
  auto unload = [] (jni::JNIEnv& env, jni::Object<Tiler>&, jni::String& keyArg) {
    auto key = jni::Make<std::string>(env, keyArg);
    cout << "Native Tiler::unload with Key:" << key << endl;
    tilesIndexMap.erase(key);
  };
  
  /**
   * Get tile by index key and (z,x,y)
   */
  auto getTile = [] (jni::JNIEnv& env, jni::Object<Tiler>&, jni::String& keyArg, jni::jint& z, jni::jint& x, jni::jint& y) {
    auto key = jni::Make<std::string>(env, keyArg);
    // cout << "getTile:" << key << " z: " << z << " x:" << x << " y:" << y << endl;
    auto index = tilesIndexMap.find(key);
    if (index == tilesIndexMap.end()) {
      throw std::invalid_argument("Key doesn't exist:" + key);
    }
    // cout << "Calling getTile" << endl;
    auto resultTile = index->second.getTile(z, x, y);
    
    vtzero::tile_builder tile;
    vtzero::layer_builder defaultLayer { tile, "default" };
    
    vtzero::value_index<vtzero::sint_value_type, int32_t, std::unordered_map> maxspeed_index{defaultLayer};
    
    for (unsigned long i=0; i<resultTile.features.size(); i++) {
      auto geometry = resultTile.features[i].geometry;
      const auto& lineString = geometry.get<mapbox::geometry::line_string<int16_t>>();
      // Check that the line string is not zero length
      if (measure(lineString) == 0.0) {
        continue;
      }
      
      vtzero::linestring_feature_builder feature{defaultLayer};
      feature.add_linestring(lineString.size());
      
      for (unsigned long j=0; j<lineString.size(); j++) {
        feature.set_point(lineString[j].x, lineString[j].y);
      }
      auto iterator = resultTile.features[i].properties.begin();
      while(iterator != resultTile.features[i].properties.end()) {
        int propertyType = iterator->second.which();
        switch(propertyType) {
          case 2:
            feature.add_property(iterator->first, (iterator->second).get<uint64_t>());
            break;
          case 4:
            feature.add_property(iterator->first, (iterator->second).get<double>());
            break;
          case 5:
            feature.add_property(iterator->first, (iterator->second).get<std::string>());
            break;
          default:
            throw std::runtime_error("Unknown variant type");
        }
        iterator++;
      }
      feature.commit();
    }
    
    const auto result = tile.serialize();
    return jni::Make<jni::Array<jni::jbyte>>(env, result);
  };
  
  // Register Native methods :
  jni::RegisterNatives(env, *jni::Class<Tiler>::Find(env), jni::MakeNativeMethod("load", load));
  jni::RegisterNatives(env, *jni::Class<Tiler>::Find(env), jni::MakeNativeMethod("unload", unload));
  jni::RegisterNatives(env, *jni::Class<Tiler>::Find(env), jni::MakeNativeMethod("getTile", getTile));
}

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*)
{
  RegisterTilerClass(vm);
  return jni::Unwrap(jni::jni_version_1_2);
}
