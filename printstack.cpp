
#ifdef _MSC_VER

#include <Windows.h>
#include <dbghelp.h>
#include <cstdio>
#include <string>
#include <stack>

#pragma comment(lib , "DbgHelp.lib")

static void getModuleName(HANDLE process, DWORD64 address, std::string* returnedModuleName)
{
	IMAGEHLP_MODULE64 module64;
	memset(&module64, 0, sizeof(module64));
	module64.SizeOfStruct = sizeof(module64);
	BOOL ret = SymGetModuleInfo64(process, address, &module64);
	if (FALSE == ret) {
		returnedModuleName->clear();
		return;
	}
	char* moduleName = module64.LoadedImageName;
	char* backslash = strrchr(moduleName, '\\');
	if (backslash) {
		moduleName = backslash + 1;
	}
	*returnedModuleName = moduleName;
}

void printStack(void)
{
	SymInitialize(GetCurrentProcess(), NULL, TRUE);

	STACKFRAME StackFrame;
	CONTEXT Context;
	memset(&StackFrame, 0, sizeof(STACKFRAME));
	memset(&Context, 0, sizeof(CONTEXT));

	Context.ContextFlags = (CONTEXT_FULL);
	RtlCaptureContext(&Context);

#if defined(_M_AMD64)
	// 64位程序
	DWORD imageType = IMAGE_FILE_MACHINE_AMD64;
	StackFrame.AddrPC.Offset = Context.Rip;
	StackFrame.AddrFrame.Offset = Context.Rbp;
	StackFrame.AddrStack.Offset = Context.Rsp;
#elif defined(_M_IX86)
	// 32位程序
	DWORD imageType = IMAGE_FILE_MACHINE_I386;
	StackFrame.AddrPC.Offset = Context.Eip;
	StackFrame.AddrFrame.Offset = Context.Ebp;
	StackFrame.AddrStack.Offset = Context.Esp;
#else
#error Neither _M_IX86 nor _M_AMD64 were defined
#endif
	StackFrame.AddrPC.Mode = AddrModeFlat;
	StackFrame.AddrFrame.Mode = AddrModeFlat;
	StackFrame.AddrStack.Mode = AddrModeFlat;

	DWORD64 dwDisplament = 0;
	SYMBOL_INFO stack_info = { 0 };
	PIMAGEHLP_SYMBOL64 pSym = (PIMAGEHLP_SYMBOL64)&stack_info;
	pSym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
	pSym->MaxNameLength = 80;
	IMAGEHLP_LINE64 ImageLine = { 0 };
	ImageLine.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

	std::string modelName;
	std::stack<std::string> stk;

	while (StackWalk(imageType, GetCurrentProcess(), GetCurrentThread(), &StackFrame, &Context, nullptr, nullptr, nullptr, nullptr))
	{
		SymGetSymFromAddr64(GetCurrentProcess(), StackFrame.AddrPC.Offset, &dwDisplament, pSym);
		DWORD Displament = (DWORD)dwDisplament;
		SymGetLineFromAddr64(GetCurrentProcess(), StackFrame.AddrPC.Offset, &Displament, &ImageLine);
		getModuleName(GetCurrentProcess(), StackFrame.AddrPC.Offset, &modelName);
		std::string str(1000, 0);
		_snprintf((char*)str.data(), str.size(), "stack[%d] %s! %s() file:%s line:%d", stk.size(), modelName.c_str(), pSym->Name, ImageLine.FileName, ImageLine.LineNumber);
		stk.push(str);

		if (stk.size() >= 10)
		{
			break;
		}
	}

	while (!stk.empty())
	{
		fprintf(stderr, "%s\n", stk.top().data());
		stk.pop();
	}
}
#else
void printStack(void)
{
	;
}
#endif
