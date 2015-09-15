#include "phpsgi.h"
#include <Zend/zend_interfaces.h>
#include <ext/standard/php_var.h>

PHP_MINIT_FUNCTION(phpsgi);
PHP_MSHUTDOWN_FUNCTION(phpsgi);
PHP_MINFO_FUNCTION(phpsgi);

#if COMPILE_DL_PHPSGI
ZEND_GET_MODULE(phpsgi)
#endif


ZEND_BEGIN_ARG_INFO_EX(arginfo_callable, 0, 0, 1)
    ZEND_ARG_INFO(0, callable)
ZEND_END_ARG_INFO()

// ZEND_BEGIN_ARG_INFO_EX(name, _unused, return_reference, required_num_args)
ZEND_BEGIN_ARG_INFO_EX(arginfo_app_call, 0, 0, 2)
    // ZEND_ARG_INFO(pass_by_ref, name)
    // ZEND_ARG_INFO(1, environment)
    ZEND_ARG_ARRAY_INFO(1, environment, 0)

    // ZEND_ARG_ARRAY_INFO(pass_by_ref, name, allow_null)
    ZEND_ARG_ARRAY_INFO(0, response, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_middleware_call, 0, 0, 2)
    // ZEND_ARG_INFO(pass_by_ref, name)
    // ZEND_ARG_INFO(1, environment)
    ZEND_ARG_ARRAY_INFO(1, environment, 0)

    // ZEND_ARG_ARRAY_INFO(pass_by_ref, name, allow_null)
    ZEND_ARG_ARRAY_INFO(0, response, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_middleware_invoke, 0, 0, 2)
    // ZEND_ARG_INFO(pass_by_ref, name)
    // ZEND_ARG_INFO(1, environment)
    ZEND_ARG_ARRAY_INFO(1, environment, 0)

    // ZEND_ARG_ARRAY_INFO(pass_by_ref, name, allow_null)
    ZEND_ARG_ARRAY_INFO(0, response, 0)
ZEND_END_ARG_INFO()



// class entries
zend_class_entry *phpsgi_ce_middleware;
zend_class_entry *phpsgi_ce_app_interface;

static const zend_function_entry phpsgi_app_interface_funcs[] = {
	PHP_ABSTRACT_ME(PHPSGIAppInterface, call, arginfo_app_call)
	PHP_FE_END
};

const zend_function_entry phpsgi_middleware_funcs[] = {
  PHP_ME(PHPSGIMiddleware, __construct, arginfo_callable, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
  PHP_ME(PHPSGIMiddleware, call, arginfo_middleware_call, ZEND_ACC_PUBLIC)
  PHP_ME(PHPSGIMiddleware, __invoke, arginfo_middleware_invoke, ZEND_ACC_PUBLIC)


  // To create a method alias:
  // ZEND_NAMED_ME(__soapCall, ZEND_MN(SoapClient___call), arginfo_soapclient___soapcall, 0)
  PHP_FE_END
};


zend_module_entry phpsgi_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    "PHPSGI",
    NULL,
    PHP_MINIT(phpsgi),
    PHP_MSHUTDOWN(phpsgi),
    NULL,
    NULL,
    PHP_MINFO(phpsgi),
#if ZEND_MODULE_API_NO >= 20010901
    "0.1",
#endif
    STANDARD_MODULE_PROPERTIES
};

static void phpsgi_register_classes(TSRMLS_D);
static int phpsgi_implement_app_interface_handler(zend_class_entry *interface, zend_class_entry *implementor TSRMLS_DC);

PHP_MINIT_FUNCTION(phpsgi) {
    phpsgi_register_classes(TSRMLS_C);
    return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(phpsgi) {
    return SUCCESS;
}

PHP_MINFO_FUNCTION(phpsgi) {
}

PHP_FUNCTION(phpsgi_hello) {
    RETURN_TRUE;
}




PHP_METHOD(PHPSGIMiddleware, __construct) {
	zval *zcallable = NULL;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &zcallable) == FAILURE) {
		return;
	}
    zend_update_property(phpsgi_ce_middleware, this_ptr, "next", sizeof("next")-1, zcallable TSRMLS_CC);
}




zval * call_middleware_method(zval * object , char * method_name , int method_name_len, zval* arg1, zval* arg2 TSRMLS_DC)
{
    zend_function *fe;
    if (zend_hash_find(&phpsgi_ce_middleware->function_table, method_name, method_name_len, (void **) &fe) == FAILURE ) {
        php_error(E_ERROR, "PHPSGI\\Middleware::%s method not found", method_name);
    }
    // call export method
    zval *z_retval = NULL;

    // zend_call_method_with_2_params(obj, obj_ce, fn_proxy, function_name, retval, arg1, arg2)
    zend_call_method_with_2_params(&object, phpsgi_ce_middleware, &fe, method_name, &z_retval, arg1, arg2);
    return z_retval;
}



static inline void _set_default_return_value(zval **return_value)
{
    array_init(*return_value);
    add_next_index_null(*return_value);
    add_next_index_null(*return_value);
    add_next_index_null(*return_value);
}

static inline int _call_next(zval **return_value, zval *znext, zval *zenv_array, zval *zresponse_array TSRMLS_DC)
{
    int status;
    zval ***args = NULL;
    zval *retval_ptr = NULL;
    args = safe_emalloc(sizeof(zval **), 2, 0);
    args[0] = &zenv_array;
    args[1] = &zresponse_array;

    status = call_user_function_ex(EG(function_table), NULL, znext, &retval_ptr, 2, args, 0, NULL TSRMLS_CC);

    efree(args);

    if (status == SUCCESS && retval_ptr != NULL) {
        **return_value = *retval_ptr;
        zval_copy_ctor(*return_value);
        zval_ptr_dtor(&retval_ptr);
        return status;
    }

    if (retval_ptr != NULL) {
        zval_ptr_dtor(&retval_ptr);
    }
    return status;
}


PHP_METHOD(PHPSGIMiddleware, call) {
    zval *zenv_array = NULL;
    zval *zresponse_array = NULL;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "aa", &zenv_array, &zresponse_array) == FAILURE) {
        return;
    }

    zval * znext = zend_read_property(phpsgi_ce_middleware, this_ptr, "next", sizeof("next")-1, 1 TSRMLS_CC);
    if (znext != NULL) {

        if (SUCCESS == _call_next(&return_value, znext, zenv_array, zresponse_array TSRMLS_CC)) {
            // php_var_dump(&return_value, 1 TSRMLS_CC);
            return;
        }
    }
    php_error(E_ERROR, "PHPSGI\\Middleware::next undefined.");
    _set_default_return_value(&return_value);
}

PHP_METHOD(PHPSGIMiddleware, __invoke) {
    zval *zenv_array = NULL;
    zval *zresponse_array = NULL;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "aa", &zenv_array, &zresponse_array) == FAILURE) {
        return;
    }


    zend_function *fe;
    if (zend_hash_find(&phpsgi_ce_middleware->function_table, "call", sizeof("call"), (void **) &fe) == FAILURE) {
        php_error(E_ERROR, "PHPSGI\\Middleware::%s method not found", "call");
    }

    zval *retval_ptr = NULL;

    zval ***params = NULL;
    params = safe_emalloc(sizeof(zval **), 2, 0);
    params[0] = &zenv_array;
    params[1] = &zresponse_array;

    zval *funcname;
    ZVAL_STRING(funcname, "call", 1);

    int status = call_user_function_ex(&phpsgi_ce_middleware->function_table, &this_ptr, funcname, &retval_ptr, 2, params, 0, NULL TSRMLS_CC);

    zval_dtor(funcname);
    efree(params);

    if (status == SUCCESS) {
        if (retval_ptr != NULL) {
            *return_value = *retval_ptr;
            zval_copy_ctor(return_value);
            zval_ptr_dtor(&retval_ptr);
            return;
        }
    }


    _set_default_return_value(&return_value);
}


/**
 * function to check if an user class implements the interface
 *
 */
static int phpsgi_implement_app_interface_handler(zend_class_entry *interface, zend_class_entry *implementor TSRMLS_DC)
{
	if (implementor->type == ZEND_USER_CLASS && !instanceof_function(implementor, phpsgi_ce_app_interface TSRMLS_CC)) {
		zend_error(E_ERROR, "DateTimeInterface can't be implemented by user classes");
	}

	return SUCCESS;
}

static void phpsgi_register_classes(TSRMLS_D)
{
	zend_class_entry ce_interface;
	INIT_CLASS_ENTRY(ce_interface, "PHPSGI\\App", phpsgi_app_interface_funcs);

	phpsgi_ce_app_interface = zend_register_internal_interface(&ce_interface TSRMLS_CC);
	phpsgi_ce_app_interface->interface_gets_implemented = phpsgi_implement_app_interface_handler;



    zend_class_entry ce_middleware;
    INIT_CLASS_ENTRY(ce_middleware, "PHPSGI\\Middleware", phpsgi_middleware_funcs);
    phpsgi_ce_middleware = zend_register_internal_class(&ce_middleware TSRMLS_CC);

    // implement one interface
    zend_class_implements(phpsgi_ce_middleware TSRMLS_CC, 1, phpsgi_ce_app_interface);

    zend_declare_property_null(phpsgi_ce_middleware, "next", sizeof("next")-1, ZEND_ACC_PUBLIC TSRMLS_CC);
}






