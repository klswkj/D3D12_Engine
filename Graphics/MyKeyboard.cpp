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

void MyKeyboard::onKeyPressed(unsigned char keycode) noexcept
{
	keystates[keycode] = true;
	keybuffer.push(KeyboardEvent(KeyboardEvent::Type::Press, keycode));
	trimBuffer(keybuffer);
}

void MyKeyboard::onKeyReleased(unsigned char keycode) noexcept
{
	keystates[keycode] = false;
	keybuffer.push(KeyboardEvent(KeyboardEvent::Type::Release, keycode));
	trimBuffer(keybuffer);
}

void MyKeyboard::onChar(char character) noexcept
{
	charbuffer.push(character);
	trimBuffer(charbuffer);
}

void MyKeyboard::clearState() noexcept
{
	keystates.reset();
}

template<typename T>
void MyKeyboard::trimBuffer(std::queue<T>& buffer) noexcept
{
	while(kBufferSize < buffer.size())
	{
		buffer.pop();
	}
}

