#include "sy/sy.h"
#include <assert.h>

sy::Logger::ptr g_logger = SY_LOG_ROOT();

void test_assert() {
    SY_LOG_INFO(g_logger) << sy::BacktraceToString(10);
    //SY_ASSERT2(0 == 1, "abcdef xx");
}

int main(int argc, char** argv) {
    test_assert();

    int arr[] = {1,3,5,7,9,11};

    SY_LOG_INFO(g_logger) << sy::BinarySearch(arr, sizeof(arr) / sizeof(arr[0]), 0);
    SY_LOG_INFO(g_logger) << sy::BinarySearch(arr, sizeof(arr) / sizeof(arr[0]), 1);
    SY_LOG_INFO(g_logger) << sy::BinarySearch(arr, sizeof(arr) / sizeof(arr[0]), 4);
    SY_LOG_INFO(g_logger) << sy::BinarySearch(arr, sizeof(arr) / sizeof(arr[0]), 13);
    SY_ASSERT(sy::BinarySearch(arr, sizeof(arr) / sizeof(arr[0]), 0) == -1);
    SY_ASSERT(sy::BinarySearch(arr, sizeof(arr) / sizeof(arr[0]), 1) == 0);
    SY_ASSERT(sy::BinarySearch(arr, sizeof(arr) / sizeof(arr[0]), 2) == -2);
    SY_ASSERT(sy::BinarySearch(arr, sizeof(arr) / sizeof(arr[0]), 3) == 1);
    SY_ASSERT(sy::BinarySearch(arr, sizeof(arr) / sizeof(arr[0]), 4) == -3);
    SY_ASSERT(sy::BinarySearch(arr, sizeof(arr) / sizeof(arr[0]), 5) == 2);
    SY_ASSERT(sy::BinarySearch(arr, sizeof(arr) / sizeof(arr[0]), 6) == -4);
    SY_ASSERT(sy::BinarySearch(arr, sizeof(arr) / sizeof(arr[0]), 7) == 3);
    SY_ASSERT(sy::BinarySearch(arr, sizeof(arr) / sizeof(arr[0]), 8) == -5);
    SY_ASSERT(sy::BinarySearch(arr, sizeof(arr) / sizeof(arr[0]), 9) == 4);
    SY_ASSERT(sy::BinarySearch(arr, sizeof(arr) / sizeof(arr[0]), 10) == -6);
    SY_ASSERT(sy::BinarySearch(arr, sizeof(arr) / sizeof(arr[0]), 11) == 5);
    SY_ASSERT(sy::BinarySearch(arr, sizeof(arr) / sizeof(arr[0]), 12) == -7);
    return 0;
}
