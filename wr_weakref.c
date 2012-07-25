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


static void wr_weakref_ref_dtor(void *ref_object, zend_object_handle ref_handle, zend_object *wref_obj TSRMLS_DC) { /* {{{ */
	wr_weakref_object *wref = (wr_weakref_object *)wref_obj;
	wref->valid = 0;
}
/* }}} */

static int wr_weakref_ref_acquire(wr_weakref_object *intern TSRMLS_DC) /* {{{ */
{
	if (intern->valid) {
		if (intern->acquired == 0) {
			// We need to register that ref so that the object doesn't get collected
			Z_OBJ_HANDLER_P(intern->ref, add_ref)(intern->ref TSRMLS_CC);
		}
		intern->acquired++;
		return SUCCESS;
	} else {
		return FAILURE;
	}
}
/* }}} */

static int wr_weakref_ref_release(wr_weakref_object *intern TSRMLS_DC) /* {{{ */
{
	if (intern->valid && (intern->acquired > 0)) {
		intern->acquired--;
		if (intern->acquired == 0) {
			// We need to unregister that ref so that the object can get collected
			Z_OBJ_HANDLER_P(intern->ref, del_ref)(intern->ref TSRMLS_CC);
		}
		return SUCCESS;
	} else {
		return FAILURE;
	}
}
/* }}} */

static void wr_weakref_object_free_storage(void *object TSRMLS_DC ZEND_FILE_LINE_DC) /* {{{ */
{
	wr_weakref_object *intern     = (wr_weakref_object *)object;

	while (intern->acquired > 0) {
		if (wr_weakref_ref_release(intern TSRMLS_CC) != SUCCESS) {
			// shouldn't occur
			zend_throw_exception(spl_ce_RuntimeException, "Failed to correctly release the reference on free", 0 TSRMLS_CC);
			break;
		}
	}

	if (intern->valid) {
		wr_store_detach((zend_object *)intern, Z_OBJ_HANDLE_P(intern->ref) TSRMLS_CC);
	}

	if (intern->ref) {
		Z_TYPE_P(intern->ref) = IS_NULL;
		zval_ptr_dtor(&intern->ref);
	}

	zend_object_std_dtor(&intern->std TSRMLS_CC);

	efree(object);
}
/* }}} */

static zend_object_value wr_weakref_object_new_ex(zend_class_entry *class_type, wr_weakref_object **obj, zval *orig, int clone_orig TSRMLS_DC) /* {{{ */
{
	zend_object_value  retval;
	wr_weakref_object *intern;

#if PHP_VERSION_ID < 50399
	zval              *tmp;
#endif

	intern = ecalloc(1, sizeof(wr_weakref_object));

	*obj = intern;

	zend_object_std_init(&intern->std, class_type TSRMLS_CC);

#if PHP_VERSION_ID >= 50399
	object_properties_init(&intern->std, class_type);
#else
	zend_hash_copy(intern->std.properties, &class_type->default_properties, (copy_ctor_func_t) zval_add_ref, (void *) &tmp, sizeof(zval *));
#endif

	if (clone_orig && orig) {
		wr_weakref_object *other = (wr_weakref_object *)zend_object_store_get_object(orig TSRMLS_CC);
		if (other->valid) {
			int acquired = 0;

			intern->valid = other->valid;
			ALLOC_INIT_ZVAL(intern->ref);
			// ZVAL_COPY_VALUE
			intern->ref->value = other->ref->value;
			Z_TYPE_P(intern->ref) = Z_TYPE_P(other->ref);

			wr_store_attach((zend_object *)intern, wr_weakref_ref_dtor, other->ref TSRMLS_CC);

			for (acquired = 0; acquired < other->acquired; acquired++) {
				wr_weakref_ref_acquire(intern TSRMLS_CC);
			}

			if (intern->acquired != other->acquired) {
				// shouldn't occur
				zend_throw_exception(spl_ce_RuntimeException, "Failed to correctly acquire clone's reference", 0 TSRMLS_CC);
			}

		} else {
			intern->valid = 0;
			intern->ref   = NULL;
			intern->acquired = 0;
		}
	} else {
		intern->valid = 0;
		intern->ref   = NULL;
		intern->acquired = 0;
	}

	retval.handle   = zend_objects_store_put(intern, (zend_objects_store_dtor_t)zend_objects_destroy_object, (zend_objects_free_object_storage_t)wr_weakref_object_free_storage, NULL TSRMLS_CC);
	retval.handlers = &wr_handler_WeakRef;

	return retval;
}
/* }}} */

static zend_object_value wr_weakref_object_clone(zval *zobject TSRMLS_DC) /* {{{ */
{
	zend_object_value      new_obj_val;
	zend_object           *old_object;
	zend_object           *new_object;
	zend_object_handle     handle = Z_OBJ_HANDLE_P(zobject);
	wr_weakref_object     *intern;

	old_object  = zend_objects_get_address(zobject TSRMLS_CC);
	new_obj_val = wr_weakref_object_new_ex(old_object->ce, &intern, zobject, 1 TSRMLS_CC);
	new_object  = &intern->std;

	zend_objects_clone_members(new_object, new_obj_val, old_object, handle TSRMLS_CC);

	return new_obj_val;
}
/* }}} */

static zend_object_value wr_weakref_object_new(zend_class_entry *class_type TSRMLS_DC) /* {{{ */
{
    wr_weakref_object *tmp;
	return wr_weakref_object_new_ex(class_type, &tmp, NULL, 0 TSRMLS_CC);
}
/* }}} */

/* {{{ proto object WeakRef::get()
 Return the reference, or null. */
PHP_METHOD(WeakRef, get)
{
	zval *object = getThis();
	wr_weakref_object *intern;

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "")) {
		return;
	}

	intern = (wr_weakref_object *)zend_object_store_get_object(object TSRMLS_CC);

	if (intern->valid) {
		RETURN_ZVAL(intern->ref, 1, 0);
	} else {
		RETURN_NULL();
	}
}
/* }}} */

/* {{{ proto bool WeakRef::acquire()
 Return the reference, or null. */
PHP_METHOD(WeakRef, acquire)
{
	zval *object = getThis();
	wr_weakref_object *intern;

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "")) {
		return;
	}

	intern = (wr_weakref_object *)zend_object_store_get_object(object TSRMLS_CC);

	if (wr_weakref_ref_acquire(intern TSRMLS_CC) == SUCCESS) {
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
	zval *object = getThis();
	wr_weakref_object *intern;

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "")) {
		return;
	}

	intern = (wr_weakref_object *)zend_object_store_get_object(object TSRMLS_CC);

	if (wr_weakref_ref_release(intern TSRMLS_CC) == SUCCESS) {
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
	zval *object = getThis();
	wr_weakref_object *intern;

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "")) {
		return;
	}

	intern = (wr_weakref_object *)zend_object_store_get_object(object TSRMLS_CC);

	RETURN_BOOL(intern->valid);
}
/* }}} */

/* {{{ proto void WeakRef::__construct(object ref)
*/
PHP_METHOD(WeakRef, __construct)
{
	zval *ref, *object = getThis();
	wr_weakref_object *intern;

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "o", &ref)) {
		return;
	}

	intern = (wr_weakref_object *)zend_object_store_get_object(object TSRMLS_CC);

	ALLOC_INIT_ZVAL(intern->ref);

	// ZVAL_COPY_VALUE
	intern->ref->value = ref->value;
	Z_TYPE_P(intern->ref) = Z_TYPE_P(ref);

	wr_store_attach((zend_object *)intern, wr_weakref_ref_dtor, intern->ref TSRMLS_CC);

	intern->valid = 1;
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
	{NULL, NULL, NULL}
};
/* }}} */

PHP_MINIT_FUNCTION(wr_weakref) /* {{{ */
{
	zend_class_entry weakref_ce;

	INIT_CLASS_ENTRY(weakref_ce, "WeakRef", wr_funcs_WeakRef);

	wr_ce_WeakRef = zend_register_internal_class(&weakref_ce TSRMLS_CC);

	wr_ce_WeakRef->ce_flags      |= ZEND_ACC_FINAL_CLASS;
	wr_ce_WeakRef->create_object  = wr_weakref_object_new;

	memcpy(&wr_handler_WeakRef, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

	wr_handler_WeakRef.clone_obj = wr_weakref_object_clone;

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
