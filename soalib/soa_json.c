/*
MIT License

Copyright (c) 2026 Błażej Dombek <blazejdombek@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "soa_json.h"

#include "soalib/soa.h"

#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#ifdef _WIN64
#define SOA_LD_FORMAT "%lld"
#define SOA_LU_FORMAT "%llu"
#else
#define SOA_LD_FORMAT "%ld"
#define SOA_LU_FORMAT "%lu"
#endif

typedef struct {
    char* str;
    size_t cap;
} _soa_str_t;

_soa_str_t _soa_str_new(size_t size) {
    return (_soa_str_t){
        .cap = size,
        .str = calloc(size, sizeof(char))
    };
}

char* _soa_str_add_size(_soa_str_t* str, size_t size){
    if(strlen(str->str) + size >= str->cap){
        str->cap += size;
        str->cap *= 2;
        str->str = realloc(str->str, str->cap);
    } 
    return str->str + strlen(str->str);
}

void _soa_str_add(_soa_str_t* str, char* new){
    _soa_str_add_size(str, strlen(new));
    strcat(str->str, new);
    // printf("%s\n", str->str);

}

void _soa_str_free(_soa_str_t* str){
    free(str->str);
}

typedef struct {
    size_t ae;
    size_t ao;
    size_t* asizes;
    size_t acap;

    size_t oe;
    size_t oo;
    size_t* osizes;
    size_t ocap;

    size_t str;
    size_t str_size;
    soa_root_t root_type;
} _json_info_t;

typedef struct {
    size_t a;
    size_t a_offset;
    size_t o;
    size_t o_offset;
    size_t s_offset;
    uint8_t* data;
    uint8_t* ptr;
} _json_read_info_t;

static _json_info_t _info_new(size_t prealloc){
    _json_info_t i = {0};

    i.acap = prealloc;
    i.asizes = malloc(prealloc * sizeof(size_t));

    i.ocap = prealloc;
    i.osizes = malloc(prealloc * sizeof(size_t));

    return i;
}

inline static size_t _info_add_obj(_json_info_t* i){
    i->oo++;
    if(i->oo > i->ocap){
        i->ocap = i->oo * 2;
        i->osizes = realloc(i->osizes, i->ocap * sizeof(size_t));
    }
    return i->oo - 1;
}

inline static size_t _info_add_arr(_json_info_t* i){
    i->ao++;
    if(i->ao > i->acap){
        i->acap = i->ao * 2;
        i->asizes = realloc(i->asizes, i->acap * sizeof(size_t));
    }
    return i->ao - 1;
} 

inline static void _info_free(_json_info_t* i){
    if(i->asizes)
        free(i->asizes);
    if(i->osizes)
        free(i->osizes);
}

inline static int _is_digit(char c){
    if(c >= '0' && c <= '9') return 1;
    return 0;
}

inline static int _is_ws_char(char c){
    if(
        c == ' ' ||
        c == '\n' ||
        c == '\r' ||
        c == '\t'
    ) return 1;
    return 0;
}

inline static const char* _skip_ws(const char* ptr){
    while(*ptr && _is_ws_char(*ptr)){
        ptr++;
    }
    return ptr;
}

inline static int _strin(const char* str1, const char* str2){
    while(*str1 && *str2){
        if(*str1 != *str2){
            return 0;
        }
        str1++;
        str2++;
    }
    return 1;
}

static const char* _parse_val(const char* ptr, _json_info_t* i);
static const char* _read_val(const char* ptr, _json_info_t* i, _json_read_info_t* r);

static uint64_t _atoull(const char* str){
    while (_is_ws_char(*str))
        ++str;

    uint64_t prev = 0;    
    uint64_t total = 0;
	char c = *str++;
    
    while(c >= '0' && c <= '9'){
        prev = total;
        total = 10 * total + c - '0';
        if(prev > total){
            return ULONG_MAX;
        }
        c = *str++;
    }

    return total;
}

static int _hexval(char c){
    if ('0' <= c && c <= '9') return c - '0';
    if ('a' <= c && c <= 'f') return c - 'a' + 10;
    if ('A' <= c && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int _read_u4(const char *s){
    int v = 0;
    for (int i = 0; i < 4; i++) {
        int h = _hexval(s[i]);
        if (h < 0) return -1;
        v = (v << 4) | h;
    }
    return v;
}

static void _write_u4(uint16_t v, char* out){
    static const char hex[] = "0123456789ABCDEF";
    out[0] = hex[(v >> 12) & 0xF];
    out[1] = hex[(v >> 8)  & 0xF];
    out[2] = hex[(v >> 4)  & 0xF];
    out[3] = hex[v & 0xF];
}

static size_t _utf8_encode(uint32_t cp, char *out){
    if (cp <= 0x7F) {
        out[0] = cp;
        return 1;
    } else if (cp <= 0x7FF) {
        out[0] = 0xC0 | (cp >> 6);
        out[1] = 0x80 | (cp & 0x3F);
        return 2;
    } else if (cp <= 0xFFFF) {
        out[0] = 0xE0 | (cp >> 12);
        out[1] = 0x80 | ((cp >> 6) & 0x3F);
        out[2] = 0x80 | (cp & 0x3F);
        return 3;
    } else {
        out[0] = 0xF0 | (cp >> 18);
        out[1] = 0x80 | ((cp >> 12) & 0x3F);
        out[2] = 0x80 | ((cp >> 6) & 0x3F);
        out[3] = 0x80 | (cp & 0x3F);
        return 4;
    }
}

static size_t _utf8_decode(const uint8_t* s, uint32_t* out){
    uint32_t cp;

    if (s[0] < 0x80) {
        *out = s[0];
        return 1;
    }

    if ((s[0] & 0xE0) == 0xC0 &&
        (s[1] & 0xC0) == 0x80) {

        cp = ((s[0] & 0x1F) << 6) |
              (s[1] & 0x3F);

        if (cp >= 0x80) {
            *out = cp;
            return 2;
        }
    }
    else if ((s[0] & 0xF0) == 0xE0 &&
             (s[1] & 0xC0) == 0x80 &&
             (s[2] & 0xC0) == 0x80) {

        cp = ((s[0] & 0x0F) << 12) |
             ((s[1] & 0x3F) << 6)  |
              (s[2] & 0x3F);

        if (cp >= 0x800 &&
            !(cp >= 0xD800 && cp <= 0xDFFF)) {
            *out = cp;
            return 3;
        }
    }
    else if ((s[0] & 0xF8) == 0xF0 &&
             (s[1] & 0xC0) == 0x80 &&
             (s[2] & 0xC0) == 0x80 &&
             (s[3] & 0xC0) == 0x80) {

        cp = ((s[0] & 0x07) << 18) |
             ((s[1] & 0x3F) << 12) |
             ((s[2] & 0x3F) << 6)  |
              (s[3] & 0x3F);

        if (cp >= 0x10000 && cp <= 0x10FFFF) {
            *out = cp;
            return 4;
        }
    }

    *out = 0xFFFD;
    return 1;
}

static const char* _parse_num(const char* ptr, uint8_t* data){
    uint8_t num_data = 0;
    if(*ptr == '-'){
        ptr++;
        num_data |= 16;
    }
    else if(*ptr == '+'){
        ptr++;
    }
    while(_is_digit(*ptr)){
        ptr++;
        num_data |= 1;
    }
    if(*ptr == '.'){
        ptr++;
        while(_is_digit(*ptr)){
            ptr++;
            num_data |= 2;
        }
    }
    
    if(*ptr == 'e' || *ptr == 'E'){
        ptr++;
        if(*ptr == '-' || *ptr == '+'){
            ptr++;
        }
        while(_is_digit(*ptr)){
            ptr++;
            num_data |= 4;
        }
    }
    if(data){
        *data = num_data;
    }
    if(num_data){
        return ptr;
    }
    return NULL;
}

static const char* _parse_num_or_bool(const char* ptr, _json_info_t* i){
    if(_strin(ptr, "null")){
        return ptr + 4;
    }
    else if(_strin(ptr, "false")){
        return ptr + 5;
    }
    else if(_strin(ptr, "true")){
        return ptr + 4;

    }
    else if((ptr = _parse_num(ptr, NULL))){
        return ptr;
    }
    else{
        return NULL;
    }
    return ++ptr;
}

static const char* _read_num_or_bool(const char* ptr, _json_info_t* i, _json_read_info_t* r, uint8_t* t){
    if(_strin(ptr, "null")){
        *t = SOA_TYPE_BOOL;
        *(soa_bool_t*)r->ptr = SOA_BOOL_NULL;
        return ptr + 4;
    }
    else if(_strin(ptr, "false")){
        *t = SOA_TYPE_BOOL;
        *(soa_bool_t*)r->ptr = SOA_BOOL_FALSE;
        return ptr + 5;
    }
    else if(_strin(ptr, "true")){
        *t = SOA_TYPE_BOOL;
        *(soa_bool_t*)r->ptr = SOA_BOOL_TRUE;
        return ptr + 4;
    }
    else{
        uint8_t num_data;
        const char* start = ptr;
        if((ptr = _parse_num(ptr, &num_data))){
            if(num_data & 16 && (num_data & 2) == 0) {
                // neg int
                *t = SOA_TYPE_INT;
                *(int64_t*)r->ptr = atoll(start);
            }
            else if((num_data & 16) == 0 && (num_data & 2) == 0) {
                // pos int
                *t = SOA_TYPE_UINT;
                *(uint64_t*)r->ptr = _atoull(start);
            }
            else if(num_data & 2 || num_data & 4) {
                // double
                *t = SOA_TYPE_FLOAT;
                *(double*)r->ptr = atof(start);
            }
            return ptr;
        }
        return NULL;
    }
    return ++ptr;
}

static const char* _parse_str(const char* ptr, _json_info_t* i){
    ptr++;
    const char* start = ptr;
    while(*ptr != '"'){
        if(!*ptr){
            soa_error_push("String not terminated properly!", 21);
            return NULL;
        }
        if(*ptr == '\\'){
            ptr++;
            if(!*ptr){
                break;
            }
        }
        ptr++;
    }

    // sso
    if(ptr - start + 1> 8){
        i->str++;
        i->str_size += ptr - start + 1; // null terminator
    }

    return ++ptr;
}

static const char* _read_str(const char* ptr, _json_info_t* i, _json_read_info_t* r, uint8_t* sso){
    ptr++;
    const char* start = ptr;
    while(*ptr && *ptr != '"'){
        if(*ptr == '\\'){
            ptr++;
        }
        ptr++;
    }
    
    size_t len = ptr - start + 1;
    char* new_str = 0;
    if(len > 8){
        *(size_t*)r->ptr = r->s_offset;
        new_str = (char*)r->data + r->s_offset;
        r->s_offset += len;
        *sso = 0;
    }
    else{
        new_str = (char*)r->ptr;
        *sso = 1;
    }

    memcpy(new_str, start, len);
    *(new_str + len - 1) = 0;

    char* ns = new_str;
    char* ds = new_str;
    while(*ns){
        if (*ns != '\\') {
            *ds++ = *ns++;
            continue;
        }
        
        ns++;
        if(!*ns) break;

        switch(*ns){
            case '"':
                *ds++ = '"';  
                ns++; 
                break;
            case '\\':
                *ds++ = '\\';  
                ns++; 
                break;
            case '/':
                *ds++ = '/';  
                ns++; 
                break;
            case 'b':
                *ds++ = '\b';  
                ns++; 
                break;
            case 'f':
                *ds++ = '\f';  
                ns++; 
                break;
            case 'n':
                *ds++ = '\n';  
                ns++; 
                break;
            case 'r':
                *ds++ = '\r';  
                ns++; 
                break;
            case 't':
                *ds++ = '\t';  
                ns++; 
                break;
            case 'u':
                int u1 = _read_u4(ns + 1);
                if (u1 < 0) { *ds++ = '?'; ns++; break; }
                ns += 5;

                uint32_t cp = u1;

                /* Surrogate pair */
                if (u1 >= 0xD800 && u1 <= 0xDBFF && ns[0] == '\\' && ns[1] == 'u') {
                    int u2 = _read_u4(ns + 2);
                    if (u2 >= 0xDC00 && u2 <= 0xDFFF) {
                        cp = 0x10000 + (((u1 - 0xD800) << 10)
                            | (u2 - 0xDC00));
                        ns += 6;
                    }
                }

                ds += _utf8_encode(cp, ds);
                break;
            default:
                *ds++ = *ns++;
                continue;
        }
    }
    *ds = '\0';

    return ++ptr;
}

static const char* _parse_obj(const char* ptr, _json_info_t* i){
    ptr = _skip_ws(++ptr);

    size_t index =_info_add_obj(i);
    size_t size = 0;
    if(!i->root_type){
        i->root_type = SOA_ROOT_OBJ;
    }
    while(*ptr != '}'){
        if(!*ptr){
            soa_error_push("Object not terminated properly!", 11);
            return NULL;
        }

        // key
        ptr = _parse_str(ptr, i);
        if(!ptr){
            return NULL;
        }
        ptr = _skip_ws(ptr);
        if(*ptr != ':'){
            soa_error_push("Invalid key: pair!!", 12);
            return NULL;
        }
        ptr = _skip_ws(++ptr);

        // value
        ptr = _parse_val(ptr, i);
        if(!ptr){
            return NULL;
        }
        ptr = _skip_ws(ptr);
        
        i->oe++;
        size++;
        if(*ptr == ','){
            ptr = _skip_ws(++ptr);
        }
    }
    i->osizes[index] = size;
    return ++ptr;
}

static const char* _read_obj(const char* ptr, _json_info_t* i, _json_read_info_t* r){
    ptr = _skip_ws(++ptr);

    *(size_t*)r->ptr = r->o_offset;
    
    r->ptr = r->data + r->o_offset;
    memcpy(r->ptr, i->osizes + r->o, sizeof(size_t));
    r->ptr += sizeof(size_t);
    r->o_offset += i->osizes[r->o] * sizeof(soa_obj_entry_t) + sizeof(size_t);
    r->o++;
    
    while(*ptr != '}'){
        // key
        uint8_t* old = r->ptr;
        r->ptr += 2 * sizeof(size_t);
        uint8_t sso;
        ptr = _read_str(ptr, i, r, &sso);
        r->ptr += -sizeof(size_t) + sizeof(uint8_t);
        *r->ptr = sso;
        r->ptr += -sizeof(size_t) - sizeof(uint8_t);
        ptr = _skip_ws(ptr);
        ptr = _skip_ws(++ptr);

        // value
        ptr = _read_val(ptr, i, r);
        r->ptr = old;
        ptr = _skip_ws(ptr);
        
        if(*ptr == ','){
            ptr = _skip_ws(++ptr);
        }

        r->ptr += sizeof(soa_obj_entry_t);
    }
    return ++ptr;
}

static const char* _parse_arr(const char* ptr, _json_info_t* i){
    ptr = _skip_ws(++ptr);

    size_t index = _info_add_arr(i);
    size_t size = 0;
    
    if(!i->root_type){
        i->root_type = SOA_ROOT_ARR;
    }
    while(*ptr != ']'){
        if(!*ptr){
            soa_error_push("Array not terminated properly!", 01);
            return NULL;
        }
        ptr = _parse_val(ptr, i);
        if(!ptr){
            return NULL;
        }
        ptr = _skip_ws(ptr);
        i->ae++;
        size++;
        if(*ptr == ','){
            ptr = _skip_ws(++ptr);
        }
    }
    i->asizes[index] = size;
    return ++ptr;
}

static const char* _read_arr(const char* ptr, _json_info_t* i, _json_read_info_t* r){
    ptr = _skip_ws(++ptr);

    *(size_t*)r->ptr = r->a_offset;

    r->ptr = r->data + r->a_offset;
    memcpy(r->ptr, i->asizes + r->a, sizeof(size_t));
    r->ptr += sizeof(size_t); 
    r->a_offset += i->asizes[r->a] * sizeof(soa_arr_entry_t) + sizeof(size_t);
    r->a++;

    while(*ptr != ']'){
        uint8_t* old = r->ptr;
        ptr = _read_val(ptr, i, r);
        r->ptr = old;
        ptr = _skip_ws(ptr);
        if(*ptr == ','){
            ptr = _skip_ws(++ptr);
        }
        r->ptr += sizeof(soa_arr_entry_t);
    }
    return ++ptr;
}

static const char* _parse_val(const char* ptr, _json_info_t* i){
    const char* val = ptr;
    
    if(!val){
        return NULL;
    }

    switch(*val){
    case '[':
        val = _parse_arr(val, i);
        break;
    case '{':
        val = _parse_obj(val, i);
        break;
    case '"':
        val = _parse_str(val, i);
        break;    
    default:
        val = _parse_num_or_bool(val, i);
        if(!val){
            soa_error_push("Value expected", 32);
            return NULL;
        }
    }
    
    return val;
} 

static const char* _read_val(const char* ptr, _json_info_t* i, _json_read_info_t* r){
    const char* val = ptr;
    uint8_t* type = (r->ptr + sizeof(soa_valu_t));

    switch(*val){
    case '[':
        val = _read_arr(val, i, r);
        *type = SOA_TYPE_ARR; 
        break;
    case '{':
        val = _read_obj(val, i, r);
        *type = SOA_TYPE_OBJ; 
        break;
    case '"':{
        uint8_t sso;
        val = _read_str(val, i, r, &sso);
        *type = sso ? SOA_TYPE_SSO : SOA_TYPE_STR; 
        break;    
    }
    default:
        val = _read_num_or_bool(val, i, r, type);
    }
    
    return val;
}

soa_doc_t soa_doc_new_from_json(const char* json){
    soa_error_pop();
    _json_info_t i = _info_new(SOA_JSON_PREALLOC);

    _parse_val(json, &i);

    // printf("ao: %lu ae: %lu\noo: %lu oe: %lu\nstr: %lu str_size: %lu\n", 
    //     i.ao, i.ae,
    //     i.oo, i.oe,
    //     i.str, i.str_size
    // );

    if(soa_error_get().code || !i.root_type) return (soa_doc_t){0};

    // for (size_t j = 0; j < i.ao; j++) {
    //     printf("arr %lu: %lu\n", j, i.asizes[j]);
    // }
    // for (size_t j = 0; j < i.oo; j++) {
    //     printf("obj %lu: %lu\n", j, i.osizes[j]);
    // }

    soa_doc_t doc = soa_doc_new();
    
    doc.size =
    (i.ao + i.oo) * sizeof(size_t) + 
    i.ae * sizeof(soa_arr_entry_t) +
    i.oe * sizeof(soa_obj_entry_t) +
    i.str_size;
    doc.cap = doc.size;
    doc.data = malloc(doc.size);
    doc.root_type = i.root_type;
    
    if(i.root_type == SOA_ROOT_ARR){
        _read_arr(json, &i, &(_json_read_info_t){
            0, 0, 
            0, i.ao * sizeof(size_t) + i.ae * sizeof(soa_arr_entry_t), 
            (i.ao + i.oo) * sizeof(size_t) + i.ae * sizeof(soa_arr_entry_t) + i.oe * sizeof(soa_obj_entry_t), 
            doc.data, doc.data
        });
        doc.root = 0;
    }
    else{
        _read_obj(json, &i, &(_json_read_info_t){
            0, 0, 
            0, i.ao * sizeof(size_t) + i.ae * sizeof(soa_arr_entry_t), 
            (i.ao + i.oo) * sizeof(size_t) + i.ae * sizeof(soa_arr_entry_t) + i.oe * sizeof(soa_obj_entry_t), 
            doc.data, doc.data
        });
        doc.root = i.ao * sizeof(size_t) + i.ae * sizeof(soa_arr_entry_t);
    }



    _info_free(&i);

    return doc;
}



void _print_val(soa_val_t* val, _soa_str_t* str, soa_json_parse_flags_t flags, size_t tabs);

void _print_tabs(_soa_str_t* str, soa_json_parse_flags_t flags, size_t tabs){
    if(flags & SOA_JSON_PRETTIFY){
        _soa_str_add(str, "\n");
        if(tabs == 0) return;
        for (size_t t = 0; t < tabs; t++) {
            _soa_str_add(str, SOA_JSON_PRETTIFY_TAB);
        }
    }
}

void _print_str(const char* s, _soa_str_t* str, soa_json_parse_flags_t flags){
    _soa_str_add(str, "\"");
    while(*s){
        uint32_t cp;
        size_t len = _utf8_decode((uint8_t*)s, &cp);
        
        switch(cp){
            case '\"':
                _soa_str_add(str, "\\\"");
                break;
            case '\\':
                _soa_str_add(str, "\\\\");
                break;
            case '/':
                _soa_str_add(str, "\\/");
                break;
            case '\b':
                _soa_str_add(str, "\\b");
                break;
            case '\f':
                _soa_str_add(str, "\\f");
                break;
            case '\n':
                _soa_str_add(str, "\\n");
                break;
            case '\r':
                _soa_str_add(str, "\\r");
                break;
            case '\t':
                _soa_str_add(str, "\\t");
                break;
            default:
                /* ASCII */
                if (cp >= 0x20 && cp <= 0x7E || (flags & SOA_JSON_ENCODE_UTF) == 0) {
                    char* added = _soa_str_add_size(str, 1);
                    size_t i = 0;
                    for (; i < len; i++) {
                        added[i] = *(s + i);
                    }
                    added[i] = 0;
                }
                /* BMP Unicode */
                else if (cp <= 0xFFFF) {
                    _soa_str_add(str, "\\u");
                    char d[5];
                    _write_u4(cp, d);
                    d[4] = 0;
                    _soa_str_add(str, d);
                }
                /* Surrogate pair */
                else {
                    cp -= 0x10000;
                    uint16_t hi = 0xD800 | (cp >> 10);
                    uint16_t lo = 0xDC00 | (cp & 0x3FF);
                    
                    char d[5];
                    d[4] = 0;

                    _soa_str_add(str, "\\u");
                    _write_u4(hi, d);
                    _soa_str_add(str, d);
                    
                    _soa_str_add(str, "\\u");
                    _write_u4(lo, d);
                    _soa_str_add(str, d);
                }
                break;
        }
        s += len;
    }
    _soa_str_add(str, "\"");
}

void _print_obj(soa_obj_t* obj, _soa_str_t* str, soa_json_parse_flags_t flags, size_t tabs){
    _soa_str_add(str, "{");
    size_t size = soa_obj_length(obj);
    for (size_t i = 0; i < size; i++) {
        _print_tabs(str, flags, tabs + 1);
        soa_val_t val = soa_obj_val_at_index(obj, i);
        _print_str(soa_obj_key_at(obj, i), str, flags);
        _soa_str_add(str, flags & SOA_JSON_PRETTIFY ? ": " : ":");
        _print_val(&val, str, flags, tabs + 1);
        if(i != size - 1 ){
            _soa_str_add(str, ",");
        }
    }

    _print_tabs(str, flags, tabs);
    _soa_str_add(str, "}");
}

void _print_arr(soa_arr_t* arr, _soa_str_t* str, soa_json_parse_flags_t flags, size_t tabs){
    _soa_str_add(str, "[");
    size_t size = soa_arr_length(arr);
    for (size_t i = 0; i < size; i++) {
        _print_tabs(str, flags, tabs + 1);
        soa_val_t val = soa_arr_val_at(arr, i);
        _print_val(&val, str, flags, tabs + 1);
        if(i != size - 1 ){
            _soa_str_add(str, ",");
        }
    }

    _print_tabs(str, flags, tabs);
    _soa_str_add(str, "]");
}

void _print_val(soa_val_t* val, _soa_str_t* str, soa_json_parse_flags_t flags, size_t tabs){
    soa_type_t type = soa_val_type(val);

    switch (type){
        case SOA_TYPE_STR:
        case SOA_TYPE_SSO:
            _print_str(soa_val_str(val), str, flags);
            break;
        case SOA_TYPE_ARR:{
            soa_arr_t arr = soa_val_arr(val);
            _print_arr(&arr, str, flags, tabs);
            break;
        }
        case SOA_TYPE_OBJ:{
            soa_obj_t obj = soa_val_obj(val);
            _print_obj(&obj, str, flags, tabs);
            break;
        }
        case SOA_TYPE_BOOL:{
            switch (soa_val_bool(val)){
                case SOA_BOOL_FALSE:
                    _soa_str_add(str, "false");
                    break;
                case SOA_BOOL_TRUE:
                    _soa_str_add(str, "true");
                    break;
                default:
                    _soa_str_add(str, "null");
                    break;
            }
            break;
        }
        case SOA_TYPE_INT:{
            size_t size = snprintf(NULL, 0, SOA_LD_FORMAT, soa_val_int(val));
            snprintf(_soa_str_add_size(str, size + 1), size + 1, SOA_LD_FORMAT, soa_val_int(val));
            break;
        }
        case SOA_TYPE_UINT:{
            size_t size = snprintf(NULL, 0, SOA_LU_FORMAT, soa_val_uint(val));
            snprintf(_soa_str_add_size(str, size + 1), size + 1, SOA_LU_FORMAT, soa_val_uint(val));
            break;
        }
        case SOA_TYPE_FLOAT:{
            size_t size = snprintf(NULL, 0, "%g", soa_val_float(val));
            snprintf(_soa_str_add_size(str, size + 1), size + 1, "%g", soa_val_float(val));
            break;
        }
        default:
            _soa_str_add(str, "null");
            break;
    }
}


char* soa_json_new_from_doc(soa_doc_t* doc, soa_json_parse_flags_t flags){
    _soa_str_t str = _soa_str_new(64);

    if(doc->root_type == SOA_ROOT_ARR){
        soa_arr_t root = soa_doc_root_arr(doc);
        _print_arr(&root, &str, flags, 0);
    }
    else {
        soa_obj_t root = soa_doc_root_obj(doc);
        _print_obj(&root, &str, flags, 0);
    }

    return str.str;
}