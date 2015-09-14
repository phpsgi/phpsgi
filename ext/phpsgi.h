#ifndef _PHPSGI_H
#define _PHPSGI_H
#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif

#ifdef ZTS
#include <TSRM.h>
#endif

#include <php.h>

extern zend_module_entry phpsgi_module_entry;

PHP_METHOD(PHPSGIMiddleware, __construct);
PHP_METHOD(PHPSGIMiddleware, call);
PHP_METHOD(PHPSGIMiddleware, __invoke);

#endif

