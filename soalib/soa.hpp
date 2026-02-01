#pragma once

#include <compare>
#include <concepts>
#include <cstring>
#include <expected>
#include <iterator>
#include <optional>
#include <string>
#include <type_traits>
#include <format>

#include "soa.h"

namespace soa{

#define SOA_SERIALIZE_FIELD_BEGIN_ARR(element_count) \
template<::soa::serializer_mode m, typename ref_type, bool R> \
constexpr static ::soa::error serializer(ref_type val_ref, const ::soa::base_val<R>& v) { \
::soa::arr arr; if constexpr (m == ::soa::serializer_mode::read) { auto arr_v = v.template as<::soa::arr>(); \
if(arr_v) { arr = arr_v.value();} else { return ::soa::err(arr_v.error()); } \
if(arr.size() < (element_count)) { return ::soa::err("invalid size", 1); } \
} else{ arr = v.d->add_arr((element_count)); v.template write<::soa::arr>(arr); }
#define SOA_PLACEHOLDER_1 }

#define SOA_ARR_FIELD(param, pos) \
if constexpr (m == ::soa::serializer_mode::read) { auto val_v = arr.at(pos).template as<decltype(val_ref.param)>(); \
if(val_v) {val_ref.param = val_v.value();} else {return ::soa::error(val_v.error());} \
} else{ arr.at(pos).template write<decltype(val_ref.param)>(val_ref.param); }

#define SOA_SERIALIZE_FIELD_BEGIN_OBJ(element_count) \
template<::soa::serializer_mode m, typename ref_type, bool R> \
constexpr static ::soa::error serializer(ref_type val_ref, const ::soa::base_val<R>& v) { \
::soa::obj obj; size_t obj_pos = 0; if constexpr (m == ::soa::serializer_mode::read) { auto obj_v = v.template as<::soa::obj>(); \
if(obj_v) { obj = obj_v.value();} else { return ::soa::err(obj_v.error()); } \
if(obj.size() < (element_count)) { return ::soa::err("invalid size", 1); } } \
else{ obj = v.d->add_obj((element_count)); v.template write<::soa::obj>(obj); }
#define SOA_PLACEHOLDER_2 }

#define SOA_OBJ_FIELD(param, key) \
if constexpr (m == ::soa::serializer_mode::read) { \
if (auto obj_pair = obj.at(key); obj_pair){ auto val_v = obj_pair.val().template as<decltype(val_ref.param)>(); \
if(val_v) {val_ref.param = val_v.value();} else {return ::soa::error(val_v.error());} \
} else{ return ::soa::error({"failed to find key: "#key, 4}); } \
} else{ auto obj_pair = obj.at(obj_pos++); obj_pair.set_key(key); obj_pair.val().template write<decltype(val_ref.param)>(val_ref.param); }

#define SOA_PLACEHOLDER_3 {
#define SOA_SERIALIZE_FILED_END() return ::soa::error(); }

template<typename bits, std::underlying_type<bits>::type max>
class flags{
public:
    using bits_type = bits;
    using mask_type = std::underlying_type<bits>::type;

    inline constexpr flags() : m_mask(0) {}
    inline constexpr flags(bits_type b) :m_mask(static_cast<mask_type>(b)) {}
    inline constexpr flags(const flags<bits, max>& other) = default;
    inline constexpr flags(mask_type m) :m_mask(m) {}

    auto operator<=> (const flags<bits, max> &) const = default;

    inline constexpr bool operator!() const {
        return !m_mask;
    }

    inline constexpr flags<bits, max> operator&(const flags<bits, max>& other) const {
        return flags<bits, max>(m_mask & other.m_mask);
    }
    inline constexpr flags<bits, max> operator|(const flags<bits, max>& other) const {
        return flags<bits, max>(m_mask | other.m_mask);
    }
    inline constexpr flags<bits, max> operator^(const flags<bits, max>& other) const {
        return flags<bits, max>(m_mask ^ other.m_mask);
    }
    inline constexpr flags<bits, max> operator~() const {
        return flags<bits, max>(m_mask ^ max);
    }
    inline constexpr flags<bits, max>& operator=(const flags<bits, max>& other) = default;
    inline constexpr flags<bits, max>& operator|=(const flags<bits, max>& other) {
        m_mask |= other.m_mask;
        return *this;
    }
    inline constexpr flags<bits, max>& operator&(const flags<bits, max>& other) {
        m_mask &= other.m_mask;
        return *this;
    }
    inline constexpr flags<bits, max>& operator^=(const flags<bits, max>& other) {
        m_mask ^= other.m_mask;
        return *this;
    }

    inline constexpr operator bool() const {
        return !!m_mask;
    }

    inline constexpr operator mask_type() const {
        return m_mask;
    }
private:
    mask_type m_mask;
};

using i64 = int64_t;
using u64 = uint64_t;
using f64 = double;
using str = std::string_view;
using string = std::string;

struct str_buffer{
    const char* json = nullptr;

    inline str_buffer(const char* j) :json(j) {}

    inline str_buffer(const str_buffer& other){
        json = strdup(other.json);
    }

    inline str_buffer(str_buffer&& other){
        json = other.json;
        other.json = nullptr;
    }

    inline ~str_buffer(){
        delete json;
    }

    inline constexpr operator str() const {
        return json;
    }
};

struct err{
    string msg;
    int code;
};

template<typename T>
using result = std::expected<T, err>;
using result_error = std::unexpected<err>;
using error = std::optional<err>;

enum class boolean : uint8_t {
    false_val,
    true_val,
    null_val
};

enum class type : uint8_t {
    boolean = 0,
    i64,
    u64,
    f64,
    str,
    sso,
    obj,
    arr
};

template<bool is_root = false>
struct base_val;
using val = base_val<false>;
using root_val = base_val<true>;
struct doc;

template<int step, typename cont, typename value>
struct step_iterator;

struct obj {
    soa_obj_t o;
    doc* d;

    struct pair;
    using iterator = step_iterator<1, obj, pair>;
    using reverse_iterator = step_iterator<-1, obj, pair>;

    inline constexpr obj(soa_obj_t o, doc* d) :o(o), d(d) {}
    constexpr obj() = default;

    constexpr obj(const obj& other) = default;
    constexpr obj(obj&& other) = default;

    constexpr obj& operator=(const obj& other) = default;
    constexpr obj& operator=(obj&& other) = default;

    inline constexpr operator bool() const { return o.doc; }

    inline constexpr bool operator==(const obj& other) const { return o.data == other.o.data && o.doc == other.o.doc; }
    inline constexpr bool operator!=(const obj& other) const { return o.data != other.o.data || o.doc != other.o.doc; }

    inline size_t size(){
        return soa_obj_length(&o);
    }

    constexpr iterator begin();
    constexpr iterator end();
    constexpr reverse_iterator rbegin();
    constexpr reverse_iterator rend();    

    pair at(const size_t pos);
    pair at(const str key);
    pair operator[](const size_t pos);
    pair operator[](const str key);


};

struct arr{
    soa_arr_t a;
    doc* d;

    using iterator = step_iterator<1, arr, val>;
    using reverse_iterator = step_iterator<-1, arr, val>;

    inline constexpr arr(soa_arr_t a, doc* d) :a(a), d(d) {}
    constexpr arr() = default;

    constexpr arr(const arr& other) = default;
    constexpr arr(arr&& other) = default;

    constexpr arr& operator=(const arr& other) = default;
    constexpr arr& operator=(arr&& other) = default;

    inline constexpr operator bool() const { return a.doc; }

    inline constexpr bool operator==(const arr& other) const { return a.data == other.a.data && a.doc == other.a.doc; }
    inline constexpr bool operator!=(const arr& other) const { return a.data != other.a.data || a.doc != other.a.doc; }

    inline constexpr size_t size(){
        return soa_arr_length(&a);
    }

    constexpr iterator begin();
    constexpr iterator end();
    constexpr reverse_iterator rbegin();
    constexpr reverse_iterator rend();

    val at(const size_t pos);
    val operator[](const size_t pos);
};

struct doc {
    soa_doc_t d;

    inline constexpr doc(soa_doc_t d) :d(d) {} 
    inline constexpr doc() {
        d = soa_doc_new();
    }

    constexpr doc(const doc&) = delete; // TODO: copy and move constructor
    
    inline constexpr doc(doc&& other) {
        d.size = other.d.size;
        d.cap = other.d.cap;
        d.data = other.d.data;
        d.root = other.d.root;
        d.root_type = other.d.root_type;
        d.size = 0;
        d.cap =  0;
        d.data = 0;
        d.root = 0;
        d.root_type = 0;
    };

    inline constexpr ~doc() {
        soa_doc_free(&d);
    }

    inline constexpr doc& operator=(doc&& other){
        d.size = other.d.size;
        d.cap = other.d.cap;
        d.data = other.d.data;
        d.root = other.d.root;
        d.root_type = other.d.root_type;
        d.size = 0;
        d.cap =  0;
        d.data = 0;
        d.root = 0;
        d.root_type = 0;
        return *this;
    }

    inline constexpr type root_type() const{
        return static_cast<type>(d.root_type);
    }
    constexpr root_val val();

    inline obj add_obj(const size_t length){
        return {soa_doc_add_obj(&d, length), this};
    }
    inline arr add_arr(const size_t length){
        return {soa_doc_add_arr(&d, length), this};
    }
    inline size_t add_str(const str str){
        return soa_doc_add_str(&d, str.data());
    }
};

enum class serializer_mode{
    read,
    write
};

template<typename T, bool R>
concept serializable = requires (T& t, const base_val<R>& v) {
    { T::template serializer<serializer_mode::read>(t, v) } -> std::same_as<error>;
    { T::template serializer<serializer_mode::write>(t, v) } -> std::same_as<error>;
};

template<typename T, bool R>
struct serializer{
};

template<typename T, bool R>
concept serializable_struct = requires (T& t, serializer<T, R> st, const base_val<R>& v) {
    { st.read(t, v) } -> std::same_as<error>;
    { st.write(t, v) } -> std::same_as<error>;
};

template<bool is_root>
struct base_val {
    soa_val_t v;
    doc* d;

    inline constexpr base_val(size_t v, doc* d) :v(soa_val_t{&d->d, v}), d(d) {}
    inline constexpr base_val(soa_val_t v, doc* d) :v(v), d(d) {}
    inline constexpr base_val(doc* d) :v(soa_val_t{&d->d, 0}), d(d) {}
    inline constexpr base_val() :v(0,0) {}

    constexpr base_val(const base_val& other) = default;
    constexpr base_val(base_val&& other) = default;
    constexpr ~base_val() = default;

    constexpr base_val& operator=(const base_val& other) = default;
    constexpr base_val& operator=(base_val&& other) = default;

    inline constexpr operator bool() const{
        return v.doc;
    }
    inline constexpr operator size_t() const{
        return v.data;
    }
    inline constexpr operator soa_val_t() const{
        return v;
    }

    inline constexpr bool operator==(const base_val& other) const { return v.data == other.v.data && v.doc == other.v.doc; }
    inline constexpr bool operator!=(const base_val& other) const { return v.data != other.v.data || v.doc != other.v.doc; }

    inline constexpr type type() const{
        if constexpr(is_root){
            return d->root_type();
        }
        else{
            return static_cast<::soa::type>(soa_val_type(&v));
        }
    }

    inline constexpr void set_type(::soa::type t) const {
        if constexpr(is_root){
            d->d.root_type = static_cast<soa_root_t>(t);
        }
        else{
            soa_val_set_type(&v, static_cast<soa_type_t>(t));
        }
    }

    template<typename T>
    inline result<T> as() const = delete;

    template<typename T> requires serializable_struct<T, is_root>
    inline result<T> as() const {
        T t{};
        error err = serializer<T, is_root>{}.read(t, *this);
        if(err.has_value()) return result_error(*err);
        return t;
    }

    template<> inline result<boolean> as() const { if constexpr (is_root) return result_error({"value is root", 5}); return static_cast<boolean>(soa_val_bool(&v)); }
    template<> inline result<bool> as() const { auto b = soa_val_bool(&v); return b == SOA_BOOL_TRUE ? true : false; }
    
    template<> inline result<i64> as() const { if constexpr (is_root) return result_error({"value is root", 5}); return soa_val_int(&v); }
    template<> inline result<int8_t> as() const { if constexpr (is_root) return result_error({"value is root", 5}); return static_cast<int8_t>(as<i64>().value()); }
    template<> inline result<int16_t> as() const { if constexpr (is_root) return result_error({"value is root", 5}); return static_cast<int16_t>(as<i64>().value()); }
    template<> inline result<int32_t> as() const { if constexpr (is_root) return result_error({"value is root", 5}); return static_cast<int32_t>(as<i64>().value()); }

    template<> inline result<u64> as() const { if constexpr (is_root) return result_error({"value is root", 5}); return soa_val_uint(&v); }
    template<> inline result<uint8_t> as() const { if constexpr (is_root) return result_error({"value is root", 5});return static_cast<uint8_t>(as<u64>().value()); }
    template<> inline result<uint16_t> as() const { if constexpr (is_root) return result_error({"value is root", 5}); return static_cast<uint16_t>(as<u64>().value()); }
    template<> inline result<uint32_t> as() const { if constexpr (is_root) return result_error({"value is root", 5}); return static_cast<uint32_t>(as<u64>().value()); }

    template<> inline result<f64> as() const { if constexpr (is_root) return result_error({"value is root", 5}); return soa_val_float(&v); }
    template<> inline result<float> as() const { if constexpr (is_root) return result_error({"value is root", 5}); return static_cast<float>(as<f64>().value());}

    template<>
    inline result<str> as() const{
        if constexpr (is_root) return result_error({"value is root", 5});
        char* str = soa_val_str(&v);
        if(!str){ return result_error({"value is not a string", 3}); }
        return str;
    }

    template<>
    inline result<obj> as() const{
        if constexpr (is_root) {
            if(d->root_type() != soa::type::obj) 
                return result_error("inavlid root type");
            return obj{soa_doc_root_obj(&d->d), d};
        }
        soa_obj_t o = soa_val_obj(&v);
        if(o.doc == 0){ return result_error({"value is not an object", 3}); }
        return obj{o, d};
    }

    template<>
    inline result<arr> as() const{
        if constexpr (is_root) {
            if(d->root_type() != soa::type::arr) 
                return result_error("inavlid root type");
            return arr{soa_doc_root_arr(&d->d), d };
        }
        soa_arr_t a = soa_val_arr(&v);
        if(a.doc == 0){ return result_error({"value is not an array", 3}); }
        return arr{a, d};
    }

    template<typename T> requires (!serializable<T, is_root> && !serializable_struct<T, is_root>)
    inline void write(const T t) const {};

    template<typename T> requires serializable_struct<T, is_root>
    inline void write(const T& t) const { serializer<T, is_root>{}.write(t, *this); }

    template<> inline void write(const boolean val) const { if constexpr (!is_root) soa_val_set_bool(&v, static_cast<soa_bool_t>(val)); }
    template<> inline void write(const bool val) const { if constexpr (!is_root) soa_val_set_bool(&v, val ? SOA_BOOL_TRUE : SOA_BOOL_FALSE); }

    template<> inline void write(const i64 val) const { if constexpr (!is_root) soa_val_set_int(&v, val);}
    template<> inline void write(const int8_t val) const { if constexpr (!is_root) write<i64>(static_cast<i64>(val)); }
    template<> inline void write(const int16_t val) const { if constexpr (!is_root) write<i64>(static_cast<i64>(val)); }
    template<> inline void write(const int32_t val) const { if constexpr (!is_root) write<i64>(static_cast<i64>(val)); }

    template<> inline void write(const u64 val) const { if constexpr (!is_root) soa_val_set_uint(&v, val); }
    template<> inline void write(const uint8_t val) const { if constexpr (!is_root) write<u64>(static_cast<u64>(val)); }
    template<> inline void write(const uint16_t val) const { if constexpr (!is_root) write<u64>(static_cast<u64>(val)); }
    template<> inline void write(const uint32_t val) const { if constexpr (!is_root) write<u64>(static_cast<u64>(val)); }


    template<> inline void write(const f64 val) const { if constexpr (!is_root) soa_val_set_float(&v, val); }
    template<> inline void write(const float val) const { if constexpr (!is_root) write<f64>(static_cast<f64>(val)); }

    template<> inline void write(const str val) const { if constexpr (!is_root) soa_val_set_str(&v, val.data()); }
    template<> inline void write(const obj val) const { 
        if constexpr (is_root){
            set_type(soa::type::obj);
            d->d.root = val.o.data;
        } 
        else{
            soa_val_set_obj(&v, &val.o); 
        }
    }
    template<> inline void write(const arr val) const { 
        if constexpr (is_root){
            set_type(soa::type::arr);
            d->d.root = val.a.data;
        } 
        else{
            soa_val_set_arr(&v, &val.a); 
        }
    }
};

constexpr root_val doc::val() {
    return root_val{this};
}

template<typename T, bool R> requires serializable<T, R>
struct serializer<T, R> {
    inline constexpr error read(T& ref, const base_val<R>& v) { return T::template serializer<serializer_mode::read, T&>(ref, v);} 
    inline constexpr error write(const T& ref, const base_val<R>& v) { return T::template serializer<serializer_mode::write, const T&>(ref, v);} 
};

struct obj::pair{
    obj* o;
    val v;
    size_t index;

    inline constexpr pair(obj* o, size_t i) :o(o), v(), index(i) { }
    inline constexpr pair(obj* o, val v, size_t i) :o(o), v(v), index(i) {} 
    constexpr pair(const pair&) = default;

    inline constexpr val& val(){
        return v;
    }

    inline constexpr str key() {
        return soa_obj_key_at(&o->o, index);
    }

    inline constexpr void set_key(str str){
        soa_obj_set_key_at(&o->o, index, str.data());
    }

    inline constexpr operator bool(){
        return v && index < o->size();
    }

    inline constexpr bool operator==(const pair& other) const { return index == other.index && *o == *other.o; }
    inline constexpr bool operator!=(const pair& other) const { return index != other.index || *o != *other.o; }
};

inline obj::pair obj::at(const size_t pos){
    if(pos > size()) return {this, size()};
    return {this, {soa_obj_val_at_index(&o, pos), d}, pos};
}

inline obj::pair obj::at(const str key){
    for (size_t i = 0; i < size(); i++) {
        auto pair = at(i);
        if(pair.key() == key){
            return pair;
        }
    }
    return {this, size()};
}

inline obj::pair obj::operator[](const size_t pos){
    return at(pos);
}

inline obj::pair obj::operator[](const str str){
    return at(str);
}

inline val arr::at(const size_t pos){
    if(pos > size()) return {};
    return {soa_arr_val_at(&a, pos), d};
}

inline val arr::operator[](const size_t pos){
    return at(pos);
}

template<int step, typename cont, typename value>
class step_iterator{
public:
    using iterator_category = std::random_access_iterator_tag;
    using difference_type   = size_t;
    using value_type        = value;
    using pointer           = value_type*;
    using reference         = value_type; // no reference needed
    using container_type    = cont;
    using container_pointer = container_type*;
    static const int step_value = step;
public:
    inline constexpr step_iterator(container_pointer c, size_t pos) :m_cont(c), m_pos(pos), m_val(c->at(pos)) { }
    constexpr step_iterator(const step_iterator&) = default;
    constexpr step_iterator& operator=(const step_iterator&) = default;
    
    inline constexpr step_iterator& operator++(){
        m_pos += step_value;
        m_val = m_cont->at(m_pos);
        return *this;
    }
    inline constexpr step_iterator operator++(int){
        auto it = *this;
        ++it;
        return it;
    }

    inline constexpr step_iterator& operator--(){
        m_pos -= step_value;
        m_val = m_cont->at(m_pos);
        return *this;
    }
    inline constexpr step_iterator operator--(int){
        auto it = *this;
        --it;
        return it;
    }

    inline constexpr step_iterator& operator+=(const difference_type n){
        m_pos += n * step_value;
        m_val = m_cont->at(m_pos);
        return *this;
    }
    inline constexpr step_iterator operator+(const difference_type n){
        return {m_cont, m_pos + n * step_value};
    }

    inline constexpr step_iterator& operator-=(const difference_type n){
        m_pos -= n * step_value;
        m_val = m_cont->at(m_pos);
        return *this;
    }
    inline constexpr step_iterator operator-(const difference_type n){
        return {m_cont, m_pos - n * step_value};
    }
    
    inline constexpr difference_type operator-(const step_iterator& other) {
        return (m_pos - other.m_pos) / step_value;
    }

    inline constexpr reference operator[](const difference_type n){
        return m_cont->at(m_pos + n * step_value);
    }

    inline constexpr std::partial_ordering operator<=>(const step_iterator& other) const {
        if(*m_cont != *other.m_cont) return std::partial_ordering::unordered;
        return m_pos <=> other.m_pos; 
    }
    inline constexpr bool operator==(const step_iterator& other) const { return *m_cont == *other.m_cont && m_pos == other.m_pos; }
    inline constexpr bool operator!=(const step_iterator& other) const { return *m_cont != *other.m_cont || m_pos != other.m_pos; }

    inline constexpr reference operator*() const {
        return m_val;
    }
    inline constexpr pointer operator->(){
        return &m_val;
    }

private:
    container_pointer m_cont;
    size_t m_pos;
    value_type m_val;
};

constexpr obj::iterator obj::begin(){
    return {this, 0};
}

constexpr obj::iterator obj::end(){
    return {this, size()};
}

constexpr obj::reverse_iterator obj::rbegin(){
    return {this, size() - 1};
}

constexpr obj::reverse_iterator obj::rend(){
    return {this, static_cast<size_t>(-1)};
}

constexpr arr::iterator arr::begin(){
    return {this, 0};
}

constexpr arr::iterator arr::end(){
    return {this, size()};
}

constexpr arr::reverse_iterator arr::rbegin(){
    return {this, size() - 1};
}

constexpr arr::reverse_iterator arr::rend(){
    return {this, static_cast<size_t>(-1)};
}

template<typename T>
concept array_container = requires(T& t, const T& ct, size_t s){
    { t.size() } -> std::same_as<size_t>;
    { t.resize(s) };
    { t.at(s) } -> std::same_as<typename T::reference>;
    { ct.at(s) } -> std::same_as<typename T::const_reference>;
};

template<typename T>
concept map_container = requires(T& t, const T& ct, const string& s, T::mapped_type v, T::const_iterator it){
    { t.size() } -> std::same_as<size_t>;
    { t.insert(std::pair<string, typename T::mapped_type>{s, v}) };
    { ct.begin() } -> std::same_as<typename T::const_iterator>; 
    { ct.end() } -> std::same_as<typename T::const_iterator>; 
    { it++ };
};

}

template <>
struct std::formatter<soa::err> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const soa::err& obj, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "code {}: {}", obj.code, obj.msg);
    }
};

template <>
struct std::formatter<soa::boolean> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const soa::boolean& obj, std::format_context& ctx) const {
        switch(obj){
        case soa::boolean::false_val:
            return std::format_to(ctx.out(), "false");
        case soa::boolean::true_val:
            return std::format_to(ctx.out(), "true");
        default:
            return std::format_to(ctx.out(), "null");
        }
    }
};

template <>
struct std::formatter<soa::type> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const soa::type& obj, std::format_context& ctx) const {
        switch(obj){
        case soa::type::boolean:
            return std::format_to(ctx.out(), "bool");
        case soa::type::i64:
            return std::format_to(ctx.out(), "int");
        case soa::type::u64:
            return std::format_to(ctx.out(), "uint");
        case soa::type::f64:
            return std::format_to(ctx.out(), "float");
        case soa::type::str:
        case soa::type::sso:
            return std::format_to(ctx.out(), "str");
        case soa::type::obj:
            return std::format_to(ctx.out(), "obj");
        case soa::type::arr:
            return std::format_to(ctx.out(), "arr");
        default:
            return std::format_to(ctx.out(), "unknown");
        }
    }
};

template <>
struct std::formatter<soa::val> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const soa::val& obj, std::format_context& ctx) const {
        switch(obj.type()){
        case soa::type::boolean:
            return std::format_to(ctx.out(), "{}", obj.as<soa::boolean>().value());
        case soa::type::i64:
            return std::format_to(ctx.out(), "{}", obj.as<soa::i64>().value());
        case soa::type::u64:
            return std::format_to(ctx.out(),  "{}", obj.as<soa::u64>().value());
        case soa::type::f64:
            return std::format_to(ctx.out(),  "{}", obj.as<soa::f64>().value());
        case soa::type::str:
        case soa::type::sso:
            return std::format_to(ctx.out(),  "{}", obj.as<soa::str>().value());
        case soa::type::obj:
            return std::format_to(ctx.out(), "obj ({})", obj.as<soa::obj>().value().size());
        case soa::type::arr:
            return std::format_to(ctx.out(), "arr ({})", obj.as<soa::arr>().value().size());
        default:
            return std::format_to(ctx.out(), "unknown");
        }
    }
};

template<typename T, bool R> requires soa::array_container<T>
struct soa::serializer<T, R>{
    constexpr soa::error read(T& vec, const soa::base_val<R>& v){
        auto arr = v.template as<::soa::arr>();
        if(arr) {
            vec.resize(arr->size());
            for (size_t i = 0; i < arr->size(); i++) {
                auto val = arr->at(i).template as<typename T::value_type>();
                if(val){
                    vec[i] = *val;
                }
                else{
                    return soa::error{val.error()};
                }
            }
        }
        else{
            return soa::error{arr.error()};
        }
        return soa::error{};
    } 
    constexpr soa::error write(const T& vec, const soa::base_val<R>& v){
        auto arr = v.d->add_arr(vec.size());
        for (size_t i = 0; i < vec.size(); i++) {
            arr.at(i).template write<typename T::value_type>(vec[i]);
        }
        v.write(arr);
        return soa::error{};
    }    
};

template<typename T, bool R> requires soa::map_container<T>
struct soa::serializer<T, R>{
    constexpr soa::error read(T& map, const soa::base_val<R>& v){
        auto obj = v.template as<::soa::obj>();
        if(obj) {
            for (size_t i = 0; i < obj->size(); i++) {
                auto pair = obj->at(i);
                auto val = pair.val().template as<typename T::mapped_type>();
                if(val){
                    map.insert(std::pair<soa::string, typename T::mapped_type>{pair.key(), val.value()});
                }
                else{
                    return soa::error{val.error()};
                }
            }
        }
        else{
            return soa::error{obj.error()};
        }
        return soa::error{};
    } 
    constexpr soa::error write(const T& map, const soa::base_val<R>& v){
        auto obj = v.d->add_obj(map.size());
        size_t i = 0;
        for ( auto it = map.begin(); it != map.end(); ++it) {
            auto pair = obj.at(i++);
            pair.set_key(it->first);
            pair.val().template write<typename T::mapped_type>(it->second);
        }
        v.write(obj);
        return soa::error{};
    }    
};
