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

    // List<int> oo;
    //    oo.emplace_back(2);
    // oo.emplace_back(1);
    // int ii = 22314;
    // oo.emplace_back(ii);
    // oo.for_each([](int ia ){
    //     printf("%d\n",ia);
    // });
    // printf("%d\n", oo[2]);s

    ThreadLoadCounter c(100, 10 * 1000 * 1000);

    c.Sleep();
    usleep(4 * 1000 * 1000);
    c.WakeUp();
    uint64_t ii = 1*10000*10000;
    while (ii--) {}
    // usleep(3 * 1000 * 1000);

       usleep(10 * 1000 * 1000);
    int aa = c.Load();
    printf("aa:%d\n", aa);
    return 0;
}