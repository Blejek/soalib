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

#include "soa.h"

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static soa_error_t s_error = {0};

soa_error_t soa_error_get(){
    return s_error;
}

void soa_error_push(char* msg, int code){
    if(s_error.msg){
        free(s_error.msg);
    }
    s_error.code = code;
    s_error.msg = strdup(msg);
}

void soa_error_pop(){
    free(s_error.msg);
    s_error.code = 0;
}

soa_doc_t soa_doc_new(){
    soa_doc_t doc = {
        .size = 0,
        .root = 0,
        .root_type = SOA_ROOT_NULL,
        .data = nullptr,
        .cap = 0
    };
    return doc;
}

void soa_doc_free(soa_doc_t* doc){
    free(doc->data);
    *doc = (soa_doc_t){0};
}

uint8_t* _soa_doc_grow(soa_doc_t* doc, size_t size){
    if(doc->size + size > doc->cap){
        doc->cap = (doc->size + size) * SOA_DOC_GROW_FACTOR;
        doc->data = realloc(doc->data, doc->cap);
    }
    uint8_t* ptr = doc->data + doc->size;
    doc->size += size;
    return ptr;
}

size_t soa_doc_add_str(soa_doc_t* doc, const char* str){
    char* last = (char*)_soa_doc_grow(doc, strlen(str) + 1);
    strcpy(last, str);
    return (uint8_t*)last - doc->data;
}

soa_obj_t soa_doc_add_obj(soa_doc_t* doc, size_t element_count){
    uint8_t* last = _soa_doc_grow(doc, sizeof(size_t) + sizeof(soa_obj_entry_t) * element_count);
    *(size_t*)last = element_count;
    
    return (soa_obj_t){.doc = doc, .data = last - doc->data};
}

soa_arr_t soa_doc_add_arr(soa_doc_t* doc, size_t element_count){
    uint8_t* last = _soa_doc_grow(doc, sizeof(size_t) + sizeof(soa_arr_entry_t) * element_count);
    *(size_t*)last = element_count;
    
    return (soa_arr_t){.doc = doc, .data = last - doc->data};
}

soa_obj_t soa_doc_root_obj(soa_doc_t* doc){
    return (soa_obj_t){.doc = doc, .data = doc->root};
}

soa_arr_t soa_doc_root_arr(soa_doc_t* doc){
    return (soa_arr_t){.doc = doc, .data = doc->root};
}

char*     soa_obj_key_at(soa_obj_t* obj, size_t index){
    soa_obj_entry_t* e = (soa_obj_entry_t*)(obj->doc->data + obj->data + index * sizeof(soa_obj_entry_t) + sizeof(size_t));
    return e->sso ? e->key.sso : (char*)(obj->doc->data + e->key.str);
}

void      soa_obj_set_key_at(soa_obj_t* obj, size_t index,  const char* key){
    soa_obj_entry_t* e = (soa_obj_entry_t*)(obj->doc->data + obj->data + index * sizeof(soa_obj_entry_t) + sizeof(size_t));
    if(strlen(key) < 8) {
        memcpy(e->key.sso, key, strlen(key) + 1);
        e->sso = 1;
    }
    else{
        e->key.str = soa_doc_add_str(obj->doc, key);
        e->sso = 0;
    }
}

size_t    soa_obj_length(soa_obj_t* obj){
    return *(size_t*)(obj->doc->data + obj->data);
}

soa_val_t soa_obj_val_at_index(soa_obj_t* obj, size_t index){
    if(index >= soa_obj_length(obj)) return (soa_val_t){0};
    return (soa_val_t){
        .doc = obj->doc,
        .data = obj->data + index * sizeof(soa_obj_entry_t) + sizeof(size_t),
    };
}

soa_val_t soa_obj_val_at_key(soa_obj_t* obj, const char* key){
    for (size_t i = 0; i < soa_obj_length(obj); i++) {
        soa_val_t val = soa_obj_val_at_index(obj, i);
        soa_obj_entry_t* entry = (soa_obj_entry_t*)obj->doc->data + val.data;
        if(
            entry->sso && strcmp(entry->key.sso, key) == 0 ||
            !entry->sso && strcmp((char*)(obj->doc->data + entry->key.str), key) == 0
        ){
            return val;
        }
    }
    return (soa_val_t){0};
}

size_t    soa_arr_length(soa_arr_t* arr){
    return *(size_t*)(arr->doc->data + arr->data);
}

soa_val_t soa_arr_val_at(soa_arr_t* arr, size_t index){
    if(index >= soa_arr_length(arr)) return (soa_val_t){0};
    return (soa_val_t){
        .doc = arr->doc,
        .data = arr->data + index * sizeof(soa_arr_entry_t) + sizeof(size_t),
    };
}

soa_type_t soa_val_type (const soa_val_t* val) {
    return (soa_type_t)*(val->doc->data + val->data + sizeof(soa_valu_t));
}

soa_bool_t soa_val_bool (const soa_val_t* val){
    switch(soa_val_type(val)) {
        case SOA_TYPE_BOOL:
            return *(soa_bool_t*)(val->doc->data + val->data);
        case SOA_TYPE_INT:
            return (soa_bool_t)*(int64_t*)(val->doc->data + val->data) > 0;
        case SOA_TYPE_UINT:
            return (soa_bool_t)*(uint64_t*)(val->doc->data + val->data) > 0;
        case SOA_TYPE_FLOAT:
            return (soa_bool_t)*(double*)(val->doc->data + val->data) > 0;
        default:
            return SOA_BOOL_NULL;
    };
}

int64_t    soa_val_int  (const soa_val_t* val){
    switch(soa_val_type(val)) {
        case SOA_TYPE_INT:
            return *(int64_t*)(val->doc->data + val->data);
        case SOA_TYPE_UINT:
            return (int64_t)*(uint64_t*)(val->doc->data + val->data);
        case SOA_TYPE_FLOAT:
            return (int64_t)*(double*)(val->doc->data + val->data);
        case SOA_TYPE_BOOL:
            return (int64_t)*(soa_bool_t*)(val->doc->data + val->data);
        default:
            return 0;
    };
}

uint64_t   soa_val_uint (const soa_val_t* val){
    switch(soa_val_type(val)) {
        case SOA_TYPE_UINT:
            return *(uint64_t*)(val->doc->data + val->data);
        case SOA_TYPE_INT:
            return (uint64_t)*(int64_t*)(val->doc->data + val->data);
        case SOA_TYPE_FLOAT:
            return (uint64_t)*(double*)(val->doc->data + val->data);
        case SOA_TYPE_BOOL:
            return (uint64_t)*(soa_bool_t*)(val->doc->data + val->data);
        default:
            return 0;
    };
}

double      soa_val_float(const soa_val_t* val){
    switch(soa_val_type(val)) {
        case SOA_TYPE_FLOAT:
            return *(double*)(val->doc->data + val->data);
        case SOA_TYPE_INT:
            return (double)*(int64_t*)(val->doc->data + val->data);
        case SOA_TYPE_UINT:
            return (double)*(uint64_t*)(val->doc->data + val->data);
        case SOA_TYPE_BOOL:
            return (double)*(soa_bool_t*)(val->doc->data + val->data);
        default:
            return 0;
    };
}

char*      soa_val_str  (const soa_val_t* val){
    soa_type_t type = soa_val_type(val);
    
    if(type == SOA_TYPE_STR){
        return (char*)(val->doc->data + *(size_t*)(val->doc->data + val->data));
    }
    else if(type == SOA_TYPE_SSO){
        return (char*)(val->doc->data + val->data);
    }

    return 0;
}

soa_obj_t  soa_val_obj  (const soa_val_t* val){
    if(soa_val_type(val) != SOA_TYPE_OBJ) return (soa_obj_t){0};
    return (soa_obj_t){.doc = val->doc, .data = *(size_t*)(val->doc->data + val->data)};
}

soa_arr_t  soa_val_arr  (const soa_val_t* val){
    if(soa_val_type(val) != SOA_TYPE_ARR) return (soa_arr_t){0};
    return (soa_arr_t){.doc = val->doc, .data = *(size_t*)(val->doc->data + val->data)};
}

void soa_val_set_type (const soa_val_t* val, const soa_type_t type){
    memcpy(val->doc->data + val->data + sizeof(soa_valu_t), &type, sizeof(uint8_t));
}

void soa_val_set_bool (const soa_val_t* val, const soa_bool_t value){
    soa_val_set_type(val, SOA_TYPE_BOOL);
    *(soa_bool_t*)(val->doc->data + val->data) = value;    
}

void soa_val_set_int  (const soa_val_t* val, const int64_t    value){
    soa_val_set_type(val, SOA_TYPE_INT);
    *(int64_t*)(val->doc->data + val->data) = value;  
}

void soa_val_set_uint (const soa_val_t* val, const uint64_t   value){
    soa_val_set_type(val, SOA_TYPE_UINT);
    *(uint64_t*)(val->doc->data + val->data) = value;  
}

void soa_val_set_float(const soa_val_t* val, const double     value){
    soa_val_set_type(val, SOA_TYPE_FLOAT);
    *(double*)(val->doc->data + val->data) = value;  
}

void soa_val_set_str  (const soa_val_t* val, const char*      value){
    if(strlen(value) < sizeof(soa_valu_t)){
        soa_val_set_type(val, SOA_TYPE_SSO);
        strcpy((char*)&val->data, value);
    }
    else{
        soa_val_set_type(val, SOA_TYPE_STR);
        *(size_t*)val->data = soa_doc_add_str(val->doc, value);  
    }
}

void soa_val_set_obj  (const soa_val_t* val, const soa_obj_t* value){
    soa_val_set_type(val, SOA_TYPE_OBJ);
    *(size_t*)(val->doc->data + val->data) = value->data;
}

void soa_val_set_arr  (const soa_val_t* val, const soa_arr_t* value){
    soa_val_set_type(val, SOA_TYPE_ARR);
    *(size_t*)(val->doc->data + val->data) = value->data;
}
