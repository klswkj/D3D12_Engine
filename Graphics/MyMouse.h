#pragma once

#include <windef.h>
#include <queue>
#include <optional>
#include <WinUser.h>

class MouseEvent;

class MyMouse
{
	friend class Window;
	friend class MouseEvent;

public:
	MyMouse() = default;
	MyMouse(const MyMouse&) = delete;
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
	void EnableRaw() noexcept;
	void DisableRaw() noexcept;
	bool RawEnabled() const noexcept;
private:
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
	void trimBuffer() noexcept;
	void TrimRawInputBuffer() noexcept;
	void OnWheelDelta(int x, int y, int delta) noexcept;
private:
	static constexpr unsigned int kBufferSize = 16u;
	POINT MousePoint;
	bool leftIsPressed = false;
	bool rightIsPressed = false;
	bool isInWindow = false;
	int wheelDeltaCarry = 0;
	bool rawEnabled = false;
	std::queue<MouseEvent> buffer;
	std::queue<POINT> rawDeltaBuffer;
};

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
private:
	Type type;
	bool leftIsPressed;
	bool rightIsPressed;
	POINT MousePoint;
public:
	MouseEvent(Type type, const MyMouse& parent) noexcept
		:
		type(type),
		leftIsPressed(parent.leftIsPressed),
		rightIsPressed(parent.rightIsPressed),
		MousePoint(parent.MousePoint)
	{
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
};