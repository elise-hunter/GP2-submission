#version 400 core

in float ndcY;
in float ndcX;

uniform float overlayAlpha;
uniform float dayPhase;
uniform float aspectRatio;
uniform float rainMode;       // 1 = render rain overlay pass, 0 = normal sky
uniform float rainIntensity;  // 0-1 current rain intensity
uniform float rainTime;       // elapsed seconds (drives drop scroll speed)

out vec4 FragColor;

#define PI 3.14159265

float hash21(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

// Smooth noise made by blending between four nearby hash values.
float smoothNoise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(hash21(i),                  hash21(i + vec2(1.0, 0.0)), f.x),
        mix(hash21(i + vec2(0.0, 1.0)), hash21(i + vec2(1.0, 1.0)), f.x),
        f.y);
}

// Three layers of noise blended together to give a cloud-like shape.
float cloudDensity(vec2 uv)
{
    float c = 0.0;
    c += smoothNoise(uv * 2.5)                       * 0.50;
    c += smoothNoise(uv * 5.0  + vec2(0.40, 0.70))  * 0.35;
    c += smoothNoise(uv * 10.0 + vec2(0.20, 0.30))  * 0.15;
    return smoothstep(0.52, 0.72, c);
}

// Diamond-shaped star using L1 distance, which suits the low-poly art style
float starLayer(vec2 uv, float s)
{
    vec2 g = fract(uv * s) - 0.5;
    vec2 i = floor(uv * s);
    vec2 jitter  = vec2(hash21(i) - 0.5, hash21(i + vec2(3.7, 9.2)) - 0.5) * 0.75;
    vec2 delta   = g - jitter;
    float brightness = hash21(i + vec2(17.3, 5.1));
    float size   = mix(0.04, 0.09, hash21(i + vec2(2.1, 8.4))); // vary star size
    float d      = abs(delta.x) + abs(delta.y);                  // L1 = diamond shape
    return smoothstep(size, size * 0.2, d) * brightness;
}

// One layer of falling rain streaks.
// uv is screen space (0,0) at top-left to (1,1) at bottom-right.
// cols is the number of vertical columns across the screen.
// speed is how fast the drops fall in screen heights per second.
// dropLen is how long each streak is as a fraction of one cell height.
// lean is the horizontal drift per unit of vertical distance (simulates wind).
float rainLayer(vec2 uv, float t, float cols, float speed, float dropLen, float lean)
{
    uv.x += uv.y * lean;
    float cellX = floor(uv.x * cols);
    float fx    = fract(uv.x * cols);

    float xPos  = 0.15 + hash21(vec2(cellX * 0.179, 3.1)) * 0.70;
    float phase = hash21(vec2(cellX * 0.231, 5.7));
    float spd   = speed * (0.8 + hash21(vec2(cellX * 0.317, 1.3)) * 0.4);

    float dropTop = fract(phase + t * spd);
    float dx      = abs(fx - xPos);
    float dy      = uv.y - dropTop;
    if (dy < 0.0) dy += 1.0;

    float xMask = smoothstep(0.12, 0.0, dx);
    float yMask = step(0.0, dy) * step(dy, dropLen) * (1.0 - dy / dropLen);
    float bri   = 0.55 + hash21(vec2(cellX * 0.153, 7.1)) * 0.45;
    return xMask * yMask * bri;
}

void main()
{
    if (overlayAlpha > 0.0)
    {
        FragColor = vec4(0.0, 0.0, 0.0, overlayAlpha);
        return;
    }

    // Rain overlay pass: draw screen-space rain streaks and return early
    if (rainMode > 0.5)
    {
        // Convert position to a 0-1 UV with (0,0) at the top-left
        vec2 uv = vec2(ndcX * 0.5 + 0.5, (1.0 - ndcY) * 0.5);

        float rain  = rainLayer(uv,                         rainTime,        20.0, 0.80, 0.25, -0.04);
        rain       += rainLayer(uv + vec2(0.27, 0.43),      rainTime * 0.85, 14.0, 0.65, 0.18, -0.03) * 0.55;
        rain        = clamp(rain, 0.0, 1.0);

        FragColor = vec4(0.75, 0.82, 0.95, rain * rainIntensity * 0.70);
        return;
    }

    float dp = dayPhase;

    // Three-phase sky: day (dp=0) -> sunset (dp=0.5) -> night (dp=1)
    vec3 horizon, mid, zenith, earth;
    if (dp < 0.5)
    {
        float t  = dp * 2.0;
        horizon  = mix(vec3(0.76, 0.88, 0.97), vec3(0.99, 0.52, 0.12), t);
        mid      = mix(vec3(0.36, 0.62, 0.90), vec3(0.72, 0.22, 0.18), t);
        zenith   = mix(vec3(0.10, 0.30, 0.72), vec3(0.18, 0.08, 0.38), t);
        earth    = mix(vec3(0.45, 0.60, 0.30), vec3(0.42, 0.28, 0.10), t);
    }
    else
    {
        float t  = (dp - 0.5) * 2.0;
        horizon  = mix(vec3(0.99, 0.52, 0.12), vec3(0.04, 0.05, 0.15), t);
        mid      = mix(vec3(0.72, 0.22, 0.18), vec3(0.01, 0.03, 0.12), t);
        zenith   = mix(vec3(0.18, 0.08, 0.38), vec3(0.00, 0.01, 0.06), t);
        earth    = mix(vec3(0.42, 0.28, 0.10), vec3(0.08, 0.10, 0.04), t);
    }

    vec3 col;
    if (ndcY < 0.0)
        col = mix(earth, horizon, smoothstep(-0.4, 0.0, ndcY));
    else
    {
        col = mix(horizon, mid,    smoothstep(0.0, 0.25, ndcY));
        col = mix(col,     zenith, smoothstep(0.25, 1.0,  ndcY));
    }

    // Stars: three layers of different density, only above horizon, fade in at night
    if (ndcY > 0.0)
    {
        float starFade  = smoothstep(0.4, 0.8, dp) * smoothstep(0.05, 0.25, ndcY);
        vec2  starUV    = vec2(ndcX, ndcY);
        float stars     = starLayer(starUV,        8.0);
        stars          += starLayer(starUV + 0.31, 15.0) * 0.55;
        stars          += starLayer(starUV + 0.73, 26.0) * 0.30;
        col += vec3(0.95, 0.97, 1.0) * stars * starFade;
    }

    // Clouds: fade in above horizon and out toward sunset
    if (ndcY > 0.04)
    {
        float cloudFade    = (1.0 - smoothstep(0.25, 0.55, dp)) * smoothstep(0.04, 0.18, ndcY);
        float density      = cloudDensity(vec2(ndcX * 0.6, ndcY * 0.9));
        vec3  cloudCol     = mix(vec3(1.0, 1.0, 1.0), vec3(0.80, 0.86, 0.94), density * 0.4);
        col = mix(col, cloudCol, density * cloudFade * 0.85);
    }

    // UV scaled by aspect ratio so the sun and moon stay circular
    vec2 uv = vec2(ndcX * aspectRatio, ndcY);

    // Sun arcs from overhead center (dp=0) toward the right horizon (setting at dp=1)
    float sunA    = mix(PI * 0.5, 0.1, dp);
    vec2  sunPos  = vec2(cos(sunA) * 0.75 * aspectRatio, sin(sunA) * 0.62);
    float dSun    = length(uv - sunPos);
    float sunDisc = smoothstep(0.06, 0.035, dSun);
    float sunHalo = exp(-dSun * 9.0) * 0.45;
    float sunFade = smoothstep(-0.05, 0.12, sunPos.y) * (1.0 - smoothstep(0.35, 0.65, dp));
    vec3 sunCol = mix(vec3(1.0, 0.96, 0.75), vec3(1.0, 0.40, 0.08), smoothstep(0.0, 0.5, dp));
    col += sunCol * (sunDisc + sunHalo) * sunFade;

    // Moon rises from the left horizon (dp~0.4) and climbs to overhead at full night (dp=1)
    float moonA    = mix(-0.1, PI * 0.47, dp);
    vec2  moonPos  = vec2(-cos(moonA) * 0.75 * aspectRatio, sin(moonA) * 0.60);
    float dMoon    = length(uv - moonPos);
    float moonDisc = smoothstep(0.045, 0.028, dMoon);
    float moonHalo = exp(-dMoon * 9.0) * 0.18;
    float moonFade = smoothstep(-0.05, 0.12, moonPos.y) * smoothstep(0.3, 0.65, dp);
    col += vec3(0.92, 0.95, 1.0) * (moonDisc + moonHalo) * moonFade;

    // Storm: blend sky toward a dark overcast grey during rain
    col = mix(col, vec3(0.22, 0.28, 0.36), rainIntensity * 0.78);

    FragColor = vec4(col, 1.0);
}
