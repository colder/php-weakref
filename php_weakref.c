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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "php.h"
#include "zend_exceptions.h"
#include "ext/standard/info.h"
#include "wr_weakref.h"
//#include "wr_weakmap.h" FIXME
#include "wr_store.h"
#include "php_weakref.h"

#ifdef ZTS
int weakref_globals_id;
#else
zend_weakref_globals weakref_globals;
#endif

PHP_MINIT_FUNCTION(weakref) /* {{{ */
{
	PHP_MINIT(wr_weakref)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(wr_weakmap)(INIT_FUNC_ARGS_PASSTHRU);
}
/* }}} */

PHP_RINIT_FUNCTION(weakref) /* {{{ */
{
	wr_store_init(TSRMLS_C);
	return SUCCESS;
}
/* }}} */

PHP_RSHUTDOWN_FUNCTION(weakref) /* {{{ */
{
	wr_store_destroy(TSRMLS_C);
	return SUCCESS;
}
/* }}} */

static PHP_GINIT_FUNCTION(weakref) /* {{{ */
{
	weakref_globals->store = NULL;
}
/* }}} */

PHP_MINFO_FUNCTION(weakref) /* {{{ */
{
	php_info_print_table_start();
	php_info_print_table_header(2, "Weak References support", "enabled");
	php_info_print_table_row(2, "Version", PHP_WEAKREF_VERSION);
	php_info_print_table_end();
}
/* }}} */

zend_module_entry weakref_module_entry = { /* {{{ */
	STANDARD_MODULE_HEADER_EX, NULL,
	NULL,
	"Weakref",
	NULL,
	PHP_MINIT(weakref),
	NULL,
	PHP_RINIT(weakref),
	PHP_RSHUTDOWN(weakref),
	PHP_MINFO(weakref),
	PHP_WEAKREF_VERSION,
	PHP_MODULE_GLOBALS(weakref),
	PHP_GINIT(weakref),
	NULL,
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
};
/* }}} */

#ifdef COMPILE_DL_WEAKREF
ZEND_GET_MODULE(weakref);
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
