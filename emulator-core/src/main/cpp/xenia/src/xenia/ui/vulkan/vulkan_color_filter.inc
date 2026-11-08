// Original XenDroid compute pass. Included in namespace xe::ui::vulkan.
#include "xendroid_color_filter.inc"
struct VulkanPresenter::ColorFilterContext {
  const VulkanDevice* device;
  VkDescriptorSetLayout layout{};
  VkPipelineLayout pipeline_layout{};
  VkPipeline pipeline{};
  VkDescriptorPool pool{};
  VkDescriptorSet descriptors{};
  std::shared_ptr<GuestOutputImage> input, output, source;
  bool initialized = false;
  explicit ColorFilterContext(const VulkanDevice* d) : device(d) {}
  ~ColorFilterContext() {
    auto& f = device->functions(); const auto d = device->device();
    if (pipeline) f.vkDestroyPipeline(d, pipeline, nullptr);
    if (pipeline_layout) f.vkDestroyPipelineLayout(d, pipeline_layout, nullptr);
    if (pool) f.vkDestroyDescriptorPool(d, pool, nullptr);
    if (layout) f.vkDestroyDescriptorSetLayout(d, layout, nullptr);
  }
};
void VulkanPresenter::ResetColorFilter() {
  if (color_context_) { paint_context_.completion_timeline.AwaitAllSubmissions(); color_context_.reset(); }
}
bool VulkanPresenter::ApplyColorFilter(VkCommandBuffer cmd, std::shared_ptr<GuestOutputImage>& source) {
  if (!paint_context_.completion_timeline.AwaitAllSubmissions()) return false;
  return ApplyColorFilterForDevice(cmd, source, vulkan_device_, color_context_);
}
// Callers own retirement: A waits above; each B Post job is reused only after
// its final output and completion owner have retired. Never share descriptors
// or work images between devices or jobs in flight.
bool VulkanPresenter::ApplyColorFilterForDevice(
    VkCommandBuffer cmd, std::shared_ptr<GuestOutputImage>& source,
    const VulkanDevice* filter_device,
    std::shared_ptr<ColorFilterContext>& filter_state) {
  auto& f = filter_device->functions(); auto device = filter_device->device();
  const auto extent = source->extent();
  if (filter_state && (filter_state->input->extent().width != extent.width || filter_state->input->extent().height != extent.height)) filter_state.reset();
  if (!filter_state) {
    VkFormatProperties formats;
    filter_device->vulkan_instance()->functions().vkGetPhysicalDeviceFormatProperties(filter_device->physical_device(), VK_FORMAT_R8G8B8A8_UNORM, &formats);
    VkFormatProperties guest_format;
    filter_device->vulkan_instance()->functions().vkGetPhysicalDeviceFormatProperties(filter_device->physical_device(), kGuestOutputFormat, &guest_format);
    if (!(formats.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT) ||
        !(formats.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) ||
        !(formats.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT) ||
        !(guest_format.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT)) return false;
    auto context = std::make_shared<ColorFilterContext>(filter_device);
    context->input = GuestOutputImage::Create(filter_device, extent.width, extent.height, VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    context->output = GuestOutputImage::Create(filter_device, extent.width, extent.height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_STORAGE_BIT);
    if (!context->input || !context->output) return false;
    VkDescriptorSetLayoutBinding bindings[2]{};
    for (unsigned i = 0; i < 2; ++i) { bindings[i].binding = i; bindings[i].descriptorCount = 1; bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT; }
    VkDescriptorSetLayoutCreateInfo li{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO}; li.bindingCount = 2; li.pBindings = bindings;
    if (f.vkCreateDescriptorSetLayout(device, &li, nullptr, &context->layout) != VK_SUCCESS) return false;
    VkPushConstantRange constants{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(int32_t)};
    VkPipelineLayoutCreateInfo pi{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO}; pi.setLayoutCount = 1; pi.pSetLayouts = &context->layout; pi.pushConstantRangeCount = 1; pi.pPushConstantRanges = &constants;
    if (f.vkCreatePipelineLayout(device, &pi, nullptr, &context->pipeline_layout) != VK_SUCCESS) return false;
    VkShaderModuleCreateInfo si{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO}; si.codeSize = sizeof(xendroid_color_filter_spv); si.pCode = xendroid_color_filter_spv;
    VkShaderModule shader;
    if (f.vkCreateShaderModule(device, &si, nullptr, &shader) != VK_SUCCESS) return false;
    VkComputePipelineCreateInfo ci{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO}; ci.layout = context->pipeline_layout;
    ci.stage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO}; ci.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT; ci.stage.module = shader; ci.stage.pName = "main";
    auto result = f.vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &ci, nullptr, &context->pipeline);
    f.vkDestroyShaderModule(device, shader, nullptr);
    if (result != VK_SUCCESS) return false;
    VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2};
    VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO}; pool.maxSets = 1; pool.poolSizeCount = 1; pool.pPoolSizes = &size;
    if (f.vkCreateDescriptorPool(device, &pool, nullptr, &context->pool) != VK_SUCCESS) return false;
    VkDescriptorSetAllocateInfo ai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO}; ai.descriptorPool = context->pool; ai.descriptorSetCount = 1; ai.pSetLayouts = &context->layout;
    if (f.vkAllocateDescriptorSets(device, &ai, &context->descriptors) != VK_SUCCESS) return false;
    VkDescriptorImageInfo images[2]{{VK_NULL_HANDLE, context->input->view(), VK_IMAGE_LAYOUT_GENERAL}, {VK_NULL_HANDLE, context->output->view(), VK_IMAGE_LAYOUT_GENERAL}};
    VkWriteDescriptorSet writes[2]{};
    for (unsigned i = 0; i < 2; ++i) { writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; writes[i].dstSet = context->descriptors; writes[i].dstBinding = i; writes[i].descriptorCount = 1; writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; writes[i].pImageInfo = &images[i]; }
    f.vkUpdateDescriptorSets(device, 2, writes, 0, nullptr);
    filter_state = context;
  }
  auto& context = *filter_state;
  auto barrier = [&](VkImage image, VkImageLayout from, VkImageLayout to, VkAccessFlags access) {
    VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER}; b.oldLayout = from; b.newLayout = to;
    b.image = image; b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; b.srcAccessMask = from == VK_IMAGE_LAYOUT_UNDEFINED ? 0 : VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT; b.dstAccessMask = access;
    b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    f.vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &b);
  };
  barrier(source->image(), kGuestOutputInternalLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT);
  barrier(context.input->image(), context.initialized ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT);
  VkImageBlit blit{}; blit.srcSubresource = blit.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
  blit.srcOffsets[1] = blit.dstOffsets[1] = {int32_t(extent.width), int32_t(extent.height), 1};
  f.vkCmdBlitImage(cmd, source->image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, context.input->image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_NEAREST);
  barrier(source->image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, kGuestOutputInternalLayout, VK_ACCESS_SHADER_READ_BIT);
  barrier(context.input->image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT);
  barrier(context.output->image(), context.initialized ? kGuestOutputInternalLayout : VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_WRITE_BIT);
  const int32_t mode = RuntimePresentation().color_filter.load();
  f.vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, context.pipeline);
  f.vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, context.pipeline_layout, 0, 1, &context.descriptors, 0, nullptr);
  f.vkCmdPushConstants(cmd, context.pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(mode), &mode);
  f.vkCmdDispatch(cmd, (extent.width + 7) / 8, (extent.height + 7) / 8, 1);
  barrier(context.output->image(), VK_IMAGE_LAYOUT_GENERAL, kGuestOutputInternalLayout, VK_ACCESS_SHADER_READ_BIT);
  context.source = source; source = context.output; context.initialized = true;
  return true;
}
