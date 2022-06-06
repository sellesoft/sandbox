#!/bin/bash
#_____________________________________________________________________________________________________
#                                            Usage
#_____________________________________________________________________________________________________
# build.sh <command> [arguments...]
#
# Commands:
#   (empty)       Compile and link (default)
#   compile       Compile only
#   link          Link with previously generated .obj/.o
#   one <file>    Compile one file and link with previously generated .obj/.o
#
# Arguments:
#   --v    Echo build commands to the console
#   --d    Build with    debug info and without optimization (default)
#   --r    Build without debug info and with    optimization
#   --s    Build certain modules as shared libraries for code implementation reloading
#
#   -platform <win32,mac,linux>           Build for specified OS: win32, mac, linux (default: builder's OS)
#   -graphics <vulkan,opengl,directx>     Build for specified Graphics API (default: vulkan)
#   -compiler <cl,gcc,clang>              Build using the specified compiler (default: cl on Windows, gcc on Mac and Linux)
#   -linker   <link,ld,lld-link>          Build using the specified linlker (default: link on Windows, ld on Mac and Linux)
#_____________________________________________________________________________________________________
#                                           Constants
#_____________________________________________________________________________________________________
#### Specify paths ####
misc_folder="$( cd -- "$( dirname -- "${BASH_SOURCE[0]:-$0}"; )" &> /dev/null && pwd 2> /dev/null; )";
root_folder="$misc_folder/.."
glfw_folder="C:/src/glfw-3.3.2.bin.WIN64"   #TODO(delle) platform specific glfw binaries
vulkan_folder="$VULKAN_SDK"


#### Specify outputs ####
build_folder="$root_folder/build"
app_name="sandbox"


#### Specify sources ####
includes="-I$root_folder/src -I$root_folder/deshi/src -I$root_folder/deshi/src/external -I$glfw_folder/include -I$vulkan_folder/include"
deshi_sources="$root_folder/deshi/src/deshi.cpp"
dll_sources="$root_folder/src/ui2.cpp"
app_sources="$root_folder/src/main.cpp"


#### Specifiy libs ####
lib_paths=(
  $glfw_folder/lib-vc2019
  $vulkan_folder/lib
)
libs=(
  glfw3
  gdi32
  shell32
  ws2_32
  opengl32
  vulkan-1
  shaderc_combined
)


#### Determine builder's OS ####
builder_platform="unknown"
builder_compiler="unknown"
builder_linker="unknown"
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
  builder_platform="linux"
  builder_compiler="gcc"
  builder_linker="ld"
elif [[ "$OSTYPE" == "darwin"* ]]; then
  builder_platform="mac"
  builder_compiler="gcc"
  builder_linker="ld"
elif [[ "$OSTYPE" == "cygwin" ]]; then
  builder_platform="win32"
  builder_compiler="cl"
  builder_linker="link"
elif [[ "$OSTYPE" == "msys" ]]; then
  builder_platform="win32"
  builder_compiler="cl"
  builder_linker="link"
elif [[ "$OSTYPE" == "win32" ]]; then
  builder_platform="win32"
  builder_compiler="cl"
  builder_linker="link"
elif [[ "$OSTYPE" == "freebsd"* ]]; then
  builder_platform="linux"
  builder_compiler="gcc"
  builder_linker="ld"
else
  echo "Unhandled development platform: $OSTYPE"
  exit 1
fi
#_____________________________________________________________________________________________________
#                                       Command Line Args
#_____________________________________________________________________________________________________
build_cmd=""
build_cmd_one_file=""
build_dir="$build_folder/debug"
build_verbose=0
build_release=0
build_shared=0
build_platform=$builder_platform
build_graphics="vulkan"
build_compiler="$builder_compiler"
build_linker="$builder_linker"


skip_arg=0
for (( i=1; i<=$#; i++)); do
  #### skip argument if already handled
  if [ $skip_arg == 1 ]; then
    skip_arg=0
    CONTINUE
  fi

  #### parse <command>
  if [ $i == 1 ]; then
    if [ "${!i}" == "compile" ]; then
      build_cmd = "compile"
      CONTINUE
    elif [ "${!i}" == "link" ]; then
      build_cmd = "link"
      CONTINUE
    elif [ "${!i}" == "one" ]; then
      build_cmd = "one"
      skip_arg=1
      next_arg=$((i+1))
      build_cmd_one_file="${!next_arg}"
      CONTINUE
    fi
  fi

  #### parse [arguments...]
  if [ "${!i}" == "--v" ]; then
    build_verbose=1
  elif [ "${!i}" == "--d" ]; then
    echo "" #### do nothing since this is default (bash has to have something inside an if)
  elif [ "${!i}" == "--r" ]; then
    build_dir="$build_folder/release"
    build_release=1
  elif [ "${!i}" == "--s" ]; then
    build_shared=1
  elif [ "${!i}" == "-platform" ]; then
    skip_arg=1
    next_arg=$((i+1))
    if [ "${!next_arg}" == "win32" ] || [ "${!next_arg}" == "mac" ] || [ "${!next_arg}" == "linux" ]; then
      build_platform="${!next_arg}"
    fi
  elif [ "${!i}" == "-graphics" ]; then
    skip_arg=1
    next_arg=$((i+1))
    if [ "${!next_arg}" == "vulkan" ] || [ "${!next_arg}" == "opengl" ] || [ "${!next_arg}" == "directx" ]; then
      build_graphics="${!next_arg}"
    fi
  elif [ "${!i}" == "-compiler" ]; then
    skip_arg=1
    next_arg=$((i+1))
    if [ "${!next_arg}" == "cl" ] || [ "${!next_arg}" == "gcc" ] || [ "${!next_arg}" == "clang" ]; then
      build_compiler="${!next_arg}"
    fi
  elif [ "${!i}" == "-linker" ]; then
    skip_arg=1
    next_arg=$((i+1))
    if [ "${!next_arg}" == "link" ] || [ "${!next_arg}" == "ld" ] || [ "${!next_arg}" == "lld-link" ]; then
      build_compiler="${!next_arg}"
    fi
  else
    echo "Unknown switch: ${!i}"
    exit 1
  fi
done
#_____________________________________________________________________________________________________
#                                         Global Defines
#_____________________________________________________________________________________________________
defines_build=""
if [ $build_release == 0 ]; then
  defines_build="-DBUILD_INTERNAL=0 -DBUILD_SLOW=0 -DBUILD_RELEASE=1"
else
  defines_build="-DBUILD_INTERNAL=1 -DBUILD_SLOW=1 -DBUILD_RELEASE=0"
fi


defines_platform=""
if [ $build_platform == "win32" ]; then
  defines_platform="-DDESHI_WINDOWS=1 -DDESHI_MAC=0 -DDESHI_LINUX=0"
elif [ $build_platform == "mac" ]; then
  defines_platform="-DDESHI_WINDOWS=0 -DDESHI_MAC=1 -DDESHI_LINUX=0" 
elif [ $build_platform == "linux" ]; then
  defines_platform="-DDESHI_WINDOWS=0 -DDESHI_MAC=0 -DDESHI_LINUX=1"
else
  echo "Platform defines not setup for platform: $build_platform"
  exit 1
fi


defines_graphics=""
if [ $build_graphics == "vulkan" ]; then
  defines_graphics="-DDESHI_VULKAN=1 -DDESHI_OPENGL=0 -DDESHI_DIRECTX12=0"
elif [ $build_graphics == "opengl" ]; then
  defines_graphics="-DDESHI_VULKAN=0 -DDESHI_OPENGL=1 -DDESHI_DIRECTX12=0"
elif [ $build_graphics == "directx" ]; then
  defines_graphics="-DDESHI_VULKAN=0 -DDESHI_OPENGL=0 -DDESHI_DIRECTX12=1"
else
  echo "Platform defines not setup for platform: $build_platform"
  exit 1
fi


defines_shared=""
if [ $build_shared == 1 ]; then
  defines_shared="-DDESHI_RELOADABLE_UI=1"
else
  defines_shared="-DDESHI_RELOADABLE_UI=0"
fi


defines="$defines_release $defines_platform $defines_graphics $defines_shared"
#_____________________________________________________________________________________________________
#                                           Build Flags
#_____________________________________________________________________________________________________
compile_flags=""
if [ $build_compiler == "cl" ]; then
  #### NOTE(delle): -diagnostics:caret shows the column and code where the error is in the source
  #### NOTE(delle): -EHsc enables exception handling
  #### NOTE(delle): -nologo prevents Microsoft copyright banner showing up
  #### NOTE(delle): -MD is used because vulkan's shader compilation lib requires dynamic linking with the CRT
  #### NOTE(delle): -MP enables building with multiple processors
  #### NOTE(delle): -Oi enables function inlining
  #### NOTE(delle): -GR enables RTTI and dynamic_cast<>()
  #### NOTE(delle): -Gm- disables minimal rebuild (recompile all files)
  #### NOTE(delle): -std:c++17 specifies to use the C++17 standard
  #### NOTE(delle): -utf-8 specifies that source files are in utf8
  compile_flags="$compile_flags -diagnostics:caret -EHsc -nologo -MD -MP -Oi -GR -Gm- -std:c++17 -utf-8"

  #### NOTE(delle): -W1 is the warning level
  #### NOTE(delle): -wd4100 disables the warning: unused function parameter
  #### NOTE(delle): -wd4189 disables the warning: unused local variabled
  #### NOTE(delle): -wd4201 disables the warning: nameless union or struct
  #### NOTE(delle): -wd4311 disables the warning: pointer truncation
  #### NOTE(delle): -wd4706 disables the warning: assignment within conditional expression
  compile_flags="$compile_flags -W1 -wd4100 -wd4189 -wd4201 -wd4311 -wd4706"

  if [ $build_release == 0 ]; then
    compile_flags="$compile_flags -Z7 -Od"
  else
    compile_flags="$compile_flags -O2"
  fi
elif [ $build_compiler == "gcc" ]; then
  echo "Compile flags not setup for compiler: $build_compiler"
  exit 1
elif [ $build_compiler == "clang" ]; then
  compile_flags="$compile_flags -std=c++17 -fexceptions -fcxx-exceptions -finline-functions -pipe"

  compile_flags="$compile_flags -Wno-unused-value -Wno-implicitly-unsigned-literal -Wno-nonportable-include-path -Wno-writable-strings -Wno-unused-function -Wno-unused-variable"

  if [ $build_release == 0 ]; then
    compile_flags="$compile_flags -g -gcodeview -O0"
  else
    compile_flags="$compile_flags -O2"
  fi
else
  echo "Compile flags not setup for compiler: $build_compiler"
  exit 1
fi


link_flags=""
link_libs=""
if [ $build_linker == "link" ]; then
  #### NOTE(delle): -nologo prevents Microsoft copyright banner showing up
  #### NOTE(delle): -opt:ref eliminates functions and data that are never used
  #### NOTE(delle): -incremental:no disables incremental linking (relink all files)
  link_flags="$link_flags -nologo -opt:ref -incremental:no"

  for ((i=0; i<${#lib_paths[@]}; i++)); do
    lib_path=${lib_paths[i]}
    link_libs="$link_libs /LIBPATH:$lib_path"
  done

  for ((i=0; i<${#libs[@]}; i++)); do
    lib_lib=${libs[i]}
    link_libs="$link_libs $lib_lib.lib"
  done
elif [ $build_linker == "ld" ]; then
  echo "Link flags not setup for linker: $build_linker"
  exit 1
elif [ $build_linker == "lld-linker" ]; then
  echo "Link flags not setup for linker: $build_linker"
  exit 1
else
  echo "Link flags not setup for linker: $build_linker"
  exit 1
fi
#_____________________________________________________________________________________________________
#                                           Execute Commands
#_____________________________________________________________________________________________________
#TODO(delle) add non-default command checking

#### function to echo and execute commands if verbose flag is set
exe(){
  if [ $build_verbose == 1 ]; then
    echo "\$ $@"; "$@";
  else
    "$@";
  fi
}

pushd $build_dir > /dev/null
if [ $builder_platform == "win32" ]; then
  if [ -e $misc_folder/ctime.exe ]; then
    ctime -begin $misc_folder/$app_name.ctm
  fi

  #### delete previous debug info
  if [ $build_shared == 1 ]; then
    rm *.pdb > /dev/null 2> /dev/null
    echo Waiting for PDB > lock.tmp
  fi

  #### compile deshi          (generates deshi.obj)
  exe $build_compiler $deshi_sources $includes -c $compile_flags $defines -Fo"deshi.obj"

  #### compile deshi DLLs     (generates deshi.dll)
  if [ $build_shared == 1 ]; then
    exe $build_compiler $dll_sources $includes -c $compile_flags $defines -Fodeshi_dlls.obj
    exe $build_linker deshi.obj deshi_dlls.obj -DLL $link_flags $link_libs -OUT:deshi.dll -PDB:deshi_dlls_$RANDOM.pdb
    rm lock.tmp
  fi

  #### compile app            (generates app_name.exe)
  exe $build_compiler $app_sources $includes -c $compile_flags $defines -Fo"$app_name.obj"
  if [ $build_shared == 1 ]; then
    exe $build_linker deshi.obj deshi.dll $app_name.obj $link_flags $link_libs -OUT:$app_name.exe
  else
    exe $build_linker deshi.obj $app_name.obj $link_flags $link_libs -OUT:$app_name.exe
  fi

  if [ -e $misc_folder/ctime.exe ]; then
    ctime -end $misc_folder/$app_name.ctm
  fi
elif [ $builder_platform == "mac" ]; then
  echo "Execute commands not setup for platform: $builder_platform"
  exit 1
elif [ $builder_platform == "linux" ]; then
  echo "Execute commands not setup for platform: $builder_platform"
  exit 1
else
  echo "Execute commands not setup for platform: $builder_platform"
  exit 1
fi
popd > /dev/null