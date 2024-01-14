#pragma once

#include <Match/Match.hpp>
#include <glm/glm.hpp>

vk::DeviceAddress get_address(Match::Buffer &buffer);
vk::DeviceAddress get_as_address(vk::AccelerationStructureKHR as);

extern Match::APIManager *ctx;
