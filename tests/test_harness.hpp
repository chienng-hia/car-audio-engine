#pragma once

#ifdef CAR_AUDIO_USE_GTEST
#include <gtest/gtest.h>
#else

#include <cmath>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace car_audio_test {

struct TestCase {
    std::string name;
    std::function<void()> fn;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

struct Registrar {
    Registrar(const char* suite, const char* name, std::function<void()> fn) {
        registry().push_back({std::string(suite) + "." + name, std::move(fn)});
    }
};

}  // namespace car_audio_test

#define TEST(suite, name)                                                                     \
    static void suite##_##name##_impl();                                                      \
    static ::car_audio_test::Registrar suite##_##name##_reg(#suite, #name,                    \
                                                            suite##_##name##_impl);           \
    static void suite##_##name##_impl()

#define EXPECT_TRUE(cond)                                                                     \
    do {                                                                                      \
        if (!(cond)) {                                                                        \
            std::ostringstream oss;                                                           \
            oss << __FILE__ << ":" << __LINE__ << " EXPECT_TRUE failed: " #cond;              \
            throw std::runtime_error(oss.str());                                              \
        }                                                                                     \
    } while (0)

#define EXPECT_FALSE(cond) EXPECT_TRUE(!(cond))

#define EXPECT_EQ(a, b)                                                                       \
    do {                                                                                      \
        if (!((a) == (b))) {                                                                  \
            std::ostringstream oss;                                                           \
            oss << __FILE__ << ":" << __LINE__ << " EXPECT_EQ failed: " #a " != " #b;         \
            throw std::runtime_error(oss.str());                                              \
        }                                                                                     \
    } while (0)

#define EXPECT_NE(a, b)                                                                       \
    do {                                                                                      \
        if ((a) == (b)) {                                                                     \
            std::ostringstream oss;                                                           \
            oss << __FILE__ << ":" << __LINE__ << " EXPECT_NE failed: " #a " == " #b;         \
            throw std::runtime_error(oss.str());                                              \
        }                                                                                     \
    } while (0)

#define EXPECT_NEAR(a, b, tol)                                                                \
    do {                                                                                      \
        double diff = std::abs(static_cast<double>(a) - static_cast<double>(b));              \
        if (diff > static_cast<double>(tol)) {                                                \
            std::ostringstream oss;                                                           \
            oss << __FILE__ << ":" << __LINE__ << " EXPECT_NEAR failed: |" << (a) << " - "    \
                << (b) << "| = " << diff << " > " << (tol);                                   \
            throw std::runtime_error(oss.str());                                              \
        }                                                                                     \
    } while (0)

#define EXPECT_LT(a, b) EXPECT_TRUE((a) < (b))
#define EXPECT_GT(a, b) EXPECT_TRUE((a) > (b))

int main() {
    int passed = 0;
    int failed = 0;
    for (const auto& tc : ::car_audio_test::registry()) {
        try {
            tc.fn();
            std::cout << "[  PASSED  ] " << tc.name << "\n";
            ++passed;
        } catch (const std::exception& ex) {
            std::cout << "[  FAILED  ] " << tc.name << ": " << ex.what() << "\n";
            ++failed;
        }
    }
    std::cout << "\nSummary: " << passed << " passed, " << failed << " failed.\n";
    return failed == 0 ? 0 : 1;
}

#endif
