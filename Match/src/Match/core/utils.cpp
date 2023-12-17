#include <Match/core/utils.hpp>
#include <fstream>

namespace Match {
    std::vector<char> read_binary_file(const std::string &filename) {
        std::vector<char> buffer(0);
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            MCH_ERROR("Cannot open file {}", filename);
            return buffer;
        }

        size_t size = static_cast<size_t>(file.tellg());
        file.seekg(0);
        buffer.resize(size);
        file.read(buffer.data(), size);
        file.close();

        return buffer;
    }
}
