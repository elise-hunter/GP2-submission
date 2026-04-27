#version 400 core

in vec2 texCoord;

uniform sampler2D fontTex;
uniform vec4      textColor;

out vec4 FragColor;

void main()
{
    // The font texture is greyscale, so use the red channel as the opacity
    float a   = texture(fontTex, texCoord).r;
    FragColor = vec4(textColor.rgb, textColor.a * a);
}
