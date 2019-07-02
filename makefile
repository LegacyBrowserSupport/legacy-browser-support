VERSION_MAJOR=1
VERSION_MINOR=0
VERSION_BUILD=0
VERSION_PATCH=0
VERSION=$(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_BUILD).$(VERSION_PATCH)
CANDLE = "C:\Program Files (x86)\WiX Toolset v3.11\bin\candle.exe"
LIGHT = "C:\Program Files (x86)\WiX Toolset v3.11\bin\light.exe"
BINDIR = bin.msvc/
DLLOBJDIR = bin.msvc/ie_bho/
DLL64OBJDIR = bin.msvc/ie_bho64/
EXEOBJDIR = bin.msvc/native_component/
EXESRCDIR = native_component/
INSTALLDIR = $(DLLSRCDIR)/installer/
INSTALLOBJDIR = bin.msvc/native_component/
DLLSRCDIR = ie_bho/
JSONDIR = third_party/json/
COREDIR = core
INC = -I ./ -I third_party/libxml/src/include/
EXEDEFINES = /MD /DUNICODE /EHsc
DLLDEFINES =  /MD /D_WINDLL /DUNICODE
RCDEFINES = -DVERSION_MAJOR=0 -DVERSION_MINOR=1 -DVERSION_BUILD=0 -DVERSION_PATCH=0 -DVERSION_NUMBER=1.0.0.0
CANDLEDEFINES = -dVersion="$(VERSION)" -dVersionMajor="$(VERSION_MAJOR)" -dVersionMinor="$(VERSION_MINOR)" -dSrcNativeComponent="$(BINDIR)/lbs_native_host.exe" -dSrcManifest="$(INSTALLDIR)/manifest.json" -dSrcIcon="$(INSTALLDIR)/mozilla.ico" -dSrcLibrary="$(BINDIR)/browser_switcher_bho.dll" -dSrc64Library="$(BINDIR)/browser_switcher_bho_x64.dll"

all: $(BINDIR) $(BINDIR)/lbs_native_host.exe  $(BINDIR)/browser_switcher_bho.dll $(BINDIR)/setup.msi

$(BINDIR)/setup.msi: $(INSTALLOBJDIR)/setup.wixobj
    $(LIGHT) -o "$(BINDIR)/setup.msi" -loc $(INSTALLDIR)/lang_en.wxl "$(INSTALLOBJDIR)/setup.wixobj"

$(INSTALLOBJDIR)/setup.wixobj: $(INSTALLDIR)/setup.wxs
    $(CANDLE)  -o "$(INSTALLOBJDIR)" $(CANDLEDEFINES) "$(INSTALLDIR)/setup.wxs"

$(BINDIR):
	-mkdir -p $(BINDIR)
	-mkdir -p $(DLLOBJDIR)
	-mkdir -p $(DLL64OBJDIR)
	-mkdir -p $(EXEOBJDIR)
	-mkdir -p $(INSTALLOBJDIR)

$(EXEOBJDIR)/lbs_native.obj: $(EXESRCDIR)/lbs_native.cc
	cl /Fo$(EXEOBJDIR) $(INC) $(EXEDEFINES) -c $**

$(EXEOBJDIR)/stdafx.obj: $(EXESRCDIR)/stdafx.cc
	cl /Fo$(EXEOBJDIR) $(INC) $(EXEDEFINES) -c $**

$(EXEOBJDIR)/browser_switcher_core.obj: $(COREDIR)/browser_switcher_core.cc
	cl /Fo$(EXEOBJDIR) $(INC) $(EXEDEFINES) -c $**

$(EXEOBJDIR)/ieem_site_list_parser.obj: $(COREDIR)/ieem_site_list_parser.cc
	cl /Fo$(EXEOBJDIR) $(INC) $(EXEDEFINES) -c $**

$(EXEOBJDIR)/libxml_utils.obj: $(COREDIR)/libxml_utils.cc
	cl /Fo$(EXEOBJDIR) $(INC) $(EXEDEFINES) -c $**

$(EXEOBJDIR)/logging.obj: $(COREDIR)/logging.cc
	cl /Fo$(EXEOBJDIR) $(INC) $(EXEDEFINES) -c $**

$(EXEOBJDIR)/json_reader.obj: $(JSONDIR)/json_reader.cpp
	cl /Fo$(EXEOBJDIR) $(INC) $(EXEDEFINES) -c $**

$(EXEOBJDIR)/json_writer.obj: $(JSONDIR)/json_writer.cpp
	cl /Fo$(EXEOBJDIR) $(INC) $(EXEDEFINES) -c $**

$(EXEOBJDIR)/json_value.obj: $(JSONDIR)/json_value.cpp
	cl /Fo$(EXEOBJDIR) $(INC) $(EXEDEFINES) -c $**

$(BINDIR)/lbs_native_host.exe: $(EXEOBJDIR)/lbs_native.obj $(EXEOBJDIR)/stdafx.obj $(EXEOBJDIR)/browser_switcher_core.obj $(EXEOBJDIR)/logging.obj $(EXEOBJDIR)/json_reader.obj $(EXEOBJDIR)/json_writer.obj $(EXEOBJDIR)/json_value.obj $(EXEOBJDIR)/ieem_site_list_parser.obj $(EXEOBJDIR)/libxml_utils.obj
	link /OUT:$(BINDIR)/lbs_native_host.exe $** AdvAPI32.Lib shell32.lib wininet.lib User32.lib libxml2_a.lib urlmon.lib /LIBPATH:third_party\libxml\src\win32\bin.msvc





























$(DLLOBJDIR)/bho.obj: $(DLLSRCDIR)/bho.cc $(DLLSRCDIR)/ie_bho.h
	cl /Fo$(DLLOBJDIR) $(INC) $(DLLDEFINES) -c $(DLLSRCDIR)/bho.cc

$(DLLOBJDIR)/ie_bho.obj: $(DLLSRCDIR)/ie_bho.cc $(DLLSRCDIR)/ie_bho.h
	cl /Fo$(DLLOBJDIR) $(INC) $(DLLDEFINES) -c $(DLLSRCDIR)/ie_bho.cc

$(DLLOBJDIR)/ie_bho_i.obj: $(DLLSRCDIR)/ie_bho_i.c
	cl /Fo$(DLLOBJDIR) $(INC) $(DLLDEFINES) -c $**

$(DLLSRCDIR)/ie_bho.h: $(DLLSRCDIR)/ie_bho.idl
	midl /out $(DLLSRCDIR) $**
	
$(DLLOBJDIR)/ie_bho.res: $(DLLSRCDIR)/ie_bho.rc
	rc -fo$@ $(RCDEFINES) $**

$(DLLOBJDIR)/browser_switcher_core.obj: $(COREDIR)/browser_switcher_core.cc
	cl /Fo$(DLLOBJDIR) $(INC) $(DLLDEFINES) -c $**

$(DLLOBJDIR)/logging.obj: $(COREDIR)/logging.cc
	cl /Fo$(DLLOBJDIR) $(INC) $(DLLDEFINES) -c $**

$(DLLOBJDIR)/ieem_site_list_parser.obj: $(COREDIR)/ieem_site_list_parser.cc
	cl /Fo$(DLLOBJDIR) $(INC) $(DLLDEFINES) -c $**

$(DLLOBJDIR)/libxml_utils.obj: $(COREDIR)/libxml_utils.cc
	cl /Fo$(DLLOBJDIR) $(INC) $(DLLDEFINES) -c $**


$(BINDIR)/browser_switcher_bho.dll: $(DLLOBJDIR)/bho.obj $(DLLOBJDIR)/ie_bho.obj $(DLLOBJDIR)/browser_switcher_core.obj $(DLLOBJDIR)/logging.obj $(DLLOBJDIR)/ieem_site_list_parser.obj  $(DLLOBJDIR)/libxml_utils.obj $(DLLOBJDIR)/ie_bho_i.obj $(DLLOBJDIR)/ie_bho.res
  link /DLL /DEF:$(DLLSRCDIR)/ie_bho.def /OUT:$(BINDIR)/browser_switcher_bho.dll $** AdvAPI32.Lib shell32.lib wininet.lib User32.lib libxml2_a.lib urlmon.lib /LIBPATH:third_party\libxml\src\win32\bin.msvc
