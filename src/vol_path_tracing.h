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
            auto&& medium    = scene.media[current_medium_id];
            Spectrum sigma_s = get_sigma_s(medium, ray.org);
            Spectrum sigma_a = get_sigma_a(medium, ray.org);
            Spectrum sigma_t = sigma_s + sigma_a;

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
    // Homework 2: Wuqiong Zhao's implementation
    auto update_medium = [](const std::optional<PathVertex>& isect, const Ray& ray, int medium) -> int {
        if (!isect || isect->interior_medium_id == isect->exterior_medium_id) { return medium; }
        bool entering = dot(ray.dir, isect->geometric_normal) < 0;
        return entering ? isect->interior_medium_id : isect->exterior_medium_id;
    };

    auto next_event_estimation = [&](const Vector3& p, const Vector3& dir_view, int current_medium,
                                     int bounces) -> Spectrum {
        // Sample light
        int light_id       = sample_light(scene, next_pcg32_real<Real>(rng));
        const Light& light = scene.lights[light_id];
        Vector2 light_uv{ next_pcg32_real<Real>(rng), next_pcg32_real<Real>(rng) };
        Real light_w = next_pcg32_real<Real>(rng);

        PointAndNormal point_on_light = sample_point_on_light(light, p, light_uv, light_w, scene);
        Vector3 dir_light             = normalize(point_on_light.position - p);
        Real dist                     = distance(point_on_light.position, p);

        // Compute transmittance to light. Skip through index-matching shapes.
        Spectrum T_light     = fromRGB(Vector3{ 1, 1, 1 });
        int shadow_medium    = current_medium;
        int shadow_bounces   = 0;
        Spectrum p_trans_dir = fromRGB(Vector3{ 1, 1, 1 }); // for multiple importance sampling

        Vector3 curr_pos = p;
        while (true) {
            Ray shadow_ray{ curr_pos, dir_light, get_shadow_epsilon(scene), dist - distance(curr_pos, p) };
            std::optional<PathVertex> isect = intersect(scene, shadow_ray);
            Real next_t                     = isect ? distance(isect->position, shadow_ray.org) : shadow_ray.tfar;

            // Account for the transmittance to next_t
            if (shadow_medium >= 0) {
                auto&& medium    = scene.media[shadow_medium];
                Spectrum sigma_t = get_sigma_s(medium, shadow_ray.org) + get_sigma_a(medium, shadow_ray.org);
                T_light *= exp(-sigma_t * next_t);
                p_trans_dir *= exp(-sigma_t * next_t);
            }

            if (!isect) {
                // Nothing is blocking, we're done
                break;
            } else if (isect->material_id >= 0) {
                // Hit an opaque surface
                return make_zero_spectrum();
            }

            // Handle index-matching surface
            ++shadow_bounces;
            if (scene.options.max_depth != -1 && bounces + shadow_bounces + 1 >= scene.options.max_depth) {
                return make_zero_spectrum();
            }

            shadow_medium = update_medium(isect, shadow_ray, shadow_medium);
            curr_pos      = isect->position;
        }

        if (max(T_light) > 0) {
            auto&& medium       = scene.media[current_medium];
            PhaseFunction phase = get_phase_function(medium);
            Spectrum phase_val  = eval(phase, dir_view, dir_light);

            Real G      = fabs(dot(dir_light, point_on_light.normal)) / (dist * dist);
            Spectrum Le = emission(light, -dir_light, Real(0), point_on_light, scene);

            Real pdf_nee   = light_pmf(scene, light_id) * pdf_point_on_light(light, point_on_light, p, scene);
            Real pdf_phase = pdf_sample_phase(phase, dir_view, dir_light) * G;

            // Power heuristic
            Real w = (pdf_nee * pdf_nee) / (pdf_nee * pdf_nee + pdf_phase * pdf_phase);

            return T_light * G * phase_val * Le * (w / pdf_nee);
        }
        return make_zero_spectrum();
    };

    // Main path tracing loop
    auto w = scene.camera.width;
    auto h = scene.camera.height;
    Vector2 screen_pos{ (x + next_pcg32_real<Real>(rng)) / w, (y + next_pcg32_real<Real>(rng)) / h };
    Ray ray = sample_primary(scene.camera, screen_pos);
    RayDifferential ray_diff{ Real(0), Real(0) };

    Spectrum current_path_throughput = fromRGB(Vector3{ 1, 1, 1 });
    Spectrum radiance                = make_zero_spectrum();
    int bounces                      = 0;
    int current_medium               = scene.camera.medium_id;

    // For multiple importance sampling
    bool never_scatter = true;
    Real dir_pdf       = 0;   // in solid angle measure
    Vector3 nee_p_cache;      // last point that can issue NEE
    Real multi_trans_pdf = 1; // product of transmittance PDFs

    while (true) {
        bool scatter                    = false;
        std::optional<PathVertex> isect = intersect(scene, ray, ray_diff);
        Real t_hit                      = isect ? distance(isect->position, ray.org) : infinity<Real>();

        Spectrum transmittance = fromRGB(Vector3{ 1, 1, 1 });
        Real trans_pdf         = 1;

        if (current_medium >= 0) {
            auto&& medium    = scene.media[current_medium];
            Spectrum sigma_s = get_sigma_s(medium, ray.org);
            Spectrum sigma_a = get_sigma_a(medium, ray.org);
            Spectrum sigma_t = sigma_s + sigma_a;

            // Sample distance t
            Real sampled_t = -log(1 - next_pcg32_real<Real>(rng)) / sigma_t[0];

            if (sampled_t < t_hit) {
                scatter       = true;
                transmittance = exp(-sigma_t * sampled_t);
                trans_pdf     = sigma_t[0] * exp(-sigma_t[0] * sampled_t);
                ray.org += sampled_t * ray.dir;
                nee_p_cache = ray.org;
            } else {
                transmittance = exp(-sigma_t * t_hit);
                trans_pdf     = exp(-sigma_t[0] * t_hit);
                if (isect) ray.org = isect->position;
            }
            multi_trans_pdf *= trans_pdf;
        } else if (isect) {
            ray.org = isect->position;
        } else {
            break; // No medium and no intersection - terminate path
        }

        current_path_throughput *= transmittance / trans_pdf;

        // Next event estimation when we scatter
        if (scatter) {
            auto&& medium             = scene.media[current_medium];
            Spectrum sigma_s          = get_sigma_s(medium, ray.org);
            Spectrum nee_contribution = next_event_estimation(ray.org, -ray.dir, current_medium, bounces);
            radiance += current_path_throughput * sigma_s * nee_contribution;
        }

        // Handle surface hit
        if (!scatter && isect) {
            if (is_light(scene.shapes[isect->shape_id])) {
                if (never_scatter) {
                    // No MIS needed for direct visibility
                    radiance += current_path_throughput * emission(*isect, -ray.dir, scene);
                } else {
                    // Apply MIS with NEE
                    Real pdf_nee = pdf_point_on_light(scene.lights[get_area_light_id(scene.shapes[isect->shape_id])],
                                                      PointAndNormal{ isect->position, isect->geometric_normal },
                                                      nee_p_cache, scene);

                    Real dir_pdf_area = dir_pdf * multi_trans_pdf * fabs(dot(-ray.dir, isect->geometric_normal)) /
                                        distance_squared(nee_p_cache, isect->position);

                    Real w = (dir_pdf_area * dir_pdf_area) / (dir_pdf_area * dir_pdf_area + pdf_nee * pdf_nee);

                    radiance += current_path_throughput * emission(*isect, -ray.dir, scene) * w;
                }
            }

            current_medium = update_medium(isect, ray, current_medium);

            // For index-matched surfaces, continue the path
            if (isect->material_id == -1) {
                ++bounces;
                continue;
            }
            break; // hit non-index-matched surface
        }

        if (scene.options.max_depth != -1 && bounces >= scene.options.max_depth - 1) { break; }

        // Sample next direction
        if (scatter) {
            auto&& medium       = scene.media[current_medium];
            PhaseFunction phase = get_phase_function(medium);
            auto next_dir       = sample_phase_function(phase, -ray.dir,
                                                        Vector2{ next_pcg32_real<Real>(rng), next_pcg32_real<Real>(rng) });
            if (!next_dir) break;

            never_scatter      = false;
            dir_pdf            = pdf_sample_phase(phase, -ray.dir, *next_dir);
            Spectrum phase_val = eval(phase, -ray.dir, *next_dir);
            Spectrum sigma_s   = get_sigma_s(medium, ray.org);

            current_path_throughput *= (phase_val / dir_pdf) * sigma_s;
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

// The fifth volumetric renderer:
// multiple monochromatic homogeneous volumes with multiple scattering
// with MIS between next event estimation and phase function sampling
// with surface lighting
Spectrum vol_path_tracing_5(const Scene& scene, int x, int y, /* pixel coordinates */
                            pcg32_state& rng) {
    // Homework 2: Wuqiong Zhao's implementation
    //
    // The vol_cbox and vol_cbox_teapot scenes are somehow dark.
    auto update_medium = [](const std::optional<PathVertex>& isect, const Ray& ray, int medium) -> int {
        if (!isect || isect->interior_medium_id == isect->exterior_medium_id) return medium;
        bool entering = dot(ray.dir, isect->geometric_normal) < 0;
        return entering ? isect->interior_medium_id : isect->exterior_medium_id;
    };

    auto next_event_estimation = [&](const Vector3& p, const Vector3& dir_view, int current_medium, int bounces,
                                     const std::optional<PathVertex>& vertex) -> Spectrum {
        // Sample light
        int light_id       = sample_light(scene, next_pcg32_real<Real>(rng));
        const Light& light = scene.lights[light_id];
        Vector2 light_uv{ next_pcg32_real<Real>(rng), next_pcg32_real<Real>(rng) };
        Real light_w = next_pcg32_real<Real>(rng);

        PointAndNormal point_on_light = sample_point_on_light(light, p, light_uv, light_w, scene);
        Vector3 dir_light             = normalize(point_on_light.position - p);
        Real dist                     = distance(point_on_light.position, p);

        // Compute transmittance to light
        Spectrum T_light     = fromRGB(Vector3{ 1, 1, 1 });
        int shadow_medium    = current_medium;
        int shadow_bounces   = 0;
        Spectrum p_trans_dir = fromRGB(Vector3{ 1, 1, 1 }); // for multiple importance sampling

        Vector3 curr_pos = p;
        while (true) {
            Ray shadow_ray{ curr_pos, dir_light, get_shadow_epsilon(scene), dist - distance(curr_pos, p) };
            std::optional<PathVertex> isect = intersect(scene, shadow_ray);
            Real next_t                     = isect ? distance(isect->position, shadow_ray.org) : shadow_ray.tfar;

            if (shadow_medium >= 0) {
                auto&& medium    = scene.media[shadow_medium];
                Spectrum sigma_t = get_sigma_s(medium, shadow_ray.org) + get_sigma_a(medium, shadow_ray.org);
                T_light *= exp(-sigma_t * next_t);
                p_trans_dir *= exp(-sigma_t * next_t);
            }

            if (!isect) {
                // Nothing is blocking, we're done
                break;
            } else if (isect->material_id >= 0) {
                // Hit an opaque surface
                return make_zero_spectrum();
            }

            // Handle index-matching surface
            ++shadow_bounces;
            if (scene.options.max_depth != -1 && bounces + shadow_bounces + 1 >= scene.options.max_depth) {
                return make_zero_spectrum();
            }

            shadow_medium = update_medium(isect, shadow_ray, shadow_medium);
            curr_pos      = isect->position;
        }

        if (max(T_light) > 0) {
            Real G = fabs(dot(dir_light, point_on_light.normal)) / (dist * dist);
            Spectrum phase_val;
            Real pdf_phase;

            if (vertex && vertex->material_id >= 0) {
                // Surface interaction
                auto&& mat = scene.materials[vertex->material_id];
                phase_val  = eval(mat, dir_view, dir_light, *vertex, scene.texture_pool);
                pdf_phase  = pdf_sample_bsdf(mat, dir_view, dir_light, *vertex, scene.texture_pool) * G;
            } else if (current_medium >= 0) {
                // Volume interaction
                auto&& medium       = scene.media[current_medium];
                PhaseFunction phase = get_phase_function(medium);
                phase_val           = eval(phase, dir_view, dir_light);
                pdf_phase           = pdf_sample_phase(phase, dir_view, dir_light) * G;
            } else {
                return make_zero_spectrum();
            }

            Spectrum Le  = emission(light, -dir_light, Real(0), point_on_light, scene);
            Real pdf_nee = light_pmf(scene, light_id) * pdf_point_on_light(light, point_on_light, p, scene);

            Real w = (pdf_nee * pdf_nee) / (pdf_nee * pdf_nee + pdf_phase * pdf_phase);
            return T_light * G * phase_val * Le * (w / pdf_nee);
        }
        return make_zero_spectrum();
    };

    // Main path tracing loop
    auto w = scene.camera.width;
    auto h = scene.camera.height;
    Vector2 screen_pos{ (x + next_pcg32_real<Real>(rng)) / w, (y + next_pcg32_real<Real>(rng)) / h };
    Ray ray = sample_primary(scene.camera, screen_pos);
    RayDifferential ray_diff{ Real(0), Real(0) };

    Spectrum current_path_throughput = fromRGB(Vector3{ 1, 1, 1 });
    Spectrum radiance                = make_zero_spectrum();
    int bounces                      = 0;
    int current_medium               = scene.camera.medium_id;

    // For MIS
    bool never_scatter = true;
    Real dir_pdf       = 0;
    Vector3 nee_p_cache;
    Real multi_trans_pdf = 1;

    while (true) {
        bool scatter                    = false;
        std::optional<PathVertex> isect = intersect(scene, ray, ray_diff);
        Real t_hit                      = isect ? distance(isect->position, ray.org) : infinity<Real>();

        Spectrum transmittance = fromRGB(Vector3{ 1, 1, 1 });
        Real trans_pdf         = 1;

        if (current_medium >= 0) {
            auto&& medium    = scene.media[current_medium];
            Spectrum sigma_s = get_sigma_s(medium, ray.org);
            Spectrum sigma_a = get_sigma_a(medium, ray.org);
            Spectrum sigma_t = sigma_s + sigma_a;

            // Sample distance t
            Real sampled_t = -log(1 - next_pcg32_real<Real>(rng)) / sigma_t[0];

            if (sampled_t < t_hit) {
                scatter       = true;
                transmittance = exp(-sigma_t * sampled_t);
                trans_pdf     = sigma_t[0] * exp(-sigma_t[0] * sampled_t);
                ray.org += sampled_t * ray.dir;
                nee_p_cache = ray.org;
            } else {
                transmittance = exp(-sigma_t * t_hit);
                trans_pdf     = exp(-sigma_t[0] * t_hit);
                if (isect) ray.org = isect->position;
            }
            multi_trans_pdf *= trans_pdf;
        } else if (isect) {
            ray.org = isect->position;
        } else {
            break;
        }

        current_path_throughput *= transmittance / trans_pdf;

        // Next event estimation
        if ((scatter && current_medium >= 0) || (!scatter && isect && isect->material_id >= 0)) {
            Spectrum nee_contrib;
            if (scatter) {
                auto&& medium    = scene.media[current_medium];
                Spectrum sigma_s = get_sigma_s(medium, ray.org);
                nee_contrib      = sigma_s * next_event_estimation(ray.org, -ray.dir, current_medium, bounces, {});
            } else {
                nee_contrib = next_event_estimation(ray.org, -ray.dir, current_medium, bounces, isect);
            }
            radiance += current_path_throughput * nee_contrib;
        }

        // Handle surface hit
        if (!scatter && isect) {
            if (is_light(scene.shapes[isect->shape_id])) {
                if (never_scatter) {
                    radiance += current_path_throughput * emission(*isect, -ray.dir, scene);
                } else {
                    Real pdf_nee = pdf_point_on_light(scene.lights[get_area_light_id(scene.shapes[isect->shape_id])],
                                                      PointAndNormal{ isect->position, isect->geometric_normal },
                                                      nee_p_cache, scene);

                    Real G =
                        fabs(dot(-ray.dir, isect->geometric_normal)) / distance_squared(nee_p_cache, isect->position);
                    Real dir_pdf_area = dir_pdf * multi_trans_pdf * G;

                    Real w = (dir_pdf_area * dir_pdf_area) / (dir_pdf_area * dir_pdf_area + pdf_nee * pdf_nee);

                    radiance += current_path_throughput * emission(*isect, -ray.dir, scene) * w;
                }
            }

            if (isect->material_id == -1) {
                // Index-matching interface
                current_medium = update_medium(isect, ray, current_medium);
                ++bounces;
                if (scene.options.max_depth != -1 && bounces >= scene.options.max_depth - 1) break;
                continue;
            } else if (isect->material_id >= 0) {
                // Sample BSDF for non-index-matched surface
                const Material& mat = scene.materials[isect->material_id];
                Vector2 bsdf_rnd_param_uv{ next_pcg32_real<Real>(rng), next_pcg32_real<Real>(rng) };
                Real bsdf_rnd_param_w = next_pcg32_real<Real>(rng);

                auto bsdf_sample =
                    sample_bsdf(mat, -ray.dir, *isect, scene.texture_pool, bsdf_rnd_param_uv, bsdf_rnd_param_w);
                if (!bsdf_sample) break;

                never_scatter = false;
                // Keep track of whether we're doing reflection or refraction
                bool is_transmission = bsdf_sample->eta != 0;

                // For transmission events, handle eta scaling
                if (is_transmission) {
                    dir_pdf = pdf_sample_bsdf(mat, -ray.dir, bsdf_sample->dir_out, *isect, scene.texture_pool);
                    dir_pdf *= bsdf_sample->eta * bsdf_sample->eta;
                } else {
                    dir_pdf = pdf_sample_bsdf(mat, -ray.dir, bsdf_sample->dir_out, *isect, scene.texture_pool);
                }

                Spectrum bsdf_val = eval(mat, -ray.dir, bsdf_sample->dir_out, *isect, scene.texture_pool);
                Real cos_theta    = abs(dot(bsdf_sample->dir_out, isect->geometric_normal));
                current_path_throughput *= (bsdf_val * cos_theta / dir_pdf);

                ray.dir   = bsdf_sample->dir_out;
                ray.org   = isect->position;
                ray.tnear = get_intersection_epsilon(scene);
                ray.tfar  = infinity<Real>();

                current_medium = update_medium(isect, ray, current_medium);
            } else {
                current_medium = update_medium(isect, ray, current_medium);
            }
        }

        if (scene.options.max_depth != -1 && bounces >= scene.options.max_depth - 1) break;

        // Sample next direction for volume scattering
        if (scatter) {
            auto&& medium       = scene.media[current_medium];
            PhaseFunction phase = get_phase_function(medium);
            auto next_dir       = sample_phase_function(phase, -ray.dir,
                                                        Vector2{ next_pcg32_real<Real>(rng), next_pcg32_real<Real>(rng) });
            if (!next_dir) break;

            never_scatter      = false;
            dir_pdf            = pdf_sample_phase(phase, -ray.dir, *next_dir);
            Spectrum phase_val = eval(phase, -ray.dir, *next_dir);
            Spectrum sigma_s   = get_sigma_s(medium, ray.org);

            current_path_throughput *= (phase_val / dir_pdf) * sigma_s;
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

// The final volumetric renderer:
// multiple chromatic heterogeneous volumes with multiple scattering
// with MIS between next event estimation and phase function sampling
// with surface lighting
Spectrum vol_path_tracing(const Scene& scene, int x, int y, /* pixel coordinates */
                          pcg32_state& rng) {
    // Homework 2: To be implemented.
    return vol_path_tracing_5(scene, x, y, rng);
}
