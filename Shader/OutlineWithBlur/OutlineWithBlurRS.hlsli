#define OutlineWithBlur_RootSignature                                                      \
    "RootFlags(0)",                                                                        \
    "DescriptorTable(CBV(b0, numDescriptors = 2), visibility = SHADER_VISIBILITY_PIXEL),"  \
    "DescriptorTable(SRV(t0, numDescriptors = 1), visibility = SHADER_VISIBILITY_PIXEL),"                                       \
    "StaticSampler(s0, visibility = SHADER_VISIBILITY_PIXEL,"                              \
        "addressU = TEXTURE_ADDRESS_MIRROR,"                                               \
        "addressV = TEXTURE_ADDRESS_MIRROR,"                                               \
        "addressW = TEXTURE_ADDRESS_MIRROR,"                                               \
        "comparisonFunc = COMPARISON_NEVER,"                                               \
        "filter = FILTER_MIN_MAG_LINEAR_MIP_POINT)"