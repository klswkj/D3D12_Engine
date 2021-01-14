#pragma once
#include <windows.h>
#include <queue>
#include <optional>
#include <Windows.h>

class MyMouse
{
	class MouseEvent
	{
	public:
		enum class Type
		{
			LPress,
			LRelease,
			RPress,
			RRelease,
			WheelUp,
			WheelDown,
			Move,
			Enter,
			Leave,
			Invalid,
			Count
		};

	public:
		MouseEvent(Type Type, MyMouse parent) noexcept
			: type(Type)
		{
			leftIsPressed = parent.leftIsPressed;
			rightIsPressed = parent.rightIsPressed;
			MousePoint.x = parent.MousePoint.x;
			MousePoint.y = parent.MousePoint.y;
		}
		Type GetType() const noexcept
		{
			return type;
		}
		POINT GetPos() const noexcept
		{
			return MousePoint;
		}
		int GetPosX() const noexcept
		{
			return MousePoint.x;
		}
		int GetPosY() const noexcept
		{
			return MousePoint.y;
		}
		bool LeftIsPressed() const noexcept
		{
			return leftIsPressed;
		}
		bool RightIsPressed() const noexcept
		{
			return rightIsPressed;
		}

	private:
		bool leftIsPressed;
		bool rightIsPressed;
		POINT MousePoint;
		Type type;
	};
public:
	MyMouse() = default;
	// MyMouse(const MyMouse&) = delete;
	MyMouse& operator=(const MyMouse&) = delete;
	POINT GetPos() const noexcept;
	POINT ReadRawDelta() noexcept;
	int GetPosX() const noexcept;
	int GetPosY() const noexcept;
	bool IsInWindow() const noexcept;
	bool LeftIsPressed() const noexcept;
	bool RightIsPressed() const noexcept;
	MouseEvent Read() noexcept;
	bool IsEmpty() const noexcept
	{
		return buffer.empty();
	}
	void Flush() noexcept;
	void EnableRawMouse() noexcept;
	void DisableRawMouse() noexcept;
	bool RawEnabled() const noexcept;
	void OnMouseMove(int x, int y) noexcept;
	void OnMouseLeave() noexcept;
	void OnMouseEnter() noexcept;
	void OnRawDelta(int dx, int dy) noexcept;
	void OnLeftPressed(int x, int y) noexcept;
	void OnLeftReleased(int x, int y) noexcept;
	void OnRightPressed(int x, int y) noexcept;
	void OnRightReleased(int x, int y) noexcept;
	void OnWheelUp(int x, int y) noexcept;
	void OnWheelDown(int x, int y) noexcept;
	void TrimBuffer() noexcept;
	void TrimRawInputBuffer() noexcept;
	void OnWheelDelta(int x, int y, int delta) noexcept;
private:
	static constexpr unsigned int kBufferSize = 16u;
	POINT MousePoint = {};
	bool leftIsPressed = false;
	bool rightIsPressed = false;
	bool isInWindow = false;
	int wheelDeltaCarry = 0;
	bool rawMouseEnabled = false;
	std::queue<MouseEvent> buffer;
	std::queue<POINT> rawDeltaBuffer;
};