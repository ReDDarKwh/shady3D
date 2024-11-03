fn sdBox(p: vec2f, b: vec2f) -> f32 {
    var d: vec2f = abs(p) - b;
    return length(max(d, vec2(0.0, 0.0))) + min(max(d.x, d.y), 0.0);
}

fn sdVesica(p: vec2f, r: f32, d: f32) -> f32 {
    let p1 = abs(p);
    let b = sqrt(r * r - d * d);
    if (p1.y - b) * d > p1.x * b {
        return length(p1 - vec2(0.0, b));
    } else {
        return length(p1 - vec2(-d, 0.0)) - r;
    }
}

fn dot2(v: vec2f) -> f32 { return dot(v, v); }
fn sdRoundedCross(p: vec2f, h: f32) -> f32 {
    let k = 0.5 * (h + 1.0 / h);

    let p1 = abs(p);

    if p1.x < 1.0 && p1.y < p1.x * (k - h) + h {
        return k - sqrt(dot2(p1 - vec2(1, k)));
    } else {

        return sqrt(min(dot2(p1 - vec2(0, h)),     // top corner
            dot2(p1 - vec2(1, 0))));
    }
     // right corner
}

fn sdEgg(p: vec2f, he: f32, ra: f32, rb: f32) -> f32 {
    let ce = 0.5 * (he * he - (ra - rb) * (ra - rb)) / (ra - rb);

    let p1 = vec2(abs(p.x), p.y);

    if p1.y < 0.0 {
        return length(p1) - ra;
    }
    if p1.y * ce - p1.x * he > he * ce { return length(vec2(p1.x, p1.y - he)) - rb;}
    return length(vec2(p1.x + ce, p1.y)) - (ce + ra);
}

fn rotateUV(uv: vec2f, rotation: f32) -> vec2f {
    let mid = 0.5;
    let cosAngle = cos(rotation);
    let sinAngle = sin(rotation);
    return vec2(
        cosAngle * (uv.x - mid) + sinAngle * (uv.y - mid) + mid,
        cosAngle * (uv.y - mid) - sinAngle * (uv.x - mid) + mid
    );
}

fn sdCircle(p: vec2f, r: f32) -> f32 {
    return length(p) - r;
}

@fragment
fn fs_main(@location(0) fragUV: vec2<f32>) -> @location(0) vec4f {


    let uv = abs((fragUV - 0.5));

    let uv2 = fract((fragUV - 0.5) * vec2(1.7, 1.6));
    let uv3 = vec2(1 - uv2.x, uv2.y);


    let s = 0.4;

    let logoOffset = 0.5;

    let smin = 0.1;
    let smax = 0.2;

    let sideIn = 1 - smoothstep(0.0, 0.005, sdCircle((uv2 - logoOffset + vec2(0.042, 0.094)) * 3.1 * vec2(1, 0.7), 0.1));
    let sideIn2 = 1 - smoothstep(0.0, 0.005, sdCircle((uv3 - logoOffset + vec2(0.042, 0.094)) * 3.1 * vec2(1, 0.7), 0.1));

    let sideOut = max(0.0, 1 - smoothstep(0.0, 0.01, sdCircle((uv2 - logoOffset + vec2(0.01, 0.13)) * 3.6, 0.1)) - (sideIn + sideIn2));
    let sideOut2 = max(0.0, 1 - smoothstep(0.0, 0.01, sdCircle((uv3 - logoOffset + vec2(0.01, 0.13)) * 3.6, 0.1)) - (sideIn + sideIn2));

    let eggIn = 1 - smoothstep(0.0, 0.12, sdEgg((rotateUV((uv2 - logoOffset + vec2(0.08, 0.075)) * 60 * vec2(0.9, 0.8), -2.43)), 2.0, 0.94, 0.0));
    let eggOut = max(0.0, 1 - smoothstep(0.0, 0.07, sdEgg((rotateUV((uv2 - logoOffset + vec2(0.092, 0.07)) * 25 * vec2(1.1, 0.8), -2.43)), 2.0, 0.94, 0.0)) - eggIn);

    let eggIn2 = 1 - smoothstep(0.0, 0.12, sdEgg((rotateUV((uv3 - logoOffset + vec2(0.08, 0.075)) * 60 * vec2(0.9, 0.8), -2.43)), 2.0, 0.94, 0.0));
    let eggOut2 = max(0.0, 1 - smoothstep(0.0, 0.07, sdEgg((rotateUV((uv3 - logoOffset + vec2(0.092, 0.07)) * 25 * vec2(1.1, 0.8), -2.43)), 2.0, 0.94, 0.0)) - eggIn2);

    let centerCross = 1 - smoothstep(0.0, 0.02, sdRoundedCross((rotateUV(uv2, 1.5707963267948966) - logoOffset + vec2(-0.04, 0.0)) * 10 * vec2(0.7, 1.0), 7.0) - 0.085);
    let tailTop = 1 - smoothstep(0.0, 0.01, sdVesica((uv2 - logoOffset + vec2(0.0, -0.09)) * 5, 1.15, 1.0));
    let tailBottom = 1 - smoothstep(0.0, 0.02, sdVesica((uv2 - logoOffset + vec2(0.0, 0.15)) * 12, 1.15, 1.0));
    let bar = 1 - smoothstep(0.0, 0.001, sdBox(uv2 - logoOffset + vec2(0, 0.09), vec2(0.06, 0.015)));
    let bg = 1 - smoothstep(0.0, 0.001, sdBox(uv - vec2(0.302, 0.314), vec2(0.22, 0.19))) - (tailTop + bar + tailBottom + centerCross + eggOut + eggOut2 + sideOut + sideOut2);

    var c = mix(vec3f(1.0, 1.0, 1.0), vec3f(0.3, 0.4, 0.0), bg);

    return vec4f(c, 1);
}

struct VertexOutput {
  @builtin(position) position: vec4f,
  @location(0) fragUV: vec2<f32>,
};

@vertex
fn vs_main(
    @builtin(vertex_index) VertexIndex: u32
) -> VertexOutput {
    var pos = array(
        vec2(-1.0, 3),
        vec2(3, -1.0),
        vec2(-1.0, -1.0)
    );

    var uv = array(
        vec2(0.0, 1.0),
        vec2(1.0, 0.0),
        vec2(0.0, 0.0),
    );

    var output: VertexOutput;
    output.position = vec4(pos[VertexIndex], 0.0, 1.0);
    output.fragUV = uv[VertexIndex] * 2.0;
    return output;
}