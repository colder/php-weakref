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

#ifndef PHP_WEAKREF_H
#define PHP_WEAKREF_H

#include <php.h>

#define PHP_WEAKREF_VERSION "0.4.0"

#ifdef PHP_WIN32
#define WEAKREF_API __declspec(dllexport)
#else
#define WEAKREF_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#include "wr_weakref.h"
#include "wr_store.h"

extern zend_module_entry weakref_module_entry;
#define phpext_weakref_ptr &weakref_module_entry

PHP_MINFO_FUNCTION(weakref);

PHP_MINIT_FUNCTION(weakref);
PHP_RINIT_FUNCTION(weakref);
PHP_RSHUTDOWN_FUNCTION(weakref);

ZEND_BEGIN_MODULE_GLOBALS(weakref)
    wr_store *store;
ZEND_END_MODULE_GLOBALS(weakref)

#ifdef ZTS
#define WR_G(v) TSRMG(weakref_globals_id, zend_weakref_globals *, v)
extern int weakref_globals_id;
#else
#define WR_G(v) (weakref_globals.v)
extern zend_weakref_globals weakref_globals;
#endif

#endif /* PHP_WEAKREF_H */

/*
 * Local Variables:
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
