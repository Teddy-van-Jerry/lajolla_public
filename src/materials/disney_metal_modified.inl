#include "../microfacet.h"
#include "material.h"

Spectrum eval_op::operator()(const DisneyMetalModified &bsdf) const {
    if (dot(vertex.geometric_normal, dir_in) < 0 ||
            dot(vertex.geometric_normal, dir_out) < 0) {
        // No light below the surface
        return make_zero_spectrum();
    }
    // Flip the shading frame if it is inconsistent with the geometry normal
    Frame frame = vertex.shading_frame;
    if (dot(frame.n, dir_in) < 0) {
        frame = -frame;
    }

    // Homework 1: Wuqiong Zhao's implementation.
    Spectrum base_color = eval(bsdf.base_color, vertex.uv, vertex.uv_screen_size, texture_pool);
    Real anisotropic = eval(bsdf.anisotropic, vertex.uv, vertex.uv_screen_size, texture_pool);
    Real roughness = eval(bsdf.roughness, vertex.uv, vertex.uv_screen_size, texture_pool);
    Real n_dot_in = dot(frame.n, dir_in);
    Real specular = eval(bsdf.specular, vertex.uv, vertex.uv_screen_size, texture_pool);
    Real metallic = eval(bsdf.metallic, vertex.uv, vertex.uv_screen_size, texture_pool);
    Real spec_tint = eval(bsdf.specular_tint, vertex.uv, vertex.uv_screen_size, texture_pool);
    Real eta = bsdf.eta;

    Vector3 lwi = to_local(frame, dir_in);
    Vector3 lwo = to_local(frame, dir_out);
    Vector3 lwh = normalize(lwi + lwo);
    Real h_dot_out = dot(lwh, lwo);

    // Clamp roughness to avoid numerical issues.
    roughness = std::clamp(roughness, Real(0.01), Real(1));
    
    auto r0_of_eta = [&](Real e) {
        Real t = (e - Real(1)) / (e + Real(1));
        return t * t;
    };
    Real r0 = r0_of_eta(eta);

    auto lum = luminance(base_color);
    Spectrum tintColor = make_const_spectrum(1.0);
    if (lum > 0.0) {
        tintColor = base_color * (Real(1.0) / lum);
    }

    Spectrum Ks = (Real(1) - spec_tint) * make_const_spectrum(1.0) + spec_tint * tintColor;
    Spectrum c0 = specular * r0 * (Real(1)-metallic) * Ks + metallic * base_color;
    Spectrum Fm = c0 + (make_const_spectrum(1.0) - c0) * std::pow(Real(1.0) - std::fabs(h_dot_out), Real(5.0));

    // 7) distribution & masking
    Real D_m   = GGX(lwh, roughness, anisotropic);
    Real G_in  = smith_masking_gtr2(lwi, roughness, anisotropic);
    Real G_out = smith_masking_gtr2(lwo, roughness, anisotropic);
    Real G_m   = G_in * G_out;
    if (fabs(n_dot_in) < Real(0.05)) {
        return make_zero_spectrum();
    }
    return (Fm * D_m * G_m) / (Real(4.0) * std::fabs(n_dot_in));
}

Real pdf_sample_bsdf_op::operator()(const DisneyMetalModified &bsdf) const {
    if (dot(vertex.geometric_normal, dir_in) < 0 ||
            dot(vertex.geometric_normal, dir_out) < 0) {
        // No light below the surface
        return 0;
    }
    // Flip the shading frame if it is inconsistent with the geometry normal
    Frame frame = vertex.shading_frame;
    if (dot(frame.n, dir_in) < 0) {
        frame = -frame;
    }
    
    // Homework 1: Wuqiong Zhao's implementation.
    Real anisotropic = eval(bsdf.anisotropic, vertex.uv, vertex.uv_screen_size, texture_pool);
    Real roughness = eval(bsdf.roughness, vertex.uv, vertex.uv_screen_size, texture_pool);
    // Clamp roughness to avoid numerical issues.
    roughness = std::clamp(roughness, Real(0.01), Real(1));
    Vector3 lwi = to_local(frame, dir_in);
    Vector3 lwo = to_local(frame, dir_out);
    if (lwi.z <= 0 || lwo.z <= 0) {
        return 0;
    }

    Vector3 lwh = normalize(lwi + lwo);

    if (fabs(dot(lwh, lwo)) < Real(0.05)) {
        return 0;
    }

    Real D_m = GGX(lwh, roughness, anisotropic);
    Real G_in = smith_masking_gtr2(lwi, roughness, anisotropic);
    return D_m * G_in * fabs(lwh.z) / (4.0 * fabs(dot(lwh, lwo)));
}

std::optional<BSDFSampleRecord>
        sample_bsdf_op::operator()(const DisneyMetalModified &bsdf) const {
    if (dot(vertex.geometric_normal, dir_in) < 0) {
        // No light below the surface
        return {};
    }
    // Flip the shading frame if it is inconsistent with the geometry normal
    Frame frame = vertex.shading_frame;
    if (dot(frame.n, dir_in) < 0) {
        frame = -frame;
    }
    
    // Homework 1: Wuqiong Zhao's implementation.
    Real roughness = eval(bsdf.roughness, vertex.uv, vertex.uv_screen_size, texture_pool);
    Real anisotropic = eval(bsdf.anisotropic, vertex.uv, vertex.uv_screen_size, texture_pool);
    Real aspect  = std::sqrt(1.0 - 0.9 * anisotropic);
    roughness = std::clamp(roughness, Real(0.01), Real(1));
    Real alpha_x = std::max(Real(0.0001), (roughness * roughness) / aspect);
    Real alpha_y = std::max(Real(0.0001), (roughness * roughness) * aspect);

    auto reflect_vector = [](const Vector3 &i, const Vector3 &m) {
        return i - 2.0 * dot(i, m) * m;
    };

    Vector3 v_local = to_local(frame, dir_in);
    // Vector3 h_local = sample_visible_normals(v_local, roughness * roughness, rnd_param_uv);
    Vector3 h_local = sample_visible_normals(v_local, alpha_x, alpha_y, rnd_param_uv);
    Vector3 lwo = reflect_vector(-v_local, h_local);

    if (lwo.z <= 0.0) {
        return {};
    }

    return BSDFSampleRecord{ to_world(frame, lwo), Real(0.0), roughness };
}

TextureSpectrum get_texture_op::operator()(const DisneyMetalModified &bsdf) const {
    return bsdf.base_color;
}
