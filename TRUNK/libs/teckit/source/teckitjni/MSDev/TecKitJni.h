
// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the TECKITJNI_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// TECKITJNI_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef TECKITJNI_EXPORTS
#define TECKITJNI_API __declspec(dllexport)
#else
#define TECKITJNI_API __declspec(dllimport)
#endif

/*
// This class is exported from the TecKitJni.dll
class TECKITJNI_API CTecKitJni {
public:
	CTecKitJni(void);
	// TODO: add your methods here.
};

extern TECKITJNI_API int nTecKitJni;

TECKITJNI_API int fnTecKitJni(void);

*/