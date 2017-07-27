return {
    -- Uranus barycenter module
    {
        Name = "UranusBarycenter",
        Parent = "SolarSystemBarycenter",
        Transform = {
            Translation = {
                Type = "SpiceTranslation",
                Target = "URANUS BARYCENTER",
                Observer = "SUN",
                Kernels = "${OPENSPACE_DATA}/spice/de430_1850-2150.bsp"
            },
        }
    },

    -- Uranus module
    {   
        Name = "Uranus",
        Parent = "UranusBarycenter",
        Renderable = {
            Type = "RenderablePlanet",
            Frame = "IAU_URANUS",
            Body = "URANUS BARYCENTER",
            Geometry = {
                Type = "SimpleSphere",
                Radius = 2.5362E7,
                Segments = 100
            },
            ColorTexture = "textures/uranus.jpg",
        },
        Tag = "planet_solarSystem",
        Transform = {
            Rotation = {
                Type = "SpiceRotation",
                SourceFrame = "IAU_URANUS",
                DestinationFrame = "ECLIPJ2000",
            },
            Scale = {
                Type = "StaticScale",
                Scale = 1,
            },
        },
    },

    -- UranusTrail module
    {   
        Name = "UranusTrail",
        Parent = "SolarSystemBarycenter",
        Renderable = {
            Type = "RenderableTrailOrbit",
            Translation = {
                Type = "SpiceTranslation",
                Target = "URANUS BARYCENTER",
                Observer = "SUN",
            },
            Color = {0.60, 0.95, 1.00 },
            Period = 30588.740,
            Resolution = 1000,
            Tag = "planetTrail_solarSystem"
        }
    }
}
