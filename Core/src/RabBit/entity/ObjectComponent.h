#pragma once
#include "RabBitCommon.h"

namespace RB::Entity
{
	#define DEFINE_COMP_TAG(name) static char* GetComponentTag() { return (name); }

	class GameObject;

	class ObjectComponent
	{
	public:
		virtual ~ObjectComponent() = default;

		virtual void Update() = 0;

	protected:
		GameObject* m_GameObject;

	private:
		friend class GameObject;

		void OnAttachedToGameObject(GameObject* obj);
	};
}