# !/bin/bash
set -e

BUILD_MODE="incr"
CMAKE_BUILD_TYPE="Debug"  # 默认 Debug

if [ $# -ge 1 ]; then
  BUILD_MODE=$1
fi
if [ $# -ge 2 ]; then
  CMAKE_BUILD_TYPE=$2
fi

git_root_path="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )"

build_dir=$git_root_path/build

if [ ! -f "${git_root_path}/version" ]; then
    echo "Error: ${git_root_path}/version file not found!" >&2
    exit 1
fi
version_str=$(< ${git_root_path}/version)          # Bash 内建读取
version_str=${version_str// /}    # 删除空格
version_str=${version_str//$'\n'/} # 删除换行
version_str=${version_str//$'\r'/} # 删除回车（Windows 换行）

if [ -z "$version_str" ]; then
    echo "Error: version file is empty!" >&2
    exit 1
fi
today=$(date "+%Y%m%d")
cur_version="MediaProcessor-V${version_str}-"$today-"Mac"

install_folder=$build_dir/install
workspace_folder=$install_folder/$cur_version
bin_folder=$workspace_folder/bin
libs_folder=$workspace_folder/libs
demo_src_folder=$workspace_folder/src
demo_include_folder=$workspace_folder/include
demo_cpp_folder=$demo_src_folder/cpp

if [ ! -f "$git_root_path/third_party/opus/build_macos_static/lib/libopus.a" ]; then
  BUILD_MODE="all"
fi
if [ ! -f "$git_root_path/third_party/libogg/build_macos_static/lib/libogg.a" ]; then
  BUILD_MODE="all"
fi
# if [ ! -f "$git_root_path/third_party/openh264/libopenh264.a" ]; then
#   BUILD_MODE="all"
# fi
# if [ ! -f "$git_root_path/third_party/libjpeg-turbo/build/libturbojpeg.a" ]; then
#   BUILD_MODE="all"
# fi
# if [ ! -f "$git_root_path/third_party/libyuv/build/libyuv.a" ]; then
#   BUILD_MODE="all"
# fi


if [ "$BUILD_MODE" = "all" ]; then
  git submodule init
  git submodule update

  # 1. build opus
  echo "------ 1. begin opus ... ------"
  opus_root_path=$git_root_path/third_party/opus
  cd $opus_root_path
  ./configure \
    --disable-shared \
    --enable-static \
    --prefix=$(pwd)/build_macos_static
  make -j$(sysctl -n hw.ncpu)
  make install

  # 2. build libogg
  echo "------ 1. begin libogg ... ------"
  libogg_root_path=$git_root_path/third_party/libogg
  cd $libogg_root_path
  ./configure \
    --disable-shared \
    --enable-static \
    --prefix=$(pwd)/build_macos_static
  make -j$(sysctl -n hw.ncpu)
  make install

#   # 1. build openh264
#   echo "------ 1. begin openh264 ... ------"
#   openh264_root_path=$git_root_path/third_party/openh264
#   cd $openh264_root_path
#   make clean
#   make ARCH=arm64 -j4
#   echo "   ====== done ======"

#   # 2. build libjpeg-turbo
#   echo "------ 2. begin libjpeg-turbo ... ------"
#   jpeg_root_path=$git_root_path/third_party/libjpeg-turbo
#   cd $jpeg_root_path
#   rm -rf build
#   mkdir -p build && cd build
#   cmake .. -DCMAKE_C_FLAGS="-fPIC" \
#     -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_CXX_STANDARD=11 \
#     -DCMAKE_OSX_ARCHITECTURES=arm64 \
#     -DENABLE_STATIC=ON -DENABLE_SHARED=ON
#   make -j4
#   echo "   ====== done ======"

#   # 3. build libyuv
#   echo "------ 3. begin libyuv ... ------"
#   libyuv_root_path=$git_root_path/third_party/libyuv
#   cd $libyuv_root_path
#   rm -rf build
#   mkdir -p build && cd build
#   cmake .. \
#     -DCMAKE_C_FLAGS="-fPIC -DLIBYUV_DISABLE_NEON -DLIBYUV_DISABLE_SVE" \
#     -DCMAKE_CXX_FLAGS="-fPIC -DLIBYUV_DISABLE_NEON -DLIBYUV_DISABLE_SVE" \
#     -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_CXX_STANDARD=11 \
#     -DCMAKE_OSX_ARCHITECTURES=arm64 -DBUILD_SHARED_LIBS=OFF
#   make -j4
#   echo "   ====== done ======"

fi

# 0. build SDK
echo "------ 4. begin SDK ... ------"
mkdir -p $build_dir && cd $build_dir

cmake .. -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_OSX_ARCHITECTURES=arm64 && make -j16
echo "   ====== done ======"

# echo "------ 5. begin merge libs ... ------"
# # 5. merge objs
# merged_objs=$build_dir/merged_objs
# rm -rf $merged_objs
# mkdir -p $merged_objs && cd $merged_objs

# rm -f $build_dir/libmediaprocessor_full.a
# ar x $git_root_path/third_party/openh264/libopenh264.a
# ar x $git_root_path/third_party/libjpeg-turbo/build/libturbojpeg.a
# ar x $git_root_path/third_party/libyuv/build/libyuv.a
# ar x $build_dir/libmediaprocessor.a

# ar rcs $build_dir/libmediaprocessor_full.a *.o

# cd "$build_dir"
# if [ "$CMAKE_BUILD_TYPE" = "Release" ]; then
#   OPT_FLAGS="-O2 -DNDEBUG"
# else
#   OPT_FLAGS="-g -O0"
# fi
# clang++ $LINK_FLAGS -shared -fPIC -std=c++11 \
#   -install_name "@rpath/libmediaprocessor_full.dylib" \
#   -o libmediaprocessor_full.dylib \
#   "$merged_objs"/*.o \
#   -lpthread

# echo "   ====== done ======"

# # 6. build JNI
# echo "------ 6. begin JNI ... ------"
# api_folder=$git_root_path/api
# java_folder=$api_folder/java/com/funaudio/mediaprocessor
# java_build_folder=$build_dir/java
# jni_build_folder=$build_dir/jni

# mkdir -p $java_build_folder
# mkdir -p $jni_build_folder
# cd $git_root_path

# clang-format -i -style='{BasedOnStyle: Google, IndentWidth: 4, BreakBeforeBraces: Attach, ColumnLimit: 120, Language: Java}' ${java_folder}/MediaConstants.java
# clang-format -i -style='{BasedOnStyle: Google, IndentWidth: 4, BreakBeforeBraces: Attach, ColumnLimit: 120, Language: Java}' ${java_folder}/MediaContext.java
# clang-format -i -style='{BasedOnStyle: Google, IndentWidth: 4, BreakBeforeBraces: Attach, ColumnLimit: 120, Language: Java}' ${java_folder}/MediaConfig.java
# clang-format -i -style='{BasedOnStyle: Google, IndentWidth: 4, BreakBeforeBraces: Attach, ColumnLimit: 120, Language: Java}' ${java_folder}/MediaPacket.java
# clang-format -i -style='{BasedOnStyle: Google, IndentWidth: 4, BreakBeforeBraces: Attach, ColumnLimit: 120, Language: Java}' ${java_folder}/MediaProcessor.java
# clang-format -i -style='{BasedOnStyle: Google, IndentWidth: 4, BreakBeforeBraces: Attach, ColumnLimit: 120, Language: Java}' ${java_folder}/NativeLoader.java

# javac -d $java_build_folder \
#   $java_folder/NativeLoader.java \
#   $java_folder/MediaProcessor.java \
#   $java_folder/MediaPacket.java \
#   $java_folder/MediaConfig.java \
#   $java_folder/MediaContext.java \
#   $java_folder/MediaConstants.java
# javac -h $jni_build_folder -d $java_build_folder \
#   $java_folder/NativeLoader.java \
#   $java_folder/MediaProcessor.java \
#   $java_folder/MediaPacket.java \
#   $java_folder/MediaConfig.java \
#   $java_folder/MediaContext.java \
#   $java_folder/MediaConstants.java

# cd $java_build_folder
# cmake ../../api/ -DCMAKE_OSX_ARCHITECTURES=arm64
# make -j1

# echo "   ====== done ======"

# echo "------ 7. begin java demo ... ------"
# clang-format -i -style='{BasedOnStyle: Google, IndentWidth: 4, BreakBeforeBraces: Attach, ColumnLimit: 120, Language: Java}' ${java_folder}/H264NaluExtractor.java
# clang-format -i -style='{BasedOnStyle: Google, IndentWidth: 4, BreakBeforeBraces: Attach, ColumnLimit: 120, Language: Java}' ${java_folder}/H264ToJpegDemo1.java
# clang-format -i -style='{BasedOnStyle: Google, IndentWidth: 4, BreakBeforeBraces: Attach, ColumnLimit: 120, Language: Java}' ${java_folder}/H264ToJpegDemo2.java
# clang-format -i -style='{BasedOnStyle: Google, IndentWidth: 4, BreakBeforeBraces: Attach, ColumnLimit: 120, Language: Java}' ${java_folder}/H264ToJpegDemo3.java
# clang-format -i -style='{BasedOnStyle: Google, IndentWidth: 4, BreakBeforeBraces: Attach, ColumnLimit: 120, Language: Java}' ${java_folder}/OpusToPcm.java
# clang-format -i -style='{BasedOnStyle: Google, IndentWidth: 4, BreakBeforeBraces: Attach, ColumnLimit: 120, Language: Java}' ${java_folder}/RawOpusToPcm.java
# clang-format -i -style='{BasedOnStyle: Google, IndentWidth: 4, BreakBeforeBraces: Attach, ColumnLimit: 120, Language: Java}' ${java_folder}/PcmToOpus.java
# clang-format -i -style='{BasedOnStyle: Google, IndentWidth: 4, BreakBeforeBraces: Attach, ColumnLimit: 120, Language: Java}' ${java_folder}/PcmToOpus2.java
# clang-format -i -style='{BasedOnStyle: Google, IndentWidth: 4, BreakBeforeBraces: Attach, ColumnLimit: 120, Language: Java}' ${java_folder}/PcmToOpus3.java
# clang-format -i -style='{BasedOnStyle: Google, IndentWidth: 4, BreakBeforeBraces: Attach, ColumnLimit: 120, Language: Java}' ${java_folder}/PcmToRawOpus.java
# javac -d $java_build_folder \
#   $java_folder/H264NaluExtractor.java \
#   $java_folder/H264ToJpegDemo1.java \
#   $java_folder/H264ToJpegDemo2.java \
#   $java_folder/H264ToJpegDemo3.java \
#   $java_folder/OpusToPcm.java \
#   $java_folder/RawOpusToPcm.java \
#   $java_folder/PcmToOpus.java \
#   $java_folder/PcmToOpus2.java \
#   $java_folder/PcmToOpus3.java \
#   $java_folder/PcmToRawOpus.java

# echo "   ====== done ======"

# # 8. pack
# echo "====== 8. packing install dir ======"

# mkdir -p $install_folder
# mkdir -p $workspace_folder
# mkdir -p $bin_folder
# mkdir -p $libs_folder
# mkdir -p $demo_src_folder
# mkdir -p $demo_include_folder
# mkdir -p $demo_cpp_folder

# # 拷贝cpp范例代码
# cp -rf $git_root_path/tools/arg_parser.h $demo_cpp_folder/
# cp -rf $git_root_path/tools/audio_process.cpp $demo_cpp_folder/
# cp -rf $git_root_path/tools/audio_process.h $demo_cpp_folder/
# cp -rf $git_root_path/tools/video_process.cpp $demo_cpp_folder/
# cp -rf $git_root_path/tools/video_process.h $demo_cpp_folder/
# cp -rf $git_root_path/tools/mproc_tools.cpp $demo_cpp_folder/

# cp -rf $git_root_path/core/include/media_code.h $demo_include_folder/
# cp -rf $git_root_path/core/include/media_constants.h $demo_include_folder/
# cp -rf $git_root_path/core/include/media_context.h $demo_include_folder/
# cp -rf $git_root_path/core/include/media_config.h $demo_include_folder/
# cp -rf $git_root_path/core/include/media_packet.h $demo_include_folder/
# cp -rf $git_root_path/core/include/media_processor.h $demo_include_folder/

# # 拷贝lib
# cp -f $build_dir/libmediaprocessor_full.a $libs_folder/
# cp -f $build_dir/libmediaprocessor_full.dylib $libs_folder/
# cp -f $build_dir/java/lib/libmediaprocessor_jni.dylib $libs_folder/

# # 拷贝java范例代码
# cp -rf $git_root_path/api/java $demo_src_folder/
# cp -rf $git_root_path/api/jni $demo_src_folder/

# # 拷贝可执行
# cp -rf $git_root_path/api/java $bin_folder/
# mkdir -p $bin_folder/java
# cp -rf $build_dir/java/com $bin_folder/java/
# cp -rf $build_dir/java/lib $bin_folder/java/
# cp -f $build_dir/libmediaprocessor_full.dylib $bin_folder/java/lib/

# cp -f $git_root_path/README.md $workspace_folder/README.md

# # build mproc
# g++ -std=c++11 \
#   -I$demo_include_folder \
#   $demo_cpp_folder/audio_process.cpp \
#   $demo_cpp_folder/video_process.cpp \
#   $demo_cpp_folder/mproc_tools.cpp \
#   -o $bin_folder/mproc \
#   $libs_folder/libmediaprocessor_full.a \
#   -lpthread

# cd $install_folder
# rm -f $cur_version.tar.gz
# tar -czf $cur_version.tar.gz $cur_version

# echo "   ====== done ======"

# echo "   ====== ALL Done ======"

# echo " "
# echo "java demo usage:"
# echo " "
# echo " cd $git_root_path"
# echo " java -Djava.library.path=build/java/lib -cp build/java com.funaudio.mediaprocessor.H264ToJpegDemo1 test/sample.h264 tmp/ 1280 720"
# echo " java -Djava.library.path=build/java/lib -cp build/java com.funaudio.mediaprocessor.OpusToPcm test/16k_mono.opus tmp/sample.pcm 48000"
# echo " java -Djava.library.path=build/java/lib -cp build/java com.funaudio.mediaprocessor.RawOpusToPcm test/16k_mono.opu2 16000 1 tmp/sample.pcm 48000"
# echo " java -Djava.library.path=build/java/lib -cp build/java com.funaudio.mediaprocessor.PcmToOpus test/16k_mono.pcm 16000 1 tmp/sample.opus 48000"
# echo " java -Djava.library.path=build/java/lib -cp build/java com.funaudio.mediaprocessor.PcmToOpus2 test/16k_mono.pcm 16000 1 tmp/sample.opus 48000"
# echo " java -Djava.library.path=build/java/lib -cp build/java com.funaudio.mediaprocessor.PcmToRawOpus test/16k_mono.pcm 16000 1 tmp/sample.opu2 48000"
# echo " "
# echo " cd $workspace_folder"
# echo " java -Djava.library.path=libs -cp bin/java com.funaudio.mediaprocessor.H264ToJpegDemo1 test/sample.h264 tmp/ 1280 720"
# echo " java -Djava.library.path=libs -cp bin/java com.funaudio.mediaprocessor.OpusToPcm test/16k_mono.opus tmp/sample.pcm 48000"
# echo " java -Djava.library.path=libs -cp bin/java com.funaudio.mediaprocessor.RawOpusToPcm test/16k_mono.opu2 16000 1 tmp/sample.pcm 48000"
# echo " java -Djava.library.path=libs -cp bin/java com.funaudio.mediaprocessor.PcmToOpus test/16k_mono.pcm 16000 1 tmp/sample.opus 48000"
# echo " java -Djava.library.path=libs -cp bin/java com.funaudio.mediaprocessor.PcmToOpus2 test/16k_mono.pcm 16000 1 tmp/sample.opus 48000"
# echo " java -Djava.library.path=libs -cp bin/java com.funaudio.mediaprocessor.PcmToRawOpus test/16k_mono.pcm 16000 1 tmp/sample.opu2 48000"
