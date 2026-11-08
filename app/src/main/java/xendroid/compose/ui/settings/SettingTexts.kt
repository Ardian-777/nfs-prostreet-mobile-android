package xendroid.compose.ui.settings

import android.content.Context
import androidx.compose.runtime.Composable
import androidx.compose.ui.res.stringResource
import xendroid.compose.R
import xendroid.compose.settings.Setting
import xendroid.compose.settings.SettingApply
import xendroid.compose.settings.SettingContract
import xendroid.compose.settings.SettingsCategory

/**
 * U02: the shown names of what players see in Settings: the Player-mode settings (title and
 * description), the category names and the save/apply note. The engine's own settings, shown
 * only in Developer mode, keep their English names, like the cvars they are.
 */
private val TITLES: Map<String, Int> = mapOf(
    "GPU|framerate_limit" to R.string.set_framerate_limit,
    "GPU|guest_display_refresh_cap" to R.string.set_refresh_cap,
    "GPU|draw_resolution_scale_x" to R.string.set_scale_x,
    "GPU|draw_resolution_scale_y" to R.string.set_scale_y,
    "Display|postprocess_scaling_and_sharpening" to R.string.set_scaling,
    "Display|postprocess_antialiasing" to R.string.set_antialiasing,
    "Display|present_letterbox" to R.string.set_letterbox,
    "Display|postprocess_dither" to R.string.set_dither,
    "GPU|anisotropic_override" to R.string.set_anisotropic,
    "Console|widescreen" to R.string.set_widescreen,
    "Console|internal_display_resolution" to R.string.set_display_mode,
    "Vulkan|vulkan_lib_path" to R.string.set_driver,
    "HID|show_touch_overlay" to R.string.set_touch_overlay,
    "HID|left_stick_deadzone_percentage" to R.string.set_left_deadzone,
    "HID|right_stick_deadzone_percentage" to R.string.set_right_deadzone,
    "UI|android_soft_keyboard" to R.string.set_soft_keyboard,
    "UI|android_message_box" to R.string.set_message_box,
    "UI|show_achievement_notification" to R.string.set_achievements,
    "Console|user_language" to R.string.set_language,
    "Console|user_country" to R.string.set_country,
    "General|apply_patches" to R.string.set_apply_patches,
    "Logging|dump_session_logs" to R.string.set_export_logs,
    // Redesign: the settings the new screens show translated, the new cvars among them.
    "Video|internal_display_resolution_x" to R.string.st_t_video_internal_display_resolution_x,
    "Video|internal_display_resolution_y" to R.string.st_t_video_internal_display_resolution_y,
    "GPU|draw_resolution_scale_threshold" to R.string.st_t_gpu_draw_resolution_scale_threshold,
    "Display|postprocess_ffx_cas_additional_sharpness" to R.string.st_t_display_postprocess_ffx_cas_additional_sharpness,
    "Display|postprocess_ffx_fsr_sharpness_reduction" to R.string.st_t_display_postprocess_ffx_fsr_sharpness_reduction,
    "Display|postprocess_ffx_fsr_max_upsampling_passes" to R.string.st_t_display_postprocess_ffx_fsr_max_upsampling_passes,
    "Kernel|kernel_display_gamma_type" to R.string.st_t_kernel_kernel_display_gamma_type,
    "Kernel|kernel_display_gamma_power" to R.string.st_t_kernel_kernel_display_gamma_power,
    "Display|present_safe_area_x" to R.string.st_t_display_present_safe_area_x,
    "Display|present_safe_area_y" to R.string.st_t_display_present_safe_area_y,
    "GPU|async_shader_compilation" to R.string.st_t_gpu_async_shader_compilation,
    "GPU|async_shader_vs_interpreter" to R.string.st_t_gpu_async_shader_vs_interpreter,
    "GPU|async_shader_skip_draws" to R.string.st_t_gpu_async_shader_skip_draws,
    "GPU|pipeline_storage_precreate" to R.string.st_t_gpu_pipeline_storage_precreate,
    "GPU|store_shaders" to R.string.st_t_gpu_store_shaders,
    "Kernel|precise_guest_delays" to R.string.st_t_kernel_precise_guest_delays,
    "APU|apu_performance_hint" to R.string.st_t_apu_apu_performance_hint,
    "Vulkan|adrenotools_force_max_clocks" to R.string.st_t_vulkan_adrenotools_force_max_clocks,
    "Vulkan|adrenotools_turbo_reassert_seconds" to R.string.st_t_vulkan_adrenotools_turbo_reassert_seconds,
    "Vulkan|vulkan_pipeline_creation_threads" to R.string.st_t_vulkan_vulkan_pipeline_creation_threads,
    "GPU|vulkan_mid_frame_submission_draws" to R.string.st_t_gpu_vulkan_mid_frame_submission_draws,
    "Kernel|guest_scheduler" to R.string.st_t_kernel_guest_scheduler,
    "APU|volume" to R.string.st_t_apu_volume,
    "APU|apu_aaudio_buffer_bursts" to R.string.st_t_apu_apu_aaudio_buffer_bursts,
    "APU|apu_aaudio_adaptive_buffer" to R.string.st_t_apu_apu_aaudio_adaptive_buffer,
    "APU|xma_decoder" to R.string.st_t_apu_xma_decoder,
    "HID|vibration" to R.string.st_t_hid_vibration,
    "HID|guide_button" to R.string.st_t_hid_guide_button,
    "GPU|clear_memory_page_state" to R.string.st_t_gpu_clear_memory_page_state,
    "GPU|depth_float24_convert_in_pixel_shader" to R.string.st_t_gpu_depth_float24_convert_in_pixel_shader,
    "GPU|occlusion_query" to R.string.st_t_gpu_occlusion_query,
    "GPU|readback_resolve_sync" to R.string.st_t_gpu_readback_resolve_sync,
    "GPU|precise_interpolation" to R.string.st_t_gpu_precise_interpolation,
    "GPU|depth_bias_shader_offset" to R.string.st_t_gpu_depth_bias_shader_offset,
    "GPU|readback_resolve" to R.string.st_t_gpu_readback_resolve,
    "GPU|memexport_enable" to R.string.st_t_gpu_memexport_enable,
    "Vulkan|vulkan_allow_reverse_z" to R.string.st_t_vulkan_vulkan_allow_reverse_z,
    "Kernel|stack_size_multiplier_hack" to R.string.st_t_kernel_stack_size_multiplier_hack,
    "CPU|collapse_memory_delay_spins" to R.string.st_t_cpu_collapse_memory_delay_spins,
    "Vulkan|turnip_debug" to R.string.st_t_vulkan_turnip_debug,
    "Vulkan|vulkan_dynamic_rendering" to R.string.st_t_vulkan_vulkan_dynamic_rendering,
    "Vulkan|vulkan_avoid_geometry_shaders" to R.string.st_t_vulkan_vulkan_avoid_geometry_shaders,
    "Vulkan|vulkan_depth_unorm24" to R.string.st_t_vulkan_vulkan_depth_unorm24,
    "UI|achievement_notification_position_by_game" to R.string.st_t_ui_achievement_notification_position_by_game,
    "General|launch_module" to R.string.st_t_general_launch_module,
    "Storage|mount_memory_unit" to R.string.st_t_storage_mount_memory_unit,
    "Kernel|console_type" to R.string.st_t_kernel_console_type,
    "Kernel|cl" to R.string.st_t_kernel_cl,
    "General|guest_crash_is_fatal" to R.string.st_t_general_guest_crash_is_fatal,
    "Kernel|apply_title_update" to R.string.st_t_kernel_apply_title_update,
    "Kernel|allow_incompatible_title_update" to R.string.st_t_kernel_allow_incompatible_title_update,
    "Kernel|network_enabled" to R.string.st_t_kernel_network_enabled,
    "Network|systemlink" to R.string.st_t_network_systemlink,
    "Network|systemlink_peers" to R.string.st_t_network_systemlink_peers,
    "Content|license_mask" to R.string.st_t_content_license_mask,
    "Console|video_standard" to R.string.st_t_console_video_standard,
    "Console|use_50Hz_mode" to R.string.st_t_console_use_50hz_mode,
    "Logging|log_level" to R.string.st_t_logging_log_level,
    "Logging|log_mask" to R.string.st_t_logging_log_mask,
    "Display|show_debug_overlay" to R.string.st_t_display_show_debug_overlay,
    "Logging|log_sessions_keep" to R.string.st_t_logging_log_sessions_keep,
)

private val DESCRIPTIONS: Map<String, Int> = mapOf(
    "GPU|framerate_limit" to R.string.set_framerate_limit_desc,
    "GPU|guest_display_refresh_cap" to R.string.set_refresh_cap_desc,
    "GPU|draw_resolution_scale_x" to R.string.set_scale_x_desc,
    "GPU|draw_resolution_scale_y" to R.string.set_scale_y_desc,
    "Display|postprocess_scaling_and_sharpening" to R.string.set_scaling_desc,
    "Display|postprocess_antialiasing" to R.string.set_antialiasing_desc,
    "Display|present_letterbox" to R.string.set_letterbox_desc,
    "Display|postprocess_dither" to R.string.set_dither_desc,
    "GPU|anisotropic_override" to R.string.set_anisotropic_desc,
    "Console|widescreen" to R.string.set_widescreen_desc,
    "Console|internal_display_resolution" to R.string.set_display_mode_desc,
    "Vulkan|vulkan_lib_path" to R.string.set_driver_desc,
    "HID|show_touch_overlay" to R.string.set_touch_overlay_desc,
    "HID|left_stick_deadzone_percentage" to R.string.set_left_deadzone_desc,
    "HID|right_stick_deadzone_percentage" to R.string.set_right_deadzone_desc,
    "UI|android_soft_keyboard" to R.string.set_soft_keyboard_desc,
    "UI|android_message_box" to R.string.set_message_box_desc,
    "UI|show_achievement_notification" to R.string.set_achievements_desc,
    "Console|user_language" to R.string.set_language_desc,
    "Console|user_country" to R.string.set_country_desc,
    "General|apply_patches" to R.string.set_apply_patches_desc,
    "Logging|dump_session_logs" to R.string.set_export_logs_desc,
    "Video|internal_display_resolution_x" to R.string.st_d_video_internal_display_resolution_x,
    "Video|internal_display_resolution_y" to R.string.st_d_video_internal_display_resolution_y,
    "GPU|draw_resolution_scale_threshold" to R.string.st_d_gpu_draw_resolution_scale_threshold,
    "Display|postprocess_ffx_cas_additional_sharpness" to R.string.st_d_display_postprocess_ffx_cas_additional_sharpness,
    "Display|postprocess_ffx_fsr_sharpness_reduction" to R.string.st_d_display_postprocess_ffx_fsr_sharpness_reduction,
    "Display|postprocess_ffx_fsr_max_upsampling_passes" to R.string.st_d_display_postprocess_ffx_fsr_max_upsampling_passes,
    "Kernel|kernel_display_gamma_type" to R.string.st_d_kernel_kernel_display_gamma_type,
    "Kernel|kernel_display_gamma_power" to R.string.st_d_kernel_kernel_display_gamma_power,
    "Display|present_safe_area_x" to R.string.st_d_display_present_safe_area_x,
    "Display|present_safe_area_y" to R.string.st_d_display_present_safe_area_y,
    "GPU|async_shader_compilation" to R.string.st_d_gpu_async_shader_compilation,
    "GPU|async_shader_vs_interpreter" to R.string.st_d_gpu_async_shader_vs_interpreter,
    "GPU|async_shader_skip_draws" to R.string.st_d_gpu_async_shader_skip_draws,
    "GPU|pipeline_storage_precreate" to R.string.st_d_gpu_pipeline_storage_precreate,
    "GPU|store_shaders" to R.string.st_d_gpu_store_shaders,
    "Kernel|precise_guest_delays" to R.string.st_d_kernel_precise_guest_delays,
    "APU|apu_performance_hint" to R.string.st_d_apu_apu_performance_hint,
    "Vulkan|adrenotools_force_max_clocks" to R.string.st_d_vulkan_adrenotools_force_max_clocks,
    "Vulkan|adrenotools_turbo_reassert_seconds" to R.string.st_d_vulkan_adrenotools_turbo_reassert_seconds,
    "Vulkan|vulkan_pipeline_creation_threads" to R.string.st_d_vulkan_vulkan_pipeline_creation_threads,
    "GPU|vulkan_mid_frame_submission_draws" to R.string.st_d_gpu_vulkan_mid_frame_submission_draws,
    "Kernel|guest_scheduler" to R.string.st_d_kernel_guest_scheduler,
    "APU|volume" to R.string.st_d_apu_volume,
    "APU|apu_aaudio_buffer_bursts" to R.string.st_d_apu_apu_aaudio_buffer_bursts,
    "APU|apu_aaudio_adaptive_buffer" to R.string.st_d_apu_apu_aaudio_adaptive_buffer,
    "APU|xma_decoder" to R.string.st_d_apu_xma_decoder,
    "HID|vibration" to R.string.st_d_hid_vibration,
    "HID|guide_button" to R.string.st_d_hid_guide_button,
    "GPU|clear_memory_page_state" to R.string.st_d_gpu_clear_memory_page_state,
    "GPU|depth_float24_convert_in_pixel_shader" to R.string.st_d_gpu_depth_float24_convert_in_pixel_shader,
    "GPU|occlusion_query" to R.string.st_d_gpu_occlusion_query,
    "GPU|readback_resolve_sync" to R.string.st_d_gpu_readback_resolve_sync,
    "GPU|precise_interpolation" to R.string.st_d_gpu_precise_interpolation,
    "GPU|depth_bias_shader_offset" to R.string.st_d_gpu_depth_bias_shader_offset,
    "GPU|readback_resolve" to R.string.st_d_gpu_readback_resolve,
    "GPU|memexport_enable" to R.string.st_d_gpu_memexport_enable,
    "Vulkan|vulkan_allow_reverse_z" to R.string.st_d_vulkan_vulkan_allow_reverse_z,
    "Kernel|stack_size_multiplier_hack" to R.string.st_d_kernel_stack_size_multiplier_hack,
    "CPU|collapse_memory_delay_spins" to R.string.st_d_cpu_collapse_memory_delay_spins,
    "Vulkan|turnip_debug" to R.string.st_d_vulkan_turnip_debug,
    "Vulkan|vulkan_dynamic_rendering" to R.string.st_d_vulkan_vulkan_dynamic_rendering,
    "Vulkan|vulkan_avoid_geometry_shaders" to R.string.st_d_vulkan_vulkan_avoid_geometry_shaders,
    "Vulkan|vulkan_depth_unorm24" to R.string.st_d_vulkan_vulkan_depth_unorm24,
    "UI|achievement_notification_position_by_game" to R.string.st_d_ui_achievement_notification_position_by_game,
    "General|launch_module" to R.string.st_d_general_launch_module,
    "Storage|mount_memory_unit" to R.string.st_d_storage_mount_memory_unit,
    "Kernel|console_type" to R.string.st_d_kernel_console_type,
    "Kernel|cl" to R.string.st_d_kernel_cl,
    "General|guest_crash_is_fatal" to R.string.st_d_general_guest_crash_is_fatal,
    "Kernel|apply_title_update" to R.string.st_d_kernel_apply_title_update,
    "Kernel|allow_incompatible_title_update" to R.string.st_d_kernel_allow_incompatible_title_update,
    "Kernel|network_enabled" to R.string.st_d_kernel_network_enabled,
    "Network|systemlink" to R.string.st_d_network_systemlink,
    "Network|systemlink_peers" to R.string.st_d_network_systemlink_peers,
    "Content|license_mask" to R.string.st_d_content_license_mask,
    "Console|video_standard" to R.string.st_d_console_video_standard,
    "Console|use_50Hz_mode" to R.string.st_d_console_use_50hz_mode,
    "Logging|log_level" to R.string.st_d_logging_log_level,
    "Logging|log_mask" to R.string.st_d_logging_log_mask,
    "Display|show_debug_overlay" to R.string.st_d_display_show_debug_overlay,
    "Logging|log_sessions_keep" to R.string.st_d_logging_log_sessions_keep,
)

private val CATEGORIES: Map<String, Int> = mapOf(
    "Essentials" to R.string.set_cat_essentials, "Video" to R.string.set_cat_video, "Storage" to R.string.set_cat_storage,
    "Controller" to R.string.set_cat_controller, "Memory" to R.string.set_cat_memory, "Display" to R.string.set_cat_display,
    "Logging" to R.string.set_cat_logging, "Content" to R.string.set_cat_content, "General" to R.string.set_cat_general,
)

@Composable
fun settingTitle(s: Setting): String = TITLES[s.key]?.let { stringResource(it) } ?: s.title

@Composable
fun settingDesc(s: Setting): String = DESCRIPTIONS[s.key]?.let { stringResource(it) } ?: s.desc

/** For search: the shown title and description, outside composition. */
fun settingSearchText(context: Context, s: Setting): String =
    listOfNotNull(TITLES[s.key]?.let(context::getString), DESCRIPTIONS[s.key]?.let(context::getString)).joinToString(" ")

@Composable
fun categoryTitle(category: SettingsCategory): String = CATEGORIES[category.title]?.let { stringResource(it) } ?: category.title

/** A list option as shown; only words are translated (sizes, codes and filter names stay). */
@Composable
fun optionLabel(s: Setting.ListChoice, value: String, english: String): String = when {
    s.key == "GPU|framerate_limit" && value == "0" -> stringResource(R.string.set_fps_unlimited)
    s.key == "GPU|anisotropic_override" && value == "-1" -> stringResource(R.string.set_aniso_game)
    s.key == "GPU|anisotropic_override" && value == "0" -> stringResource(R.string.set_option_off)
    s.name.endsWith("_stick_deadzone_percentage") && value == "0.0" -> stringResource(R.string.set_option_off)
    else -> english
}

/** U03: what saving the setting does, in the shown language. */
@Composable
fun contractText(contract: SettingContract): String = when {
    !contract.available -> stringResource(R.string.set_driver_unavailable)
    contract.apply == SettingApply.IMMEDIATE_ACTION -> stringResource(R.string.set_contract_action)
    contract.scope == "globally" -> stringResource(R.string.set_contract_global)
    else -> stringResource(R.string.set_contract_game)
}
