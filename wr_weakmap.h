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
	zend_object            std;
	HashTable              map;
	HashPosition           pos;
	zend_function         *fptr_offset_get;
	zend_function         *fptr_offset_set;
	zend_function         *fptr_offset_has;
	zend_function         *fptr_offset_del;
	zend_function         *fptr_count;
} wr_weakmap_object;

extern WEAKREF_API zend_class_entry *wr_ce_WeakMap;

#endif /* WR_WEAKMAP_H */

/*
 * Local Variables:
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
