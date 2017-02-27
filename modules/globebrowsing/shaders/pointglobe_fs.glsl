/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014                                                                    *
 *                                                                                       *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this  *
 * software and associated documentation files (the "Software"), to deal in the Software *
 * without restriction, including without limitation the rights to use, copy, modify,    *
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to    *
 * permit persons to whom the Software is furnished to do so, subject to the following   *
 * conditions:                                                                           *
 *                                                                                       *
 * The above copyright notice and this permission notice shall be included in all copies *
 * or substantial portions of the Software.                                              *
 *                                                                                       *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,   *
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A         *
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT    *
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF  *
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE  *
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                         *
 ****************************************************************************************/

#include "fragment.glsl"

uniform vec3 directionToSunViewSpace;
uniform vec3 positionCameraSpace;

in vec4 vs_positionClipSpace;

Fragment getFragment() {

    vec2 pointCoord = (gl_PointCoord.xy - vec2(0.5)) * 2;
    pointCoord.y = -pointCoord.y; // y should point up in cam space
    if(length(pointCoord) > 1) // Outside of circle radius?
      discard;

    // z_coord of sphere
    float zCoord = sqrt(1 - pow(length(pointCoord),2));

    // Light calculations
    vec3 normal = normalize(vec3(pointCoord, zCoord));
    float cosTerm = max(dot(directionToSunViewSpace, normal), 0);

    vec3 color = vec3(1,1,1) * 0.7;
    vec3 shadedColor = cosTerm * color;

    Fragment frag;
    frag.color = vec4(shadedColor,1);
    frag.depth = vs_positionClipSpace.w;
    return frag;
}

