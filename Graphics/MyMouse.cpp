#include "MyMouse.h"

POINT MyMouse::GetPos() const noexcept
{
	return MousePoint;
}

POINT MyMouse::ReadRawDelta() noexcept
{
	if (rawDeltaBuffer.empty())
	{
		return POINT{ 0,0 };
	}
	const POINT d = rawDeltaBuffer.front();
	rawDeltaBuffer.pop();
	return d;
}

int MyMouse::GetPosX() const noexcept
{
	return MousePoint.x;
}

int MyMouse::GetPosY() const noexcept
{
	return MousePoint.y;
}

bool MyMouse::IsInWindow() const noexcept
{
	return isInWindow;
}

bool MyMouse::LeftIsPressed() const noexcept
{
	return leftIsPressed;
}

bool MyMouse::RightIsPressed() const noexcept
{
	return rightIsPressed;
}

MyMouse::MouseEvent MyMouse::Read() noexcept
{
	if (0u < buffer.size())
	{
		MouseEvent e = buffer.front();
		buffer.pop();
		return e;
	}
	return MouseEvent(MouseEvent::Type::Invalid, *this);
}

void MyMouse::Flush() noexcept
{
	buffer = std::queue<MouseEvent>();
}

void MyMouse::EnableRaw() noexcept
{
	rawEnabled = true;
}

void MyMouse::DisableRaw() noexcept
{
	rawEnabled = false;
}

bool MyMouse::RawEnabled() const noexcept
{
	return rawEnabled;
}

void MyMouse::OnMouseMove(int newx, int newy) noexcept
{
	MousePoint.x = newx;
	MousePoint.y = newy;

	buffer.push(MouseEvent(MouseEvent::Type::Move, *this));
	TrimBuffer();
}

void MyMouse::OnMouseLeave() noexcept
{
	isInWindow = false;
	buffer.push(MouseEvent(MouseEvent::Type::Leave, *this));
	TrimBuffer();
}

void MyMouse::OnMouseEnter() noexcept
{
	isInWindow = true;
	buffer.push(MouseEvent(MouseEvent::Type::Enter, *this));
	TrimBuffer();
}

void MyMouse::OnRawDelta(int dx, int dy) noexcept
{
	rawDeltaBuffer.push({dx, dy});
	TrimBuffer();
}

void MyMouse::OnLeftPressed(int x, int y) noexcept
{
	leftIsPressed = true;

	buffer.push(MouseEvent(MouseEvent::Type::LPress, *this));
	TrimBuffer();
}

void MyMouse::OnLeftReleased(int x, int y) noexcept
{
	leftIsPressed = false;

	buffer.push(MouseEvent(MouseEvent::Type::LRelease, *this));
	TrimBuffer();
}

void MyMouse::OnRightPressed(int x, int y) noexcept
{
	rightIsPressed = true;

	buffer.push(MouseEvent(MouseEvent::Type::RPress, *this));
	TrimBuffer();
}

void MyMouse::OnRightReleased(int x, int y) noexcept
{
	rightIsPressed = false;

	buffer.push(MouseEvent(MouseEvent::Type::RRelease, *this));
	TrimBuffer();
}

void MyMouse::OnWheelUp(int x, int y) noexcept
{
	buffer.push(MouseEvent(MouseEvent::Type::WheelUp, *this));
	TrimBuffer();
}

void MyMouse::OnWheelDown(int x, int y) noexcept
{
	buffer.push(MouseEvent(MouseEvent::Type::WheelDown, *this));
	TrimBuffer();
}

void MyMouse::TrimBuffer() noexcept
{
	while (kBufferSize < buffer.size())
	{
		buffer.pop();
	}
}

void MyMouse::TrimRawInputBuffer() noexcept
{
	while (rawDeltaBuffer.size() > kBufferSize)
	{
		rawDeltaBuffer.pop();
	}
}

void MyMouse::OnWheelDelta(int x, int y, int delta) noexcept
{
	wheelDeltaCarry += delta;

	while (WHEEL_DELTA <= wheelDeltaCarry)
	{
		wheelDeltaCarry -= WHEEL_DELTA;
		OnWheelUp(x, y);
	}
	while (wheelDeltaCarry <= -WHEEL_DELTA)
	{
		wheelDeltaCarry += WHEEL_DELTA;
		OnWheelDown(x, y);
	}
}
