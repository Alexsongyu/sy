#include "sy/uri.h"
#include <iostream>

int main(int argc, char** argv) {
    //sy::Uri::ptr uri = sy::Uri::Create("http://www.sy.top/test/uri?id=100&name=sy#frg");
    //sy::Uri::ptr uri = sy::Uri::Create("http://admin@www.sy.top/test/中文/uri?id=100&name=sy&vv=中文#frg中文");
    sy::Uri::ptr uri = sy::Uri::Create("http://admin@www.sy.top");
    //sy::Uri::ptr uri = sy::Uri::Create("http://www.sy.top/test/uri");
    std::cout << uri->toString() << std::endl;
    auto addr = uri->createAddress();
    std::cout << *addr << std::endl;
    return 0;
}
