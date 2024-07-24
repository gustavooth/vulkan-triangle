#include "defines.h"
#include "core/logger.h"
#include "core/events.h"
#include "containers/rexarray.h"

#include "platform/platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef PLATFORM_WAYLAND
    #define VK_USE_PLATFORM_WAYLAND_KHR
    #include "platform/linux/platform_wayland.h"
#elif Win32
    #define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

typedef struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;

    u32 format_count;
    VkSurfaceFormatKHR* formats;

    u32 present_mode_count;
    VkPresentModeKHR* present_modes;
} SwapchainSupportDetails;

typedef struct QueueIndex {
    u32 family_index;
    u32 index;
} QueueIndex;

struct vkstate {
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;

    VkPhysicalDevice physical_device;
    VkDevice device;
    QueueIndex graphics_queue_index;
    QueueIndex present_queue_index;
    QueueIndex transfer_queue_index;
    QueueIndex compute_queue_index;
    u32* queue_count; // rexarray
    u32* queue_family_indexes; //rexarray
    VkQueue graphics_queue;
    VkQueue present_queue;
    VkQueue transfer_queue;
    VkQueue compute_queue;

    VkSurfaceKHR surface;

    VkSwapchainKHR swapchain;
    SwapchainSupportDetails swapchain_support;
    u32 framebuffer_width;
    u32 framebuffer_height;
    VkSurfaceFormatKHR image_format;
    u32 image_count;
    VkImage* swapchain_images;
    VkImageView* swapchain_image_views;
    u32 max_frames_in_flight;
    u32 image_index;
    u32 frame_index;
    
    VkFramebuffer* framebuffers;

    VkRenderPass render_pass;

    VkPipelineLayout pipeline_layout;
    VkPipeline graphics_pipeline;

    VkCommandPool commando_pool;
    VkCommandBuffer command_buffer;

    VkSemaphore image_available_semaphore;
    VkSemaphore render_finished_semaphore;
    VkFence in_flight_fence;
};

b8 running = true;
static struct vkstate vkstate;
static Window window;

b8 create_instance() {
    REXDEBUG("Creating instance...");
    VkApplicationInfo app_info = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app_info.pApplicationName = "Triangle";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "Triangle";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo instance_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instance_info.pApplicationInfo = &app_info;

    u32 instance_ext_count = 3;
    const char** instance_extensions = malloc(sizeof(const char*) * instance_ext_count);
    const char* surface_ext = VK_KHR_SURFACE_EXTENSION_NAME;
    const char* debug_ext = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
#ifdef PLATFORM_WAYLAND
    const char* platform_ext = "VK_KHR_wayland_surface";
#elif PLATFORM_WIN32
    const char* platform_ext = "VK_KHR_win32_surface";
#endif
    instance_extensions[0] = surface_ext;
    instance_extensions[1] = debug_ext;
    instance_extensions[2] = platform_ext;

    instance_info.enabledExtensionCount = instance_ext_count;
    instance_info.ppEnabledExtensionNames = instance_extensions;

    u32 instance_layers_count = 1;
    const char** instance_layers = malloc(sizeof(const char*) * instance_layers_count);
    const char* validation_layer = "VK_LAYER_KHRONOS_validation";
    instance_layers[0] = validation_layer;

    instance_info.enabledLayerCount = instance_layers_count;
    instance_info.ppEnabledLayerNames = instance_layers;

    if (vkCreateInstance(&instance_info, 0, &vkstate.instance))
    {
        REXFATAL("failed to create instance!");
        return false;
    }

    free(instance_extensions);

    return true;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    switch (messageSeverity) {
        default:
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            REXERROR(pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            REXWARN(pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            REXINFO(pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            REXTRACE(pCallbackData->pMessage);
            break;
    }

    return VK_FALSE;
}

b8 setup_debug_messenger() {
    REXDEBUG("Creating debug messenger...");
    VkDebugUtilsMessengerCreateInfoEXT debug_info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_info.pfnUserCallback = debug_callback;

    PFN_vkCreateDebugUtilsMessengerEXT func =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vkstate.instance, "vkCreateDebugUtilsMessengerEXT");

    if (func == 0)
    {
        REXFATAL("failed to get debug messenger function!");
        return false;
    }
    
    
    if (func(vkstate.instance, &debug_info, 0, &vkstate.debug_messenger) != VK_SUCCESS)
    {
        REXFATAL("failed to create debug messenger!");
        return false;
    }

    return true;
}

b8 create_surface() {
    REXDEBUG("Creating wayland surface...");
    WaylandState* state = (WaylandState*)window.internal_state;
    VkWaylandSurfaceCreateInfoKHR surface_info = {VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR};
    surface_info.display = state->display;
    surface_info.surface = state->surface;

    if (vkCreateWaylandSurfaceKHR(vkstate.instance, &surface_info, 0, &vkstate.surface) != VK_SUCCESS)
    {
        REXFATAL("failed to create wayland surface!");
        return false;
    }
    

    return true;
}

b8 query_swapchain_support(VkPhysicalDevice device, SwapchainSupportDetails* out_swapchain_support) {
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, vkstate.surface, &out_swapchain_support->capabilities);

    vkGetPhysicalDeviceSurfaceFormatsKHR(device, vkstate.surface, &out_swapchain_support->format_count, 0);
    if (!out_swapchain_support->format_count) return false;
    out_swapchain_support->formats = malloc(sizeof(VkSurfaceFormatKHR) * out_swapchain_support->format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, vkstate.surface, &out_swapchain_support->format_count, out_swapchain_support->formats);

    vkGetPhysicalDeviceSurfacePresentModesKHR(device, vkstate.surface, &out_swapchain_support->present_mode_count, 0);
    if(!out_swapchain_support->present_mode_count) return false;
    out_swapchain_support->present_modes = malloc(sizeof(VkPresentModeKHR) * out_swapchain_support->present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, vkstate.surface, &out_swapchain_support->present_mode_count, out_swapchain_support->present_modes);

    return true;
}

void destroy_swapchain_support(SwapchainSupportDetails* swapchain_support) {
    if (swapchain_support->formats) free(swapchain_support->formats);
    if (swapchain_support->present_modes) free(swapchain_support->present_modes);
    memset(swapchain_support, 0, sizeof(SwapchainSupportDetails));
}

b8 pick_physical_device() {
    REXDEBUG("Choosing physical device...");
    u32 device_count = 0;
    vkEnumeratePhysicalDevices(vkstate.instance, &device_count, 0);

    if (device_count == 0) {
        REXFATAL("failed to find GPUs with Vulkan support!");
        return false;
    }

    VkPhysicalDevice* physical_devices = malloc(sizeof(VkPhysicalDevice) * device_count);
    vkEnumeratePhysicalDevices(vkstate.instance, &device_count, physical_devices);

    b8 found = false;

    for (u32 i = 0; i < device_count; i++)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physical_devices[i], &properties);

        if (properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) continue;
        if (!query_swapchain_support(physical_devices[i], &vkstate.swapchain_support)) {
            destroy_swapchain_support(&vkstate.swapchain_support);
            continue;
        }

        vkstate.physical_device = physical_devices[i];
        found = true;
        REXINFO("Selected device: %s", properties.deviceName);
        break;
    }

    if (!found) return false;

    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(vkstate.physical_device, &queue_family_count, 0);
    VkQueueFamilyProperties* queue_families = malloc(sizeof(VkQueueFamilyProperties) * queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(vkstate.physical_device, &queue_family_count, queue_families);

    vkstate.graphics_queue_index.family_index = -1;
    vkstate.compute_queue_index.family_index = -1;
    vkstate.transfer_queue_index.family_index = -1;
    vkstate.present_queue_index.family_index = -1;

    vkstate.queue_count = REXARRAY(u32);
    vkstate.queue_family_indexes = REXARRAY(u32);

    for (u32 i = 0; i < queue_family_count; ++i) {
        if (vkstate.graphics_queue_index.family_index != -1 && vkstate.present_queue_index.family_index != -1 && vkstate.transfer_queue_index.family_index != -1 && vkstate.compute_queue_index.family_index != -1)
        {
            break;
        }
        
        u32 index_count = 0;
        if (vkstate.graphics_queue_index.family_index == -1 && queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            vkstate.graphics_queue_index.family_index = i;
            vkstate.graphics_queue_index.index = index_count;
            index_count++;
            if (queue_families[i].queueCount == index_count) {
                rexarray_push(vkstate.queue_count, &index_count);
                rexarray_push(vkstate.queue_family_indexes, &i);
                continue;
            };
        }

        if (vkstate.compute_queue_index.family_index == -1 && queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            vkstate.compute_queue_index.family_index = i;
            vkstate.compute_queue_index.index = index_count;
            index_count++;
            if (queue_families[i].queueCount == index_count) {
                rexarray_push(vkstate.queue_count, &index_count);
                rexarray_push(vkstate.queue_family_indexes, &i);
                continue;
            };
        }

        if (vkstate.transfer_queue_index.family_index == -1 && queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
            vkstate.transfer_queue_index.family_index = i;
            vkstate.transfer_queue_index.index = index_count;
            index_count++;
            if (queue_families[i].queueCount == index_count) {
                rexarray_push(vkstate.queue_count, &index_count);
                rexarray_push(vkstate.queue_family_indexes, &i);
                continue;
            };
        }

        if (vkstate.present_queue_index.family_index == -1)
        {
            VkBool32 supports_present = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(vkstate.physical_device, i, vkstate.surface, &supports_present);
            if (supports_present) {
                vkstate.present_queue_index.family_index = i;
                vkstate.present_queue_index.index = index_count;
                index_count++;
                if (queue_families[i].queueCount == index_count) {
                    rexarray_push(vkstate.queue_count, &index_count);
                    rexarray_push(vkstate.queue_family_indexes, &i);
                    continue;
                };
            }
        }

        rexarray_push(vkstate.queue_count, &index_count);
        rexarray_push(vkstate.queue_family_indexes, &i);
    }

    REXDEBUG("Queue family index : Queue index ________");
    REXDEBUG(" Graphics | Compute | Transfer | Present |");
    REXDEBUG("   %i:%i    |   %i:%i   |   %i:%i    |   %i:%i   |", vkstate.graphics_queue_index.family_index, vkstate.graphics_queue_index.index, vkstate.compute_queue_index.family_index, vkstate.compute_queue_index.index, vkstate.transfer_queue_index.family_index, vkstate.transfer_queue_index.index, vkstate.present_queue_index.family_index, vkstate.present_queue_index.index);
    REXDEBUG("_________________________________________");

    if (vkstate.graphics_queue_index.family_index == -1 || vkstate.present_queue_index.family_index == -1 || vkstate.transfer_queue_index.family_index == -1 || vkstate.compute_queue_index.family_index == -1)
    {
        REXFATAL("Queue family indexes not found!");
        return false;
    }

    free(physical_devices);

    return true;
}

b8 create_logical_device() {
    REXDEBUG("Creating logical device...");

    u32 queue_count = rexarray_len(vkstate.queue_count);
    VkDeviceQueueCreateInfo* queue_info = malloc(sizeof(VkDeviceQueueCreateInfo) * queue_count);
    memset(queue_info, 0, sizeof(VkDeviceQueueCreateInfo) * queue_count);
    f32 queue_priority[] = {1.f, .9f, .8f, .7f, .6f};
    for (u32 i = 0; i < queue_count; i++)
    {
        queue_info[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info[i].queueFamilyIndex = vkstate.queue_family_indexes[i];
        queue_info[i].queueCount = vkstate.queue_count[i];
        queue_info[i].pQueuePriorities = &queue_priority[i];
    }

    VkPhysicalDeviceFeatures device_features = {0};
    device_features.samplerAnisotropy = VK_TRUE;

    const char* swapchain_ext = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

    VkDeviceCreateInfo device_info = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    device_info.queueCreateInfoCount = queue_count;
    device_info.pQueueCreateInfos = queue_info;
    device_info.pEnabledFeatures = &device_features;
    device_info.enabledExtensionCount = 1;
    device_info.ppEnabledExtensionNames = &swapchain_ext;

    if (vkCreateDevice(vkstate.physical_device, &device_info, 0, &vkstate.device) != VK_SUCCESS)
    {
        REXFATAL("failed to create logical device!");
        return false;
    }

    vkGetDeviceQueue(vkstate.device, vkstate.graphics_queue_index.family_index, vkstate.graphics_queue_index.index, &vkstate.graphics_queue);
    vkGetDeviceQueue(vkstate.device, vkstate.present_queue_index.family_index, vkstate.graphics_queue_index.index, &vkstate.present_queue);
    vkGetDeviceQueue(vkstate.device, vkstate.compute_queue_index.family_index, vkstate.graphics_queue_index.index, &vkstate.compute_queue);
    vkGetDeviceQueue(vkstate.device, vkstate.transfer_queue_index.family_index, vkstate.graphics_queue_index.index, &vkstate.transfer_queue);

    return true;
}

b8 create_swapchain() {
    REXDEBUG("Creating swpachain...");

    u32 format_index = 0;
    for (u32 i = 0; i < vkstate.swapchain_support.format_count; i++)
    {
        if (vkstate.swapchain_support.formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && vkstate.swapchain_support.formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            format_index = i;
            break;
        }
    }

    u32 present_mode_index = -1;
    //IF DON'T FIND, USE VK_PRESENT_MODE_FIFO_KHR
    for (u32 i = 0; i < vkstate.swapchain_support.present_mode_count; i++)
    {
        if (vkstate.swapchain_support.present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            present_mode_index = i;
            break;
        }
    }

    VkExtent2D min = vkstate.swapchain_support.capabilities.minImageExtent;
    VkExtent2D max = vkstate.swapchain_support.capabilities.maxImageExtent;
    u32 width = REXCLAMP(vkstate.framebuffer_width, min.width, max.width);
    u32 height = REXCLAMP(vkstate.framebuffer_height, min.height, max.height);

    u32 image_count = vkstate.swapchain_support.capabilities.minImageCount + 1;
    if (vkstate.swapchain_support.capabilities.maxImageCount > 0 && image_count > vkstate.swapchain_support.capabilities.maxImageCount) {
        image_count = vkstate.swapchain_support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchain_info = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchain_info.surface = vkstate.surface;
    swapchain_info.minImageCount = image_count;
    swapchain_info.imageFormat = vkstate.swapchain_support.formats[format_index].format;
    swapchain_info.imageColorSpace = vkstate.swapchain_support.formats[format_index].colorSpace;
    swapchain_info.imageExtent.width = width;
    swapchain_info.imageExtent.height = height;
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_info.preTransform = vkstate.swapchain_support.capabilities.currentTransform;
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    if (present_mode_index == -1) swapchain_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    else swapchain_info.presentMode = vkstate.swapchain_support.present_modes[present_mode_index];
    swapchain_info.clipped = VK_TRUE;

    if(vkstate.graphics_queue_index.family_index != vkstate.present_queue_index.family_index) {
        u32 queueFamilyIndices[] = {
            vkstate.graphics_queue_index.family_index,
            vkstate.present_queue_index.family_index,
        };
        swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT,
        swapchain_info.queueFamilyIndexCount = 2,
        swapchain_info.pQueueFamilyIndices = queueFamilyIndices;
    }else {
        swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        swapchain_info.queueFamilyIndexCount = 0,
        swapchain_info.pQueueFamilyIndices = 0;
    }

    vkCreateSwapchainKHR(vkstate.device, &swapchain_info, 0, &vkstate.swapchain);

    if (vkstate.swapchain == VK_NULL_HANDLE) return false;

    vkstate.image_format = vkstate.swapchain_support.formats[format_index];

    REXDEBUG("Retrieving the swapchain images...");

    vkGetSwapchainImagesKHR(vkstate.device, vkstate.swapchain, &vkstate.image_count, 0);
    vkstate.swapchain_images = malloc(sizeof(VkImage) * vkstate.image_count);
    vkGetSwapchainImagesKHR(vkstate.device, vkstate.swapchain, &vkstate.image_count, vkstate.swapchain_images);

    REXDEBUG("Creating image viwes...");

    vkstate.swapchain_image_views = malloc(sizeof(VkImageView) * vkstate.image_count);

    for (u32 i = 0; i < vkstate.image_count; i++)
    {
        VkImageViewCreateInfo image_view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        image_view_info.image = vkstate.swapchain_images[i];
        image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_info.format = vkstate.image_format.format;
        image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_view_info.subresourceRange.baseMipLevel = 0;
        image_view_info.subresourceRange.levelCount = 1;
        image_view_info.subresourceRange.baseArrayLayer = 0;
        image_view_info.subresourceRange.layerCount = 1;

        if(vkCreateImageView(vkstate.device, &image_view_info, 0, &vkstate.swapchain_image_views[i]) != VK_SUCCESS) {
            REXFATAL("failed to create image views!");
            return false;
        }
    }

    return true;
}

b8 create_render_pass() {
    REXDEBUG("Creating renderpass...");

    VkAttachmentDescription color_attachment = {0};
    color_attachment.format = vkstate.image_format.format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref = {0};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    VkSubpassDependency dependency = {0};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    if (vkCreateRenderPass(vkstate.device, &render_pass_info, 0, &vkstate.render_pass) != VK_SUCCESS) {
        REXFATAL("failed to create renderpass!");
        return false;
    }

    return true;
}

b8 read_file(const char* file_name, u32* out_file_size, u8* out_buffer) {
    FILE* file = fopen(file_name, "rb");
    if (file == 0) {
        REXFATAL("failed to open file: [%s]", file_name);
        return false;
    }

    fseek(file, 0, SEEK_END);
    *out_file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (out_buffer == 0) return true;

    if (fread(out_buffer, 1, *out_file_size, file) != *out_file_size) {
        REXFATAL("failed to read file!");
        fclose(file);
        return false;
    }

    fclose(file);
    return true;
}

b8 create_shader_module(u8* buffer, u32 buffer_size, VkShaderModule* out_shader) {
    VkShaderModuleCreateInfo shader_info = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    shader_info.codeSize = buffer_size;
    shader_info.pCode = (u32*)buffer;

    if (vkCreateShaderModule(vkstate.device, &shader_info, 0, out_shader) != VK_SUCCESS) return false;
    return true;
}

b8 create_graphics_pipeline() {
    REXDEBUG("Creating graphics pipeline...");

    u32 vert_shader_buffer_size;
    if (!read_file("shader/triangle.vert.spv", &vert_shader_buffer_size, 0)) return false;
    u8* vert_shader_buffer = malloc(vert_shader_buffer_size);
    if (!read_file("shader/triangle.vert.spv", &vert_shader_buffer_size, vert_shader_buffer)) return false;

    u32 frag_shader_buffer_size;
    if (!read_file("shader/triangle.frag.spv", &frag_shader_buffer_size, 0)) return false;
    u8* frag_shader_buffer = malloc(frag_shader_buffer_size);
    if (!read_file("shader/triangle.frag.spv", &frag_shader_buffer_size, frag_shader_buffer)) return false;

    VkShaderModule vert_shader;
    VkShaderModule frag_shader;
    if(!create_shader_module(vert_shader_buffer, vert_shader_buffer_size, &vert_shader)) {
        REXFATAL("failed to create vertex shader module!");
        return false;
    }
    if(!create_shader_module(frag_shader_buffer, frag_shader_buffer_size, &frag_shader)) {
        REXFATAL("failed to create fragment shader module!");
        return false;
    }

    VkPipelineShaderStageCreateInfo vert_shader_stage_info = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_shader_stage_info.module = vert_shader;
    vert_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_shader_stage_info = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_stage_info.module = frag_shader;
    frag_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info, frag_shader_stage_info};

    VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamic_state_info = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamic_state_info.dynamicStateCount = 2;
    dynamic_state_info.pDynamicStates = dynamic_states;

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

    VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_info.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {0};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (f32)vkstate.framebuffer_width;
    viewport.height = (f32)vkstate.framebuffer_height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {0};
    scissor.offset = (VkOffset2D){0, 0};
    scissor.extent.width = vkstate.framebuffer_width;
    scissor.extent.width = vkstate.framebuffer_height;
    
    VkPipelineViewportStateCreateInfo viewport_state_info = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewport_state_info.viewportCount = 1;
    viewport_state_info.pViewports = &viewport;
    viewport_state_info.scissorCount = 1;
    viewport_state_info.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer_info = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterizer_info.depthClampEnable = VK_FALSE;
    rasterizer_info.rasterizerDiscardEnable = VK_FALSE;
    rasterizer_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer_info.lineWidth = 1.0f;
    rasterizer_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer_info.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling_info = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisampling_info.sampleShadingEnable = VK_FALSE;
    multisampling_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState color_blend_attachment = {0};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_TRUE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo color_blending_info = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    color_blending_info.logicOpEnable = VK_FALSE;
    color_blending_info.logicOp = VK_LOGIC_OP_COPY; // Optional
    color_blending_info.attachmentCount = 1;
    color_blending_info.pAttachments = &color_blend_attachment;

    VkPipelineLayoutCreateInfo pipeline_layout_info = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};

    if (vkCreatePipelineLayout(vkstate.device, &pipeline_layout_info, 0, &vkstate.pipeline_layout) != VK_SUCCESS) {
        REXFATAL("failed to create pipeline layout!");
        return false;
    }

    VkGraphicsPipelineCreateInfo pipeline_info = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly_info;
    pipeline_info.pViewportState = &viewport_state_info;
    pipeline_info.pRasterizationState = &rasterizer_info;
    pipeline_info.pMultisampleState = &multisampling_info;
    pipeline_info.pDepthStencilState = 0; // Optional
    pipeline_info.pColorBlendState = &color_blending_info;
    pipeline_info.pDynamicState = &dynamic_state_info;
    pipeline_info.layout = vkstate.pipeline_layout;
    pipeline_info.renderPass = vkstate.render_pass;
    pipeline_info.subpass = 0;

    if(vkCreateGraphicsPipelines(vkstate.device, 0, 1, &pipeline_info, 0, &vkstate.graphics_pipeline) != VK_SUCCESS) {
        REXFATAL("failed to create graphics pipeline!");
        return false;
    }


    vkDestroyShaderModule(vkstate.device, vert_shader, 0);
    vkDestroyShaderModule(vkstate.device, frag_shader, 0);
    free(vert_shader_buffer);
    free(frag_shader_buffer);
    return true;
}

b8 create_framebuffers() {
    REXDEBUG("Creating framebuffers...");

    vkstate.framebuffers = malloc(sizeof(VkFramebuffer) * vkstate.image_count);

    for (u32 i = 0; i < vkstate.image_count; i++)
    {
        VkImageView attachments[] = {vkstate.swapchain_image_views[i]};

        VkFramebufferCreateInfo framebuffer_info = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebuffer_info.renderPass = vkstate.render_pass;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = attachments;
        framebuffer_info.width = vkstate.framebuffer_width;
        framebuffer_info.height = vkstate.framebuffer_height;
        framebuffer_info.layers = 1;

        if(vkCreateFramebuffer(vkstate.device, &framebuffer_info, 0, &vkstate.framebuffers[i]) != VK_SUCCESS) {
            REXFATAL("failed to create framebuffer[%i]!", i);
            return false;
        }
    }

    return true;
}

b8 create_command_pool() {
    REXDEBUG("Creating commando pool...");

    VkCommandPoolCreateInfo pool_info = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = vkstate.graphics_queue_index.family_index;

    if(vkCreateCommandPool(vkstate.device, &pool_info, 0, &vkstate.commando_pool) != VK_SUCCESS) {
        REXFATAL("failed to create commando pool!");
        return false;
    }

    return true;
}

b8 allocate_command_buffers() {
    REXDEBUG("Allocating command buffers..");

    VkCommandBufferAllocateInfo command_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    command_info.commandPool = vkstate.commando_pool;
    command_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_info.commandBufferCount = 1;

    if(vkAllocateCommandBuffers(vkstate.device, &command_info, &vkstate.command_buffer) != VK_SUCCESS) {
        REXFATAL("failed to allocate command buffers!");
        return false;
    }

    return true;
}

b8 record_command_buffer() {
    VkCommandBufferBeginInfo command_begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

    if(vkBeginCommandBuffer(vkstate.command_buffer, &command_begin_info) != VK_SUCCESS) {
        REXFATAL("failed to start command buffer!");
        return false;
    }

    VkRenderPassBeginInfo renderpass_info = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    renderpass_info.renderPass = vkstate.render_pass;
    renderpass_info.framebuffer = vkstate.framebuffers[vkstate.image_index];
    renderpass_info.renderArea.offset = (VkOffset2D){0, 0};
    renderpass_info.renderArea.extent.width = vkstate.framebuffer_width;
    renderpass_info.renderArea.extent.height = vkstate.framebuffer_height;
    
    VkClearValue clear_color = {{{0.0f, 0.0f, 0.1f, 1.0f}}};
    renderpass_info.clearValueCount = 1;
    renderpass_info.pClearValues = &clear_color;

    vkCmdBeginRenderPass(vkstate.command_buffer, &renderpass_info, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(vkstate.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkstate.graphics_pipeline);

    VkViewport viewport = {0};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (f32)vkstate.framebuffer_width;
    viewport.height = (f32)vkstate.framebuffer_height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(vkstate.command_buffer, 0, 1, &viewport);

    VkRect2D scissor = {0};
    scissor.offset = (VkOffset2D){0, 0};
    scissor.extent = (VkExtent2D){vkstate.framebuffer_width, vkstate.framebuffer_height};
    vkCmdSetScissor(vkstate.command_buffer, 0, 1, &scissor);

    vkCmdDraw(vkstate.command_buffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(vkstate.command_buffer);

    if(vkEndCommandBuffer(vkstate.command_buffer) != VK_SUCCESS) {
        REXFATAL("failed to finish command buffer!");
        return false;
    }

    return true;
}

b8 create_sync_objects() {
    VkSemaphoreCreateInfo semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

    VkFenceCreateInfo fence_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateSemaphore(vkstate.device, &semaphore_info, 0, &vkstate.image_available_semaphore) != VK_SUCCESS ||
        vkCreateSemaphore(vkstate.device, &semaphore_info, 0, &vkstate.render_finished_semaphore) != VK_SUCCESS ||
        vkCreateFence(vkstate.device, &fence_info, 0, &vkstate.in_flight_fence) != VK_SUCCESS) {

        REXFATAL("failed to create sync objects!");
        return false;
    }

    return true;
}

b8 init_vulkan() {
    REXDEBUG("Starting vulkan renderer...");

    if(!create_instance()) return false;
    if(!setup_debug_messenger()) return false;
    if(!create_surface()) return false;
    if(!pick_physical_device()) return false;
    if(!create_logical_device()) return false;
    if(!create_swapchain()) return false;
    if(!create_render_pass()) return false;
    if(!create_graphics_pipeline()) return false;
    if(!create_framebuffers()) return false;
    if(!create_command_pool()) return false;
    if(!allocate_command_buffers()) return false;
    if(!create_sync_objects()) return false;

    REXINFO("Vulkan renderer started successfully");
    return true;
}

void draw_frame() {
    vkWaitForFences(vkstate.device, 1, &vkstate.in_flight_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(vkstate.device, 1, &vkstate.in_flight_fence);

    vkDeviceWaitIdle(vkstate.device);

    vkAcquireNextImageKHR(vkstate.device, vkstate.swapchain, UINT64_MAX, 
        vkstate.image_available_semaphore, 0, &vkstate.image_index);
    
    vkResetCommandBuffer(vkstate.command_buffer, 0);

    if(!record_command_buffer()) return;

    VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};

    VkSemaphore wait_semaphores[] = {vkstate.image_available_semaphore};
    VkSemaphore signal_semaphores[] = {vkstate.render_finished_semaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = waitStages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &vkstate.command_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    if(vkQueueSubmit(vkstate.graphics_queue, 1, &submit_info, vkstate.in_flight_fence) != VK_SUCCESS) {
        REXFATAL("failed to send queue!");
        return;
    }

    VkPresentInfoKHR present_info = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &vkstate.swapchain;
    present_info.pImageIndices = &vkstate.image_index;

    vkQueuePresentKHR(vkstate.present_queue, &present_info);
}

void loop() {
    platform_process_window_messages(&window);
    draw_frame();
}

void cleanup () {
    vkDeviceWaitIdle(vkstate.device);

    vkDestroySemaphore(vkstate.device, vkstate.image_available_semaphore, 0);
    vkDestroySemaphore(vkstate.device, vkstate.render_finished_semaphore, 0);
    vkDestroyFence(vkstate.device, vkstate.in_flight_fence, 0);

    vkDestroyCommandPool(vkstate.device, vkstate.commando_pool, 0);

    for (u32 i = 0; i < vkstate.image_count; i++) vkDestroyFramebuffer(vkstate.device, vkstate.framebuffers[i], 0);
    free(vkstate.framebuffers);

    vkDestroyPipeline(vkstate.device, vkstate.graphics_pipeline, 0);
    vkDestroyPipelineLayout(vkstate.device, vkstate.pipeline_layout, 0);
    vkDestroyRenderPass(vkstate.device, vkstate.render_pass, 0);

    for (u32 i = 0; i < vkstate.image_count; i++) vkDestroyImageView(vkstate.device, vkstate.swapchain_image_views[i], 0);
    free(vkstate.swapchain_image_views);

    vkDestroySwapchainKHR(vkstate.device, vkstate.swapchain, 0);
    vkDestroyDevice(vkstate.device, 0);
    destroy_swapchain_support(&vkstate.swapchain_support);
    vkDestroySurfaceKHR(vkstate.instance, vkstate.surface, 0);

    PFN_vkDestroyDebugUtilsMessengerEXT func = 
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vkstate.instance, "vkDestroyDebugUtilsMessengerEXT");
    func(vkstate.instance, vkstate.debug_messenger, 0);

    vkDestroyInstance(vkstate.instance, 0);

    platform_destroy_window(&window);
}

b8 close_event(u16 code, void* sender, EventContext data) {
    REXINFO("close event!");
    running = false;
    return false;
}

int main() {
    logger_initialize();
    event_initialize();

    event_register(EVENT_CODE_APPLICATION_QUIT, 0, close_event);

    platform_create_window("Triangle", 200, 200, 1280, 720, &window);

    platform_show_window(&window);

    vkstate.framebuffer_width = 1280;
    vkstate.framebuffer_height = 720;

    if (!init_vulkan())
    {
        REXFATAL("failed to initilize vulkan renderer!");
        running = false;
    }

    while (running)
    {
        loop();
    }

    cleanup();

    return 0;
}