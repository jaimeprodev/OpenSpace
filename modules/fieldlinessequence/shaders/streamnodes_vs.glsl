/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014-2020                                                               *
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

#version __CONTEXT__

// General Uniforms that's always needed
uniform vec4      lineColor;
uniform mat4      modelViewProjection;

// Uniforms needed to color by quantity
uniform int       colorMode;
uniform sampler1D colorTable;
uniform vec2      colorTableRange;

// Uniforms needed for Particle Flow
uniform vec4      flowColor;
uniform int       particleSize;
uniform int       particleSpeed;
uniform int       particleSpacing;
uniform double    time;
uniform bool      usingParticles;

// Masking Uniforms
uniform bool      usingMasking;
uniform vec2      maskingRange;

// Domain Uniforms
uniform bool      usingDomain;
uniform vec2      domainLimX;
uniform vec2      domainLimY;
uniform vec2      domainLimZ;
uniform vec2      domainLimR;

// Streamnodes specific uniforms
uniform float nodeSize;
uniform float nodeSizeLargerFlux;
uniform vec4 streamColor;
uniform float thresholdFlux;
uniform float filterRadius;
uniform float filterUpper;
uniform int ScalingMode;
uniform int NodeskipMethod;
uniform int Nodeskip;
uniform int Nodeskipdefault;
uniform float NodeskipFluxThreshold;
uniform float NodeskipRadiusThreshold;
uniform float fluxColorAlpha;
uniform vec3 earthPos;
uniform float DistanceThreshold;
uniform int DistanceMethod;
uniform int activestreamnumber;

// Inputs
// Should be provided in meters
layout(location = 0) in vec3 in_position;

// The extra value used to color lines. Location must correspond to _VA_COLOR in 
// renderablefieldlinessequence.h
layout(location = 1) in float fluxValue;

// The extra value used to mask out parts of lines. Location must correspond to 
// _VA_MASKING in renderablefieldlinessequence.h
layout(location = 2)
in float rValue;

// The vertex index of every node. Location must correspond to 
// _VA_INDEX in renderableStreamNodes.h
layout(location = 3)
in int nodeIndex;
// The vertex streamnumber of every node. Location must correspond to 
// VaStreamnumber in renderableStreamNodes.h
layout(location = 4)
in int Streamnumber;

// These should correspond to the enum 'ColorMode' in renderablestreamnodes.cpp
const int uniformColor     = 0;
const int colorByFluxValue  = 1;

const int uniformskip = 0;
const int Fluxskip = 1;
const int Radiusskip = 2;


const int Fluxmode = 0;
const int RFlux = 1;
const int R2Flux = 2;
const int log10RFlux = 3;
const int lnRFlux = 4;
out vec4 vs_color;
out float vs_depth;
//out vec4 vs_gPosition;

vec4 getTransferFunctionColor() {
    // Remap the color scalar to a [0,1] range
    float scalevalue = 0;
    if(ScalingMode == Fluxmode){
        scalevalue = fluxValue;
    }
    else if(ScalingMode == RFlux){
       scalevalue = rValue * fluxValue;
    }
    else if(ScalingMode == log10RFlux){
        //conversion from logbase e to log10 since glsl does not support log10. 
        float logtoTen = log(rValue) / log(10);
        scalevalue = logtoTen * fluxValue;
    }
    else if(ScalingMode == lnRFlux){
        scalevalue = log(rValue) * fluxValue;
    }
    else if(ScalingMode == R2Flux){
        scalevalue = rValue * rValue * fluxValue;
    }

    //if(scalevalue > thresholdFlux){
        float lookUpVal = (scalevalue - colorTableRange.x)/(colorTableRange.y - colorTableRange.x);
        return texture(colorTable, lookUpVal);
    //}
   // return vec4(0);
}

bool isPartOfParticle(const double time, const int vertexId, const int particleSize,
                      const int particleSpeed, const int particleSpacing) {
    int modulusResult = int(double(particleSpeed) * time + vertexId) % particleSpacing;
    return modulusResult > 0 && modulusResult <= particleSize;
}
bool CheckvertexIndex(){
    if(NodeskipMethod == uniformskip){
        
        if(mod(nodeIndex, Nodeskip) == 0){
        return true;
        }
    }
    else if(NodeskipMethod == Fluxskip){
        
        if(fluxValue > NodeskipFluxThreshold && mod(nodeIndex, Nodeskip) == 0){
        return true;
        }
        if(fluxValue < NodeskipFluxThreshold && mod(nodeIndex, Nodeskipdefault) == 0){
        return true;
        }
    }
    else if(NodeskipMethod == Radiusskip){
        if(rValue < NodeskipRadiusThreshold && mod(nodeIndex, Nodeskip) == 0){
        return true;
        }
        if(rValue > NodeskipRadiusThreshold && mod(nodeIndex, Nodeskipdefault) == 0){
        return true;
        }
    }
    return false;
}

void main() {

    //vs_color = streamColor;
    if(CheckvertexIndex()){
    if(rValue > filterRadius && rValue < filterUpper){ //if(rValue > filterRadius){
        if(in_position.z > domainLimZ.x && in_position.z < domainLimZ.y){
            if(colorMode == 0){
                vs_color = streamColor;
            }
            else{ //else if (colorMode == 1){
                vec4 fluxColor = getTransferFunctionColor();

                if(fluxValue > thresholdFlux){
                    vs_color = vec4(fluxColor.xyz, fluxColor.w);                
                }
                else{
                    vs_color = vec4(fluxColor.xyz, fluxColorAlpha);   
                }
            }
        }
        else{
            vs_color = vec4(0);
            }
        }
    else{
        vs_color = vec4(0);
        }
    }
    else{
        vs_color = vec4(0);
    }

    if(fluxValue < thresholdFlux){
        gl_PointSize = nodeSize;
    }
    else{
        gl_PointSize = nodeSizeLargerFlux;
    }
    if(Streamnumber != activestreamnumber && NodeskipMethod == 3){
        vs_color = vec4(0);
    }
  

    
        
        vec3 vertexPosRelativetoEarth;
        vertexPosRelativetoEarth.x = in_position.x; // + earthPos.x;
        vertexPosRelativetoEarth.y = in_position.y;// + earthPos.y;
        vertexPosRelativetoEarth.z = in_position.z;// + earthPos.z; 
        // + in_position;
        if(DistanceMethod == 0){
        if(distance(earthPos, vertexPosRelativetoEarth) < DistanceThreshold){
        //if(earthPos.x - vertexPosRelativetoEarth.x < DistanceThreshold){
        gl_PointSize = 10;
        }
        }
        else if(DistanceMethod == 1){
        if(abs(earthPos.x - vertexPosRelativetoEarth.x) < DistanceThreshold){
        gl_PointSize = 10;
        }
        }
        
        else if(DistanceMethod == 2){
        if(abs(earthPos.y - vertexPosRelativetoEarth.y) < DistanceThreshold){
        gl_PointSize = 10;
        }
        }

        else if(DistanceMethod == 3){
        if(abs(earthPos.z - vertexPosRelativetoEarth.z) < DistanceThreshold){
        gl_PointSize = 10;
        }
        }

        
        vec4 position_in_meters = vec4(in_position, 1);
        vec4 positionClipSpace = modelViewProjection * position_in_meters;
        //vs_gPosition = vec4(modelViewTransform * dvec4(in_point_position, 1));
        
        //gl_PointSize = nodeSize;
        gl_Position = vec4(positionClipSpace.xy, 0, positionClipSpace.w);
          
        vs_depth = gl_Position.w;
}
