#include <Match/vulkan/renderer.hpp>
#include "inner.hpp"

namespace Match {
    Renderer::Renderer(CommandPool &command_pool, uint32_t max_in_flight_num) : current_in_flight(0), max_in_flight_num(max_in_flight_num) {
        command_buffers.resize(max_in_flight_num);
        image_available_semaphores.resize(max_in_flight_num);
        render_finished_semaphores.resize(max_in_flight_num);
        in_flight_fences.resize(max_in_flight_num);

        command_pool.allocate_command_buffer(command_buffers.data(), max_in_flight_num);

        VkSemaphoreCreateInfo semaphore_create_info { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        VkFenceCreateInfo fence_create_info { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        for (uint32_t i = 0; i < max_in_flight_num; i ++) {
            vk_check(vkCreateSemaphore(manager->device->device, &semaphore_create_info, manager->alloctor, &image_available_semaphores[i]));
            vk_check(vkCreateSemaphore(manager->device->device, &semaphore_create_info, manager->alloctor, &render_finished_semaphores[i]));
            vk_check(vkCreateFence(manager->device->device, &fence_create_info, manager->alloctor, &in_flight_fences[i]));
        }
    }

    Renderer::~Renderer() {
        vkDeviceWaitIdle(manager->device->device);
        for (uint32_t i = 0; i < max_in_flight_num; i ++) {
            vkDestroySemaphore(manager->device->device, image_available_semaphores[i], manager->alloctor);
            vkDestroySemaphore(manager->device->device, render_finished_semaphores[i], manager->alloctor);
            vkDestroyFence(manager->device->device, in_flight_fences[i], manager->alloctor);
        }
    }

    void Renderer::begin_render() {
        vkWaitForFences(manager->device->device, 1, &in_flight_fences[current_in_flight], VK_TRUE, UINT64_MAX);
        auto result = vkAcquireNextImageKHR(manager->device->device, manager->swapchain->swapchain, UINT64_MAX, image_available_semaphores[current_in_flight], VK_NULL_HANDLE, &index);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            manager->recreate_swapchin();
            return;
        }
        vkResetFences(manager->device->device, 1, &in_flight_fences[current_in_flight]);

        vkResetCommandBuffer(command_buffers[current_in_flight], 0);

        VkCommandBufferBeginInfo begin_info { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        vk_check(vkBeginCommandBuffer(command_buffers[current_in_flight], &begin_info));

        VkRenderPassBeginInfo render_pass_begin_info { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        render_pass_begin_info.renderPass = manager->render_pass->render_pass;
        render_pass_begin_info.framebuffer = manager->framebuffer_set->framebuffers[index]->framebuffer;
        render_pass_begin_info.renderArea.offset = { 0, 0 };
        render_pass_begin_info.renderArea.extent = { runtime_setting->get_window_size().width, runtime_setting->get_window_size().height };
        VkClearValue clear_color {
            .color = { .float32 = { 0.0f, 0.0f, 0.0f, 1.0f } },
        };
        render_pass_begin_info.clearValueCount = 1;
        render_pass_begin_info.pClearValues = &clear_color;
        vkCmdBeginRenderPass(command_buffers[current_in_flight], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    }

    void Renderer::end_render() {
        vkCmdEndRenderPass(command_buffers[current_in_flight]);
        vk_check(vkEndCommandBuffer(command_buffers[current_in_flight]));

        VkSubmitInfo submit_info { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &image_available_semaphores[current_in_flight];
        submit_info.pWaitDstStageMask = wait_stages;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffers[current_in_flight];
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &render_finished_semaphores[current_in_flight];
        vk_check(vkQueueSubmit(manager->device->graphics_queue, 1, &submit_info, in_flight_fences[current_in_flight]));

        VkPresentInfoKHR present_info { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &render_finished_semaphores[current_in_flight];
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &manager->swapchain->swapchain;
        present_info.pImageIndices = &index;
        present_info.pResults = nullptr;
        auto result = vkQueuePresentKHR(manager->device->present_queue, &present_info);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            manager->recreate_swapchin();
        }
        current_in_flight = (current_in_flight + 1) % max_in_flight_num;
    }

    void Renderer::bind_shader_program(std::shared_ptr<ShaderProgram> shader_program) {
        vkCmdBindPipeline(command_buffers[current_in_flight], shader_program->bind_point, shader_program->pipeline);
    }

    VkCommandBuffer Renderer::get_command_buffer() {
        return command_buffers[current_in_flight];
    }
}
