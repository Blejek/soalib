#ifndef soa_h
#define soa_h

#ifdef __cplusplus
extern "C" { 
#endif

#ifndef SOA_DOC_GROW_FACTOR
#define SOA_DOC_GROW_FACTOR 2
#endif

#include <stdint.h>
#include <stddef.h>

typedef struct {
    char* msg;
    int code;    
} soa_error_t;

soa_error_t soa_error_get();
void soa_error_push(char* msg, int code);
void soa_error_pop();

typedef enum {
    SOA_TYPE_BOOL = 0, // 0 = false, 1 = true, 2 = null
    SOA_TYPE_INT,
    SOA_TYPE_UINT,
    SOA_TYPE_FLOAT,
    SOA_TYPE_STR,
    SOA_TYPE_SSO,
    SOA_TYPE_OBJ,
    SOA_TYPE_ARR,
} soa_type_bit_t;
typedef uint8_t soa_type_t; 

typedef enum {
    SOA_ROOT_NULL = 0,
    SOA_ROOT_OBJ = SOA_TYPE_OBJ,
    SOA_ROOT_ARR = SOA_TYPE_ARR
} soa_root_bit_t;
typedef uint8_t soa_root_t;

typedef struct {
    uint8_t* data;
    size_t size;
    size_t cap;
    size_t root;
    soa_root_t root_type; 
} soa_doc_t;

typedef enum {
    SOA_BOOL_FALSE = 0,
    SOA_BOOL_TRUE = 1,
    SOA_BOOL_NULL = 2
} soa_bool_bit_t;
typedef uint8_t soa_bool_t;

typedef union {
    soa_bool_t b;
    int64_t i;
    uint64_t u;
    double f;
    size_t s;
    char sso[8];
    size_t o;
    size_t a;
} soa_valu_t; 

typedef struct {
    soa_valu_t value;
    uint8_t type;
    uint8_t sso;
    union {
        size_t str; 
        char sso[8];
    } key;
} soa_obj_entry_t;

typedef struct {
    soa_valu_t value;
    uint8_t type;
} soa_arr_entry_t;

typedef struct {
    soa_doc_t* doc;
    size_t data;
} soa_val_t;

typedef struct {
    soa_doc_t* doc;
    size_t data;
} soa_obj_t;

typedef struct {
    soa_doc_t* doc;
    size_t data;
} soa_arr_t;

soa_doc_t soa_doc_new();
void soa_doc_free(soa_doc_t* doc);

soa_obj_t soa_doc_root_obj(soa_doc_t* doc);
soa_arr_t soa_doc_root_arr(soa_doc_t* doc);

uint8_t* _soa_doc_grow(soa_doc_t* doc, size_t size);
soa_obj_t soa_doc_add_obj(soa_doc_t* doc, size_t element_count);
soa_arr_t soa_doc_add_arr(soa_doc_t* doc, size_t element_count);
size_t soa_doc_add_str(soa_doc_t* doc, const char* str);

char*     soa_obj_key_at(soa_obj_t* obj, size_t index);
void      soa_obj_set_key_at(soa_obj_t* obj, size_t index, const char* key);
size_t    soa_obj_length(soa_obj_t* obj);
soa_val_t soa_obj_val_at_index(soa_obj_t* obj, size_t index);
soa_val_t soa_obj_val_at_key(soa_obj_t* obj, const char* key);

size_t    soa_arr_length(soa_arr_t* arr);
soa_val_t soa_arr_val_at(soa_arr_t* arr, size_t index);

soa_type_t soa_val_type (const soa_val_t* val);
soa_bool_t soa_val_bool (const soa_val_t* val);
int64_t    soa_val_int  (const soa_val_t* val);
uint64_t   soa_val_uint (const soa_val_t* val);
double     soa_val_float(const soa_val_t* val);
char*      soa_val_str  (const soa_val_t* val);
soa_obj_t  soa_val_obj  (const soa_val_t* val);
soa_arr_t  soa_val_arr  (const soa_val_t* val);

void soa_val_set_type (const soa_val_t* val, const soa_type_t type);
void soa_val_set_bool (const soa_val_t* val, const soa_bool_t value);
void soa_val_set_int  (const soa_val_t* val, const int64_t    value);
void soa_val_set_uint (const soa_val_t* val, const uint64_t   value);
void soa_val_set_float(const soa_val_t* val, const double     value);
void soa_val_set_str  (const soa_val_t* val, const char*      value);
void soa_val_set_obj  (const soa_val_t* val, const soa_obj_t* value);
void soa_val_set_arr  (const soa_val_t* val, const soa_arr_t* value);


#ifdef __cplusplus
} 
#endif

#endif