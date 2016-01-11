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
#include "ext/spl/spl_exceptions.h"
#include "wr_weakref.h"
#include "php_weakref.h"

zend_object_handlers wr_handler_WeakRef;
WEAKREF_API zend_class_entry  *wr_ce_WeakRef;

static inline wr_weakref_object* wr_weakref_fetch(zend_object *obj) {
	return (wr_weakref_object *)((char *)obj - XtOffsetOf(wr_weakref_object, std));
}

#define Z_WEAKREF_OBJ_P(zv) wr_weakref_fetch(Z_OBJ_P(zv));


static int wr_weakref_ref_acquire(wr_weakref_object *wref) /* {{{ */
{
	if (wref->valid) {
		if (wref->acquired == 0) {
			// From now on we hold a proper reference
			GC_REFCOUNT(wref->ref_obj)++;
		}
		wref->acquired++;
		return SUCCESS;
	} else {
		return FAILURE;
	}
}
/* }}} */

static int wr_weakref_ref_release(wr_weakref_object *wref) /* {{{ */
{
	if (wref->valid && (wref->acquired > 0)) {
		wref->acquired--;
		if (wref->acquired == 0) {
			// We no longer need a proper reference
			OBJ_RELEASE(wref->ref_obj);
		}
		return SUCCESS;
	} else {
		return FAILURE;
	}
}
/* }}} */

static void wr_weakref_ref_dtor(zend_object *wref_obj, zend_object *ref_obj) { /* {{{ */
	wr_weakref_object *wref = wr_weakref_fetch(wref_obj);

	/* During shutdown, the ref might be dtored before the wref is released */
	while (wref->acquired > 0) {
		if (wr_weakref_ref_release(wref) != SUCCESS) {
			// shouldn't occur
			zend_throw_exception(spl_ce_RuntimeException, "Failed to correctly release the reference during shutdown", 0);
			break;
		}
	}

	wref->valid = 0;
}
/* }}} */

static void wr_weakref_object_free_storage(zend_object *wref_obj) /* {{{ */
{
	wr_weakref_object *wref     = wr_weakref_fetch(wref_obj);

	while (wref->acquired > 0) {
		if (wr_weakref_ref_release(wref) != SUCCESS) {
			// shouldn't occur
			zend_throw_exception(spl_ce_RuntimeException, "Failed to correctly release the reference on free", 0);
			break;
		}
	}

	if (wref->valid) {
		wr_store_untrack(wref_obj, wref->ref_obj);
	}

	zend_object_std_dtor(&wref->std);
}
/* }}} */

static zend_object* wr_weakref_object_new_ex(zend_class_entry *ce, zend_object *cloneOf) /* {{{ */
{
	wr_weakref_object *wref = ecalloc(1, sizeof(wr_weakref_object) + zend_object_properties_size(ce));

	zend_object_std_init(&wref->std, ce);
	object_properties_init(&wref->std, ce);

	if (cloneOf) {
		wr_weakref_object *other = wr_weakref_fetch(cloneOf);
		wref->valid   = other->valid;
		wref->ref_obj = other->ref_obj;

		wr_store_track(&wref->std, wr_weakref_ref_dtor, other->ref_obj);

		while(wref->acquired < other->acquired && (wr_weakref_ref_acquire(wref) == SUCCESS)) {
			// NOOP
		}
	}
	//if (clone_orig && orig) {
		//wr_weakref_object *other = (wr_weakref_object *)zend_object_store_get_object(orig);
		//if (other->valid) {
		//	int acquired = 0;

		//	intern->valid = other->valid;
		//	ALLOC_INIT_ZVAL(intern->ref);
		//	// ZVAL_COPY_VALUE
		//	intern->ref->value = other->ref->value;
		//	Z_TYPE_P(intern->ref) = Z_TYPE_P(other->ref);

		//	wr_store_track((zend_object *)intern, wr_weakref_ref_dtor, other->ref);

		//	for (acquired = 0; acquired < other->acquired; acquired++) {
		//		wr_weakref_ref_acquire(intern);
		//	}

		//	if (intern->acquired != other->acquired) {
		//		// shouldn't occur
		//		zend_throw_exception(spl_ce_RuntimeException, "Failed to correctly acquire clone's reference", 0);
		//	}

		//} else {
		//	intern->valid    = 0;
		//	intern->ref_obj  = NULL;
		//	intern->acquired = 0;
		//}
	//} else {

	//}

	wref->std.handlers = &wr_handler_WeakRef;

	return &wref->std;
}
/* }}} */

static zend_object* wr_weakref_object_clone(zval *cloneOf_zv) /* {{{ */
{
	zend_object *cloneOf = Z_OBJ_P(cloneOf_zv);
	zend_object *clone = wr_weakref_object_new_ex(cloneOf->ce, cloneOf);

	zend_objects_clone_members(clone, Z_OBJ_P(cloneOf_zv));

	return clone;
}
/* }}} */

static zend_object* wr_weakref_object_new(zend_class_entry *ce) /* {{{ */
{
	return wr_weakref_object_new_ex(ce, NULL);
}
/* }}} */

/* {{{ proto object WeakRef::get()
 Return the reference, or null. */
PHP_METHOD(WeakRef, get)
{
	wr_weakref_object *wref = Z_WEAKREF_OBJ_P(getThis());
	zval ret;

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	if (wref->valid) {
		ZVAL_OBJ(&ret, wref->ref_obj);
		RETURN_ZVAL(&ret, 1, 0);
	} else {
		RETURN_NULL();
	}
}
/* }}} */

/* {{{ proto bool WeakRef::acquire()
 Return the reference, or null. */
PHP_METHOD(WeakRef, acquire)
{
	wr_weakref_object *wref = Z_WEAKREF_OBJ_P(getThis());

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	if (wr_weakref_ref_acquire(wref) == SUCCESS) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto bool WeakRef::release()
 Return the reference, or null. */
PHP_METHOD(WeakRef, release)
{
	wr_weakref_object *wref = Z_WEAKREF_OBJ_P(getThis());

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	if (wr_weakref_ref_release(wref) == SUCCESS) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto bool WeakRef::valid()
 Return whether the reference still valid. */
PHP_METHOD(WeakRef, valid)
{
	wr_weakref_object *wref = Z_WEAKREF_OBJ_P(getThis());

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	RETURN_BOOL(wref->valid);
}
/* }}} */

/* {{{ proto void WeakRef::__construct(object ref)
*/
PHP_METHOD(WeakRef, __construct)
{
	zval *ref;
	zend_object *wref_obj   = Z_OBJ_P(getThis());
	wr_weakref_object *wref = wr_weakref_fetch(wref_obj);

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS(), "o", &ref)) {
		return;
	}

	wref->ref_obj = Z_OBJ_P(ref);

	wr_store_track(wref_obj, wr_weakref_ref_dtor, Z_OBJ_P(ref));

	wref->valid = 1;
}
/* }}} */

/*  Function/Class/Method definitions */
ZEND_BEGIN_ARG_INFO(arginfo_wr_weakref_void, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_wr_weakref_obj, 0)
	ZEND_ARG_INFO(0, object)
ZEND_END_ARG_INFO()

static const zend_function_entry wr_funcs_WeakRef[] = {
	PHP_ME(WeakRef, __construct,     arginfo_wr_weakref_obj,             ZEND_ACC_PUBLIC)
	PHP_ME(WeakRef, valid,           arginfo_wr_weakref_void,            ZEND_ACC_PUBLIC)
	PHP_ME(WeakRef, get,             arginfo_wr_weakref_void,            ZEND_ACC_PUBLIC)
	PHP_ME(WeakRef, acquire,         arginfo_wr_weakref_void,            ZEND_ACC_PUBLIC)
	PHP_ME(WeakRef, release,         arginfo_wr_weakref_void,            ZEND_ACC_PUBLIC)
	PHP_FE_END
};
/* }}} */

PHP_MINIT_FUNCTION(wr_weakref) /* {{{ */
{
	zend_class_entry weakref_ce;

	INIT_CLASS_ENTRY(weakref_ce, "WeakRef", wr_funcs_WeakRef);

	wr_ce_WeakRef = zend_register_internal_class(&weakref_ce);

	wr_ce_WeakRef->ce_flags      |= ZEND_ACC_FINAL;
	wr_ce_WeakRef->create_object  = wr_weakref_object_new;

	memcpy(&wr_handler_WeakRef, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

	wr_handler_WeakRef.clone_obj = wr_weakref_object_clone;
	wr_handler_WeakRef.free_obj  = wr_weakref_object_free_storage;
	wr_handler_WeakRef.offset    = XtOffsetOf(wr_weakref_object, std);

	return SUCCESS;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
