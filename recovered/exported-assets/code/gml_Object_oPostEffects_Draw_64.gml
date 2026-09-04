var blur_steps = round(5.25) + 1;
var sigma = max(0.2, 0.0001);
var bloom_threshold = 0.5;
var bloom_range = 0.1;
var bloom_intensity = 1;
var bloom_darken = 1;
var bloom_saturation = 1;
if (!surface_exists(srf_ping))
{
    srf_ping = surface_create(app_w, app_h);
    bloom_texture = surface_get_texture(srf_ping);
}
if (!surface_exists(srf_pong))
{
    srf_pong = surface_create(app_w, app_h);
}
shader_set(shader_bloom_lum);
shader_set_uniform_f(u_bloom_threshold, bloom_threshold);
shader_set_uniform_f(u_bloom_range, bloom_range);
surface_set_target(srf_ping);
draw_surface(application_surface, 0, 0);
surface_reset_target();
gpu_set_tex_filter(1);
shader_set(shader_blur);
shader_set_uniform_f(u_blur_steps, blur_steps);
shader_set_uniform_f(u_sigma, sigma);
shader_set_uniform_f(u_blur_vector, 1, 0);
shader_set_uniform_f(u_texel_size, texel_w, texel_h);
surface_set_target(srf_pong);
draw_surface(srf_ping, 0, 0);
surface_reset_target();
shader_set_uniform_f(u_blur_vector, 0, 1);
surface_set_target(srf_ping);
draw_surface(srf_pong, 0, 0);
surface_reset_target();
gpu_set_tex_filter(0);
shader_set(shader_bloom_blend);
shader_set_uniform_f(u_bloom_intensity, bloom_intensity);
shader_set_uniform_f(u_bloom_darken, bloom_darken);
shader_set_uniform_f(u_bloom_saturation, bloom_saturation);
texture_set_stage(u_bloom_texture, bloom_texture);
gpu_set_tex_filter_ext(u_bloom_texture, 1);
draw_surface(application_surface, 0, 0);
shader_reset();
