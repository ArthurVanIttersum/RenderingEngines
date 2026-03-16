#version 400 core
out vec4 FragColor;
in vec3 fNor;
in vec2 uv;
uniform sampler2D text;

void main()
{
    vec4 white = vec4(0);
    vec2 texel = 1.0 / textureSize(text, 0);
    vec2 off = vec2(-1, -1);

    float kernel[9] = float[ ]
    (
        1.0, 1.0, 1.0,
        1.0, -8.0, 1.0,
        1.0, 1.0, 1.0
    );

    
    for (int i = 0; i < 9; i++)
    {
        int hor = i % 3;
        int ver = i / 3;
        vec2 offset = (vec2(hor, ver) + off) * texel;
        white += texture(text, uv + offset) * kernel[i];
        
    }

    FragColor = texture(text, uv);

    if (white.x > 0.1 || white.y > 0.1 || white.z > 0.1)
    {
        FragColor = vec4(1);
    }
    
    FragColor.a=1;
}