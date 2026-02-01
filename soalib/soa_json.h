#ifndef soa_json_h
#define soa_json_h

#ifdef __cplusplus
extern "C" { 
#endif

#include "soa.h"

#ifndef SOA_JSON_PREALLOC
#define SOA_JSON_PREALLOC 8
#endif

#ifndef SOA_JSON_PRETTIFY_TAB
#define SOA_JSON_PRETTIFY_TAB "    "
#endif

soa_doc_t soa_doc_new_from_json(const char* json);

typedef enum {
    SOA_JSON_NONE = 0,
    SOA_JSON_PRETTIFY = 1,
    SOA_JSON_ENCODE_UTF = 2
} soa_json_flag_bit_t;

typedef uint32_t soa_json_parse_flags_t;

// User is responsible for freeing memory
char* soa_json_new_from_doc(soa_doc_t* doc, soa_json_parse_flags_t flags);

#ifdef __cplusplus
} 
#endif

#endif