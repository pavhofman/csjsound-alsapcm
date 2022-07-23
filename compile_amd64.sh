#! /bin/bash

# System.getProperty("os.arch")
JAVA_OS_ARCH="amd64"

if [ -z "$JAVA_HOME" ]; then
  JAVA_BIN="$(which javac)"
  if [ -z "$JAVA_BIN" ]; then
    echo "Cannot determine \$JAVA_HOME, is java installed?"
    exit 1
  fi
  JAVA_HOME=$(dirname $(dirname $(readlink -f $JAVA_BIN)))
fi

echo "Using JAVA_HOME: $JAVA_HOME"

BASEDIR=$(dirname "$0")
rm $BASEDIR/*.o $BASEDIR/*.so

for FILE in jni_iface impl ; do
  gcc -c -fPIC -I${JAVA_HOME}/include -I${JAVA_HOME}/include/linux  $BASEDIR/$FILE.c -o $BASEDIR/$FILE.o
done

gcc -shared -Wl,--hash-style=both -Wl,-z,defs -Wl,-O1 -m64 -Wl,-z,noexecstack -Wl,--exclude-libs,ALL -Wl,-z,origin -Wl,-rpath,\$ORIGIN -Wl,-soname=libcsjsound_amd64.so $BASEDIR/*.o -o $BASEDIR/libcsjsound_${JAVA_OS_ARCH}.so -lasound
