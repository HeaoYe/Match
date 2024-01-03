#include "utils.hpp"

Match::APIManager *ctx;

vk::DeviceAddress get_address(Match::Buffer &buffer) {
    return ctx->device->device.getBufferAddress(vk::BufferDeviceAddressInfo().setBuffer(buffer.buffer));
}
