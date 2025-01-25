#include "../microfacet.h"

Spectrum eval_op::operator()(const DisneySheen &bsdf) const {
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
    Vector3 lwi = to_local(frame, dir_in);
    Vector3 lwo = to_local(frame, dir_out);
    if (lwi.z <= 0 || lwo.z <= 0) {
        // Sheen is only above the surface
        return make_zero_spectrum();
    }

    Vector3 lwh = lwi + lwo;
    Real len = length(lwh);
    lwh /= len;

    Spectrum base_color = eval(bsdf.base_color, vertex.uv, vertex.uv_screen_size, texture_pool);
    Real sheen_tint = eval(bsdf.sheen_tint, vertex.uv, vertex.uv_screen_size, texture_pool);

    Real lum = luminance(base_color);
    Spectrum c_tint = make_const_spectrum(1.0);
    if (lum > 0.0) {
        c_tint = base_color * (1.0 / lum); // hue of base_color
    }
    Spectrum c_sheen = (1.0 - sheen_tint) * make_const_spectrum(1.0)
                       + sheen_tint * c_tint;

    Real h_dot_out = dot(lwh, lwo);
    Real sheen_term = std::pow(1.0 - std::fabs(h_dot_out), 5.0);
    Real n_dot_out = lwo.z;

    return c_sheen * (sheen_term * n_dot_out);
}

Real pdf_sample_bsdf_op::operator()(const DisneySheen &bsdf) const {
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
    return fmax(dot(frame.n, dir_out), Real(0)) / c_PI;
}

std::optional<BSDFSampleRecord>
        sample_bsdf_op::operator()(const DisneySheen &bsdf) const {
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
    return BSDFSampleRecord{
        to_world(frame, sample_cos_hemisphere(rnd_param_uv)),
        Real(0) /* eta */, Real(1.0)};
}

TextureSpectrum get_texture_op::operator()(const DisneySheen &bsdf) const {
    return bsdf.base_color;
}
