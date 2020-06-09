#include <utils/function_traits.h>

using namespace common_library;

int main() {
  function_traits<int(char *, int, int, float)> f;
  function_traits<int(const char *, int, int, float)>::args<0>::type ii;
  ii = "fffff123";
printf("ii:%s\n",ii);
  return 0;
}