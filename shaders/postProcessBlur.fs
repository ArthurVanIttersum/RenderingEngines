#version 400 core
out vec4 FragColor;
in vec3 fNor;
in vec2 uv;
uniform sampler2D text;

void main()
{
    vec4 color = vec4(0);
    vec2 texel = 1.0 / textureSize(text, 0);
    vec2 off = vec2(-3, -3);

    float kernel[49] = float[ ]
    (
        0.0, 1.0, 1.5, 1.5, 1.5, 1.0, 0.0,
        1.0, 1.5, 2.0, 2.5, 2.0, 1.5, 1.0,
        1.5, 2.0, 3.0, 3.5, 3.0, 2.0, 1.5,
        1.5, 2.5, 3.5, 3.5, 3.5, 2.5, 1.5,
        1.5, 2.0, 3.0, 3.5, 3.0, 2.0, 1.5,
        1.0, 1.5, 2.0, 2.5, 2.0, 1.5, 1.0,
        0.0, 1.0, 1.5, 1.5, 1.5, 1.0, 0.0
    );

    
    
    for (int i = 0; i < 49; i++)
    {
        int hor = i % 7;
        int ver = i / 7;
        vec2 offset = (vec2(hor, ver) + off) * texel;
        color += texture(text, uv + offset) * kernel[i] / 87.5;
        
    }

    FragColor = color;

    FragColor.a=1;
}