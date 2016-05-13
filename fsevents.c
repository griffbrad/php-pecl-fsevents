#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_fsevents.h"
#include <CoreServices/CoreServices.h>

ZEND_BEGIN_ARG_INFO_EX(arginfo_fsevents_add_watch, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_INFO(0, path)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_fsevents_start, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

#if ZEND_MODULE_API_NO >= 20071006
const
#endif

zend_function_entry fsevents_functions[] = {
	PHP_FE(fsevents_add_watch, arginfo_fsevents_add_watch)
	PHP_FE(fsevents_start, arginfo_fsevents_start)
	{NULL, NULL, NULL}	/* Must be the last line in fsevents_functions[] */
};

zend_module_entry fsevents_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"fsevents",
	fsevents_functions,
	PHP_MINIT(fsevents),     // Start of module
	PHP_MSHUTDOWN(fsevents), // End of module
	PHP_RINIT(fsevents),     // Start of request
	NULL,                    // End of request
	PHP_MINFO(fsevents),     // phpinfo additions
#if ZEND_MODULE_API_NO >= 20010901
	PHP_FSEVENTS_VERSION,
#endif
	STANDARD_MODULE_PROPERTIES
};

static void php_fsevents_init_globals(zend_fsevents_globals *fsevents_globals TSRMLS_DC)
{
}

#ifdef COMPILE_DL_FSEVENTS
ZEND_GET_MODULE(fsevents)
#endif

ZEND_DECLARE_MODULE_GLOBALS(fsevents)

PHP_MINIT_FUNCTION(fsevents)
{
	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(fsevents)
{
	return SUCCESS;
}

PHP_RINIT_FUNCTION(fsevents)
{
    ZEND_INIT_MODULE_GLOBALS(fsevents, php_fsevents_init_globals, NULL);
    return SUCCESS;
}

PHP_MINFO_FUNCTION(fsevents)
{
	php_info_print_table_start();
	php_info_print_table_row(2, "Version", PHP_FSEVENTS_VERSION);
	php_info_print_table_end();
}

void call_user_watch_callback(int watch_index)
{
    Watch *watch_ret;

    watch_ret = &FSEVENTS_GLOBAL(watches)[watch_index];

    zval *retval = NULL;
    zval **params[1];
    zval *event_array;

    MAKE_STD_ZVAL(retval);
    MAKE_STD_ZVAL(event_array);
    array_init(event_array);

    params[0] = &event_array;

    watch_ret->fci.params         = params;
    watch_ret->fci.param_count    = 1;
    watch_ret->fci.retval_ptr_ptr = &retval;
    watch_ret->fci.no_separation  = 0;

    zend_call_function(&watch_ret->fci, NULL TSRMLS_CC);
}

bool starts_with(const char *str, const char *pre)
{
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? false : strncmp(pre, str, lenpre) == 0;
}

void handle_events(
    ConstFSEventStreamRef         stream_ref,
    void                          *client_callback_info,
    size_t                        num_events,
    void                          *event_paths,
    const FSEventStreamEventFlags event_flags[],
    const FSEventStreamEventId    event_ids[]
)
{
    int   i;
    int   n;
    char  **paths = event_paths;
    char  *path   = NULL;
    Watch watch;

    for (i = 0; i < num_events; i++) {
        path = paths[i];

        for (n = 0; n < 100; n += 1) {
            watch = FSEVENTS_GLOBAL(watches)[n];

            if (NULL != watch.path && starts_with(path, watch.path)) {
                call_user_watch_callback(n);
            }
        }
    }
}

void start_watches()
{
    CFStringRef watch_paths[100];
    int         path_count = 0;
    int         i;

    for (i = 0; i < 100; i += 1) {
        if (NULL != FSEVENTS_GLOBAL(watches)[i].path) {
            watch_paths[i] = CFStringCreateWithCString(NULL, FSEVENTS_GLOBAL(watches)[i].path, kCFStringEncodingUTF8);
            path_count++;
        }
    }

    void *callback_info = NULL; // could put stream-specific data here.

    CFArrayRef       watch_path_array = CFArrayCreate(NULL, (void *) watch_paths, path_count, NULL);
    CFAbsoluteTime   latency          = .75; /* Latency in seconds */
    CFRunLoopRef     run_loop         = CFRunLoopGetMain();
    FSEventStreamRef stream;

    /* Create the stream, passing in a callback */
    stream = FSEventStreamCreate(
        NULL,
        (FSEventStreamCallback)&handle_events,
        callback_info,
        watch_path_array,
        kFSEventStreamEventIdSinceNow,
        latency,
        kFSEventStreamCreateFlagNone
    );

    FSEventStreamScheduleWithRunLoop(
        stream,
        run_loop,
        kCFRunLoopDefaultMode
    );

    FSEventStreamStart(stream);
    CFRunLoopRun();
    FSEventStreamFlushSync(stream);
    FSEventStreamStop(stream);
}

void store_watch(char *realpath, zend_fcall_info fci, zend_fcall_info_cache fci_cache)
{
    Watch watch;

    watch.fci  = fci;
    watch.fcc  = fci_cache;
    watch.path = realpath;

    Z_ADDREF_P(watch.fci.function_name);

    FSEVENTS_GLOBAL(watches)[FSEVENTS_GLOBAL(counter)] = watch;
    FSEVENTS_GLOBAL(counter)++;
}

PHP_FUNCTION(fsevents_add_watch)
{
    char                  *realpath;
    char                  *path;
    int                   path_len;
    zend_fcall_info       fci;
    zend_fcall_info_cache fci_cache;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sf", &path, &path_len, &fci, &fci_cache) == FAILURE) {
        RETURN_FALSE;
    }

    realpath = expand_filepath(path, NULL TSRMLS_CC);

    if (!realpath) {
        RETURN_FALSE;
    }

    store_watch(realpath, fci, fci_cache);

    RETURN_TRUE;
}

PHP_FUNCTION(fsevents_start)
{
    start_watches();

    RETURN_TRUE;
}
