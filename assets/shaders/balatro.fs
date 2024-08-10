#version 100

precision highp float;

// Default Raylib Shader Variables
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// Input vertex attributes (from vertex shader)
varying vec2 fragTexCoord;
varying vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Custom Variables
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// Delta Time
uniform highp float delta_time;

// Constants
const float SPIN_EASE = 1.0;
const float PI = 3.1415926535;

// Change variables
uniform highp float spin_rotation;
uniform highp float spin_speed;
uniform highp vec4 colour_1;
uniform highp vec4 colour_2;
uniform highp vec4 colour_3;
uniform highp float contrast;
uniform highp float spin_amount;
uniform highp float pixel_filter;

bool polar_coordinates = false;  //cool polar coordinates effect
vec2 polar_center = vec2(0.5);
float polar_zoom = 1.;
float polar_repeat = 1.;

// Functions
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

vec4 effect(vec2 screen_size, vec2 screen_coords) {
	//Convert to UV coords (0-1) and floor for pixel effect
    highp float pixel_size = length(screen_size.xy) / pixel_filter;
    highp vec2 uv = (floor(screen_coords.xy * (1. / pixel_size)) * pixel_size - 0.5 * screen_size.xy) / length(screen_size.xy);
    highp float uv_len = length(uv);

	//Adding in a center swirl, changes with time. Only applies meaningfully if the 'spin amount' is a non-zero number
    highp float speed = spin_rotation;
    highp float new_pixel_angle = (atan(uv.y, uv.x)) + speed - SPIN_EASE * 20. * (1. * spin_amount * uv_len + (1. - 1. * spin_amount));
    highp vec2 mid = (screen_size.xy / length(screen_size.xy)) / 2.;
    uv = (vec2((uv_len * cos(new_pixel_angle) + mid.x), (uv_len * sin(new_pixel_angle) + mid.y)) - mid);

	//Now add the paint effect to the swirled UV
    uv *= 30.;
    speed = delta_time * spin_speed;
    highp vec2 uv2 = vec2(uv.x + uv.y);

    for(int i = 0; i < 5; i++) {
        uv2 += sin(max(uv.x, uv.y)) + uv;
        uv += 0.5 * vec2(cos(5.1123314 + 0.353 * uv2.y + speed * 0.131121), sin(uv2.x - 0.113 * speed));
        uv -= 1.0 * cos(uv.x + uv.y) - 1.0 * sin(uv.x * 0.711 - uv.y);
    }

	//Make the paint amount range from 0 - 2
    highp float contrast_mod = (0.25 * contrast + 0.5 * spin_amount + 1.2);
    highp float paint_res = min(2., max(0., length(uv) * (0.035) * contrast_mod));
    highp float c1p = max(0., 1. - contrast_mod * abs(1. - paint_res));
    highp float c2p = max(0., 1. - contrast_mod * abs(paint_res));
    highp float c3p = 1. - min(1., c1p + c2p);

    highp vec4 ret_col = (0.3 / contrast) * colour_1 + (1. - 0.3 / contrast) * (colour_1 * c1p + colour_2 * c2p + vec4(c3p * colour_3.rgb, c3p * colour_1.a));
    return ret_col;
}

vec2 polar_coords(vec2 uv, vec2 center, float zoom, float repeat) {
    vec2 dir = uv - center;
    float radius = length(dir) * 2.0;
    float angle = atan(dir.y, dir.x) * 1.0 / (PI * 2.0);
    return mod(vec2(radius * zoom, angle * repeat), 1.0);
}

// Main
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

void main() {
    vec2 coords = fragTexCoord.xy;

    if(polar_coordinates) {
        coords = polar_coords(coords, polar_center, polar_zoom, polar_repeat);
    }

    gl_FragColor = effect(vec2(1, 1), coords) * colDiffuse;
}