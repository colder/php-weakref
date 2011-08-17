dnl config.m4 for extension Weakref

PHP_ARG_ENABLE(weakref, enable Weakref suppport,
[  --enable-weakref     Enable Weakref])

if test "$PHP_WEAKREF" != "no"; then
  PHP_NEW_EXTENSION(weakref, weakref.c, $ext_shared)
fi
