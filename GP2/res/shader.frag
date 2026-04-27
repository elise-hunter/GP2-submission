#version 400 core

in vec2 texCoord0;
in vec3 Normal0;
in vec3 WorldPos0;
in mat3 TBN;

uniform sampler2D diffuse;
uniform sampler2D normalMap;
uniform int       hasNormalMap; // 1 = sample normal map, 0 = use vertex normal
uniform vec3  skyLight;         // colour from above (shifts day->night)
uniform vec3  groundLight;      // colour from below (shifts day->night)
uniform vec3  cameraPos;
uniform float hitFlash;         // 0-1, tints toward red on rock hit
uniform float dayPhase;         // 0 = full day, 1 = full night
uniform vec3  fogColor;         // horizon fog colour (shifts with day/night)
uniform vec3  trainPos;         // world position of the train (for headlights)
uniform vec3  colorMult;        // per-draw colour tint (default 1,1,1)
uniform float rainIntensity;    // 0 = clear, 1 = full rain (boosts fog and cools tint)

out vec4 FragColor;

void main()
{
    vec4 tex = texture(diffuse, texCoord0);
    if (tex.a < 0.1) discard;   // discard transparent pixels (used for bushes and other sprites)

    // Use the normal map if there is one, otherwise use the vertex normal as-is
    vec3 n;
    if (hasNormalMap == 1)
    {
        vec3 sampledNormal = texture(normalMap, texCoord0).rgb;
        sampledNormal = sampledNormal * 2.0 - 1.0;
        n = normalize(TBN * sampledNormal);
    }
    else
    {
        n = normalize(Normal0);
    }

    // Ambient light: bright from above, dimmer bounce light from below
    float hemi = n.y * 0.5 + 0.5;
    vec3 light = mix(groundLight, skyLight, hemi);

    // Sun (day) -> Moon (night) directional light
    vec3 sunDir  = normalize(vec3(0.6, 1.2, 0.4));
    vec3 moonDir = normalize(vec3(-0.3, 0.8, -0.5));
    vec3 lightDir = normalize(mix(sunDir, moonDir, dayPhase));
    vec3 lightCol = mix(vec3(1.0, 0.97, 0.88), vec3(0.55, 0.65, 0.90), dayPhase);
    float diff = max(dot(n, lightDir), 0.0) * mix(0.60, 0.35, dayPhase);
    light += lightCol * diff;

    // Specular highlight from the main light source
    vec3 viewDir = normalize(cameraPos - WorldPos0);
    vec3 halfVec = normalize(lightDir + viewDir);
    float spec = pow(max(dot(n, halfVec), 0.0), 48.0) * mix(0.25, 0.15, dayPhase);
    light += lightCol * spec;

    // Train headlight: warm yellow point light at the front of the engine
    vec3 headPos = trainPos + vec3(0.0, 1.5, 3.5);
    vec3 toHead  = headPos - WorldPos0;
    float hDist  = length(toHead);
    float atten  = 1.0 / (1.0 + 0.08 * hDist + 0.012 * hDist * hDist);
    float hDiff  = max(dot(n, normalize(toHead)), 0.0) * atten;
    light += vec3(1.0, 0.88, 0.55) * hDiff * mix(0.8, 5.0, dayPhase);

    vec3 col = tex.rgb * colorMult * light;

    // Atmospheric fog; rain thickens it and shifts the fog colour to a dark stormy grey
    float dist = length(WorldPos0 - cameraPos);
    vec3  stormFogColor = vec3(0.22, 0.28, 0.36);
    vec3  effectiveFog  = mix(fogColor, stormFogColor, rainIntensity * 0.88);
    float fog = exp(-dist * mix(0.007, 0.060, rainIntensity));
    col = mix(effectiveFog, col, clamp(fog, 0.0, 1.0));

    // Subtle cool tint on nearby geometry during rain (fog handles distant objects)
    col = mix(col, col * vec3(0.80, 0.86, 0.95), rainIntensity * 0.28);

    col = mix(col, vec3(1.0, 0.1, 0.05), hitFlash * 0.65);

    FragColor = vec4(col, tex.a);
}
