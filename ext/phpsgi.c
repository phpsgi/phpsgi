#include "phpsgi.h"

PHP_MINIT_FUNCTION(phpsgi);
PHP_MSHUTDOWN_FUNCTION(phpsgi);
PHP_MINFO_FUNCTION(phpsgi);

#if COMPILE_DL_PHPSGI
ZEND_GET_MODULE(phpsgi)
#endif

// ZEND_BEGIN_ARG_INFO_EX(name, _unused, return_reference, required_num_args)
ZEND_BEGIN_ARG_INFO_EX(arginfo_app_call, 0, 0, 2)
    // ZEND_ARG_INFO(pass_by_ref, name)
	// ZEND_ARG_INFO(1, environment)
	ZEND_ARG_ARRAY_INFO(1, environment, 0)

    // ZEND_ARG_ARRAY_INFO(pass_by_ref, name, allow_null)
	ZEND_ARG_ARRAY_INFO(0, response, 0)
ZEND_END_ARG_INFO()


zend_class_entry *phpsgi_ce_app_interface;

static const zend_function_entry phpsgi_app_interface_funcs[] = {
	PHP_ABSTRACT_ME(AppInterface, call, arginfo_app_call)
	// PHP_ABSTRACT_ME(AppInterface, __invoke, NULL)
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
}


