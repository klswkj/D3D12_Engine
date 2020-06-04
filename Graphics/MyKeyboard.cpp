// #include "stdafx.h"
#include "MyKeyboard.h"

BOOL MyKeyboard::KeyIsPressed(unsigned char keycode) const noexcept
{
	return keystates[keycode];
}

KeyboardEvent MyKeyboard::ReadKey() noexcept
{
	if(0u < keybuffer.size())
	{
		KeyboardEvent key = keybuffer.front();
		keybuffer.pop();
		return key;
	}
	else
	{
		return KeyboardEvent(KeyboardEvent::Type::Invalid, 0);
	}
}

BOOL MyKeyboard::KeyIsEmpty() const noexcept
{
	return keybuffer.empty();
}

char MyKeyboard::ReadChar() noexcept
{
	if(0u < charbuffer.size())
	{
		unsigned char charcode = charbuffer.front();
		charbuffer.pop();
		return charcode;
	}
	return 0;
}

BOOL MyKeyboard::CharIsEmpty() const noexcept
{
	return charbuffer.empty();
}

void MyKeyboard::FlushKey() noexcept
{
	keybuffer = std::queue<KeyboardEvent>();
}

void MyKeyboard::FlushChar() noexcept
{
	charbuffer = std::queue<char>();
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
	keystates[keycode] = true;
	keybuffer.push(KeyboardEvent(KeyboardEvent::Type::Press, keycode));
	TrimBuffer(keybuffer);
}

void MyKeyboard::OnKeyReleased(unsigned char keycode) noexcept
{
	keystates[keycode] = false;
	keybuffer.push(KeyboardEvent(KeyboardEvent::Type::Release, keycode));
	TrimBuffer(keybuffer);
}

void MyKeyboard::OnChar(char character) noexcept
{
	charbuffer.push(character);
	TrimBuffer(charbuffer);
}

void MyKeyboard::ClearState() noexcept
{
	keystates.reset();
}

template<typename T>
void MyKeyboard::TrimBuffer(std::queue<T>& buffer) noexcept
{
	while(kBufferSize < buffer.size())
	{
		buffer.pop();
	}
}

