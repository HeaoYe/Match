#include <Match/vulkan/resource/vertex_attribute_set.hpp>
#include <Match/core/utils.hpp>

namespace Match {
    VertexAttributeSet::VertexAttributeSet(const std::vector<InputBindingInfo> &binding_infos) {
        for (const auto &binding_info : binding_infos) {
            add_input_binding(binding_info);
        }
    }

    void VertexAttributeSet::add_input_binding(const InputBindingInfo &binding_info) {
        uint32_t offset = 0, size;
        for (const auto &attribute : binding_info.attributes) {
            attributes.push_back({
                location,
                binding_info.binding,
                transform<vk::Format>(attribute.type),
                offset,
            });
            location += 1;
            if (attribute.stride == 0) {
                size = transform<uint32_t>(attribute.type);
            } else {
                size = attribute.stride;
            }
            if (size > 16) {
                location += 1;
            }
            offset += size;
        }

        bindings.push_back({
            binding_info.binding,
            offset,
            transform<vk::VertexInputRate>(binding_info.rate),
        });
    }

    VertexAttributeSet::~VertexAttributeSet() {
        attributes.clear();
        bindings.clear();
    }
}
