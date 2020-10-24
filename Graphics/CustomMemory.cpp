#include "stdafx.h"
#include "CustomMemory.h"

LinearAllocator* LinearAllocator::Instance = nullptr;

void* custom::Alloc(size_t sizeInBytes)
{
	return LinearAllocator::Instance->Alloc(sizeInBytes);
}

void custom::Free(void* buffer)
{
	LinearAllocator::Instance->Free((byte*)buffer);
}

void* SystemHeapAllocator::Alloc(size_t size)
{
	return (void*)new byte[size];
}

void SystemHeapAllocator::Free(byte* buffer)
{
	delete[] buffer;
}
