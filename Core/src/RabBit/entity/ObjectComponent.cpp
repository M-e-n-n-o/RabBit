#include "RabBitCommon.h"
#include "ObjectComponent.h"

namespace RB::Entity
{
	void ObjectComponent::OnAttachedToGameObject(GameObject* obj)
	{
		m_GameObject = obj;
	}
}