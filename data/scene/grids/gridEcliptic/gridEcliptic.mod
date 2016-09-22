return {
    -- SphericalGrid module
    {   
        Name = "SphericalGrid",
        Parent = "Root",
        Renderable = {
            Type = "RenderableSphericalGrid",
            GridType = "ECLIPJ2000",
            GridColor = { 0.4, 0.0, 0.0, 1},
            GridMatrix = { -0.05487554,  0.4941095, -0.8676661   , 0.0,
                           -0.9938214 , -0.1109906, -0.0003515167, 0.0,
                           -0.09647644,  0.8622859,  0.4971472   , 0.0,
                            0.0       ,  0.0      ,  0.0         , 1.0 },
            GridSegments = 36,
        }
    }
}