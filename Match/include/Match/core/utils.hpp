#include <Match/commons.hpp>

namespace Match {
    template<typename DSTTYPE, typename SRCTYPE>
    DSTTYPE transform(SRCTYPE src);
    std::vector<char> read_binary_file(const std::string &filename);
}
