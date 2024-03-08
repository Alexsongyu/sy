#include "sy/sy.h"
#include "sy/ds/hash_map.h"

static sy::Logger::ptr g_logger = SY_LOG_ROOT();

struct PidVid {
    PidVid(uint32_t p = 0, uint32_t v = 0)
        :pid(p), vid(v) {}
    uint32_t pid;
    uint32_t vid;

    bool operator<(const PidVid& o) const {
        return memcmp(this, &o, sizeof(o)) < 0;
    }
};

void gen() {
    sy::ds::HashMap<int, PidVid> tmp;
    for(int i = 0; i < 500000; ++i) {
        int32_t len = rand() % 10 + 5;
        int k = rand();
        for(int n = 0; n < len; ++n) {
            tmp.set(k, PidVid(rand(), rand()));
        }
    }

    std::ofstream ofs("./hashmap.data");
    tmp.writeTo(ofs);
}

void test() {
    for(int i = 0; i < 10000; ++i) {
        SY_LOG_INFO(g_logger) << "i=" << i;
        std::ifstream ifs("./hashmap.data");
        sy::ds::HashMap<int, PidVid> tmp;
        if(!tmp.readFrom(ifs)) {
            SY_LOG_INFO(g_logger) << "error";
        }
        if(i % 100 == 0) {
            SY_LOG_INFO(g_logger) << "over..." << (i + 1);
        }
    }
}

int main(int argc, char** argv) {
    gen();
    test();
    return 0;
}
