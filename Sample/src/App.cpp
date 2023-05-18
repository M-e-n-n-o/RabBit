#include <RabBit.h>

class App : public RB::Application
{
public:
	void Start() override
	{
		RB_LOG("Hoiii");
	}

	void Update() override
	{

	}
};

RB::Application* RB::CreateApplication()
{
	return new App();
}