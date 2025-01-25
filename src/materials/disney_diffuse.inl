Spectrum eval_op::operator()(const DisneyDiffuse& bsdf) const {
    if (dot(vertex.geometric_normal, dir_in) < 0 || dot(vertex.geometric_normal, dir_out) < 0) {
        // No light below the surface
        return make_zero_spectrum();
    }
    // Flip the shading frame if it is inconsistent with the geometry normal
    Frame frame = vertex.shading_frame;
    if (dot(frame.n, dir_in) < 0) { frame = -frame; }

    // Homework 1: Wuqiong Zhao's implementation.
    Vector3 half_vector = normalize(dir_in + dir_out);
    Spectrum base_color = eval(bsdf.base_color, vertex.uv, vertex.uv_screen_size, texture_pool);
    Real subsurface     = eval(bsdf.subsurface, vertex.uv, vertex.uv_screen_size, texture_pool);
    Real roughness      = eval(bsdf.roughness, vertex.uv, vertex.uv_screen_size, texture_pool);
    Real n_dot_in       = dot(frame.n, dir_in);
    Real n_dot_out      = dot(frame.n, dir_out);
    Real h_dot_out      = dot(half_vector, dir_out);

    // Clamp roughness to avoid numerical issues.
    roughness = std::clamp(roughness, Real(0.01), Real(1));

    Real F_D90                    = Real(0.5) + Real(2) * roughness * (h_dot_out * h_dot_out);
    auto F_D                      = [&](Real omega) { return 1 + (F_D90 - 1) * pow(1 - fabs(omega), 5); };
    Spectrum base_diffuse_contrib = (base_color / c_PI) * F_D(n_dot_in) * F_D(n_dot_out);

    Real F_SS90 = roughness * (h_dot_out * h_dot_out);
    auto F_SS   = [&](Real omega) { return 1 + (F_SS90 - 1) * pow(1 - fabs(omega), 5); };
    Spectrum subsurface_contrib =
        (Real(1.25) * base_color / c_PI) *
        (F_SS(n_dot_in) * F_SS(n_dot_out) * (Real(1.0) / (fabs(n_dot_in) + fabs(n_dot_out)) - Real(0.5)) + Real(0.5));

    return ((Real(1.0) - subsurface) * base_diffuse_contrib + subsurface * subsurface_contrib) * fabs(n_dot_out);
}

Real pdf_sample_bsdf_op::operator()(const DisneyDiffuse& bsdf) const {
    if (dot(vertex.geometric_normal, dir_in) < 0 || dot(vertex.geometric_normal, dir_out) < 0) {
        // No light below the surface
        return Real(0);
    }
    // Flip the shading frame if it is inconsistent with the geometry normal
    Frame frame = vertex.shading_frame;
    if (dot(frame.n, dir_in) < 0) { frame = -frame; }

    // Homework 1: Wuqiong Zhao's implementation.
    return fmax(dot(frame.n, dir_out), Real(0)) / c_PI;
}

std::optional<BSDFSampleRecord> sample_bsdf_op::operator()(const DisneyDiffuse& bsdf) const {
    if (dot(vertex.geometric_normal, dir_in) < 0) {
        // No light below the surface
        return {};
    }
    // Flip the shading frame if it is inconsistent with the geometry normal
    Frame frame = vertex.shading_frame;
    if (dot(frame.n, dir_in) < 0) { frame = -frame; }

    // Homework 1: Wuqiong Zhao's implementation.
    Real roughness = eval(bsdf.roughness, vertex.uv, vertex.uv_screen_size, texture_pool);
    return BSDFSampleRecord{ to_world(frame, sample_cos_hemisphere(rnd_param_uv)), Real(0) /* eta */, roughness };
}

TextureSpectrum get_texture_op::operator()(const DisneyDiffuse& bsdf) const { return bsdf.base_color; }
