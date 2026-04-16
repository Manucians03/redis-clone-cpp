#include "TestFramework.h"

#include <exception>
#include <iostream>

int main()
{
  int failures = 0;

  for (const TestCase &test : testRegistry())
  {
    try
    {
      test.fn();
      std::cout << "[PASS] " << test.name << '\n';
    }
    catch (const std::exception &ex)
    {
      failures++;
      std::cerr << "[FAIL] " << test.name << ": " << ex.what() << '\n';
    }
  }

  std::cout << "Ran " << testRegistry().size() << " test(s)\n";
  if (failures != 0)
  {
    std::cerr << failures << " test(s) failed\n";
    return 1;
  }

  return 0;
}
