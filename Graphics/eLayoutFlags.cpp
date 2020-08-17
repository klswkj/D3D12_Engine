#include "eLayoutFlags.h"

enum class eVertexLayoutFlags
{
    ATTRIB_MASK_0 = (1 << 0),
    ATTRIB_MASK_1 = (1 << 1),
    ATTRIB_MASK_2 = (1 << 2),
    ATTRIB_MASK_3 = (1 << 3),
    ATTRIB_MASK_4 = (1 << 4),
    ATTRIB_MASK_5 = (1 << 5),
    ATTRIB_MASK_6 = (1 << 6),
    ATTRIB_MASK_7 = (1 << 7),
    ATTRIB_MASK_8 = (1 << 8),
    ATTRIB_MASK_9 = (1 << 9),
    ATTRIB_MASK_10 = (1 << 10),
    ATTRIB_MASK_11 = (1 << 11),
    ATTRIB_MASK_12 = (1 << 12),
    ATTRIB_MASK_13 = (1 << 13),
    ATTRIB_MASK_14 = (1 << 14),
    ATTRIB_MASK_15 = (1 << 15),

    ATTRIB_MASK_POSITION_F3         = ATTRIB_MASK_0, // Postion float3
    ATTRIB_MASK_NORMAL_F3           = ATTRIB_MASK_1, // Normal  float2
    ATTRIB_MASK_TEXCOORD_F2         = ATTRIB_MASK_2, // Texture Coordinate float2
    ATTRIB_MASK_NORMAL_TANGENT_F3   = ATTRIB_MASK_3, // Normal Tagent Vector float3
    ATTRIB_MASK_NORMAL_BITANGENT_F3 = ATTRIB_MASK_4,  // Normal BiTangent Vector float3
    ATTRIB_MASK_COUNT               = ATTRIB_MASK_5
};

enum class ePixelLayoutFlags
{
    ATTRIB_MASK_0 = (1 << 16),
    ATTRIB_MASK_1 = (1 << 17),
    ATTRIB_MASK_2 = (1 << 18),
    ATTRIB_MASK_3 = (1 << 19),
    ATTRIB_MASK_4 = (1 << 20),
    ATTRIB_MASK_5 = (1 << 21),
    ATTRIB_MASK_6 = (1 << 22),
    ATTRIB_MASK_7 = (1 << 23),
    ATTRIB_MASK_8 = (1 << 24),
    ATTRIB_MASK_9 = (1 << 25),
    ATTRIB_MASK_10 = (1 << 26),
    ATTRIB_MASK_11 = (1 << 27),
    ATTRIB_MASK_12 = (1 << 28),
    ATTRIB_MASK_13 = (1 << 29),
    ATTRIB_MASK_14 = (1 << 30),
    ATTRIB_MASK_15 = (1 << 31),

    ATTRIB_MASK_DIFFUSE_TEXTURE_F2  = ATTRIB_MASK_0,
    ATTRIB_MASK_MATERIAL_COLOR_F3   = ATTRIB_MASK_1,
    ATTRIB_MASK_SPEC_GLOSS_ALPHA_B1 = ATTRIB_MASK_2,
    ATTRIB_MASK_SPEC_MAP_B1         = ATTRIB_MASK_3,
    ATTRIB_MASK_SPEC_TEXTURE_F2     = ATTRIB_MASK_4,
    ATTRIB_MASK_SPEC_COLOR_F3       = ATTRIB_MASK_5,
    ATTRIB_MASK_SPEC_WEIGHT_F1      = ATTRIB_MASK_6,
    ATTRIB_MASK_SPEC_GLOSS_F1       = ATTRIB_MASK_7,
    ATTRIB_MASK_NORMAL_B1           = ATTRIB_MASK_8,
    ATTRIB_MASK_NORMAL_WEIGHT_F1    = ATTRIB_MASK_9,
    ATTRIB_MASK_NORMAL_TEXTURE_F2   = ATTRIB_MASK_10,
    ATTRIB_MASK_COUNT               = ATTRIB_MASK_11
};