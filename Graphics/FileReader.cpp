#include "FileReader.h"
#include "stdafx.h"
#include <fstream>
#include <mutex>
#include <ppl.h>
#include <ppltasks.h>
#include <zlib.h> // From Nuget Package

namespace fileReader
{
	std::shared_ptr<std::vector<byte>> NullFile = std::make_shared<std::vector<byte>>(std::vector<byte>());
}

std::shared_ptr<std::vector<byte>> DecompressZippedFile(std::wstring& fileName);

std::shared_ptr<std::vector<byte>> ReadFileHelper(const std::wstring& fileName)
{
    struct _stat64 fileStat;
    int fileExists = _wstat64(fileName.c_str(), &fileStat);
    if (fileExists == -1)
        return fileReader::NullFile;

    std::ifstream file(fileName, std::ios::in | std::ios::binary);
    if (!file)
        return fileReader::NullFile;

    std::shared_ptr<std::vector<byte>> byteArray = std::make_shared<std::vector<byte> >(file.seekg(0, std::ios::end).tellg());
    file.seekg(0, std::ios::beg).read((char*)byteArray->data(), byteArray->size());
    file.close();

    ASSERT(byteArray->size() == (size_t)fileStat.st_size);

    return byteArray;
}

std::shared_ptr<std::vector<byte>> ReadFileHelperEx(std::shared_ptr<std::wstring> fileName)
{
    std::wstring zippedFileName = *fileName + L".gz";
    std::shared_ptr<std::vector<byte>> firstTry = DecompressZippedFile(zippedFileName);
    if (firstTry != fileReader::NullFile)
        return firstTry;

    return ReadFileHelper(*fileName);
}

std::shared_ptr<std::vector<byte>> Inflate(std::shared_ptr<std::vector<byte>> CompressedSource, int& err, uint32_t ChunkSize = 0x100000)
{
    // Create a dynamic buffer to hold compressed blocks
    std::vector<std::unique_ptr<byte> > blocks;

    z_stream strm = {};
    strm.data_type = Z_BINARY;
    strm.total_in = strm.avail_in = (uInt)CompressedSource->size();
    strm.next_in = CompressedSource->data();

    err = inflateInit2(&strm, (15 + 32)); //15 window bits, and the +32 tells zlib to to detect if using gzip or zlib

    while (err == Z_OK || err == Z_BUF_ERROR)
    {
        strm.avail_out = ChunkSize;
        strm.next_out = (byte*)malloc(ChunkSize);
        blocks.emplace_back(strm.next_out);
        err = inflate(&strm, Z_NO_FLUSH);
    }

    if (err != Z_STREAM_END)
    {
        inflateEnd(&strm);
        return fileReader::NullFile;
    }

    ASSERT(strm.total_out > 0, "Nothing to decompress");

    std::shared_ptr<std::vector<byte>> byteArray = std::make_shared<std::vector<byte> >(strm.total_out);

    // Allocate actual memory for this.
    // copy the bits into that RAM.
    // Free everything else up!!
    void* curDest = byteArray->data();
    size_t remaining = byteArray->size();

    for (size_t i = 0; i < blocks.size(); ++i)
    {
        ASSERT(remaining > 0);

        size_t CopySize = min(remaining, (size_t)ChunkSize);

        memcpy(curDest, blocks[i].get(), CopySize);
        curDest = (byte*)curDest + CopySize;
        remaining -= CopySize;
    }

    inflateEnd(&strm);

    return byteArray;
}

std::shared_ptr<std::vector<byte>> DecompressZippedFile(std::wstring& fileName)
{
    std::shared_ptr<std::vector<byte>> CompressedFile = ReadFileHelper(fileName);
    if (CompressedFile == fileReader::NullFile)
        return fileReader::NullFile;

    int error;
    std::shared_ptr<std::vector<byte>> DecompressedFile = Inflate(CompressedFile, error);
    if (DecompressedFile->size() == 0)
    {
        wprintf(L"Couldn't unzip file %s:  Error = %d\n", fileName.c_str(), error);
        return fileReader::NullFile;
    }

    return DecompressedFile;
}

namespace fileReader
{
    std::shared_ptr<std::vector<byte>> ReadFileSync(const std::wstring& fileName)
    {
        return ReadFileHelperEx(std::make_shared<std::wstring>(fileName));
    }

    Concurrency::task<std::shared_ptr<std::vector<byte>>> ReadFileAsync(const std::wstring& fileName)
    {
        using namespace Concurrency;

        std::shared_ptr<std::wstring> SharedPtr = std::make_shared<std::wstring>(fileName);
        return create_task([=] { return ReadFileHelperEx(SharedPtr); });
    }
}
