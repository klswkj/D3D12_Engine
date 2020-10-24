#include "stdafx.h"
#include "MyKeyboard.h"

BOOL MyKeyboard::KeyIsPressed(unsigned char keycode) const noexcept
{
	return keyStatesBitMap[keycode];
}

KeyboardEvent MyKeyboard::ReadEvent() noexcept
{
	if(0u < keyboardEventBuffer.size())
	{
		KeyboardEvent key = keyboardEventBuffer.front();
		keyboardEventBuffer.pop();
		return key;
	}
	else
	{
		return KeyboardEvent(KeyboardEvent::Type::Invalid, 0);
	}
}

BOOL MyKeyboard::KeyIsEmpty() const noexcept
{
	return keyboardEventBuffer.empty();
}

char MyKeyboard::ReadChar() noexcept
{
	if(0u < charBuffer.size())
	{
		unsigned char charcode = charBuffer.front();
		charBuffer.pop();
		return charcode;
	}
	return 0;
}

BOOL MyKeyboard::CharIsEmpty() const noexcept
{
	return charBuffer.empty();
}

void MyKeyboard::FlushKey() noexcept
{
	keyboardEventBuffer = std::queue<KeyboardEvent>();
}

void MyKeyboard::FlushChar() noexcept
{
	charBuffer = std::queue<char>();
}

void MyKeyboard::Flush() noexcept
{
	FlushKey();
	FlushChar();
}

void MyKeyboard::EnableAutorepeat() noexcept
{
	autorepeatEnabled = true;
}

void MyKeyboard::DisableAutorepeat() noexcept
{
	autorepeatEnabled = false;
}

BOOL MyKeyboard::AutorepeatIsEnabled() const noexcept
{
	return autorepeatEnabled;
}

void MyKeyboard::OnKeyPressed(unsigned char keycode) noexcept
{
	keyStatesBitMap[keycode] = true;
	keyboardEventBuffer.push(KeyboardEvent(KeyboardEvent::Type::Press, keycode));
	TrimBuffer(keyboardEventBuffer);
}

void MyKeyboard::OnKeyReleased(unsigned char keycode) noexcept
{
	keyStatesBitMap[keycode] = false;
	keyboardEventBuffer.push(KeyboardEvent(KeyboardEvent::Type::Release, keycode));
	TrimBuffer(keyboardEventBuffer);
}

void MyKeyboard::OnChar(char character) noexcept
{
	charBuffer.push(character);
	TrimBuffer(charBuffer);
}

void MyKeyboard::ClearState() noexcept
{
	keyStatesBitMap.reset();
}

template<typename T>
void MyKeyboard::TrimBuffer(std::queue<T>& buffer) noexcept
{
	while(kBufferSize < buffer.size())
	{
		buffer.pop();
	}
}

