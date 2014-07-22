/* $Id: php_fsevents.h 2014-07-20 14:14:35Z bgriffith $ */

#ifndef PHP_FSEVENTS_H
#define PHP_FSEVENTS_H 1

#ifdef ZTS
#include "TSRM.h"
#endif

#include <stdio.h>
#include <CoreServices/CoreServices.h>

typedef struct Watch {
    zend_fcall_info       fci;
    zend_fcall_info_cache fcc;
    char                  *path;
} Watch;

ZEND_BEGIN_MODULE_GLOBALS(fsevents)
    int   counter;
    Watch watches[100];
ZEND_END_MODULE_GLOBALS(fsevents)

#ifdef ZTS
#define FSEVENTS_GLOBAL(v) TSRMG(fsevents_globals_id, zend_fsevents_globals *, v)
#else
#define FSEVENTS_GLOBAL(v) (fsevents_globals.v)
#endif

#define PHP_FSEVENTS_VERSION "0.1.0"
#define PHP_FSEVENTS_EXTNAME "fsevents"

PHP_MINIT_FUNCTION(fsevents);
PHP_MSHUTDOWN_FUNCTION(fsevents);
PHP_RINIT_FUNCTION(fsevents);
PHP_MINFO_FUNCTION(fsevents);

PHP_FUNCTION(fsevents_add_watch);
PHP_FUNCTION(fsevents_start);

extern zend_module_entry fsevents_module_entry;
#define phpext_fsevents_ptr &fsevents_module_entry

#endif	/* PHP_FSEVENTS_H */

