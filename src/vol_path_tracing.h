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
    // Homework 2: Wuqiong Zhao's implementation
    auto w = scene.camera.width;
    auto h = scene.camera.height;
    Vector2 screen_pos{ (x + next_pcg32_real<Real>(rng)) / w, (y + next_pcg32_real<Real>(rng)) / h };
    Ray ray = sample_primary(scene.camera, screen_pos);

    RayDifferential ray_diff{ Real(0), Real(0) };

    std::optional<PathVertex> vertex = intersect(scene, ray, ray_diff);

    // Scattering for ray with no hit should also be considered!
    Real t_hit = vertex ? distance(vertex->position, ray.org) : infinity<Real>();

    if (auto medium_id = scene.camera.medium_id; medium_id >= 0) {
        auto&& medium    = scene.media[medium_id];
        Spectrum sigma_s = get_sigma_s(medium, ray.org);
        Spectrum sigma_a = get_sigma_a(medium, ray.org);
        Spectrum sigma_t = sigma_s + sigma_a;

        // Compute L_s1 using Monte Carlo simulation
        auto L_s1 = [&](const Vector3& p, const Vector3& dir_view) -> std::pair<Spectrum, Real> {
            // Sample light
            int light_id       = sample_light(scene, next_pcg32_real<Real>(rng));
            const Light& light = scene.lights[light_id];
            Vector2 light_uv{ next_pcg32_real<Real>(rng), next_pcg32_real<Real>(rng) };
            Real light_w = next_pcg32_real<Real>(rng);

            PointAndNormal point_on_light = sample_point_on_light(light, p, light_uv, light_w, scene);

            Vector3 dir_to_light = normalize(point_on_light.position - p);
            Real dist_to_light   = distance(point_on_light.position, p);

            PhaseFunction phase = get_phase_function(medium);
            Spectrum phase_val  = eval(phase, dir_view, dir_to_light);

            Spectrum Le = emission(light, -dir_to_light, Real(0), point_on_light, scene);
            Real G      = fabs(dot(dir_to_light, point_on_light.normal)) / (dist_to_light * dist_to_light);

            Spectrum transmittance_to_light = exp(-sigma_t * dist_to_light);

            Real light_pdf = light_pmf(scene, light_id) * pdf_point_on_light(light, point_on_light, p, scene);

            return { phase_val * transmittance_to_light * Le * G, light_pdf };
        };

        if (Real sampled_t = -log(1 - next_pcg32_real<Real>(rng)) / sigma_t[0]; sampled_t < t_hit) {
            Spectrum transmittance         = exp(-sigma_t * sampled_t);
            Real trans_pdf                 = sigma_t[0] * exp(-sigma_t[0] * sampled_t);
            Vector3 p                      = ray.org + sampled_t * ray.dir;
            auto [L_s1_estimate, L_s1_pdf] = L_s1(p, -ray.dir);
            return (transmittance / trans_pdf) * sigma_s * (L_s1_estimate / L_s1_pdf);
        } else {
            // Direct visibility case
            Spectrum transmittance = exp(-sigma_t * t_hit);
            Real trans_pdf         = exp(-sigma_t[0] * t_hit);

            if (is_light(scene.shapes[vertex->shape_id])) {
                Spectrum Le = emission(*vertex, -ray.dir, scene);
                return (transmittance / trans_pdf) * Le;
            }
            return make_zero_spectrum();
        }
    }
    return make_zero_spectrum();
}

// The third volumetric renderer (not so simple anymore):
// multiple monochromatic homogeneous volumes with multiple scattering
// no need to handle surface lighting, only directly visible light source
Spectrum vol_path_tracing_3(const Scene& scene, int x, int y, /* pixel coordinates */
                            pcg32_state& rng) {
    // Homework 2: Wuqiong Zhao's implementation
    auto w = scene.camera.width;
    auto h = scene.camera.height;
    Vector2 screen_pos{ (x + next_pcg32_real<Real>(rng)) / w, (y + next_pcg32_real<Real>(rng)) / h };
    Ray ray = sample_primary(scene.camera, screen_pos);
    RayDifferential ray_diff{ Real(0), Real(0) };

    Spectrum current_path_throughput = fromRGB(Vector3{ 1, 1, 1 });
    Spectrum radiance                = make_zero_spectrum();
    int bounces                      = 0;
    int current_medium_id            = scene.camera.medium_id;

    while (true) {
        bool scatter                     = false;
        std::optional<PathVertex> vertex = intersect(scene, ray, ray_diff);
        Real t_hit                       = vertex ? distance(vertex->position, ray.org) : infinity<Real>();

        Spectrum transmittance = fromRGB(Vector3{ 1, 1, 1 });
        Real trans_pdf         = 1;

        if (current_medium_id >= 0) {
            const Medium& medium = scene.media[current_medium_id];
            Spectrum sigma_s     = get_sigma_s(medium, ray.org);
            Spectrum sigma_a     = get_sigma_a(medium, ray.org);
            Spectrum sigma_t     = sigma_s + sigma_a;

            // Sample distance based on extinction coefficient
            if (Real sampled_t = -log(1 - next_pcg32_real<Real>(rng)) / sigma_t[0]; sampled_t < t_hit) {
                scatter       = true;
                transmittance = exp(-sigma_t * sampled_t);
                trans_pdf     = sigma_t[0] * exp(-sigma_t[0] * sampled_t);
                ray.org       = ray.org + sampled_t * ray.dir;
            } else {
                transmittance = exp(-sigma_t * t_hit);
                trans_pdf     = exp(-sigma_t[0] * t_hit);
                ray.org       = ray.org + t_hit * ray.dir;
            }
        }

        // Update path throughput
        current_path_throughput *= (transmittance / trans_pdf);

        // Handle surface intersection or scattering
        if (!scatter) {
            if (vertex && is_light(scene.shapes[vertex->shape_id])) {
                radiance += current_path_throughput * emission(*vertex, -ray.dir, scene);
            }
        }

        if (scene.options.max_depth != -1 && bounces >= scene.options.max_depth - 1) break;

        // Index-matching interface
        if (!scatter && vertex) {
            if (vertex->material_id == -1) {
                current_medium_id = (dot(ray.dir, vertex->geometric_normal) > 0) ? vertex->exterior_medium_id
                                                                                 : vertex->interior_medium_id;
                ++bounces;
                continue;
            }
            break; // hit non-index-matched surface
        }

        // Sample next direct & update path throughput
        if (scatter) {
            auto&& medium       = scene.media[current_medium_id];
            PhaseFunction phase = get_phase_function(medium);
            auto next_dir       = sample_phase_function(phase, -ray.dir,
                                                        Vector2{ next_pcg32_real<Real>(rng), next_pcg32_real<Real>(rng) });
            if (!next_dir) break;

            Real phase_pdf     = pdf_sample_phase(phase, -ray.dir, *next_dir);
            Spectrum phase_val = eval(phase, -ray.dir, *next_dir);
            Spectrum sigma_s   = get_sigma_s(medium, ray.org);

            current_path_throughput *= (phase_val / phase_pdf) * sigma_s;
            ray.dir = *next_dir;
        }

        // Russian roulette
        if (Real rr_prob = 1; bounces >= scene.options.rr_depth) {
            rr_prob = min(max(luminance(current_path_throughput), Real(0.0)), Real(0.95));
            if (next_pcg32_real<Real>(rng) > rr_prob) break;
            current_path_throughput /= rr_prob;
        }

        ++bounces;
    }

    return radiance;
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
