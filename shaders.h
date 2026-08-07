<surface>

	<vertex>

#version 330 core
layout(location = 0) in vec3 coords;
layout(location = 1) in vec3 coords_side;

uniform mat4 MVP;
uniform float t;
uniform float f;
uniform float factr;
uniform bool is_grid;

uniform int XSZ;
uniform int YSZ;

out vec3 fragPos;
out vec3 sidePos;
out vec3 normal;

float gauss(float x)
{
    return exp(-x);
}

<copy spiral>
float spiral_y(vec3 p) {

    float r = length(p.xz);
    float theta = atan(p.z, p.x);

    float centerFade = 1.0 - exp(-8.0 * r * r);

    float phase = 4.0 * theta * centerFade - 5.0 * r + 1.5 * t;
    return 0.65 * exp(-0.6 * r) * sin(phase);
}

</copy>

void main()
{

    int x = gl_VertexID;


    vec3 pos = coords;
    vec3 side_pos = coords_side;
    
    side_pos.y = spiral_y(side_pos) / 2;

    pos.y = spiral_y(pos) / 2;

    if (is_grid) {
        pos.y += 0.001;
    }
    
    vec4 Pos = MVP * vec4(pos, 1.0);

    fragPos = pos;
    sidePos = coords_side;


    gl_Position = Pos;
}

</vertex>

<fragment>

#version 330 core

in vec3 fragPos;
in vec3 sidePos;
out vec4 FragColor;

in vec3 normal; 
uniform bool is_grid;
uniform vec4 grid_clr;

void main()
{
    if (is_grid) {
        FragColor   = grid_clr;

    }
    else {
        float h = 2.5 * fragPos.y;
        FragColor = vec4(h + h * h + 3 * h * h * h, 0.8 - h * h, 1 - 0.5 * exp(h), 1);

    }
}

</fragment>

</surface>


<grid> 

<vertex>

</vertex>
</grid>

