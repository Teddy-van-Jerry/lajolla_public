#pragma once

// The simplest volumetric renderer:
// single absorption only homogeneous volume
// only handle directly visible light sources
Spectrum vol_path_tracing_1(const Scene& scene, int x, int y, /* pixel coordinates */
                            pcg32_state& rng) {
    // Homework 2: Wuqiong Zhao's implementation
    auto w = scene.camera.width;
    auto h = scene.camera.height;
    Vector2 screen_pos{ (x + next_pcg32_real<Real>(rng)) / w, (y + next_pcg32_real<Real>(rng)) / h };
    Ray ray = sample_primary(scene.camera, screen_pos);

    RayDifferential ray_diff{ Real(0), Real(0) };

    std::optional<PathVertex> vertex = intersect(scene, ray, ray_diff);
    if (!vertex) return make_zero_spectrum();
    Real t_hit = distance(vertex->position, ray.org);

    if (auto medium_id = scene.camera.medium_id; medium_id >= 0) {
        auto&& medium          = scene.media[medium_id];
        Spectrum sigma_a       = get_sigma_a(medium, ray.org);
        Spectrum transmittance = exp(-sigma_a * t_hit);
        if (is_light(scene.shapes[vertex->shape_id])) {
            Spectrum Le = emission(*vertex, -ray.dir, scene);
            return transmittance * Le;
        } else return make_zero_spectrum();
    } else return make_zero_spectrum();
}

// The second simplest volumetric renderer:
// single monochromatic homogeneous volume with single scattering,
// no need to handle surface lighting, only directly visible light source
Spectrum vol_path_tracing_2(const Scene& scene, int x, int y, /* pixel coordinates */
                            pcg32_state& rng) {
    // Homework 2: implememt this!
    return make_zero_spectrum();
}

// The third volumetric renderer (not so simple anymore):
// multiple monochromatic homogeneous volumes with multiple scattering
// no need to handle surface lighting, only directly visible light source
Spectrum vol_path_tracing_3(const Scene& scene, int x, int y, /* pixel coordinates */
                            pcg32_state& rng) {
    // Homework 2: implememt this!
    return make_zero_spectrum();
}

// The fourth volumetric renderer:
// multiple monochromatic homogeneous volumes with multiple scattering
// with MIS between next event estimation and phase function sampling
// still no surface lighting
Spectrum vol_path_tracing_4(const Scene& scene, int x, int y, /* pixel coordinates */
                            pcg32_state& rng) {
    // Homework 2: implememt this!
    return make_zero_spectrum();
}

// The fifth volumetric renderer:
// multiple monochromatic homogeneous volumes with multiple scattering
// with MIS between next event estimation and phase function sampling
// with surface lighting
Spectrum vol_path_tracing_5(const Scene& scene, int x, int y, /* pixel coordinates */
                            pcg32_state& rng) {
    // Homework 2: implememt this!
    return make_zero_spectrum();
}

// The final volumetric renderer:
// multiple chromatic heterogeneous volumes with multiple scattering
// with MIS between next event estimation and phase function sampling
// with surface lighting
Spectrum vol_path_tracing(const Scene& scene, int x, int y, /* pixel coordinates */
                          pcg32_state& rng) {
    // Homework 2: implememt this!
    return make_zero_spectrum();
}
