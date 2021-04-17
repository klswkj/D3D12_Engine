#pragma once

#if defined(UNICODE) || defined(_UNICODE)
#define tcout wcout
#else
#define tcout cout
#endif

static bool   g_bStackWalkingInitialized = false;
static bool   g_bNeedToRefreshSymbols    = false;
static HANDLE g_hProcessHandle           = INVALID_HANDLE_VALUE;

typedef bool  (WINAPI* TFEnumProcesses)(uint32_t* lpidProcess, uint32_t cb, uint32_t* cbNeeded);
typedef bool  (WINAPI* TFEnumProcessModules)(HANDLE hProcess, HMODULE* lphModule, uint32_t cb, LPDWORD lpcbNeeded);

#if WINVER > 0x502
typedef uint32_t(WINAPI* TFGetModuleBaseName)(HANDLE hProcess, HMODULE hModule, LPWSTR lpBaseName, uint32_t nSize);
typedef uint32_t(WINAPI* TFGetModuleFileNameEx)(HANDLE hProcess, HMODULE hModule, LPWSTR lpFilename, uint32_t nSize);
#else
typedef uint32(WINAPI* TFGetModuleBaseName)(HANDLE hProcess, HMODULE hModule, LPSTR lpBaseName, uint32_t nSize);
typedef uint32(WINAPI* TFGetModuleFileNameEx)(HANDLE hProcess, HMODULE hModule, LPSTR lpFilename, uint32_t nSize);
#endif

typedef bool (WINAPI* TFGetModuleInformation)(HANDLE hProcess, HMODULE hModule, LPMODULEINFO lpmodinfo, uint32_t cb);

static TFEnumProcesses			FEnumProcesses;
static TFEnumProcessModules		FEnumProcessModules;
static TFGetModuleBaseName		FGetModuleBaseName;
static TFGetModuleFileNameEx	FGetModuleFileNameEx;
static TFGetModuleInformation	FGetModuleInformation;

namespace custom
{
    class CStackWalk
    {
    public:
        explicit CStackWalk()
            :
            m_FrameSize(0u)
        {
            ::ZeroMemory(&m_pStacks, _countof(m_pStacks) * sizeof(PVOID));
        }
        struct CStackFrame final
        {
            explicit CStackFrame(DWORD address, DWORD offset, TCHAR* moduleName)
            {
                this->Address = address;
                this->Offset = offset;
                _tcscpy_s(this->ModuleName, moduleName);
            }

            DWORD Address;
            DWORD Offset;
            TCHAR ModuleName[256];
        };

        WORD GetCallStack(_Out_ std::list<CStackFrame>& stackFrameList)
        {
            ASSERT(!stackFrameList.size())

            m_FrameSize = ::RtlCaptureStackBackTrace(0ul, 63ul, m_pStacks, (PDWORD)nullptr);

            loadModuleList();

            for (WORD i = 0u; i < m_FrameSize; ++i)
            {
                DWORD StackAddress = (DWORD)(*reinterpret_cast<DWORD*>(&m_pStacks[i]));

                for (std::list<Module>::iterator it = m_ModuleList.begin(); it != m_ModuleList.end(); ++it)
                {
                    Module& Iter = *it;

                    if (Iter.IsContaining(StackAddress))
                    {
                        stackFrameList.emplace_back(CStackFrame(StackAddress, StackAddress - Iter.ModuleAddress, Iter.name));
                        break;
                    }
                }
            }

            return m_FrameSize;
        }

        WORD GetCallStack(_Out_ std::vector<CStackFrame>& stackFrames)
        {
            ASSERT(!stackFrames.size())

            m_FrameSize = ::RtlCaptureStackBackTrace(0ul, 63ul, m_pStacks, (PDWORD)nullptr);

            loadModuleList();

            for (WORD i = 0u; i < m_FrameSize; ++i)
            {
                DWORD StackAddress = (DWORD)(*reinterpret_cast<DWORD*>(&m_pStacks[i]));

                for (std::list<Module>::iterator it = m_ModuleList.begin(); it != m_ModuleList.end(); ++it)
                {
                    Module& Iter = *it;

                    if (Iter.IsContaining(StackAddress))
                    {
                        stackFrames.emplace_back(CStackFrame(StackAddress, StackAddress - Iter.ModuleAddress, Iter.name));
                        break;
                    }
                }
            }

            return m_FrameSize;
        }

    private:
        struct Module final
        {
            bool IsContaining(DWORD address)
            {
                return (this->ModuleAddress <= address) && (address <= this->ModuleAddress + Size);
            }

            DWORD ModuleAddress;
            DWORD Size;
            TCHAR name[256];
        };
        void loadModuleList()
        {
            MODULEENTRY32 module32 = {};
            module32.dwSize = sizeof(MODULEENTRY32);

            HANDLE hSnapShot = ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);

            DWORD LastError = ::GetLastError();
            if (::GetLastError() != ERROR_SUCCESS)
            {
                __debugbreak();
            }

            if (hSnapShot != INVALID_HANDLE_VALUE)
            {
                if (Module32First(hSnapShot, &module32))
                {
                    do
                    {
                        Module module = {};
                        module.ModuleAddress = (DWORD)(*reinterpret_cast<DWORD*>(&module32.modBaseAddr));
                        module.Size = module32.modBaseSize;
                        _tcscpy_s(module.name, module32.szModule);

                        m_ModuleList.push_back(module);

                    } while (Module32Next(hSnapShot, &module32));
                }

                ::CloseHandle(hSnapShot);
            }
        }

    private:
        PVOID             m_pStacks[63];
        WORD              m_FrameSize;
        std::list<Module> m_ModuleList;
    };

    void PrintStack()
    {
#if defined (_DEBUG)
        CStackWalk _StackWalk = {};
        WORD      StackSize  = 0u;

        std::list<CStackWalk::CStackFrame> OutStackFrameList;

        _StackWalk.GetCallStack(OutStackFrameList);

        ::printf("StackFrame Size : %d\n", StackSize);

        for (std::list<CStackWalk::CStackFrame>::iterator it = OutStackFrameList.begin(); it != OutStackFrameList.end(); it++)
        {
            CStackWalk::CStackFrame frame = *it;
            std::tcout << frame.ModuleName << "! 0x" << std::hex << frame.Offset << " (0x" << frame.Address << ")" << std::endl;
        }
#endif
    }

    bool InitStackWalkingInternal(void* Process)
    {
        if (g_hProcessHandle != INVALID_HANDLE_VALUE && !g_bNeedToRefreshSymbols)
        {
            return true;
        }

        g_hProcessHandle = Process;

        static custom::RAII_CS InitStackWalking_RAII_CS;
        custom::Scoped_CS InitStackWalking_Scoped_Lock(InitStackWalking_RAII_CS);

        // Only initialize once.
        if (!g_bStackWalkingInitialized)
        {
            HINSTANCE DllHandle = nullptr;
            DllHandle = ::LoadLibrary(TEXT("PSAPI.DLL"));

            if (!DllHandle)
            {
                DWORD LastError = ::GetLastError();
                ASSERT(!LastError);
            }
            
            // Load dynamically linked PSAPI routines.
            // FEnumProcesses = (TFEnumProcesses)::GetProcAddress(DllHandle, TEXT("EnumProcesses"));
            // FEnumProcessModules = (TFEnumProcessModules)::GetProcAddress(DllHandle, TEXT("EnumProcessModules"));
            FEnumProcesses      = (TFEnumProcesses)::GetProcAddress(DllHandle, "EnumProcesses");
            FEnumProcessModules = (TFEnumProcessModules)::GetProcAddress(DllHandle, "EnumProcessModules");

#if  0x502 < WINVER
            // FGetModuleFileNameEx = (TFGetModuleFileNameEx)::GetProcAddress(DllHandle, TEXT("GetModuleFileNameExW"));
            // FGetModuleBaseName = (TFGetModuleBaseName)::GetProcAddress(DllHandle, TEXT("GetModuleBaseNameW"));
            FGetModuleFileNameEx = (TFGetModuleFileNameEx)::GetProcAddress(DllHandle, "GetModuleFileNameExW");
            FGetModuleBaseName = (TFGetModuleBaseName)::GetProcAddress(DllHandle, "GetModuleBaseNameW");
#else
            // FGetModuleFileNameEx = (TFGetModuleFileNameEx)::GetProcAddress(DllHandle, TEXT("GetModuleFileNameExA"));
            // FGetModuleBaseName = (TFGetModuleBaseName)::GetProcAddress(DllHandle, TEXT("GetModuleBaseNameA"));
            FGetModuleFileNameEx = (TFGetModuleFileNameEx)::GetProcAddress(DllHandle, "GetModuleFileNameExA");
            FGetModuleBaseName = (TFGetModuleBaseName)::GetProcAddress(DllHandle, "GetModuleBaseNameA");
#endif
            // FGetModuleInformation = (TFGetModuleInformation)::GetProcAddress(DllHandle, TEXT("GetModuleInformation"));
            FGetModuleInformation = (TFGetModuleInformation)::GetProcAddress(DllHandle, "GetModuleInformation");

            // Abort if we can't look up the functions.
            ASSERT(!FEnumProcesses || !FEnumProcessModules || !FGetModuleFileNameEx || !FGetModuleBaseName || !FGetModuleInformation)
            
            // Set up the symbol engine.
            uint32_t SymOpts = SymGetOptions();

            SymOpts |= SYMOPT_LOAD_LINES;
            SymOpts |= SYMOPT_FAIL_CRITICAL_ERRORS;
            SymOpts |= SYMOPT_DEFERRED_LOADS;
            SymOpts |= SYMOPT_EXACT_SYMBOLS;

            // This option allows for undecorated names to be handled by the symbol engine.
            SymOpts |= SYMOPT_UNDNAME;

            // Disable by default as it can be very spammy/slow.  Turn it on if you are debugging symbol look-up!
            //		SymOpts |= SYMOPT_DEBUG;

            // Not sure these are important or desirable
            //		SymOpts |= SYMOPT_ALLOW_ABSOLUTE_SYMBOLS;
            //		SymOpts |= SYMOPT_CASE_INSENSITIVE;

            SymSetOptions(SymOpts);

            FString SymbolSearchPath = GetSymbolSearchPath();

            // Initialize the symbol engine.
#if WINVER > 0x502
            SymInitializeW(g_hProcessHandle, SymbolSearchPath.IsEmpty() ? nullptr : *SymbolSearchPath, true);
#else
            SymInitialize(GProcessHandle, nullptr, true);
#endif

            g_bNeedToRefreshSymbols = false;
            g_bStackWalkingInitialized = true;

            if (!FPlatformProperties::IsMonolithicBuild() && FPlatformStackWalk::WantsDetailedCallstacksInNonMonolithicBuilds())
            {
                const FString RemoteStorage = GetRemoteStorage(GetDownstreamStorage());
                LoadSymbolsForProcessModules(RemoteStorage);
            }
        }
#if WINVER > 0x502
        else if (g_bNeedToRefreshSymbols)
        {
            // Refresh and reload symbols
            SymRefreshModuleList(g_hProcessHandle);

            g_bNeedToRefreshSymbols = false;

            if (!FPlatformProperties::IsMonolithicBuild() && FPlatformStackWalk::WantsDetailedCallstacksInNonMonolithicBuilds())
            {
                const FString RemoteStorage = GetRemoteStorage(GetDownstreamStorage());
                // When refresh is needed we cannot track which modules have been loaded and are interesting
                // so load symbols for all modules the process has loaded.
                LoadSymbolsForProcessModules(RemoteStorage);
            }
        }
#endif

        return g_bStackWalkingInitialized;
    }

    static constexpr size_t MAX_NAME_LENGTH = 1024ul;

    struct ProgramCounterSymbolInfo final
    {
        /** Module name. */
        char ModuleName[MAX_NAME_LENGTH];
        char FunctionName[MAX_NAME_LENGTH];
        char Filename[MAX_NAME_LENGTH];

        int32_t LineNumber;
        int32_t SymbolDisplacement;
        uint64_t OffsetInModule;
        uint64_t ProgramCounter;
    };

    void ProgramCounterToSymbolInfo(uint64_t ProgramCounter, ProgramCounterSymbolInfo& out_SymbolInfo)
    {
        // Set the program counter.
        out_SymbolInfo.ProgramCounter = ProgramCounter;

        uint32_t LastError   = 0u;
        HANDLE ProcessHandle = ::GetCurrentProcess();

        // Initialize symbol.
        char SymbolBuffer[sizeof(IMAGEHLP_SYMBOL64) + MAX_NAME_LENGTH] = { 0 };
        IMAGEHLP_SYMBOL64* pSymbol                                      = (IMAGEHLP_SYMBOL64*)SymbolBuffer;
        pSymbol->SizeOfStruct = sizeof(SymbolBuffer);
        pSymbol->MaxNameLength = MAX_NAME_LENGTH;

        // Get function name.
        if (SymGetSymFromAddr64(ProcessHandle, ProgramCounter, nullptr, pSymbol))
        {
            // Skip any funky chars in the beginning of a function name.
            int32_t Offset = 0;

            while ((pSymbol->Name[Offset] < 32) || (127 < pSymbol->Name[Offset]))
            {
                ++Offset;
            }

            // Write out function name.
            strncpy(out_SymbolInfo.FunctionName, pSymbol->Name + Offset, MAX_NAME_LENGTH);
            strncat(out_SymbolInfo.FunctionName, "()", MAX_NAME_LENGTH);
        }
        else
        {
            // No symbol found for this address.
            LastError = GetLastError();
        }

        // Get filename and line number.
        IMAGEHLP_LINE64	ImageHelpLine = { 0 };
        ImageHelpLine.SizeOfStruct = sizeof(ImageHelpLine);
        if (SymGetLineFromAddr64(ProcessHandle, ProgramCounter, (::DWORD*)&out_SymbolInfo.SymbolDisplacement, &ImageHelpLine))
        {
            strncpy(out_SymbolInfo.Filename, ImageHelpLine.FileName, MAX_NAME_LENGTH);
            out_SymbolInfo.LineNumber = ImageHelpLine.LineNumber;
        }
        else
        {
            LastError = GetLastError();
        }

        // Get module name.
        IMAGEHLP_MODULE64 ImageHelpModule = { 0 };
        ImageHelpModule.SizeOfStruct = sizeof(ImageHelpModule);
        if (SymGetModuleInfo64(ProcessHandle, ProgramCounter, &ImageHelpModule))
        {
            // Write out module information.
            strncpy(out_SymbolInfo.ModuleName, ImageHelpModule.ImageName, MAX_NAME_LENGTH);
        }
        else
        {
            LastError = GetLastError();
        }
    }

}