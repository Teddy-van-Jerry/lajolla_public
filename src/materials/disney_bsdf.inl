#include "../microfacet.h"

Spectrum eval_op::operator()(const DisneyBSDF& bsdf) const {
    // bool reflect = dot(vertex.geometric_normal, dir_in) *
    //                dot(vertex.geometric_normal, dir_out) > 0;
    // Flip the shading frame if it is inconsistent with the geometry normal
    Frame frame = vertex.shading_frame;
    if (dot(frame.n, dir_in) * dot(vertex.geometric_normal, dir_in) < 0) { frame = -frame; }
    // Homework 1: Wuqiong Zhao's implementation.
    // Check inside vs outside
    bool inside = (dot(vertex.geometric_normal, dir_in) < 0.0);

    Real metallic  = eval(bsdf.metallic, vertex.uv, vertex.uv_screen_size, texture_pool);
    Real specTrans = eval(bsdf.specular_transmission, vertex.uv, vertex.uv_screen_size, texture_pool);
    Real sheen     = eval(bsdf.sheen, vertex.uv, vertex.uv_screen_size, texture_pool);
    Real clearcoat = eval(bsdf.clearcoat, vertex.uv, vertex.uv_screen_size, texture_pool);

    if (inside) {
        DisneyGlass glassBSDF;
        // fill in all fields
        glassBSDF.base_color  = bsdf.base_color;
        glassBSDF.roughness   = bsdf.roughness;
        glassBSDF.anisotropic = bsdf.anisotropic;
        glassBSDF.eta         = bsdf.eta;

        return eval_op{ dir_in, dir_out, vertex, texture_pool, dir }(glassBSDF);
    }

    // Diffuse
    DisneyDiffuse diffBSDF;
    diffBSDF.base_color = bsdf.base_color;
    diffBSDF.roughness  = bsdf.roughness;
    Spectrum f_diffuse  = eval_op{ dir_in, dir_out, vertex, texture_pool, dir }(diffBSDF);

    // Sheen
    DisneySheen sheenBSDF;
    sheenBSDF.base_color = bsdf.base_color;
    sheenBSDF.sheen_tint = bsdf.sheen_tint;
    Spectrum f_sheen     = eval_op{ dir_in, dir_out, vertex, texture_pool, dir }(sheenBSDF);

    // Modified Metal
    DisneyMetalModified metalBSDF;
    metalBSDF.base_color    = bsdf.base_color;
    metalBSDF.roughness     = bsdf.roughness;
    metalBSDF.anisotropic   = bsdf.anisotropic;
    metalBSDF.metallic      = bsdf.metallic;
    metalBSDF.specular      = bsdf.specular;
    metalBSDF.specular_tint = bsdf.specular_tint;
    metalBSDF.eta           = bsdf.eta;

    Spectrum f_metal = eval_op{ dir_in, dir_out, vertex, texture_pool, dir }(metalBSDF);

    // -- Clearcoat
    DisneyClearcoat coatBSDF;
    coatBSDF.clearcoat_gloss = bsdf.clearcoat_gloss;
    Spectrum f_clearcoat     = eval_op{ dir_in, dir_out, vertex, texture_pool, dir }(coatBSDF);

    // -- Glass
    DisneyGlass glassBSDF;
    glassBSDF.base_color  = bsdf.base_color;
    glassBSDF.roughness   = bsdf.roughness;
    glassBSDF.anisotropic = bsdf.anisotropic;
    glassBSDF.eta         = bsdf.eta;
    Spectrum f_glass      = eval_op{ dir_in, dir_out, vertex, texture_pool, dir }(glassBSDF);

    Real wDiffuse   = (1 - specTrans) * (1 - metallic);
    Real wSheen     = (1 - metallic) * sheen;
    Real wMetal     = (1 - specTrans * (1 - metallic));
    Real wClearcoat = 0.25 * clearcoat;
    Real wGlass     = (1 - metallic) * specTrans;

    return wDiffuse * f_diffuse + wSheen * f_sheen + wMetal * f_metal + wClearcoat * f_clearcoat + wGlass * f_glass;
}

Real pdf_sample_bsdf_op::operator()(const DisneyBSDF& bsdf) const {
    // bool reflect = dot(vertex.geometric_normal, dir_in) *
    //                dot(vertex.geometric_normal, dir_out) > 0;
    // Flip the shading frame if it is inconsistent with the geometry normal
    Frame frame = vertex.shading_frame;
    if (dot(frame.n, dir_in) * dot(vertex.geometric_normal, dir_in) < 0) { frame = -frame; }
    // Homework 1: Wuqiong Zhao's implementation.
    bool inside = (dot(vertex.geometric_normal, dir_in) < 0.0);

    Real metallic  = eval(bsdf.metallic, vertex.uv, vertex.uv_screen_size, texture_pool);
    Real specTrans = eval(bsdf.specular_transmission, vertex.uv, vertex.uv_screen_size, texture_pool);
    Real clearcoat = eval(bsdf.clearcoat, vertex.uv, vertex.uv_screen_size, texture_pool);

    if (inside) {
        // Only Glass
        DisneyGlass glassBSDF;
        glassBSDF.base_color  = bsdf.base_color;
        glassBSDF.roughness   = bsdf.roughness;
        glassBSDF.anisotropic = bsdf.anisotropic;
        glassBSDF.eta         = bsdf.eta;

        return pdf_sample_bsdf_op{ dir_in, dir_out, vertex, texture_pool, dir }(glassBSDF);
    }

    Real wD   = (1 - specTrans) * (1 - metallic);
    Real wM   = (1 - specTrans * (1 - metallic));
    Real wC   = 0.25 * clearcoat;
    Real wG   = (1 - metallic) * specTrans;
    Real sumW = (wD + wM + wC + wG);

    if (sumW < Real(0.05)) { return 0.0; }

    // Diffuse
    DisneyDiffuse diffBSDF;
    diffBSDF.base_color = bsdf.base_color;
    diffBSDF.roughness  = bsdf.roughness;
    Real pdfD           = pdf_sample_bsdf_op{ dir_in, dir_out, vertex, texture_pool, dir }(diffBSDF);

    // Metal
    DisneyMetalModified metalBSDF;
    metalBSDF.base_color    = bsdf.base_color;
    metalBSDF.roughness     = bsdf.roughness;
    metalBSDF.anisotropic   = bsdf.anisotropic;
    metalBSDF.metallic      = bsdf.metallic;
    metalBSDF.specular      = bsdf.specular;
    metalBSDF.specular_tint = bsdf.specular_tint;
    metalBSDF.eta           = bsdf.eta;
    Real pdfM               = pdf_sample_bsdf_op{ dir_in, dir_out, vertex, texture_pool, dir }(metalBSDF);

    // Clearcoat
    DisneyClearcoat coatBSDF;
    coatBSDF.clearcoat_gloss = bsdf.clearcoat_gloss;
    Real pdfC                = pdf_sample_bsdf_op{ dir_in, dir_out, vertex, texture_pool, dir }(coatBSDF);

    // Glass
    DisneyGlass glassBSDF;
    glassBSDF.base_color  = bsdf.base_color;
    glassBSDF.roughness   = bsdf.roughness;
    glassBSDF.anisotropic = bsdf.anisotropic;
    glassBSDF.eta         = bsdf.eta;
    Real pdfG             = pdf_sample_bsdf_op{ dir_in, dir_out, vertex, texture_pool, dir }(glassBSDF);

    return (wD * pdfD + wM * pdfM + wC * pdfC + wG * pdfG) / sumW;
}

std::optional<BSDFSampleRecord> sample_bsdf_op::operator()(const DisneyBSDF& bsdf) const {
    // Flip the shading frame if it is inconsistent with the geometry normal
    Frame frame = vertex.shading_frame;
    if (dot(frame.n, dir_in) * dot(vertex.geometric_normal, dir_in) < 0) { frame = -frame; }
    // Homework 1: Wuqiong Zhao's implementation.
    // Check inside
    bool inside = (dot(vertex.geometric_normal, dir_in) < 0.0);

    Real metallic  = eval(bsdf.metallic, vertex.uv, vertex.uv_screen_size, texture_pool);
    Real specTrans = eval(bsdf.specular_transmission, vertex.uv, vertex.uv_screen_size, texture_pool);
    Real clearcoat = eval(bsdf.clearcoat, vertex.uv, vertex.uv_screen_size, texture_pool);

    // If inside => sample only glass
    if (inside) {
        DisneyGlass glassBSDF;
        glassBSDF.base_color  = bsdf.base_color;
        glassBSDF.roughness   = bsdf.roughness;
        glassBSDF.anisotropic = bsdf.anisotropic;
        glassBSDF.eta         = bsdf.eta;

        return sample_bsdf_op{ dir_in, vertex, texture_pool, rnd_param_uv, rnd_param_w, dir }(glassBSDF);
    }

    // Weights
    Real wD   = (1 - specTrans) * (1 - metallic);
    Real wM   = (1 - specTrans * (1 - metallic));
    Real wC   = 0.25 * clearcoat;
    Real wG   = (1 - metallic) * specTrans;
    Real sumW = wD + wM + wC + wG;
    if (sumW < Real(0.05)) { return {}; }

    Real Xi = rnd_param_uv.x * sumW;

    if (Xi < wD) {
        DisneyDiffuse diffBSDF;
        diffBSDF.base_color = bsdf.base_color;
        diffBSDF.roughness  = bsdf.roughness;
        Real newX           = Xi / wD;
        return sample_bsdf_op{ dir_in, vertex, texture_pool, Vector2(newX, rnd_param_uv.y), rnd_param_w, dir }(
            diffBSDF);
    }
    Xi -= wD;

    // Metal
    if (Xi < wM) {
        DisneyMetalModified metalBSDF;
        metalBSDF.base_color    = bsdf.base_color;
        metalBSDF.roughness     = bsdf.roughness;
        metalBSDF.anisotropic   = bsdf.anisotropic;
        metalBSDF.metallic      = bsdf.metallic;
        metalBSDF.specular      = bsdf.specular;
        metalBSDF.specular_tint = bsdf.specular_tint;
        metalBSDF.eta           = bsdf.eta;

        Real newX = Xi / wM;
        return sample_bsdf_op{ dir_in, vertex, texture_pool, Vector2(newX, rnd_param_uv.y), rnd_param_w, dir }(
            metalBSDF);
    }
    Xi -= wM;

    // Clearcoat
    if (Xi < wC) {
        DisneyClearcoat coatBSDF;
        coatBSDF.clearcoat_gloss = bsdf.clearcoat_gloss;
        Real newX                = Xi / wC;
        return sample_bsdf_op{ dir_in, vertex, texture_pool, Vector2(newX, rnd_param_uv.y), rnd_param_w, dir }(
            coatBSDF);
    }
    Xi -= wC;

    // Glass
    {
        DisneyGlass glassBSDF;
        glassBSDF.base_color  = bsdf.base_color;
        glassBSDF.roughness   = bsdf.roughness;
        glassBSDF.anisotropic = bsdf.anisotropic;
        glassBSDF.eta         = bsdf.eta;

        Real newX = Xi / wG;
        return sample_bsdf_op{ dir_in, vertex, texture_pool, Vector2(newX, rnd_param_uv.y), rnd_param_w, dir }(
            glassBSDF);
    }
}

TextureSpectrum get_texture_op::operator()(const DisneyBSDF& bsdf) const { return bsdf.base_color; }
