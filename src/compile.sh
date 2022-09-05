#! /bin/bash


# sudo apt install gcc make gcc-aarch64-linux-gnu binutils-aarch64-linux-gnu

# System.getProperty("os.arch")
JAVA_OS_ARCH=${1:-amd64}
case $JAVA_OS_ARCH in

  amd64)
    GCC=gcc
    GCC_EXTRA=-m64
    ;;

  aarch64)
    GCC=aarch64-linux-gnu-gcc
    GCC_EXTRA=
    ;;

  *)
    echo "Unsupported JAVA_OS_ARCH $JAVA_OS_ARCH"
    exit 1
    ;;
esac

echo -n "Compiling for $JAVA_OS_ARCH, using $GCC"

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
rm $BASEDIR/*.o $BASEDIR/libcsjsound_${JAVA_OS_ARCH}.so

for FILE in jni_iface impl ; do
  $GCC $GCC_EXTRA -c -fPIC -I${JAVA_HOME}/include -I${JAVA_HOME}/include/linux -I$BASEDIR/../ $BASEDIR/$FILE.c -o $BASEDIR/$FILE.o
done

$GCC -shared $GCC_EXTRA -Wl,--hash-style=both -Wl,-z,defs -Wl,-O1 -Wl,-z,noexecstack -Wl,--exclude-libs,ALL -Wl,-z,origin -Wl,-rpath,\$ORIGIN -Wl,-soname=libcsjsound_amd64.so $BASEDIR/*.o -o $BASEDIR/libcsjsound_${JAVA_OS_ARCH}.so -lasound
