#include "RabBitCommon.h"
#include "Misc.h"

namespace RB::Math
{
    AABB TransformAABBToWorld(const AABB& local, const Float4x4& model)
    {
        Float3 center = (local.min + local.max) * 0.5f;
        Float3 extents = (local.max - local.min) * 0.5f;
        Float3 world_center = center * model;

        // Compute world extents using absolute rotation
        Float3 world_extents;
        world_extents.x = Abs(model.a00) * extents.x + Abs(model.a01) * extents.y + Abs(model.a02) * extents.z;
        world_extents.y = Abs(model.a10) * extents.x + Abs(model.a11) * extents.y + Abs(model.a12) * extents.z;
        world_extents.z = Abs(model.a20) * extents.x + Abs(model.a21) * extents.y + Abs(model.a22) * extents.z;

        return { world_center - world_extents, world_center + world_extents };
    }
}