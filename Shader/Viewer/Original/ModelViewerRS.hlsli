#define ModelViewer_RootSignature                                                          \
"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), "                                          \
"CBV(b0, visibility = SHADER_VISIBILITY_VERTEX),"                                          \
"CBV(b0, visibility = SHADER_VISIBILITY_PIXEL),"                                           \
"DescriptorTable(SRV(t0, numDescriptors = 6),  visibility = SHADER_VISIBILITY_PIXEL),"     \
"DescriptorTable(SRV(t64, numDescriptors = 6), visibility = SHADER_VISIBILITY_PIXEL),"     \
"StaticSampler(s0, maxAnisotropy = 8, visibility = SHADER_VISIBILITY_PIXEL),"              \
"StaticSampler(s1, visibility = SHADER_VISIBILITY_PIXEL,"                                  \
        "addressU = TEXTURE_ADDRESS_CLAMP,"                                                \
        "addressV = TEXTURE_ADDRESS_CLAMP,"                                                \
        "addressW = TEXTURE_ADDRESS_CLAMP,"                                                \
        "comparisonFunc = COMPARISON_GREATER_EQUAL,"                                       \
        "filter = FILTER_MIN_MAG_LINEAR_MIP_POINT)"

