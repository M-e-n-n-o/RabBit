#include <RabBit.h>

class App : public RB::Application
{
public:
	App()
	{
		RB_ASSERT_FATAL_RELEASE(1 == 1, "Ow neeee");

		RB_LOG("Hoiii");
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