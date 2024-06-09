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

		virtual void Update() {}

		GameObject* GetGameObject() const { return m_GameObject; }

	protected:
		GameObject* m_GameObject;

	private:
		friend class GameObject;

		void OnAttachedToGameObject(GameObject* obj);
	};
}