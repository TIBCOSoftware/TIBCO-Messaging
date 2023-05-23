/*
  Copyright (c) 2009-2020 Dave Gamble and cJSON contributors

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#ifndef cJSON__h
#define cJSON__h

#ifdef __cplusplus
extern "C"
{
#endif

/* project version */
#define CJSON_VERSION_MAJOR 1
#define CJSON_VERSION_MINOR 5
#define CJSON_VERSION_PATCH 0

#include <stddef.h>

/* cJSON Types: */
#define _cJSON_Invalid (0)
#define _cJSON_False  (1 << 0)
#define _cJSON_True   (1 << 1)
#define _cJSON_NULL   (1 << 2)
#define _cJSON_Number (1 << 3)
#define _cJSON_String (1 << 4)
#define _cJSON_Array  (1 << 5)
#define _cJSON_Object (1 << 6)
#define _cJSON_Raw    (1 << 7) /* raw json */

#define _cJSON_IsReference 256
#define _cJSON_StringIsConst 512

/* The cJSON structure: */
typedef struct _cJSON
{
    /* next/prev allow you to walk array/object chains. Alternatively, use GetArraySize/GetArrayItem/GetObjectItem */
    struct _cJSON *next;
    struct _cJSON *prev;
    /* An array or object item will have a child pointer pointing to a chain of the items in the array/object. */
    struct _cJSON *child;

    /* The type of the item, as above. */
    unsigned int type;

    /* The item's string, if type==_cJSON_String  and type == _cJSON_Raw */
    char *valuestring;
    /* writing to valueint is DEPRECATED, use _cJSON_SetNumberValue instead */
    int valueint;
    /* The item's number, if type==_cJSON_Number */
    double valuedouble;

    /* The item's name string, if this item is the child of, or is in the list of subitems of an object. */
    char *string;
} _cJSON;

typedef struct _cJSON_Hooks
{
      void *(*malloc_fn)(size_t sz);
      void (*free_fn)(void *ptr);
} _cJSON_Hooks;

typedef int _cJSON_bool;

#if !defined(__WINDOWS__) && (defined(WIN32) || defined(WIN64) || defined(_MSC_VER) || defined(_WIN32))
#define __WINDOWS__
#endif
#ifdef __WINDOWS__

/* When compiling for windows, we specify a specific calling convention to avoid issues where we are being called from a project with a different default calling convention.  For windows you have 2 define options:

CJSON_HIDE_SYMBOLS - Define this in the case where you don't want to ever dllexport symbols
CJSON_EXPORT_SYMBOLS - Define this on library build when you want to dllexport symbols (default)
CJSON_IMPORT_SYMBOLS - Define this if you want to dllimport symbol

For *nix builds that support visibility attribute, you can define similar behavior by

setting default visibility to hidden by adding
-fvisibility=hidden (for gcc)
or
-xldscope=hidden (for sun cc)
to CFLAGS

then using the CJSON_API_VISIBILITY flag to "export" the same symbols the way CJSON_EXPORT_SYMBOLS does

*/

/* export symbols by default, this is necessary for copy pasting the C and header file */
#if !defined(CJSON_HIDE_SYMBOLS) && !defined(CJSON_IMPORT_SYMBOLS) && !defined(CJSON_EXPORT_SYMBOLS)
#define CJSON_EXPORT_SYMBOLS
#endif

#if defined(CJSON_HIDE_SYMBOLS)
#define CJSON_PUBLIC(type)   type __stdcall
#elif defined(CJSON_EXPORT_SYMBOLS)
#define CJSON_PUBLIC(type)   __declspec(dllexport) type __stdcall
#elif defined(CJSON_IMPORT_SYMBOLS)
#define CJSON_PUBLIC(type)   __declspec(dllimport) type __stdcall
#endif
#else /* !WIN32 */
#if (defined(__GNUC__) || defined(__SUNPRO_CC) || defined (__SUNPRO_C)) && defined(CJSON_API_VISIBILITY)
#define CJSON_PUBLIC(type)   __attribute__((visibility("default"))) type
#else
#define CJSON_PUBLIC(type) type
#endif
#endif

/* Limits how deeply nested arrays/objects can be before cJSON rejects to parse them.
 * This is to prevent stack overflows. */
#ifndef CJSON_NESTING_LIMIT
#define CJSON_NESTING_LIMIT 1000
#endif

/* returns the version of cJSON as a string */
CJSON_PUBLIC(const char*) _cJSON_Version(void);

/* Supply malloc, realloc and free functions to cJSON */
CJSON_PUBLIC(void) _cJSON_InitHooks(_cJSON_Hooks* hooks);

/* Memory Management: the caller is always responsible to free the results from all variants of _cJSON_Parse (with _cJSON_Delete) and _cJSON_Print (with stdlib free, _cJSON_Hooks.free_fn, or _cJSON_free as appropriate). The exception is _cJSON_PrintPreallocated, where the caller has full responsibility of the buffer. */
/* Supply a block of JSON, and this returns a cJSON object you can interrogate. */
CJSON_PUBLIC(_cJSON *) _cJSON_Parse(const char *value);
/* Render a cJSON entity to text for transfer/storage. */
CJSON_PUBLIC(char *) _cJSON_Print(const _cJSON *item);
/* Render a cJSON entity to text for transfer/storage without any formatting. */
CJSON_PUBLIC(char *) _cJSON_PrintUnformatted(const _cJSON *item);
/* Render a cJSON entity to text using a buffered strategy. prebuffer is a guess at the final size. guessing well reduces reallocation. fmt=0 gives unformatted, =1 gives formatted */
CJSON_PUBLIC(char *) _cJSON_PrintBuffered(const _cJSON *item, int prebuffer, _cJSON_bool fmt);
/* Render a cJSON entity to text using a buffer already allocated in memory with given length. Returns 1 on success and 0 on failure. */
/* NOTE: cJSON is not always 100% accurate in estimating how much memory it will use, so to be safe allocate 5 bytes more than you actually need */
CJSON_PUBLIC(_cJSON_bool) _cJSON_PrintPreallocated(_cJSON *item, char *buffer, const int length, const _cJSON_bool format);
/* Delete a cJSON entity and all subentities. */
CJSON_PUBLIC(void) _cJSON_Delete(_cJSON *c);

/* Returns the number of items in an array (or object). */
CJSON_PUBLIC(int) _cJSON_GetArraySize(const _cJSON *array);
/* Retrieve item number "item" from array "array". Returns NULL if unsuccessful. */
CJSON_PUBLIC(_cJSON *) _cJSON_GetArrayItem(const _cJSON *array, int index);
/* Get item "string" from object. Case insensitive. */
CJSON_PUBLIC(_cJSON *) _cJSON_GetObjectItem(const _cJSON * const object, const char * const string);
CJSON_PUBLIC(_cJSON *) _cJSON_GetObjectItemCaseSensitive(const _cJSON * const object, const char * const string);
CJSON_PUBLIC(_cJSON_bool) _cJSON_HasObjectItem(const _cJSON *object, const char *string);
/* For analysing failed parses. This returns a pointer to the parse error. You'll probably need to look a few chars back to make sense of it. Defined when _cJSON_Parse() returns 0. 0 when _cJSON_Parse() succeeds. */
CJSON_PUBLIC(const char *) _cJSON_GetErrorPtr(void);

/* These functions check the type of an item */
CJSON_PUBLIC(_cJSON_bool) _cJSON_IsInvalid(const _cJSON * const item);
CJSON_PUBLIC(_cJSON_bool) _cJSON_IsFalse(const _cJSON * const item);
CJSON_PUBLIC(_cJSON_bool) _cJSON_IsTrue(const _cJSON * const item);
CJSON_PUBLIC(_cJSON_bool) _cJSON_IsBool(const _cJSON * const item);
CJSON_PUBLIC(_cJSON_bool) _cJSON_IsNull(const _cJSON * const item);
CJSON_PUBLIC(_cJSON_bool) _cJSON_IsNumber(const _cJSON * const item);
CJSON_PUBLIC(_cJSON_bool) _cJSON_IsString(const _cJSON * const item);
CJSON_PUBLIC(_cJSON_bool) _cJSON_IsArray(const _cJSON * const item);
CJSON_PUBLIC(_cJSON_bool) _cJSON_IsObject(const _cJSON * const item);
CJSON_PUBLIC(_cJSON_bool) _cJSON_IsRaw(const _cJSON * const item);

/* These calls create a cJSON item of the appropriate type. */
CJSON_PUBLIC(_cJSON *) _cJSON_CreateNull(void);
CJSON_PUBLIC(_cJSON *) _cJSON_CreateTrue(void);
CJSON_PUBLIC(_cJSON *) _cJSON_CreateFalse(void);
CJSON_PUBLIC(_cJSON *) _cJSON_CreateBool(_cJSON_bool boolean);
CJSON_PUBLIC(_cJSON *) _cJSON_CreateNumber(double num);
CJSON_PUBLIC(_cJSON *) _cJSON_CreateString(const char *string);
/* raw json */
CJSON_PUBLIC(_cJSON *) _cJSON_CreateRaw(const char *raw);
CJSON_PUBLIC(_cJSON *) _cJSON_CreateArray(void);
CJSON_PUBLIC(_cJSON *) _cJSON_CreateObject(void);

/* These utilities create an Array of count items. */
CJSON_PUBLIC(_cJSON *) _cJSON_CreateIntArray(const int *numbers, int count);
CJSON_PUBLIC(_cJSON *) _cJSON_CreateFloatArray(const float *numbers, int count);
CJSON_PUBLIC(_cJSON *) _cJSON_CreateDoubleArray(const double *numbers, int count);
CJSON_PUBLIC(_cJSON *) _cJSON_CreateStringArray(const char **strings, int count);

/* Append item to the specified array/object. */
CJSON_PUBLIC(void) _cJSON_AddItemToArray(_cJSON *array, _cJSON *item);
CJSON_PUBLIC(void) _cJSON_AddItemToObject(_cJSON *object, const char *string, _cJSON *item);
/* Use this when string is definitely const (i.e. a literal, or as good as), and will definitely survive the cJSON object.
 * WARNING: When this function was used, make sure to always check that (item->type & _cJSON_StringIsConst) is zero before
 * writing to `item->string` */
CJSON_PUBLIC(void) _cJSON_AddItemToObjectCS(_cJSON *object, const char *string, _cJSON *item);
/* Append reference to item to the specified array/object. Use this when you want to add an existing cJSON to a new cJSON, but don't want to corrupt your existing cJSON. */
CJSON_PUBLIC(void) _cJSON_AddItemReferenceToArray(_cJSON *array, _cJSON *item);
CJSON_PUBLIC(void) _cJSON_AddItemReferenceToObject(_cJSON *object, const char *string, _cJSON *item);

/* Remove/Detatch items from Arrays/Objects. */
CJSON_PUBLIC(_cJSON *) _cJSON_DetachItemViaPointer(_cJSON *parent, _cJSON * const item);
CJSON_PUBLIC(_cJSON *) _cJSON_DetachItemFromArray(_cJSON *array, int which);
CJSON_PUBLIC(void) _cJSON_DeleteItemFromArray(_cJSON *array, int which);
CJSON_PUBLIC(_cJSON *) _cJSON_DetachItemFromObject(_cJSON *object, const char *string);
CJSON_PUBLIC(_cJSON *) _cJSON_DetachItemFromObjectCaseSensitive(_cJSON *object, const char *string);
CJSON_PUBLIC(void) _cJSON_DeleteItemFromObject(_cJSON *object, const char *string);
CJSON_PUBLIC(void) _cJSON_DeleteItemFromObjectCaseSensitive(_cJSON *object, const char *string);

/* Update array items. */
CJSON_PUBLIC(void) _cJSON_InsertItemInArray(_cJSON *array, int which, _cJSON *newitem); /* Shifts pre-existing items to the right. */
CJSON_PUBLIC(_cJSON_bool) _cJSON_ReplaceItemViaPointer(_cJSON * const parent, _cJSON * const item, _cJSON * replacement);
CJSON_PUBLIC(void) _cJSON_ReplaceItemInArray(_cJSON *array, int which, _cJSON *newitem);
CJSON_PUBLIC(void) _cJSON_ReplaceItemInObject(_cJSON *object,const char *string,_cJSON *newitem);
CJSON_PUBLIC(void) _cJSON_ReplaceItemInObjectCaseSensitive(_cJSON *object,const char *string,_cJSON *newitem);

/* Duplicate a cJSON item */
CJSON_PUBLIC(_cJSON *) _cJSON_Duplicate(const _cJSON *item, _cJSON_bool recurse);
/* Duplicate will create a new, identical cJSON item to the one you pass, in new memory that will
need to be released. With recurse!=0, it will duplicate any children connected to the item.
The item->next and ->prev pointers are always zero on return from Duplicate. */
/* Recursively compare two cJSON items for equality. If either a or b is NULL or invalid, they will be considered unequal.
 * case_sensitive determines if object keys are treated case sensitive (1) or case insensitive (0) */
CJSON_PUBLIC(_cJSON_bool) _cJSON_Compare(const _cJSON * const a, const _cJSON * const b, const _cJSON_bool case_sensitive);


/* ParseWithOpts allows you to require (and check) that the JSON is null terminated, and to retrieve the pointer to the final byte parsed. */
/* If you supply a ptr in return_parse_end and parsing fails, then return_parse_end will contain a pointer to the error. If not, then _cJSON_GetErrorPtr() does the job. */
CJSON_PUBLIC(_cJSON *) _cJSON_ParseWithOpts(const char *value, const char **return_parse_end, _cJSON_bool require_null_terminated);

CJSON_PUBLIC(void) _cJSON_Minify(char *json);

/* Macros for creating things quickly. */
#define _cJSON_AddNullToObject(object,name) _cJSON_AddItemToObject(object, name, _cJSON_CreateNull())
#define _cJSON_AddTrueToObject(object,name) _cJSON_AddItemToObject(object, name, _cJSON_CreateTrue())
#define _cJSON_AddFalseToObject(object,name) _cJSON_AddItemToObject(object, name, _cJSON_CreateFalse())
#define _cJSON_AddBoolToObject(object,name,b) _cJSON_AddItemToObject(object, name, _cJSON_CreateBool(b))
#define _cJSON_AddNumberToObject(object,name,n) _cJSON_AddItemToObject(object, name, _cJSON_CreateNumber(n))
#define _cJSON_AddStringToObject(object,name,s) _cJSON_AddItemToObject(object, name, _cJSON_CreateString(s))
#define _cJSON_AddRawToObject(object,name,s) _cJSON_AddItemToObject(object, name, _cJSON_CreateRaw(s))

/* When assigning an integer value, it needs to be propagated to valuedouble too. */
#define _cJSON_SetIntValue(object, number) ((object) ? (object)->valueint = (object)->valuedouble = (number) : (number))
/* helper for the _cJSON_SetNumberValue macro */
CJSON_PUBLIC(double) _cJSON_SetNumberHelper(_cJSON *object, double number);
#define _cJSON_SetNumberValue(object, number) ((object != NULL) ? _cJSON_SetNumberHelper(object, (double)number) : (number))

/* Macro for iterating over an array */
#define _cJSON_ArrayForEach(element, array) for(element = (array != NULL) ? (array)->child : NULL; element != NULL; element = element->next)

/* malloc/free objects using the malloc/free functions that have been set with _cJSON_InitHooks */
CJSON_PUBLIC(void *) _cJSON_malloc(size_t size);
CJSON_PUBLIC(void) _cJSON_free(void *object);

#ifdef __cplusplus
}
#endif

#endif
