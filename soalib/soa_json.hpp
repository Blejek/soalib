#pragma once

#include "soa.hpp"
#include "soa.h"
#include "soa_json.h"

namespace soa::json {

inline static auto parse(const str json)-> result<doc>{
    auto doc = soa_doc_new_from_json(json.data());
    soa_error_t e = soa_error_get();
    if(e.code){
        auto result = err{e.msg, e.code};
        soa_error_pop();
        return result_error(result);
    }
    return doc;
}

enum class parse_flag_bits : uint32_t {
    none = 0,
    prettify = 1,
    encode_utf = 2
};
using parse_flags = flags<parse_flag_bits,
    (size_t)parse_flag_bits::prettify | (size_t)parse_flag_bits::encode_utf
>;

inline static str_buffer stringify(doc& doc, parse_flags flags){
    return soa_json_new_from_doc(&doc.d, static_cast<soa_json_parse_flags_t>(flags));
}

}

