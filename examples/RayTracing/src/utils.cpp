#include "utils.hpp"

Match::APIManager *ctx;

vk::DeviceAddress get_address(Match::Buffer &buffer) {
    return ctx->device->device.getBufferAddress(vk::BufferDeviceAddressInfo().setBuffer(buffer.buffer));
}

vk::DeviceAddress get_as_address(vk::AccelerationStructureKHR as) {
    return ctx->device->device.getAccelerationStructureAddressKHR(vk::AccelerationStructureDeviceAddressInfoKHR().setAccelerationStructure(as), ctx->dispatcher);
}
