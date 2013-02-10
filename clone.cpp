#include<iostream>
#include"vixDiskLib.h"
#include<string.h>
using namespace std;

#define COMMAND_CLONE           (1 << 9)
#define VIXDISKLIB_VERSION_MAJOR 5
#define VIXDISKLIB_VERSION_MINOR 0

static struct {
    int command;
    VixDiskLibAdapterType adapterType;
    char *transportModes;
    char *diskPath;
    char *parentPath;
    char *metaKey;
    char *metaVal;
    int filler;
    unsigned mbSize;
    VixDiskLibSectorType numSectors;
    VixDiskLibSectorType startSector;
    VixDiskLibSectorType bufSize;
    uint32 openFlags;
    unsigned numThreads;
    Bool success;
    Bool isRemote;
    char *host;
    char *userName;
    char *password;
    char *thumbPrint;
    int port;
    char *srcPath;
    VixDiskLibConnection connection;
    char *vmxSpec;
    bool useInitEx;
    char *cfgFile;
    char *libdir;
    char *ssMoRef;
} appGlobals;

static void DoClone(void);
#define THROW_ERROR(vixError) \
   throw VixDiskLibErrWrapper((vixError), __FILE__, __LINE__)

#define CHECK_AND_THROW(vixError)                                    \
   do {                                                              \
      if (VIX_FAILED((vixError))) {                                  \
         throw VixDiskLibErrWrapper((vixError), __FILE__, __LINE__); \
      }                                                              \
   } while (0)

// Wrapper class for VixDiskLib disk objects.

class VixDiskLibErrWrapper
{
public:
    explicit VixDiskLibErrWrapper(VixError errCode, const char* file, int line)
          :
          _errCode(errCode),
          _file(file),
          _line(line)
    {
        char* msg = VixDiskLib_GetErrorText(errCode, NULL);
        _desc = msg;
        VixDiskLib_FreeErrorText(msg);
    }

    VixDiskLibErrWrapper(const char* description, const char* file, int line)
          :
         _errCode(VIX_E_FAIL),
         _desc(description),
         _file(file),
         _line(line)
    {
    }

    string Description() const { return _desc; }
    VixError ErrorCode() const { return _errCode; }
    string File() const { return _file; }
    int Line() const { return _line; }

private:
    VixError _errCode;
    string _desc;
    string _file;
    int _line;
};
void initialize()
{
	//char source[30],dest[30];
	memset(&appGlobals, 0, sizeof appGlobals);
	appGlobals.command = 0;
	appGlobals.adapterType = VIXDISKLIB_ADAPTER_SCSI_BUSLOGIC;
	appGlobals.startSector = 0;
	appGlobals.numSectors = 1;
	appGlobals.mbSize = 100;
	appGlobals.filler = 0xff;
	appGlobals.openFlags = 0;
	appGlobals.numThreads = 1;
	appGlobals.success = TRUE;
	appGlobals.isRemote = FALSE;
        appGlobals.command |= COMMAND_CLONE;
}
int main(int argc,char *argv[])
{
	int retval;
	bool bVixInit(false);
	char source[30],dest[30];
	initialize();
	cout<<"source      : ";
	cin>>source;
	cout<<"Destination : ";
	cin>>dest;
        appGlobals.srcPath = source;
	appGlobals.diskPath = dest;
	cout<<appGlobals.srcPath;
	VixDiskLibConnectParams cnxParams = {0};
    	VixError vixError;
	try{
	vixError = VixDiskLib_Init(VIXDISKLIB_VERSION_MAJOR, VIXDISKLIB_VERSION_MINOR,
                   NULL, NULL, NULL, // Log, warn, panic
                   appGlobals.libdir);
	CHECK_AND_THROW(vixError);
	bVixInit = true;
	vixError = VixDiskLib_Connect(&cnxParams,&appGlobals.connection);
	CHECK_AND_THROW(vixError);
	DoClone();
	retval=0;
	} catch (const VixDiskLibErrWrapper& e) {
       	cout << "Error: [" << e.File() << ":" << e.Line() << "]  " <<
               std::hex << e.ErrorCode() << " " << e.Description() << "\n";
       	retval = 1;
    	}
	if (appGlobals.connection != NULL) 
		VixDiskLib_Disconnect(appGlobals.connection);
    	if (bVixInit)
		VixDiskLib_Exit();
	return retval;
}

static Bool CloneProgressFunc(void * /*progressData*/,      // IN
                  int percentCompleted)         // IN
{
   cout << "Cloning : " << percentCompleted << "% Done" << "\n";
   return TRUE;
}
static void DoClone()
{
	VixDiskLibConnection srcConnection;
	VixDiskLibConnectParams cnxParams = { 0 };
	VixError vixError = VixDiskLib_Connect(&cnxParams, &srcConnection);
	CHECK_AND_THROW(vixError);
	VixDiskLibCreateParams createParams;
	createParams.adapterType = appGlobals.adapterType;
	createParams.capacity = appGlobals.mbSize * 2048;
	createParams.diskType = VIXDISKLIB_DISK_MONOLITHIC_SPARSE;
	createParams.hwVersion = VIXDISKLIB_HWVERSION_WORKSTATION_5;

	vixError = VixDiskLib_Clone(appGlobals.connection,
                               appGlobals.diskPath,
                               srcConnection,
                               appGlobals.srcPath,
                               &createParams,
                               CloneProgressFunc,
                               NULL,   // clientData
                               TRUE);  // doOverWrite
	VixDiskLib_Disconnect(srcConnection);
	CHECK_AND_THROW(vixError);
	cout << "\n Done" << "\n";
}
	
