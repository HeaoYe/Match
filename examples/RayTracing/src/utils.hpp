#pragma once

#include <Match/Match.hpp>
#include <glm/glm.hpp>

vk::DeviceAddress get_address(Match::Buffer &buffer);

extern Match::APIManager *ctx;
