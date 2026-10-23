// Verre « liquide » d'Enhance (design-system §6.4, ENH-004 specs §5.2).
// Une seule passe : refraction en lentille dans le biseau, flou par mipmaps, saturation,
// teinte adaptative a la luminance du fond, lisere de 1 px physique, reflet dans le biseau.
// Entrees et sortie en alpha premultiplie (conventions Qt Quick).
#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec2 origin;        // coin haut gauche de la surface dans la capture (px logiques)
    vec2 itemSize;      // taille de la surface (px logiques)
    vec2 backdropSize;  // taille de l'Item capture (px logiques)
    float radius;
    float bezel;
    float refraction;   // deplacement max au bord (px logiques)
    float blurLod;
    float blurStep;     // ecart des 4 echantillons de lissage (px logiques)
    float saturation;
    float tintScale;
    vec4 tintCurve;     // alphaA, alphaB, lumaA, lumaB
    vec4 tint;
    vec4 rim;
    vec4 rimLow;
    vec4 specularColor;
    float specular;
    vec2 light;         // centre du reflet (px logiques, repere de la surface)
    vec2 lightDir;      // direction de la lumiere (lisere)
    float dpr;
} ubuf;

layout(binding = 1) uniform sampler2D source;

// Lobe du reflet : ellipse de 160 x 70 px (maquette, radial-gradient 160px 70px).
const vec2 kSpecularRadius = vec2(80.0, 35.0);

float sdRoundRect(vec2 p, vec2 halfSize, float r) {
    vec2 q = abs(p) - halfSize + vec2(r);
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
}

vec4 sampleBackdrop(vec2 posLogical) {
    vec2 uv = clamp((ubuf.origin + posLogical) / ubuf.backdropSize, vec2(0.0), vec2(1.0));
    vec2 d = vec2(ubuf.blurStep) / ubuf.backdropSize;
    vec4 c = textureLod(source, uv, ubuf.blurLod) * 0.4;
    c += textureLod(source, uv + vec2(d.x, d.y), ubuf.blurLod) * 0.15;
    c += textureLod(source, uv + vec2(-d.x, d.y), ubuf.blurLod) * 0.15;
    c += textureLod(source, uv + vec2(d.x, -d.y), ubuf.blurLod) * 0.15;
    c += textureLod(source, uv + vec2(-d.x, -d.y), ubuf.blurLod) * 0.15;
    return c;
}

void main() {
    vec2 p = qt_TexCoord0 * ubuf.itemSize;
    vec2 halfSize = ubuf.itemSize * 0.5;
    vec2 c = p - halfSize;
    float r = min(ubuf.radius, min(halfSize.x, halfSize.y));
    float d = sdRoundRect(c, halfSize, r);
    float coverage = clamp(0.5 - d * ubuf.dpr, 0.0, 1.0);
    if (coverage <= 0.0) {
        fragColor = vec4(0.0);
        return;
    }

    // Normale sortante (gradient de la distance signee).
    const float e = 0.5;
    vec2 g = vec2(sdRoundRect(c + vec2(e, 0.0), halfSize, r) - sdRoundRect(c - vec2(e, 0.0), halfSize, r),
                  sdRoundRect(c + vec2(0.0, e), halfSize, r) - sdRoundRect(c - vec2(0.0, e), halfSize, r));
    vec2 n = length(g) > 1e-5 ? normalize(g) : vec2(0.0, -1.0);
    float inside = max(-d, 0.0);

    // Refraction : profil de lentille, maximal au bord, nul au-dela du biseau.
    float t = clamp(inside / max(ubuf.bezel, 1e-3), 0.0, 1.0);
    float lens = (1.0 - t) * (1.0 - t);
    vec4 base = sampleBackdrop(p - n * ubuf.refraction * lens);

    // Saturation sur la couleur non premultipliee.
    vec3 rgb = base.a > 1e-4 ? base.rgb / base.a : vec3(0.0);
    float l = dot(rgb, vec3(0.2126, 0.7152, 0.0722));
    rgb = clamp(mix(vec3(l), rgb, ubuf.saturation), 0.0, 1.0);

    // Teinte adaptative : alpha = mix(A, B, smoothstep(L1, L2, Lb)) (design-system §6.3).
    float lb = dot(pow(rgb, vec3(2.2)), vec3(0.2126, 0.7152, 0.0722));
    float a = clamp(mix(ubuf.tintCurve.x, ubuf.tintCurve.y,
                        smoothstep(ubuf.tintCurve.z, ubuf.tintCurve.w, lb)) * ubuf.tintScale, 0.0, 1.0);
    vec4 col = ubuf.tint * a + vec4(rgb * base.a, base.a) * (1.0 - a);

    // Lisere : 1 px physique, clair du cote de la lumiere et a l'oppose, discret ailleurs.
    float rimBand = clamp(1.0 - inside * ubuf.dpr, 0.0, 1.0);
    float facing = abs(dot(n, normalize(ubuf.lightDir)));
    vec4 rimColor = mix(ubuf.rimLow, ubuf.rim, facing * facing);
    col = rimColor * rimBand + col * (1.0 - rimColor.a * rimBand);

    // Reflet confine au biseau (aucun texte dedans).
    float lobe = clamp(1.0 - length((p - ubuf.light) / kSpecularRadius), 0.0, 1.0);
    float bezelMask = 1.0 - smoothstep(ubuf.bezel - 1.0, ubuf.bezel, inside);
    float s = clamp(lobe * lobe * ubuf.specular * bezelMask, 0.0, 1.0);
    col = ubuf.specularColor * s + col * (1.0 - ubuf.specularColor.a * s);

    fragColor = col * coverage * ubuf.qt_Opacity;
}
