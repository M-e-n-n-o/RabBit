#include <RabBit.h>

class App : public RB::Application
{
public:
	App()
	{
		
	}

	~App()
	{

	}

	void Update() override
	{

	}
};

RB::Application* RB::CreateApplication()
{
	return new App();
}