#include "RabBitCommon.h"
#include "Transform.h"
#include "entity/GameObject.h"

namespace RB::Entity
{
    Math::Float4x4 Transform::GetLocalToWorldMatrix() const
    {
        Math::Float4x4 model_matrix = rotation.ToMatrix();
        model_matrix.Scale(scale);
        model_matrix.SetPosition(position);

        GameObject* parent_obj = m_GameObject->GetParent();
        if (parent_obj)
        {
            const Transform* parent_transform = parent_obj->GetComponent<Transform>();
            return model_matrix * parent_transform->GetLocalToWorldMatrix();
        }

        return model_matrix;
    }
}