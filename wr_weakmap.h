/*
   +----------------------------------------------------------------------+
   | Weakref                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2011 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Etienne Kneuss <colder@php.net>                             |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

#ifndef WR_WEAKMAP_H
#define WR_WEAKMAP_H

#include "php_weakref.h"

typedef struct _wr_weakmap_object {
	HashTable              map;
	HashPosition           pos;
	zend_object            std;
} wr_weakmap_object;

typedef struct _wr_weakmap_refval {
	zend_object *ref;
	zval         val;
} wr_weakmap_refval;

extern WEAKREF_API zend_class_entry *wr_ce_WeakMap;

PHP_MINIT_FUNCTION(wr_weakmap);

#endif /* WR_WEAKMAP_H */

/*
 * Local Variables:
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
