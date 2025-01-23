#pragma once

#include "lajolla.h"
#include "spectrum.h"

/// A microfacet model assumes that the surface is composed of infinitely many little mirrors/glasses.
/// The orientation of the mirrors determines the amount of lights reflected.
/// The distribution of the orientation is determined empirically.
/// The distribution that fits the best to the data we have so far (which is not a lot of data)
/// is from Trowbridge and Reitz's 1975 paper "Average irregularity representation of a rough ray reflection",
/// wildly known as "GGX" (seems to stand for "Ground Glass X" https://twitter.com/CasualEffects/status/783018211130441728).
/// 
/// We will use a generalized version of GGX called Generalized Trowbridge and Reitz (GTR),
/// proposed by Brent Burley and folks at Disney (https://www.disneyanimation.com/publications/physically-based-shading-at-disney/)
/// as our normal distribution function. GTR2 is equivalent to GGX.

/// Schlick's Fresnel equation approximation
/// from "An Inexpensive BRDF Model for Physically-based Rendering", Schlick
/// https://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.50.2297&rep=rep1&type=pdf
/// See "Memo on Fresnel equations" from Sebastien Lagarde
/// for a really nice introduction.
/// https://seblagarde.wordpress.com/2013/04/29/memo-on-fresnel-equations/
template <typename T>
inline T schlick_fresnel(const T &F0, Real cos_theta) {
    return F0 + (Real(1) - F0) *
        pow(max(1 - cos_theta, Real(0)), Real(5));
}

/// Fresnel equation of a dielectric interface.
/// https://seblagarde.wordpress.com/2013/04/29/memo-on-fresnel-equations/
/// n_dot_i: abs(cos(incident angle))
/// n_dot_t: abs(cos(transmission angle))
/// eta: eta_transmission / eta_incident
inline Real fresnel_dielectric(Real n_dot_i, Real n_dot_t, Real eta) {
    assert(n_dot_i >= 0 && n_dot_t >= 0 && eta > 0);
    Real rs = (n_dot_i - eta * n_dot_t) / (n_dot_i + eta * n_dot_t);
    Real rp = (eta * n_dot_i - n_dot_t) / (eta * n_dot_i + n_dot_t);
    Real F = (rs * rs + rp * rp) / 2;
    return F;
}

/// https://seblagarde.wordpress.com/2013/04/29/memo-on-fresnel-equations/
/// This is a specialized version for the code above, only using the incident angle.
/// The transmission angle is derived from 
/// n_dot_i: cos(incident angle) (can be negative)
/// eta: eta_transmission / eta_incident
inline Real fresnel_dielectric(Real n_dot_i, Real eta) {
    assert(eta > 0);
    Real n_dot_t_sq = 1 - (1 - n_dot_i * n_dot_i) / (eta * eta);
    if (n_dot_t_sq < 0) {
        // total internal reflection
        return 1;
    }
    Real n_dot_t = sqrt(n_dot_t_sq);
    return fresnel_dielectric(fabs(n_dot_i), n_dot_t, eta);
}

// F_c = R0(1.5) + (1 - R0(1.5)) * (1 - |h·w_out|)^5
inline Real fresnel_clearcoat(Real h_dot_out) {
    auto r0_from_eta = [](Real eta) {
        Real t = (eta - Real(1)) / (eta + Real(1));
        return t * t;
    };
    static const Real eta        = Real(1.5);
    static const Real r0_fixed   = r0_from_eta(eta); // e.g. ~0.04
    Real base = r0_fixed + (Real(1) - r0_fixed) * std::pow(Real(1) - std::fabs(h_dot_out), Real(5));
    return base;
}

inline Real GTR2(Real n_dot_h, Real roughness) {
    Real alpha = roughness * roughness;
    Real a2 = alpha * alpha;
    Real t = 1 + (a2 - 1) * n_dot_h * n_dot_h;
    return a2 / (c_PI * t*t);
}

inline Real GGX(Real n_dot_h, Real roughness) {
    return GTR2(n_dot_h, roughness);
}

inline Real GGX(const Vector3 &h, Real roughness, Real anisotropic) {
    Real aspect = sqrt(Real(1.0) - anisotropic * Real(0.9));
    const Real alpha_min = Real(0.0001);
    Real alpha_x = std::max(alpha_min, roughness * roughness / aspect);
    Real alpha_y = std::max(alpha_min, roughness * roughness * aspect);
    return Real(1.0) / (c_PI * alpha_x * alpha_y * pow(h.x * h.x / alpha_x / alpha_x + h.y * h.y / alpha_y / alpha_y + h.z * h.z, Real(2.0)));
}

/// The masking term models the occlusion between the small mirrors of the microfacet models.
/// See Eric Heitz's paper "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"
/// for a great explanation.
/// https://jcgt.org/published/0003/02/03/paper.pdf
/// The derivation is based on Smith's paper "Geometrical shadowing of a random rough surface".
/// Note that different microfacet distributions have different masking terms.
inline Real smith_masking_gtr2(const Vector3 &v_local, Real roughness) {
    Real alpha = roughness * roughness;
    Real a2 = alpha * alpha;
    Vector3 v2 = v_local * v_local;
    Real Lambda = (-1 + sqrt(1 + (v2.x * a2 + v2.y * a2) / v2.z)) / 2;
    return 1.0 / (1.0 + Lambda);
}

inline Real smith_masking_gtr2(const Vector3 &v_local, Real roughness, Real anisotropic) {
    Real aspect = sqrt(Real(1.0) - anisotropic * Real(0.9));
    const Real alpha_min = Real(0.0001);
    Real alpha_x = std::max(alpha_min, roughness * roughness / aspect);
    Real alpha_y = std::max(alpha_min, roughness * roughness * aspect);
    Real ax2 = alpha_x * alpha_x;
    Real ay2 = alpha_y * alpha_y;
    Vector3 v2 = v_local * v_local;
    Real Lambda = (-1 + sqrt(1 + (v2.x * v2.x * ax2 + v2.y * v2.y * ay2) / v2.z / v2.z)) / 2;
    return 1.0 / (1.0 + Lambda);
}

/// See "Sampling the GGX Distribution of Visible Normals", Heitz, 2018.
/// https://jcgt.org/published/0007/04/01/
inline Vector3 sample_visible_normals(const Vector3 &local_dir_in, Real alpha, const Vector2 &rnd_param) {
    // The incoming direction is in the "ellipsodial configuration" in Heitz's paper
    if (local_dir_in.z < 0) {
        // Ensure the input is on top of the surface.
        return -sample_visible_normals(-local_dir_in, alpha, rnd_param);
    }

    // Transform the incoming direction to the "hemisphere configuration".
    Vector3 hemi_dir_in = normalize(
        Vector3{alpha * local_dir_in.x, alpha * local_dir_in.y, local_dir_in.z});

    // Parameterization of the projected area of a hemisphere.
    // First, sample a disk.
    Real r = sqrt(rnd_param.x);
    Real phi = 2 * c_PI * rnd_param.y;
    Real t1 = r * cos(phi);
    Real t2 = r * sin(phi);
    // Vertically scale the position of a sample to account for the projection.
    Real s = (1 + hemi_dir_in.z) / 2;
    t2 = (1 - s) * sqrt(1 - t1 * t1) + s * t2;
    // Point in the disk space
    Vector3 disk_N{t1, t2, sqrt(max(Real(0), 1 - t1*t1 - t2*t2))};

    // Reprojection onto hemisphere -- we get our sampled normal in hemisphere space.
    Frame hemi_frame(hemi_dir_in);
    Vector3 hemi_N = to_world(hemi_frame, disk_N);

    // Transforming the normal back to the ellipsoid configuration
    return normalize(Vector3{alpha * hemi_N.x, alpha * hemi_N.y, max(Real(0), hemi_N.z)});
}

inline Vector3 sample_visible_normals(
    const Vector3 &v_local,
    Real alpha_x,
    Real alpha_y,
    const Vector2 &rnd_param)
{
    // If the incoming dir is below z=0, flip it (the distribution is symmetrical).
    if (v_local.z < 0) {
        return -sample_visible_normals(-v_local, alpha_x, alpha_y, rnd_param);
    }

    Vector3 v_hemi = normalize(Vector3(
        alpha_x * v_local.x,
        alpha_y * v_local.y,
        v_local.z
    ));

    Real r   = std::sqrt(rnd_param.x);
    Real phi = 2 * c_PI * rnd_param.y;
    Real t1  = r * std::cos(phi);
    Real t2  = r * std::sin(phi);

    Real s = 0.5 * (1.0 + v_hemi.z);
    t2 = (1.0 - s) * std::sqrt(std::max(Real(0), 1 - t1 * t1)) + s * t2;

    Vector3 diskN(
        t1,
        t2,
        std::sqrt(std::max(Real(0), 1 - t1 * t1 - t2 * t2))
    );

    Frame hemiFrame(v_hemi);
    Vector3 n_hemi = to_world(hemiFrame, diskN);

    return normalize(Vector3(
        alpha_x * n_hemi.x,
        alpha_y * n_hemi.y,
        std::max(Real(0), n_hemi.z)
    ));
}

inline Real clearcoat_distribution(const Vector3 &h_local, Real alpha_g)
{
    Real a2    = alpha_g * alpha_g;
    Real denom = Real(1) + (a2 - Real(1)) * (h_local.z * h_local.z);
    Real loga2 = std::log(a2);
    return (a2 - Real(1)) / (c_PI * loga2 * denom);
}
