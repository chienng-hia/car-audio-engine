#pragma once

#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace car_audio {

/// Lightweight, self-contained JSON value representation.
class JsonValue {
public:
    using Array = std::vector<JsonValue>;
    using Object = std::map<std::string, JsonValue>;
    using Variant = std::variant<std::nullptr_t, bool, double, std::string, Array, Object>;

    JsonValue() : data_(nullptr) {}
    JsonValue(std::nullptr_t) : data_(nullptr) {}
    JsonValue(bool b) : data_(b) {}
    JsonValue(int i) : data_(static_cast<double>(i)) {}
    JsonValue(double d) : data_(d) {}
    JsonValue(const char* s) : data_(std::string(s)) {}
    JsonValue(std::string s) : data_(std::move(s)) {}
    JsonValue(Array a) : data_(std::move(a)) {}
    JsonValue(Object o) : data_(std::move(o)) {}

    [[nodiscard]] bool is_null() const { return std::holds_alternative<std::nullptr_t>(data_); }
    [[nodiscard]] bool is_bool() const { return std::holds_alternative<bool>(data_); }
    [[nodiscard]] bool is_number() const { return std::holds_alternative<double>(data_); }
    [[nodiscard]] bool is_string() const { return std::holds_alternative<std::string>(data_); }
    [[nodiscard]] bool is_array() const { return std::holds_alternative<Array>(data_); }
    [[nodiscard]] bool is_object() const { return std::holds_alternative<Object>(data_); }

    [[nodiscard]] bool as_bool(bool fallback = false) const {
        return is_bool() ? std::get<bool>(data_) : fallback;
    }

    [[nodiscard]] double as_double(double fallback = 0.0) const {
        return is_number() ? std::get<double>(data_) : fallback;
    }

    [[nodiscard]] int64_t as_int(int64_t fallback = 0) const {
        return is_number() ? static_cast<int64_t>(std::get<double>(data_)) : fallback;
    }

    [[nodiscard]] std::string as_string(const std::string& fallback = "") const {
        return is_string() ? std::get<std::string>(data_) : fallback;
    }

    [[nodiscard]] const Array& as_array() const {
        if (!is_array()) throw std::runtime_error("JsonValue is not an array");
        return std::get<Array>(data_);
    }

    [[nodiscard]] const Object& as_object() const {
        if (!is_object()) throw std::runtime_error("JsonValue is not an object");
        return std::get<Object>(data_);
    }

    [[nodiscard]] bool contains(const std::string& key) const {
        if (!is_object()) return false;
        const auto& obj = std::get<Object>(data_);
        return obj.find(key) != obj.end();
    }

    [[nodiscard]] const JsonValue& at(const std::string& key) const {
        if (!is_object()) throw std::runtime_error("JsonValue is not an object");
        const auto& obj = std::get<Object>(data_);
        auto it = obj.find(key);
        if (it == obj.end()) {
            throw std::runtime_error("Missing JSON key: " + key);
        }
        return it->second;
    }

    [[nodiscard]] JsonValue get_or(const std::string& key, const JsonValue& fallback) const {
        if (!is_object()) return fallback;
        const auto& obj = std::get<Object>(data_);
        auto it = obj.find(key);
        return it != obj.end() ? it->second : fallback;
    }

    static JsonValue parse(const std::string& text);
    static JsonValue parse_file(const std::string& path);

private:
    Variant data_;
};

}  // namespace car_audio
