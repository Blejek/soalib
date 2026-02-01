#include <assert.h>
#include <expected>
#include <print>
#include <unordered_map>
#include <vector>

#include "soalib/soa.hpp"
#include "soalib/soa_json.hpp"

struct point {
    double x, y;

    SOA_SERIALIZE_FIELD_BEGIN_OBJ(2)
    SOA_OBJ_FIELD(x, "x");
    SOA_OBJ_FIELD(y, "y");
    SOA_SERIALIZE_FILED_END()
};

template<>
struct std::formatter<point>{
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const point& p, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "[{}, {}]", p.x, p.y);
    }
};

int main(){
    {
        std::string json = "{\"test\":123, \"helo\":\"word\", \"p2\": { \"x\": 69.67, \"z\": 420}"
        "\"test_vec\": [10,9,8,7,6,5,4,3]}";

        auto doc = soa::json::parse(json);
        if(!doc){
            std::print("Json parsing error: {}\n", doc.error());
            return 1;
        }

        auto root = doc->val().as<soa::obj>().value();
        auto hello = root.at("helo").val();
        std::print("Hello: {}\n", hello.as<soa::str>().value());
        
        point p1{1, 2};
        hello.write<point>(p1);

        auto p2 = root.at("p2").val().as<point>();
        if(p2){
            std::print("p2 = [{}, {}]\n", p2->x, p2->y);
        }
        else{
            std::print("Error: {}\n", p2.error());
        }

        std::vector<point> vector = {
            {1, 2},
            {3, 4},
            {5, 6}
        };

        auto test = root.at("test").val();
        test.write<std::vector<point>>(vector);

        auto json_str = soa::json::stringify(*doc, soa::json::parse_flag_bits::encode_utf);
        std::print("My Json: \n{}\n", static_cast<soa::str>(json_str));

        auto test_vec = root.at("test_vec").val().as<std::vector<int>>();
        if(test_vec){
            std::print("test_vec: ");
            for(auto& v : test_vec.value()){
                std::print("{}, ", v);
            }
            std::print("\n");
        }
        else{
            std::print("Error: {}\n", test_vec.error());
        }
    }
    std::print("=======================================================================\n");
    {
        std::string json = "[{\"x\":1, \"y\":2},{\"x\":-15.5, \"y\":100.2},{\"x\":2, \"y\":3}]";
        auto doc = soa::json::parse(json);
        if(doc){
            auto root = doc->val();
            auto arr = root.as<std::vector<point>>();
            if(arr){
                std::print("len: {}:\n", arr->size());
                for(auto& p : *arr){
                    std::print("{},\n", p);
                }
            }
            else{
                std::print("Error: {}\n", arr.error());    
            }
        }
        else{
            std::print("Error: {}\n", doc.error());
        }
    }
    std::print("=======================================================================\n");
    {
        soa::doc doc;
        auto root = doc.val();

        std::unordered_map<std::string, int> map = {
            {"kupa", 23},
            {"andrei", 67},
            {"matty", 18}
        };

        root.write(map);

        auto json = soa::json::stringify(doc, soa::json::parse_flag_bits::prettify);
        std::print("{}\n", json.json);

        auto map2 = root.as<std::unordered_map<std::string, int>>();
        if(map2){
            for(const auto& [k, v] : map2.value()){
                std::print("[{} = {}],\n", k, v);
            }
        } 
        else{
            std::print("Error: {}\n", map2.error());    
        }
    }

    return 0;
}