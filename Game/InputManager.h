#pragma once

class InputManager
{
public:
	void OnKeyDown(int key) { m_keys[key] = true; }
	void OnKeyUp(int key) { m_keys[key] = false; }
	void OnMouseMove(int x, int y)
	{
		m_mouseDeltaX = x - m_mouseX;
		m_mouseDeltaY = y - m_mouseY;
		m_mouseX = x;
		m_mouseY = y;
	}
	void OnMouseWheel(int delta)
	{
		m_mouseWheelDelta = delta;
	}
	void OnMouseButtonDown(int button) { m_mouseButtons[button] = true; }
	void OnMouseButtonUp(int button) { m_mouseButtons[button] = false; }


	bool IsKeyDown(int vkCode) const { return m_keys[vkCode]; }

	bool IsMouseButtonDown(int button) const { return m_mouseButtons[button]; }
	int  GetMouseDeltaX() const { return m_mouseDeltaX; }
	int  GetMouseDeltaY() const { return m_mouseDeltaY; }
	int  GetMouseWheelDelta() const { return m_mouseWheelDelta; }
	void ResetMouseDeltas()
	{
		m_mouseDeltaX = 0;
		m_mouseDeltaY = 0;
		m_mouseWheelDelta = 0;
	}

private:
	bool m_keys[256] = {};

	// Mouse state
	bool m_mouseButtons[3] = {};  // 0: left, 1: right, 2: middle
	int  m_mouseX = 0;
	int  m_mouseY = 0;
	int  m_mouseDeltaX = 0;
	int  m_mouseDeltaY = 0;
	int  m_mouseWheelDelta = 0;

};

