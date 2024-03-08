#ifndef __SY_LIBRARY_H__
#define __SY_LIBRARY_H__

#include <memory>
#include "module.h"

namespace sy {

class Library {
public:
    static Module::ptr GetModule(const std::string& path);
};

}

#endif
