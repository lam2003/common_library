// #include <functional>
// #include <iostream>
#include <thread/task_executor.h>
#include <utils/function_traits.h>
#include <utils/list.h>
#include <utils/once_token.h>
#include <utils/utils.h>

#include <thread>

using namespace common_library;

// #include <thread/semaphore.h>

#include <unistd.h>

class AAAAAAAAAAAAAAAAAAAAAAa {
  public:
    AAAAAAAAAAAAAAAAAAAAAAa() {}
};
void test()
{
    printf("%lld\n", get_current_milliseconds());
}

int main()
{
    // TaskCancelableImpl<int(int)> f([](int i) {
    //   // return 0;
    //   printf("%d\n", i);

    //   return 123;
    // });

    // bool cancelable = f;
    // printf("cancelable:%d\n", cancelable);

    // int ii = f(2);
    // printf("ii:%d\n", ii);
    // f.Cancel();
    // cancelable = f;
    // printf("c222ancelable:%d\n", cancelable);
    // f(1);
    // std::thread([]() { test(); }).detach();
    // std::thread([]() { test(); }).detach();
    // std::thread([]() { test(); }).detach();
    // std::thread([]() { test(); }).detach();
    // std::thread([]() { test(); }).detach();
    // std::thread([]() { test(); }).detach();
    // std::thread([]() { test(); }).detach();
    // std::thread([]() { test(); }).detach();

    // sleep(11111111);

    List<int> list;

    int a = 2;
    list.emplace_front(1);
    list.emplace_front(a);
    list.emplace_back(3);

    list.for_each([](int a) { printf("a:%d\n", a); });

    list.clear();

      list.emplace_back(3);
        list.emplace_back(3);
          list.emplace_back(3);
          list.pop_front();
                   list.pop_front();
                            list.pop_front();
                                     list.pop_front();
                                     
     list.for_each([](int a) { printf("aa:%d\n", a); });
}