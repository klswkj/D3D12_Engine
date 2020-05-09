#include "ExceptionMessage.h"
#include <iostream>
#include <sstream>
ExceptionMessage::ExceptionMessage(int line, const char* file) noexcept
	:
	line(line),
	file(file)
{
}

const char* ExceptionMessage::what() const noexcept
{
	std::ostringstream oss;
	oss << GetType() << std::endl
		<< GetOriginString();
	whatBuffer = oss.str();
	return whatBuffer.c_str();
}

const char* ExceptionMessage::GetType() const noexcept
{
	return "ExceptionMessage";
}

int ExceptionMessage::GetLine() const noexcept
{
	return line;
}

const std::string& ExceptionMessage::GetFile() const noexcept
{
	return file;
}

std::string ExceptionMessage::GetOriginString() const noexcept
{
	std::ostringstream oss;
	oss << "[File] " << file << std::endl
		<< "[Line] " << line;
	return oss.str();
}