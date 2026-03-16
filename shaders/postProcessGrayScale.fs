#version 400 core
out vec4 FragColor;
in vec3 fNor;
in vec2 uv;
uniform sampler2D text;

void main()
{
    vec4 diffuse = texture(text, uv);
    

    float grayValue = (diffuse.x + diffuse.y + diffuse.z)/3;
    FragColor = vec4(grayValue, grayValue, grayValue, 1);
    FragColor.a=1;
    
}