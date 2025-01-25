#include "../microfacet.h"

Spectrum eval_op::operator()(const DisneyClearcoat& bsdf) const {
    if (dot(vertex.geometric_normal, dir_in) < 0 || dot(vertex.geometric_normal, dir_out) < 0) {
        // No light below the surface
        return make_zero_spectrum();
    }
    // Flip the shading frame if it is inconsistent with the geometry normal
    Frame frame = vertex.shading_frame;
    if (dot(frame.n, dir_in) < 0) { frame = -frame; }

    // Homework 1: Wuqiong Zhao's implementation.
    Real gloss   = eval(bsdf.clearcoat_gloss, vertex.uv, vertex.uv_screen_size, texture_pool);
    Real alpha_g = (Real(1) - gloss) * Real(0.1) + gloss * Real(0.001);

    Vector3 lwi = to_local(frame, dir_in);
    Vector3 lwo = to_local(frame, dir_out);
    if (lwi.z <= 0 || lwo.z <= 0) { return make_zero_spectrum(); }

    Vector3 lwh = normalize(lwi + lwo);
    if (lwh.z <= 0) { return make_zero_spectrum(); }

    Real D_c               = clearcoat_distribution(lwh, alpha_g);
    Real F_c               = fresnel_clearcoat(dot(lwh, lwo)); // or dot(lwh, lwi), same for reflection
    auto clearcoat_gfactor = [](const Vector3& v_local) -> Real {
        Real rx     = Real(0.25) * v_local.x;
        Real ry     = Real(0.25) * v_local.y;
        Real rr     = (rx * rx + ry * ry) / (v_local.z * v_local.z);
        Real tmp    = std::sqrt(Real(1) + rr);
        Real Lambda = (tmp - Real(1)) * Real(0.5);
        return Real(1) / (Real(1) + Lambda);
    };
    Real G_c = clearcoat_gfactor(lwi) * clearcoat_gfactor(lwo);

    Real n_dot_in = dot(frame.n, dir_in);
    Real f        = (F_c * D_c * G_c) / (4.0 * fabs(n_dot_in));
    return Spectrum(f, f, f);
}

Real pdf_sample_bsdf_op::operator()(const DisneyClearcoat& bsdf) const {
    if (dot(vertex.geometric_normal, dir_in) < 0 || dot(vertex.geometric_normal, dir_out) < 0) {
        // No light below the surface
        return 0;
    }
    // Flip the shading frame if it is inconsistent with the geometry normal
    Frame frame = vertex.shading_frame;
    if (dot(frame.n, dir_in) < 0) { frame = -frame; }

    // Homework 1: Wuqiong Zhao's implementation.
    Real gloss   = eval(bsdf.clearcoat_gloss, vertex.uv, vertex.uv_screen_size, texture_pool);
    Real alpha_g = (Real(1) - gloss) * Real(0.1) + gloss * Real(0.001);

    Vector3 lwi = to_local(frame, dir_in);
    Vector3 lwo = to_local(frame, dir_out);
    if (lwi.z <= 0 || lwo.z <= 0) { return 0; }

    Vector3 lwh = normalize(lwi + lwo);
    if (lwh.z <= 0) { return 0; }

    Real D_c = clearcoat_distribution(lwh, alpha_g);

    Real n_dot_h  = lwh.z;
    Real wo_dot_h = dot(lwo, lwh);

    return (D_c * n_dot_h) / (4.0 * fabs(wo_dot_h));
}

inline Vector3 sample_clearcoat_half_vector(Real alpha, const Vector2& u) {
    Real a2       = alpha * alpha;
    Real powTerm  = std::pow(a2, Real(1) - u.x);
    Real cosTheta = std::sqrt((Real(1) - powTerm) / (Real(1) - a2));
    Real sinTheta = std::sqrt(std::max(Real(0), Real(1) - cosTheta * cosTheta));
    Real phi      = 2 * c_PI * u.y;

    return Vector3(sinTheta * std::cos(phi), sinTheta * std::sin(phi), cosTheta);
}

std::optional<BSDFSampleRecord> sample_bsdf_op::operator()(const DisneyClearcoat& bsdf) const {
    if (dot(vertex.geometric_normal, dir_in) < 0) {
        // No light below the surface
        return {};
    }
    // Flip the shading frame if it is inconsistent with the geometry normal
    Frame frame = vertex.shading_frame;
    if (dot(frame.n, dir_in) < 0) { frame = -frame; }

    // Homework 1: Wuqiong Zhao's implementation.
    auto reflect_vector = [](const Vector3& i, const Vector3& m) { return i - 2.0 * dot(i, m) * m; };

    Real gloss   = eval(bsdf.clearcoat_gloss, vertex.uv, vertex.uv_screen_size, texture_pool);
    Real alpha_g = (Real(1) - gloss) * Real(0.1) + gloss * Real(0.001);

    Vector3 lwi = to_local(frame, dir_in);
    if (lwi.z <= 0.0) { return {}; }
    Vector3 h_local = sample_clearcoat_half_vector(alpha_g, rnd_param_uv);
    Vector3 lwo     = reflect_vector(-lwi, h_local);
    if (lwo.z <= 0.0) { return {}; }

    return BSDFSampleRecord{ to_world(frame, lwo), Real(1.0), alpha_g };
}

TextureSpectrum get_texture_op::operator()(const DisneyClearcoat& bsdf) const {
    return make_constant_spectrum_texture(make_zero_spectrum());
}
