#include "car_audio/common/json.hpp"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace car_audio {
namespace {

class JsonParser {
public:
    explicit JsonParser(const std::string& input) : src_(input) {}

    JsonValue parse() {
        skip_ws();
        JsonValue val = parse_value();
        skip_ws();
        if (pos_ != src_.size()) {
            throw std::runtime_error("Unexpected trailing characters in JSON");
        }
        return val;
    }

private:
    const std::string& src_;
    size_t pos_ = 0;

    void skip_ws() {
        while (pos_ < src_.size() && std::isspace(static_cast<unsigned char>(src_[pos_]))) {
            ++pos_;
        }
    }

    char peek() const {
        if (pos_ >= src_.size()) {
            throw std::runtime_error("Unexpected end of JSON input");
        }
        return src_[pos_];
    }

    char get() {
        char c = peek();
        ++pos_;
        return c;
    }

    JsonValue parse_value() {
        skip_ws();
        char c = peek();
        if (c == '{') return parse_object();
        if (c == '[') return parse_array();
        if (c == '"') return JsonValue(parse_string());
        if (c == 't') return parse_literal("true", JsonValue(true));
        if (c == 'f') return parse_literal("false", JsonValue(false));
        if (c == 'n') return parse_literal("null", JsonValue(nullptr));
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) return parse_number();
        throw std::runtime_error(std::string("Unexpected JSON character: ") + c);
    }

    JsonValue parse_literal(const char* lit, JsonValue value) {
        for (const char* p = lit; *p; ++p) {
            if (get() != *p) {
                throw std::runtime_error(std::string("Invalid JSON literal, expected ") + lit);
            }
        }
        return value;
    }

    std::string parse_string() {
        if (get() != '"') throw std::runtime_error("Expected '\"'");
        std::string out;
        while (true) {
            char c = get();
            if (c == '"') break;
            if (c == '\\') {
                char esc = get();
                switch (esc) {
                    case '"': out.push_back('"'); break;
                    case '\\': out.push_back('\\'); break;
                    case '/': out.push_back('/'); break;
                    case 'b': out.push_back('\b'); break;
                    case 'f': out.push_back('\f'); break;
                    case 'n': out.push_back('\n'); break;
                    case 'r': out.push_back('\r'); break;
                    case 't': out.push_back('\t'); break;
                    default: out.push_back(esc); break;
                }
            } else {
                out.push_back(c);
            }
        }
        return out;
    }

    JsonValue parse_number() {
        size_t start = pos_;
        if (src_[pos_] == '-') ++pos_;
        while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) ++pos_;
        if (pos_ < src_.size() && src_[pos_] == '.') {
            ++pos_;
            while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) ++pos_;
        }
        if (pos_ < src_.size() && (src_[pos_] == 'e' || src_[pos_] == 'E')) {
            ++pos_;
            if (pos_ < src_.size() && (src_[pos_] == '+' || src_[pos_] == '-')) ++pos_;
            while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) ++pos_;
        }
        double val = std::strtod(src_.substr(start, pos_ - start).c_str(), nullptr);
        return JsonValue(val);
    }

    JsonValue parse_array() {
        get();  // '['
        JsonValue::Array arr;
        skip_ws();
        if (peek() == ']') {
            get();
            return JsonValue(std::move(arr));
        }
        while (true) {
            arr.push_back(parse_value());
            skip_ws();
            char c = get();
            if (c == ']') break;
            if (c != ',') throw std::runtime_error("Expected ',' or ']' in JSON array");
        }
        return JsonValue(std::move(arr));
    }

    JsonValue parse_object() {
        get();  // '{'
        JsonValue::Object obj;
        skip_ws();
        if (peek() == '}') {
            get();
            return JsonValue(std::move(obj));
        }
        while (true) {
            skip_ws();
            std::string key = parse_string();
            skip_ws();
            if (get() != ':') throw std::runtime_error("Expected ':' in JSON object");
            obj[std::move(key)] = parse_value();
            skip_ws();
            char c = get();
            if (c == '}') break;
            if (c != ',') throw std::runtime_error("Expected ',' or '}' in JSON object");
        }
        return JsonValue(std::move(obj));
    }
};

}  // namespace

JsonValue JsonValue::parse(const std::string& text) {
    JsonParser parser(text);
    return parser.parse();
}

JsonValue JsonValue::parse_file(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        throw std::runtime_error("Failed to open JSON file: " + path);
    }
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return parse(oss.str());
}

}  // namespace car_audio
