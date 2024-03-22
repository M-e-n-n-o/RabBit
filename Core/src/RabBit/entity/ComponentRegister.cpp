#include "RabBitCommon.h"
#include "ComponentRegister.h"

namespace RB::Entity
{
	ComponentRegister* g_ComponentRegister = nullptr;
	
	ComponentRegister::ComponentRegister()
		: m_NextID(0)
	{
	}
}