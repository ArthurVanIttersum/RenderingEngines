#version 400
layout (location = 0) in vec3 aPos; // obj space
layout (location = 1) in vec3 aNor; // obj space
layout (location = 2) in vec2 aUv;
uniform mat4 mvpMatrix; // object ->(M) world ->(V) camera ->(P) "screen"
uniform mat4 mMatrix;   // object ->(M) world
out vec3 fPos; // world space
out vec3 fNor; // world space
out vec2 uv;

void main() {
  fPos = aPos;
  fPos = (mMatrix * vec4(aPos, 1)).xyz; // experimental
  fNor = normalize((mMatrix * vec4(aNor, 0)).xyz); // NOTE: All good now :-) 
  gl_Position = mvpMatrix * vec4(aPos, 1.0);
  uv = aUv;
}