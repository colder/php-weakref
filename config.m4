dnl config.m4 for extension Weakref

PHP_ARG_ENABLE(weakref, enable Weakref suppport,
[  --enable-weakref     Enable Weakref])

if test "$PHP_WEAKREF" != "no"; then
  PHP_NEW_EXTENSION(weakref, php_weakref.c wr_weakref.c wr_weakmap.c, $ext_shared)
fi
