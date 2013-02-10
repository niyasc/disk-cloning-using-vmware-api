#include<iostream>
#include<gtk/gtk.h>
#include"vixDiskLib.h"
#include<string.h>
using namespace std;

#define COMMAND_CLONE           (1 << 9)
#define VIXDISKLIB_VERSION_MAJOR 5
#define VIXDISKLIB_VERSION_MINOR 0
#define THROW_ERROR(vixError) \
   throw VixDiskLibErrWrapper((vixError), __FILE__, __LINE__)

#define CHECK_AND_THROW(vixError)                                    \
   do {                                                              \
      if (VIX_FAILED((vixError))) {                                  \
         throw VixDiskLibErrWrapper((vixError), __FILE__, __LINE__); \
      }                                                              \
   } while (0)
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
//Gtk initialize
GtkWidget *window;	//window
GtkWidget *box,*sbox,*pbox,*tbox;	//packing boxes
GtkWidget *sbrowse,*tbrowse,*bclone;	//buttons
GtkWidget *sframe,*tframe;	//frames
GtkWidget *slabel,*tlabel;	//labels
GtkWidget *pbar;
void gtk_initialize()
{
	window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	//gtk_window_set_resizable(GTK_WINDOW(window),FALSE);
	gtk_window_set_default_size(GTK_WINDOW(window),300,200);
	gtk_window_set_title(GTK_WINDOW(window),"Cloning Tool");

	box=gtk_box_new(GTK_ORIENTATION_VERTICAL,1);
	sbox=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,1);
	tbox=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,1);
	pbox=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,1);

	sbrowse=gtk_button_new_from_stock(GTK_STOCK_OPEN);
	tbrowse=gtk_button_new_from_stock(GTK_STOCK_SAVE);
	bclone=gtk_button_new_with_label("Clone");
	
	sframe=gtk_frame_new("Source");
	tframe=gtk_frame_new("Target");

	slabel=gtk_label_new(NULL);
	tlabel=gtk_label_new(NULL);
	pbar=gtk_progress_bar_new();
	
}
void gtk_packing()
{
	gtk_container_add(GTK_CONTAINER(window),box);

	gtk_box_pack_start(GTK_BOX(box),sframe,0,1,1);
	gtk_box_pack_start(GTK_BOX(box),pbox,0,1,1);
	gtk_box_pack_start(GTK_BOX(box),tframe,0,1,1);
	gtk_box_pack_start(GTK_BOX(box),bclone,0,1,1);

	gtk_container_add(GTK_CONTAINER(sframe),sbox);
	gtk_container_add(GTK_CONTAINER(tframe),tbox);

	gtk_box_pack_start(GTK_BOX(sbox),slabel,1,1,1);
	gtk_box_pack_start(GTK_BOX(sbox),sbrowse,0,1,1);

	//gtk_box_pack_start(GTK_BOX(pbox),spinner,1,1,1);
	gtk_box_pack_start(GTK_BOX(pbox),pbar,1,1,1);

	gtk_box_pack_start(GTK_BOX(tbox),tlabel,1,1,1);
	gtk_box_pack_start(GTK_BOX(tbox),tbrowse,0,1,1);
}
void open()
{
	GtkWidget *dialog;

	dialog = gtk_file_chooser_dialog_new("Select source file",NULL,GTK_FILE_CHOOSER_ACTION_OPEN,
	GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,NULL);

	if (gtk_dialog_run(GTK_DIALOG(dialog))==GTK_RESPONSE_ACCEPT)
   	{
     		char *filename;
     		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
     		//printf("%s",(char *)filename);
		gtk_label_set_text(GTK_LABEL(slabel),filename);
	}
	gtk_widget_destroy (dialog);
}
void save()
{
	GtkWidget *dialog;

	dialog = gtk_file_chooser_dialog_new("Target file name",NULL,GTK_FILE_CHOOSER_ACTION_SAVE,
	GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,NULL);

	if (gtk_dialog_run(GTK_DIALOG(dialog))==GTK_RESPONSE_ACCEPT)
   	{
     		char *filename;
     		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
     		//printf("%s",(char *)filename);
		gtk_label_set_text(GTK_LABEL(tlabel),filename);
	}
	gtk_widget_destroy (dialog);
}
static Bool progress(void * /*progressData*/,int percent)
{
	char text[30];
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pbar),(float)(percent/100.0));
	sprintf(text,"%d% completed",percent);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(pbar),"Cloning");
	while (gtk_events_pending()) gtk_main_iteration ();
	return true;

}
void doclone()
{
		//clone function
	
	VixDiskLibConnection srcConnection;
	VixDiskLibConnectParams cnxParams = { 0 };
	VixError vixError = VixDiskLib_Connect(&cnxParams, &srcConnection);
	CHECK_AND_THROW(vixError);
	cout<<"Connected";
	VixDiskLibCreateParams createParams;
	createParams.adapterType = appGlobals.adapterType;
	createParams.capacity = appGlobals.mbSize * 2048;
	createParams.diskType = VIXDISKLIB_DISK_MONOLITHIC_SPARSE;
	createParams.hwVersion = VIXDISKLIB_HWVERSION_WORKSTATION_5;
	cout<<appGlobals.diskPath<<appGlobals.srcPath;
	vixError = VixDiskLib_Clone(appGlobals.connection,
                               appGlobals.diskPath,
                               srcConnection,
                               appGlobals.srcPath,
                               &createParams,
                               progress,
                               NULL,   // clientData
                               TRUE);  // doOverWrite
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(pbar),"Cloning completed");
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pbar),0);
	VixDiskLib_Disconnect(srcConnection);
	CHECK_AND_THROW(vixError);
	//end of clone
}
void cloning()
{

	char source[30],targ[30];
	bool bVixInit(false);
	strcpy(source,(char *) gtk_label_get_text(GTK_LABEL(slabel)));
	strcpy(targ,(char *) gtk_label_get_text(GTK_LABEL(tlabel)));
//	printf("%s %s",source,targ);

        appGlobals.srcPath =source;
	appGlobals.diskPath =targ;
	VixDiskLibConnectParams cnxParams = {0};
    	VixError vixError;
	vixError = VixDiskLib_Init(VIXDISKLIB_VERSION_MAJOR, VIXDISKLIB_VERSION_MINOR,
                   NULL, NULL, NULL, // Log, warn, panic
                   appGlobals.libdir);
	CHECK_AND_THROW(vixError);
	//cout<<"Initialized";
	bVixInit = true;
	vixError = VixDiskLib_Connect(&cnxParams,&appGlobals.connection);
	CHECK_AND_THROW(vixError);
	//cout<<"Connected";
	doclone();

	if (appGlobals.connection != NULL) 
		VixDiskLib_Disconnect(appGlobals.connection);
    	if (bVixInit)
		VixDiskLib_Exit();
	//cout<<"main okey\n";
}
void gtk_signals()
{
	g_signal_connect(window,"destroy",gtk_main_quit,NULL);

	g_signal_connect(sbrowse,"clicked",open,NULL);
	g_signal_connect(tbrowse,"clicked",save,NULL);
	g_signal_connect(bclone,"clicked",cloning,NULL);
}

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
	initialize();
	gtk_init(&argc,&argv);
	gtk_initialize();
	gtk_packing();
	gtk_signals();
	gtk_widget_show_all(window);
	gtk_main();

}	
