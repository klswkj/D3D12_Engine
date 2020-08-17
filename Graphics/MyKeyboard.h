// CODE
// (search for "[SECTION]" in the code to find them)
// Index of this file:
// [SECTION] Forward Declarations

#pragma once
#include <windows.h>
#include <queue>
#include <bitset>
#include <optional>

class KeyboardEvent
{
public:
	enum class Type
	{
		Press,
		Release,
		Invalid,
		Count
	};
public:
	KeyboardEvent(Type type, unsigned char code) noexcept
		: type(type), code(code)
	{
	}
	bool IsPress() const noexcept
	{
		return type == Type::Press;
	}
	bool IsRelease() const noexcept
	{
		return type == Type::Release;
	}
	unsigned char GetCode() const noexcept
	{
		return code;
	}
private:
	Type          type;
	unsigned char code;
};

class MyKeyboard
{
public:
	MyKeyboard()                             = default;
	MyKeyboard(const MyKeyboard&)            = delete;
	MyKeyboard& operator=(const MyKeyboard&) = delete;

	// key event stuff
	BOOL          KeyIsPressed(unsigned char keycode) const noexcept;
	KeyboardEvent ReadKey()                           noexcept;
	BOOL          KeyIsEmpty()                        const noexcept;
	void          FlushKey()                          noexcept;

	// char event stuff
	char ReadChar()    noexcept;
	BOOL CharIsEmpty() const noexcept;
	void FlushChar()   noexcept;
	void Flush()       noexcept;

	// autorepeat control
	void EnableAutorepeat()    noexcept;
	void DisableAutorepeat()   noexcept;
	BOOL AutorepeatIsEnabled() const noexcept;

	template<typename T>
	static void TrimBuffer(std::queue<T>& buffer)    noexcept;

	void        OnKeyPressed(unsigned char keycode)  noexcept;
	void        OnKeyReleased(unsigned char keycode) noexcept;
	void        OnChar(char character)               noexcept;
	void        ClearState()                         noexcept;

private:
	static constexpr unsigned int        kMaxKey           = 256u;
	static constexpr unsigned int        kBufferSize       = 16u;
	BOOL                                 autorepeatEnabled = false;
	std::bitset<kMaxKey>                 keystates;
	std::queue<KeyboardEvent>            keybuffer;
	std::queue<char>                     charbuffer;
};
