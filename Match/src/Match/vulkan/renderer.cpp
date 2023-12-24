#include <Match/vulkan/renderer.hpp>
#include "inner.hpp"

namespace Match {
    Renderer::Renderer(std::shared_ptr<RenderPassBuilder> builder) : render_pass_builder(builder), resized(false), current_in_flight(0) {
        render_pass = std::make_unique<RenderPass>(builder);
        framebuffer_set = std::make_unique<FrameBufferSet>(*this);

        command_buffers.resize(setting.max_in_flight_frame);
        image_available_semaphores.resize(setting.max_in_flight_frame);
        render_finished_semaphores.resize(setting.max_in_flight_frame);
        in_flight_fences.resize(setting.max_in_flight_frame);

        manager->command_pool->allocate_command_buffer(command_buffers.data(), setting.max_in_flight_frame);
        current_in_flight = 0;
        runtime_setting->current_in_flight = 0;
        current_buffer = command_buffers[0];

        VkSemaphoreCreateInfo semaphore_create_info { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        VkFenceCreateInfo fence_create_info { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        for (uint32_t i = 0; i < setting.max_in_flight_frame; i ++) {
            vk_check(vkCreateSemaphore(manager->device->device, &semaphore_create_info, manager->allocator, &image_available_semaphores[i]));
            vk_check(vkCreateSemaphore(manager->device->device, &semaphore_create_info, manager->allocator, &render_finished_semaphores[i]));
            vk_check(vkCreateFence(manager->device->device, &fence_create_info, manager->allocator, &in_flight_fences[i]));
        }
    }


    void Renderer::set_resize_flag() {
        resized = true;
    }

    void Renderer::wait_for_destroy() {
        vkDeviceWaitIdle(manager->device->device);
    }

    void Renderer::update_resources() {
        manager->recreate_swapchin();
        framebuffer_set.reset();
        framebuffer_set = std::make_unique<FrameBufferSet>(*this);
    }

    void Renderer::update_renderpass() {
        render_pass.reset();
        render_pass = std::make_unique<RenderPass>(render_pass_builder);
        update_resources();
    }

    Renderer::~Renderer() {
        wait_for_destroy();
        for (uint32_t i = 0; i < setting.max_in_flight_frame; i ++) {
            vkDestroySemaphore(manager->device->device, image_available_semaphores[i], manager->allocator);
            vkDestroySemaphore(manager->device->device, render_finished_semaphores[i], manager->allocator);
            vkDestroyFence(manager->device->device, in_flight_fences[i], manager->allocator);
        }
        framebuffer_set.reset();
        render_pass.reset();
        render_pass_builder.reset();
    }

    void Renderer::set_clear_value(const std::string &name, const VkClearValue &value) {
        uint32_t index = render_pass_builder->attachments_map.at(name);
        render_pass_builder->attachments[index].clear_value = value;
    }

    void Renderer::begin_render() {
        current_subpass = 0;
        vkWaitForFences(manager->device->device, 1, &in_flight_fences[current_in_flight], VK_TRUE, UINT64_MAX);
        auto result = vkAcquireNextImageKHR(manager->device->device, manager->swapchain->swapchain, UINT64_MAX, image_available_semaphores[current_in_flight], VK_NULL_HANDLE, &index);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            update_resources();
            return;
        }
        vkResetFences(manager->device->device, 1, &in_flight_fences[current_in_flight]);

        vkResetCommandBuffer(current_buffer, 0);

        VkCommandBufferBeginInfo begin_info { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        vk_check(vkBeginCommandBuffer(current_buffer, &begin_info));

        VkRenderPassBeginInfo render_pass_begin_info { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        render_pass_begin_info.renderPass = render_pass->render_pass;
        render_pass_begin_info.framebuffer = framebuffer_set->framebuffers[index]->framebuffer;
        render_pass_begin_info.renderArea.offset = { 0, 0 };
        render_pass_begin_info.renderArea.extent = { runtime_setting->get_window_size().width, runtime_setting->get_window_size().height };
        std::vector<VkClearValue> clear_values;
        clear_values.reserve(render_pass_builder->attachments.size());
        for (const auto &attachment_description : render_pass_builder->attachments) {
            clear_values.push_back(attachment_description.clear_value);
        }
        render_pass_begin_info.clearValueCount = clear_values.size();
        render_pass_begin_info.pClearValues = clear_values.data();
        vkCmdBeginRenderPass(current_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    }

    void Renderer::end_render() {
        vkCmdEndRenderPass(current_buffer);
        vk_check(vkEndCommandBuffer(current_buffer));

        VkSubmitInfo submit_info { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &image_available_semaphores[current_in_flight];
        submit_info.pWaitDstStageMask = wait_stages;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &current_buffer;
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
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || resized) {
            update_resources();
        }
        current_in_flight = (current_in_flight + 1) % setting.max_in_flight_frame;
        runtime_setting->current_in_flight = current_in_flight;
        current_buffer = command_buffers[current_in_flight];
    }

    void Renderer::begin_layer_render(const std::string &name) {
        layers[layers_map.at(name)]->begin_render();
    }

    void Renderer::end_layer_render(const std::string &name) {
        layers[layers_map.at(name)]->end_render();
    }

    void Renderer::bind_shader_program(std::shared_ptr<ShaderProgram> shader_program) {
        vkCmdBindPipeline(current_buffer, shader_program->bind_point, shader_program->pipeline);
        if (!shader_program->descriptor_sets.empty()) {
            vkCmdBindDescriptorSets(current_buffer, shader_program->bind_point, shader_program->layout, 0, 1, &shader_program->descriptor_sets[current_in_flight], 0, nullptr);
        }
        for (const auto &[stage, offset_size] : shader_program->constant_offset_size_map) {
            vkCmdPushConstants(current_buffer, shader_program->layout, stage, offset_size.first, offset_size.second, shader_program->constants.data() + offset_size.first);
        }
    }

    void Renderer::bind_vertex_buffer(const std::shared_ptr<VertexBuffer> &vertex_buffer) {
        VkDeviceSize size = 0;
        vkCmdBindVertexBuffers(current_buffer, 0, 1, &vertex_buffer->buffer->buffer, &size);
    }

    void Renderer::bind_vertex_buffers(const std::vector<std::shared_ptr<VertexBuffer>> &vertex_buffers) {
        std::vector<VkBuffer> buffers(vertex_buffers.size());
        std::vector<VkDeviceSize> sizes(vertex_buffers.size());
        for (uint32_t i = 0; i < vertex_buffers.size(); i ++) {
            buffers[i] = vertex_buffers[i]->buffer->buffer;
            sizes[i] = 0;
        }
        vkCmdBindVertexBuffers(current_buffer, 0, vertex_buffers.size(), buffers.data(), sizes.data());
    }

    void Renderer::bind_index_buffer(std::shared_ptr<IndexBuffer> index_buffer) {
        vkCmdBindIndexBuffer(current_buffer, index_buffer->buffer->buffer, 0, index_buffer->type);
    }

    void Renderer::set_viewport(float x, float y, float width, float height) {
        set_viewport(x, y, width, height, 0.0f, 1.0f);
    }

    void Renderer::set_viewport(float x, float y, float width, float height, float min_depth, float max_depth) {
        VkViewport viewport {
            .x = x,
            .y = y,
            .width = width,
            .height = height,
            .minDepth = min_depth,
            .maxDepth = max_depth,
        };
        vkCmdSetViewport(current_buffer, 0, 1, &viewport);
    }

    void Renderer::set_scissor(int x, int y, uint32_t width, uint32_t height) {
        VkRect2D scissor {
            .offset = { x, y },
            .extent = { width, height }
        };
        vkCmdSetScissor(current_buffer, 0, 1, &scissor);
    }

    void Renderer::draw_indexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, uint32_t vertex_offset, uint32_t first_instance) {
        vkCmdDrawIndexed(current_buffer, index_count, instance_count, first_index, vertex_offset, first_instance);
    }

    void Renderer::next_subpass() {
        vkCmdNextSubpass(current_buffer, VK_SUBPASS_CONTENTS_INLINE);
        current_subpass += 1;
    }

    void Renderer::continue_subpass_to(const std::string &subpass_name) {
        auto subpass_idx = render_pass_builder->subpasses_map.at(subpass_name);
        while (current_subpass != subpass_idx) {
            next_subpass();
        }
    }

    void Renderer::draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance) {
        vkCmdDraw(current_buffer, vertex_count, instance_count, first_vertex, first_instance);
    }

    VkCommandBuffer Renderer::get_command_buffer() {
        return current_buffer;
    }
}
