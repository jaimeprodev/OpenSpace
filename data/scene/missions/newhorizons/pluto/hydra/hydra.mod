local charon_radius = 0.53E5

if UseAccurateNewHorizonsKernels then
    NewHorizonsKernels = {
        "${SPICE}/nh_20170126/spk/NavSE_plu047_od122.bsp"
    }
else
    NewHorizonsKernels = {
        "${SPICE}/NewHorizonsKernels/nh_p4p5_revised.bsp"
    }
end

return {
    -- Hydra module
    {   
        Name = "Hydra",
        Parent = "PlutoBarycenter",
        Renderable = {
            Type = "RenderablePlanet",
            Frame = "IAU_PLUTO",
            Body = "HYDRA",
            Radius = charon_radius,
            Geometry = {
                Type = "SimpleSphere",
                Radius = charon_radius,
                Segments = 100
            },
            ColorTexture = "textures/gray.jpg",
        },
        Transform = {
            Translation = {
                Type = "SpiceTranslation",
                Target = "HYDRA",
                Observer = "PLUTO BARYCENTER",
                Kernels = NewHorizonsKernels
            },
            Rotation = {
                Type = "SpiceRotation",
                SourceFrame = "IAU_PLUTO",
                DestinationFrame = "ECLIPJ2000"
            },
        },
    },
    {
        Name = "HydraText",
        Parent = "Hydra",
        Renderable = {
            Type = "RenderablePlane",
            Size = 10.0^6.3,
            Origin = "Center",
            Billboard = true,
            Texture = "textures/Hydra-Text.png"
        },
        Transform = {
            Translation = {
                Type = "StaticTranslation",
                Position = {1000000, 0, 1000000},
            },
        },
    },    
    -- HydraTrail module
    {   
        Name = "HydraTrail",
        Parent = "PlutoBarycenter",
        Renderable = {
            Type = "RenderableTrailOrbit",
            Translation = {
                Type = "SpiceTranslation",
                Target = "HYDRA",
                Observer = "PLUTO BARYCENTER",
            },
            Color = {0.00, 0.62, 1.00},
            Period = 38.20177,
            Resolution = 1000
        },
    }
    
}
