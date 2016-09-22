return {
    {   
        Name = "ToyVolume 1",
        Parent = "Sun",
        Renderable = {
             Type = "RenderableToyVolume",
             Color = {1.0, 0.0, 0.0, 0.7},
             Translation = {0, 0, 0},
             Rotation = {0.5, 0, 0},
             ScalingExponent = 11
        },
    },
    {   
        Name = "ToyVolume 2",
        Parent = "Earth",
        Renderable = {
             Type = "RenderableToyVolume",
             Color = {1.0, 0.8, 0.0, 0.7},
             Translation = {0.0, 0.0, 0.0},
             Scaling = {5.0, 2.5, 5.0},
             ScalingExponent = 6
        },
    },
    {   
        Name = "ToyVolume 3",
        Parent = "Earth",
        Renderable = {
             Type = "RenderableToyVolume",
             Scaling = {2.0, 2.0, 2.0},
             Rotation = {3.14/2.0, 0, 0},
             Color = {1.0, 1.0, 1.0, 0.7},
             Translation = {0.0, 0.0, 0.0},
             ScalingExponent = 6
        },
    }
}
