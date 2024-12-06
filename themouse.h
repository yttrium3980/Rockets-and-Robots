#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

class TheMouse {

	LPDIRECTINPUT8 pdi;
	LPDIRECTINPUTDEVICE8 pmouse;

public:

	TheMouse(HWND hwnd)
	{
		HRESULT hr;
		hr = DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, (LPVOID*)&pdi, NULL);
		if(FAILED(hr)) fatalerror("call to DirectInput8Create failed");

		hr = pdi->CreateDevice(GUID_SysMouse, &pmouse, NULL);
		if(FAILED(hr)) fatalerror("call to CreateDevice failed");

		hr = pmouse->SetDataFormat(&c_dfDIMouse);
		if(FAILED(hr)) fatalerror("call to SetDataFormat failed");

		//hr = pmouse->SetCooperativeLevel(hwnd, DISCL_BACKGROUND | DISCL_EXCLUSIVE);
		//if(FAILED(hr)) fatalerror("call to SetCooperativeLevel failed");

		pmouse->Acquire();
	}

	void getstate(int *dx, int *dy)
	{
		DIMOUSESTATE m_state;
		pmouse->GetDeviceState(sizeof(DIMOUSESTATE), &m_state);
		*dx = m_state.lX;
		*dy = m_state.lY;
	}

void acquire(void)
	{
pmouse->Acquire();
	}

	void unacquire(void)
	{
pmouse->Unacquire();
	}

	~TheMouse()
	{
		if (!pmouse) return;
		pmouse->Unacquire();
		pmouse->Release();
	}


};