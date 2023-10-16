/*
 *  Copyright (c) 2009 Facebook
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */

#ifndef PHP_XHPROF_H
#define PHP_XHPROF_H

extern zend_module_entry xhprof_module_entry;
#define phpext_xhprof_ptr &xhprof_module_entry

#ifdef PHP_WIN32
#define PHP_XHPROF_API __declspec(dllexport)
#else
#define PHP_XHPROF_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#if PHP_VERSION_ID >= 80000
#include "zend_observer.h"
#endif


/**
 * **********************
 * GLOBAL MACRO CONSTANTS
 * **********************
 */

/* XHProf version                           */
#define XHPROF_VERSION       "2.3.9"

#define XHPROF_FUNC_HASH_COUNTERS_SIZE   1024

/* Fictitious function name to represent top of the call tree. The paranthesis
 * in the name is to ensure we don't conflict with user function names.  */
#define ROOT_SYMBOL                "main()"

/* Size of a temp scratch buffer            */
#define SCRATCH_BUF_LEN            512

/* Various XHPROF modes. If you are adding a new mode, register the appropriate
 * callbacks in hp_begin() */
#define XHPROF_MODE_HIERARCHICAL            1
#define XHPROF_MODE_SAMPLED            620002      /* Rockfort's zip code */

/* Hierarchical profiling flags.
 *
 * Note: Function call counts and wall (elapsed) time are always profiled.
 * The following optional flags can be used to control other aspects of
 * profiling.
 */
#define XHPROF_FLAGS_NO_BUILTINS   0x0001         /* do not profile builtins */
#define XHPROF_FLAGS_CPU           0x0002      /* gather CPU times for funcs */
#define XHPROF_FLAGS_MEMORY        0x0004   /* gather memory usage for funcs */

/* Constants for XHPROF_MODE_SAMPLED        */
#define XHPROF_DEFAULT_SAMPLING_INTERVAL       100000      /* In microsecs        */
#define XHPROF_MINIMAL_SAMPLING_INTERVAL          100      /* In microsecs        */

/* Constant for ignoring functions, transparent to hierarchical profile */
#define XHPROF_MAX_IGNORED_FUNCTIONS  256

#if !defined(uint32)
    typedef unsigned int uint32;
#endif

#if !defined(uint8)
    typedef unsigned char uint8;
#endif

/* Bloom filter for function names to be ignored */
#define INDEX_2_BYTE(index)  (index >> 3)
#define INDEX_2_BIT(index)   (1 << (index & 0x7));

#define register_trace_callback(function_name, cb) zend_hash_str_update_mem(XHPROF_G(trace_callbacks), function_name, sizeof(function_name) - 1, &cb, sizeof(hp_trace_callback));

/* XHProf maintains a stack of entries being profiled. The memory for the entry
 * is passed by the layer that invokes BEGIN_PROFILING(), e.g. the hp_execute()
 * function. Often, this is just C-stack memory.
 *
 * This structure is a convenient place to track start time of a particular
 * profile operation, recursion depth, and the name of the function being
 * profiled. */
typedef struct hp_entry_t {
    struct hp_entry_t      *prev_hprof;    /* ptr to prev entry being profiled */
    zend_string            *name_hprof;                       /* function name */
    int                     rlvl_hprof;        /* recursion level for function */
    long int                mu_start_hprof;                    /* memory usage */
    long int                pmu_start_hprof;              /* peak memory usage */
    zend_ulong              tsc_start;         /* start value for TSC counter  */
    zend_ulong              cpu_start;
    zend_ulong              hash_code;     /* hash_code for the function name  */
#if PHP_VERSION_ID >= 80000
    int                     is_trace;
#endif
} hp_entry_t;

typedef struct hp_ignored_functions {
    zend_string **names;
    zend_ulong filter[XHPROF_MAX_IGNORED_FUNCTIONS];
} hp_ignored_functions;

typedef zend_string* (*hp_trace_callback) (zend_string *symbol, zend_execute_data *data);

/* Various types for XHPROF callbacks       */
typedef void (*hp_init_cb)           ();
typedef void (*hp_exit_cb)           ();
typedef void (*hp_begin_function_cb) (hp_entry_t **entries, hp_entry_t *current);
typedef void (*hp_end_function_cb)   (hp_entry_t **entries);

/**
 * ***********************
 * GLOBAL STATIC VARIABLES
 * ***********************
 */

/* Pointer to the origianl execute_internal function */
static void (*_zend_execute_internal) (zend_execute_data *data, zval *return_value);
ZEND_DLEXPORT void hp_execute_internal(zend_execute_data *execute_data, zval *return_value);

/* Pointer to the original compile function */
static zend_op_array * (*_zend_compile_file) (zend_file_handle *file_handle, int type);
ZEND_DLEXPORT zend_op_array* hp_compile_file(zend_file_handle *file_handle, int type);

#if PHP_VERSION_ID < 80000
/* Pointer to the original compile string function (used by eval) */
static zend_op_array * (*_zend_compile_string) (zval *source_string, char *filename);
ZEND_DLEXPORT zend_op_array* hp_compile_string(zval *source_string, char *filename);

/* Pointer to the original execute function */
static void (*_zend_execute_ex) (zend_execute_data *execute_data);
ZEND_DLEXPORT void hp_execute_ex (zend_execute_data *execute_data);
#elif PHP_VERSION_ID >= 80200
/* Pointer to the original compile string function (used by eval) */
static zend_op_array * (*_zend_compile_string) (zend_string *source_string, const char *filename, zend_compile_position position);
ZEND_DLEXPORT zend_op_array* hp_compile_string(zend_string *source_string, const char *filename, zend_compile_position position);

static zend_observer_fcall_handlers tracer_observer(zend_execute_data *execute_data);
static void tracer_observer_begin(zend_execute_data *ex);
static void tracer_observer_end(zend_execute_data *ex, zval *return_value);
#else
/* Pointer to the original compile string function (used by eval) */
static zend_op_array * (*_zend_compile_string) (zend_string *source_string, const char *filename);
ZEND_DLEXPORT zend_op_array* hp_compile_string(zend_string *source_string, const char *filename);

static zend_observer_fcall_handlers tracer_observer(zend_execute_data *execute_data);
static void tracer_observer_begin(zend_execute_data *ex);
static void tracer_observer_end(zend_execute_data *ex, zval *return_value);
#endif

/**
 * ****************************
 * STATIC FUNCTION DECLARATIONS
 * ****************************
 */
static void hp_register_constants(INIT_FUNC_ARGS);

static void hp_begin(zend_long level, zend_long xhprof_flags);
static void hp_stop();
static void hp_end();

static inline zend_ulong cycle_timer();

static void hp_free_the_free_list();
static void hp_fast_free_hprof_entry(hp_entry_t *p);

static void incr_us_interval(struct timeval *start, zend_ulong incr);

static void hp_get_ignored_functions_from_arg(zval *args);

static inline void hp_array_del(zend_string **names);

void hp_init_trace_callbacks();

double get_timebase_conversion();

hp_ignored_functions *hp_ignored_functions_init(zval *values);

/* Struct to hold the various callbacks for a single xhprof mode */
typedef struct hp_mode_cb {
    hp_init_cb             init_cb;
    hp_exit_cb             exit_cb;
    hp_begin_function_cb   begin_fn_cb;
    hp_end_function_cb     end_fn_cb;
} hp_mode_cb;

/* Xhprof's global state.
 *
 * This structure is instantiated once.  Initialize defaults for attributes in
 * hp_init_profiler_state() Cleanup/free attributes in
 * hp_clean_profiler_state() */
ZEND_BEGIN_MODULE_GLOBALS(xhprof)

    /*       ----------   Global attributes:  -----------       */

    /* Indicates if xhprof is currently enabled */
    int              enabled;

    /* Indicates if xhprof was ever enabled during this request */
    int              ever_enabled;

    /* Holds all the xhprof statistics */
    zval            stats_count;

    /* Indicates the current xhprof mode or level */
    int              profiler_level;

    /* Top of the profile stack */
    hp_entry_t      *entries;

    /* freelist of hp_entry_t chunks for reuse... */
    hp_entry_t      *entry_free_list;

    /* Callbacks for various xhprof modes */
    hp_mode_cb       mode_cb;

    /*       ----------   Mode specific attributes:  -----------       */

    /* Global to track the time of the last sample in time and ticks */
    struct timeval   last_sample_time;
    zend_ulong       last_sample_tsc;
    /* XHPROF_SAMPLING_INTERVAL in ticks */
    zend_long        sampling_interval;
    zend_ulong       sampling_interval_tsc;
    zend_long        sampling_depth;
    /* XHProf flags */
    uint32 xhprof_flags;

    zend_string *root;

    /* counter table indexed by hash value of function names. */
    zend_ulong func_hash_counters[XHPROF_FUNC_HASH_COUNTERS_SIZE];

    HashTable *trace_callbacks;

    /* Table of ignored function names and their filter */
    hp_ignored_functions *ignored_functions;

    double timebase_conversion;

    zend_bool collect_additional_info;

ZEND_END_MODULE_GLOBALS(xhprof)

PHP_MINIT_FUNCTION(xhprof);
PHP_MSHUTDOWN_FUNCTION(xhprof);
PHP_RINIT_FUNCTION(xhprof);
PHP_RSHUTDOWN_FUNCTION(xhprof);
PHP_MINFO_FUNCTION(xhprof);

PHP_FUNCTION(xhprof_enable);
PHP_FUNCTION(xhprof_disable);
PHP_FUNCTION(xhprof_sample_enable);
PHP_FUNCTION(xhprof_sample_disable);

#ifdef ZTS
#define XHPROF_G(v) TSRMG(xhprof_globals_id, zend_xhprof_globals *, v)
#else
#define XHPROF_G(v) (xhprof_globals.v)
#endif

extern ZEND_DECLARE_MODULE_GLOBALS(xhprof);
#endif /* PHP_XHPROF_H */
