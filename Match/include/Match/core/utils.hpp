#pragma once

#include <Match/commons.hpp>

namespace Match {
    template<typename DSTTYPE, typename SRCTYPE>
    MATCH_API DSTTYPE transform(SRCTYPE src);
    
    MATCH_API std::vector<char> read_binary_file(const std::string &filename);

    template <class T>
    ClassHashCode get_class_hash_code() {
        return typeid(std::remove_reference_t<T>).hash_code();
    }
}
