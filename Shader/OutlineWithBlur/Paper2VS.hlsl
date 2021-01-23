void main
(
    in uint VertID : SV_VertexID,
    out float4 Pos : SV_Position,
    out float2 Tex : TexCoord0
)
{
    Pos = float4(0.0f, 0.0f, 0.0f, 0.0f);
    Tex = float2(0.0f, 0.0f);
    
    [branch]
    switch (VertID)
    {
        case 0:
        {
                Pos = float4(-1.0f, 1.0f, 0.0f, 1.0f);
                // Tex = float2(0.0f, 0.0f);
                break;
            }
        case 1:
        {
                Pos = float4(1.0f, 1.0f, 0.0f, 1.0f);
                // Tex = float2(1.0f, 0.0f);
                break;
            }
        case 2:
        {
                Pos = float4(-1.0f, -1.0f, 0.0f, 1.0f);
                // Tex = float2(0.0f, 1.0f);
                break;
            }
        case 3:
        {
                Pos = float4(1.0f, 1.0f, 0.0f, 1.0f);
                // Tex = float2(1.0f, 0.0f);
                break;
            }
        case 4:
        {
                Pos = float4(1.0f, -1.0f, 0.0f, 1.0f);
                // Tex = float2(1.0f, 1.0f);
                break;
            }
        case 5:
        {
                Pos = float4(-1.0f, -1.0f, 0.0f, 1.0f);
                // Tex = float2(0.0f, 1.0f);
                break;
            }
    }
    
    Tex = float2((Pos.x + 1) / 2.0f, -(Pos.y - 1) / 2.0f);
    // Tex = float2((Pos.x + 2) / 2.0f, -(Pos.y - 2) / 2.0f);
}