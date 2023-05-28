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

	void Stop() override
	{

	}
};

RB::Application* RB::CreateApplication()
{
	return new App();
}