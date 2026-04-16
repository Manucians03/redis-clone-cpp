#pragma once

#include <cmath>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

struct TestCase
{
  std::string name;
  std::function<void()> fn;
};

inline std::vector<TestCase> &testRegistry()
{
  static std::vector<TestCase> registry;
  return registry;
}

struct TestRegistrar
{
  TestRegistrar(const std::string &name, std::function<void()> fn)
  {
    testRegistry().push_back({name, std::move(fn)});
  }
};

inline std::string buildFailureMessage(const std::string &message, const char *file, int line)
{
  std::ostringstream oss;
  oss << file << ':' << line << ": " << message;
  return oss.str();
}

inline void failTest(const std::string &message, const char *file, int line)
{
  throw std::runtime_error(buildFailureMessage(message, file, line));
}

template <typename T, typename = void>
struct IsStreamable : std::false_type
{
};

template <typename T>
struct IsStreamable<T, std::void_t<decltype(std::declval<std::ostream &>() << std::declval<const T &>())>> : std::true_type
{
};

template <typename T>
std::string printableValue(const T &value)
{
  std::ostringstream oss;
  if constexpr (IsStreamable<T>::value)
    oss << value;
  else if constexpr (std::is_enum_v<T>)
    oss << static_cast<std::underlying_type_t<T>>(value);
  else
    oss << "<unprintable>";
  return oss.str();
}

template <typename T, typename U>
void expectEqual(const T &actual, const U &expected, const char *actualExpr, const char *expectedExpr, const char *file, int line)
{
  if (!(actual == expected))
  {
    std::ostringstream oss;
    oss << "expected " << actualExpr << " == " << expectedExpr << ", got [" << printableValue(actual) << "] vs [" << printableValue(expected) << ']';
    failTest(oss.str(), file, line);
  }
}

inline void expectNear(double actual, double expected, double tolerance, const char *actualExpr, const char *expectedExpr, const char *file, int line)
{
  if (std::fabs(actual - expected) > tolerance)
  {
    std::ostringstream oss;
    oss << "expected " << actualExpr << " ~= " << expectedExpr << " within " << tolerance << ", got [" << actual << "] vs [" << expected << ']';
    failTest(oss.str(), file, line);
  }
}

#define TEST(name)                                                                 \
  static void name();                                                              \
  static TestRegistrar name##_registrar(#name, name);                             \
  static void name()

#define EXPECT_TRUE(expr)                                                          \
  do                                                                               \
  {                                                                                \
    if (!(expr))                                                                   \
      failTest(std::string("expected true: ") + #expr, __FILE__, __LINE__);       \
  } while (0)

#define EXPECT_FALSE(expr)                                                         \
  do                                                                               \
  {                                                                                \
    if (expr)                                                                      \
      failTest(std::string("expected false: ") + #expr, __FILE__, __LINE__);      \
  } while (0)

#define EXPECT_EQ(actual, expected)                                                \
  do                                                                               \
  {                                                                                \
    expectEqual((actual), (expected), #actual, #expected, __FILE__, __LINE__);    \
  } while (0)

#define EXPECT_NEAR(actual, expected, tolerance)                                   \
  do                                                                               \
  {                                                                                \
    expectNear((actual), (expected), (tolerance), #actual, #expected, __FILE__, __LINE__); \
  } while (0)
