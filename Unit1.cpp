//---------------------------------------------------------------------------
#include <winsock.h>
 #include <windows.h>
 #include <stdio.h>
#include <vcl.h>
#include <winioctl.h>
#include <tlhelp32.hpp>
#pragma hdrstop

#include "Unit1.h"
#include "Unit2.h"
#include "Unit3.h"
#include "Unit4.h"
#include "Unit5.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "RzTabs"
#pragma link "RzButton"
#pragma link "RzPanel"
#pragma link "RzChkLst"
#pragma link "RzLstBox"
#pragma link "RzRadGrp"
#pragma link "RzTray"
#pragma link "RzRadChk"
#pragma link "RzSpnEdt"
#pragma link "RzEdit"
#pragma link "RzStatus"
#pragma resource "*.dfm"

#define LANGENG "00000409" // код англійської розкладки клавіатури
#define LANGRUS "00000419" // код російської розкладки клавіатури
#define LANGUKR "00000422" // код аукраїнської розкладки клавіатури

#define SIZE_BUF 4096 // розмір буфера
TIniFile *ini;

AnsiString progPath; // шлях до робочого каталогу програми

int buttpicFlag = 0; // прапорець для режиму відображення кнопок

TForm1 *Form1;
typedef struct DiskInfo // структура, що зберігає інформацію про диск
{
 AnsiString name;       // імя (літера) диску
 short type;            // тип диску
 __int64 totalSpace;    // загальний обєм
 __int64 emptySpace;    // вільне місце
 __int64 usedSpace;     // зайняте місце
 AnsiString reserved;   // резерв
} DiskInfo;

DiskInfo arrDisk[26];   // масив дисків (можливі 26 літер)

TGridRect gr = {-1,-1,-1,-1}; // діапазон виділених комірок (за межами таблиці)

AnsiString LANGarr [3] = { LANGENG , LANGRUS , LANGUKR };  // масив мов
bool LANGbutton [3] = {true, true, true}; // масив стану мов

byte Lang; // interface language
           // 1 - english
           // 2 - russian
           // 3 - ukrainian

DWORD LangStart = 0; // маска набору мов

AnsiString color1; // тимчасова зимінна для зберігання кольору
AnsiString InitialCaption = "   Програма 'ВінТ' (WinT - Windows Tweaker) створена студенткою ЧІІБіП групи ЗКН-071 Валько Світланою у 2012 році для дипломного проекту";

//---------------------------------------------------------------------------
__fastcall TForm1::TForm1(TComponent* Owner)
        : TForm(Owner)
{
}
//--- отримання переліку активних процесів -----------------------------------

PACKAGE void __fastcall GetActiveAppList (TStringList *slist)
{
if (!slist) return;

slist->Clear();

TProcessEntry32 prj;
prj.dwSize = sizeof(PROCESSENTRY32);

HANDLE hss = CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0);
if (((int)(hss)) == (-1)) return;

AnsiString fname;
try {
for (bool loop = Process32First(hss, &prj); loop; loop = Process32Next(hss, &prj))
// цикл перебору процесів
{ fname = AnsiString(prj.szExeFile).Trim().UpperCase();
if (fname.IsEmpty()) continue;
slist->Add(fname);   // додавання імені процесу до списку
}
} // try
catch (...)
{ slist->Clear(); } // очистка списку

CloseHandle(hss);
}

// --- отримання даних про логічні диски -------------------------------------
BOOL GetDriveGeometry(DISK_GEOMETRY *pdg, const char* driveName)
{
  HANDLE hDevice;
  BOOL bResult;
  DWORD junk;

// формування рядка для отримання хендла диску у форматі “\\.\C:”
  char str[7];
  str[0]=str[1]=str[3]='\\';
  str[2]='.';
  str[4]=driveName[0];
  str[5]=driveName[1];
  str[6]='\0';

// отримання хендла диску
  hDevice = CreateFileA(str,
                        0,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL);

// якщо не вдалося отримати хендл, повертаємо помилку
  if (hDevice == INVALID_HANDLE_VALUE)
    return FALSE;

// отримання даних про диск
  bResult = DeviceIoControl(hDevice,
                            IOCTL_DISK_GET_DRIVE_GEOMETRY,
                            NULL,
                            0,
                            pdg,
        // pdg - посилання на структуру, що зберігає необхідні параметри диску
                            sizeof(*pdg),
                            &junk,
                            (LPOVERLAPPED) NULL);
// закриття хендлу
  CloseHandle(hDevice);
  return bResult;
}

// === перетворення номеру типу диску на назву ===============================
AnsiString TypeDiskString(int type)
{
AnsiString typeDstr;
// отримання літерного типу за вказаним номером
 switch (type)
 {
  case 0:typeDstr = "UNKNOWN";           break;
  case 1:typeDstr = "DRIVE NO ROOT DIR"; break;
  case 2:typeDstr = "REMOVABLE";         break;
  case 3:typeDstr = "FIXED";             break;
  case 4:typeDstr = "REMOTE";            break;
  case 5:typeDstr = "CD/DVD-ROM";        break;
  case 6:typeDstr = "RAMDISK";           break;
 };
return(typeDstr);
};

// === процедура інізіалізації під час створення форми ======================
void __fastcall TForm1::FormCreate(TObject *Sender)
{
progPath = ExtractFilePath(Application->ExeName);
                // отримання шляху робочого каталогу програми
SaveDialogInfo->InitialDir = progPath;
SaveDialogReg->InitialDir = progPath;

ActionConfigLoadExecute(Sender); // завантаження конфігурації
RzRGToolButtonPictClick(Sender); // режим відображення кнопока панелі інструментів
ActionOptionInterfaceViewExecute(Sender);   // режим відображення панелей
ActionOptionInterfaceWindowExecute(Sender); // режим вікна

// параметри таблиці для відображення інформації про логічні диски
ActionCreateStrGridHDDExecute(Sender);

// параметри рухомого рядка
LabelRunning->Left = RzPanel1->ClientWidth;
LabelRunning->Caption = InitialCaption;

// процедури виведення інформації про автора та ліцензію
 ActionInfoAuthorExecute(Sender);
 ActionInfoLicenseExecute(Sender);

// процедури перевірик значень реєстру
 ActionRegDebugExecute(Sender);
 ActionRegKillProcessExecute(Sender);
 ActionRegAutorunExecute(Sender);
 ActionRegSpaceMessageExecute(Sender);
 ActionRegDefragExecute(Sender);

// процедури перевірик значень реєстру
 ActionRegAutoCleanExecute(Sender);
 ActionRegPassExecute(Sender);
 ActionRegPassChangeExecute(Sender);
 ActionRegProfExecute(Sender);
 ActionRegRemoteExecute(Sender);
 ActionRegMonitorExecute(Sender);
 ActionRegMonitorBackExecute(Sender);
 ActionRegMonitorScreenExecute(Sender);
 ActionRegMonitorOptionExecute(Sender);
 ActionRegAllHideExecute(Sender);
 ActionRegMyCompExecute(Sender);
 ActionRegNetConnectExecute(Sender);
 ActionRegNetExecute(Sender);
 ActionRegPrintFaxExecute(Sender);
 ActionRegHintExecute(Sender);
 ActionRegMasterCleanExecute(Sender);


 //визначення шляху ini-файлу
 ini = new TIniFile(progPath+"\\win.ini"); //присвоюємо змінній ini шлях до файлу 

}

//--- встановлення режиму відображення панелей --------------------------------
void __fastcall TForm1::RzCheckListViewChange(TObject *Sender, int Index,
      TCheckBoxState NewState)
{
switch (Index)
{
 case 0: // меню
 {
  if (RzCheckListView->ItemChecked[Index])
  { Form1->Menu = MainMenu1;  }
  else
  { Form1->Menu = NULL;  }
  break;
 }
 case 1: // панель
 {
  RzPanel1->Visible = RzCheckListView->ItemChecked[Index];
  break;
 }
 case 2: // панель інструментів
 {
  RzPanelToolBar->Visible = RzCheckListView->ItemChecked[Index];
  break;
 }
 case 3: // рядок стану
 {
  RzStatusBar1->Visible = RzCheckListView->ItemChecked[Index];
  break;
 }
 default:
 {
  break;
 }
}

}

//--- ініціалізація параметрів відображення панелей --------------------------
void __fastcall TForm1::ActionOptionInterfaceViewExecute(TObject *Sender)
{
// головне меню
  if (RzCheckListView->ItemChecked[0])
  { Form1->Menu = MainMenu1;  }
  else
  { Form1->Menu = NULL;  }
// панель
  RzPanel1->Visible = RzCheckListView->ItemChecked[1];
// панель інструментів
  RzPanelToolBar->Visible = RzCheckListView->ItemChecked[2];
// рядок стану
  RzStatusBar1->Visible = RzCheckListView->ItemChecked[3];
}

//--- ініціалізація параметрів вікна ------------------------------------------
void __fastcall TForm1::ActionOptionInterfaceWindowExecute(TObject *Sender)
{
TBorderIcons tempBI = Form1->BorderIcons;

// відображення у треї
  RzTrayIconMain->Enabled = !RzCheckListAdd->ItemChecked[0];
// можливість зміни розміру вікна
  if (!RzCheckListAdd->ItemChecked[2])
  {    Form1->BorderStyle = bsSizeable;  }
  else
  {   Form1->BorderStyle = bsSingle;     }
// наявність системного меню вікна
  if (RzCheckListAdd->ItemChecked[3])
  {
   tempBI >> biSystemMenu;
   Form1->BorderIcons = tempBI;
  }
  else
  {
   tempBI << biSystemMenu;
   Form1->BorderIcons = tempBI;
  }
// можливість мінімізації (звернення)
  if (RzCheckListAdd->ItemChecked[4])
  {
   tempBI >> biMinimize; // додати кнопку
   Form1->BorderIcons = tempBI;
  }
  else
  {
   tempBI << biMinimize; // вилучити кнопку
   Form1->BorderIcons = tempBI;
  }
// можоивість максимізації (розвернення)
  if (RzCheckListAdd->ItemChecked[5])
  {
   tempBI >> biMaximize; // додати кнопку
   Form1->BorderIcons = tempBI;
  }
  else
  {
   tempBI << biMaximize; // вилучити кнопку
   Form1->BorderIcons = tempBI;
  }

}

//--- обробка зміни мови інтерфесу ------------------------------------------
void __fastcall TForm1::RzRGLangClick(TObject *Sender)
{
  Lang = RzRGLang->ItemIndex + 1;
  ActionOptionChangeLangExecute(Sender); // виклик процедури зміни
}

//--- процедура зміни мови інтерфейсу ----------------------------------------
void __fastcall TForm1::ActionOptionChangeLangExecute(TObject *Sender)
{
AnsiString filename; // імя файлу
switch (Lang)
{
 case 1:
 {
  filename = progPath + "lang\\eng.lng";      //вибір файлу
  break;
 }
 case 2:
 {
  filename = progPath + "lang\\rus.lng";
  break;
 }
 case 3:
 {
  filename = progPath + "lang\\ukr.lng";
  break;
 }
 default:
 {
  break;
 }
}

TStringList *List = new TStringList;
List->LoadFromFile(filename);     //завантаження з файлу
// menu
File1->Caption =            List->Strings[1];
SaveInfo1->Caption =         List->Strings[2];
SaveReg1->Caption =          List->Strings[3];
SaveConfig1->Caption =       List->Strings[4];
Exit1->Caption =            List->Strings[5];
Info1->Caption =            List->Strings[6];
About1->Caption =           List->Strings[7];
Help1->Caption =            List->Strings[8];
Select1->Caption =          List->Strings[9];
ClientServer1->Caption=     List->Strings[10];
// tool buttons
RzShB1Save->Caption =       List->Strings[12];
RzShB2Reg->Caption =        List->Strings[13];
RzShB3Config->Caption =     List->Strings[14];
RzShB4Help->Caption =       List->Strings[15];
RzShB5About->Caption =      List->Strings[16];
RzShB6Exit->Caption =       List->Strings[17];
// tabs
TabSheet0_1->Caption =  List->Strings[19];
TabSheet1_1->Caption =             List->Strings[20];
TabSheet1_2->Caption =             List->Strings[21];
TabSheet1_3->Caption =             List->Strings[22];
TabSheet1_4->Caption =             List->Strings[23];
TabSheet1_5->Caption =             List->Strings[24];
TabSheet1_6->Caption =             List->Strings[25];
TabSheet0_2->Caption =  List->Strings[26];
TabSheet2_1->Caption =             List->Strings[27];
TabSheet2_2->Caption =             List->Strings[28];
TabSheet0_3->Caption =  List->Strings[29];
TabSheet3_1->Caption =             List->Strings[30];
TabSheet3_2->Caption =             List->Strings[31];
TabSheet3_3->Caption =             List->Strings[32];
TabSheet0_4->Caption =  List->Strings[33];
TabSheet4_1->Caption =             List->Strings[34];
TabSheet0_5->Caption =  List->Strings[35];
TabSheet5_1->Caption =             List->Strings[36];
TabSheet5_2->Caption =             List->Strings[37];
TabSheet5_3->Caption =             List->Strings[38];
TabSheet5_4->Caption =             List->Strings[39];
TabSheet0_6->Caption =  List->Strings[40];
TabSheet6_1->Caption =             List->Strings[41];
TabSheet6_2->Caption =             List->Strings[42];
TabSheet1_7->Caption =             List->Strings[43];
// tab1
LabelFolders->Caption =            List->Strings[45];
ButtonFolders->Caption =           List->Strings[46];
LabellProcessor->Caption =         List->Strings[47];
ButtonProcessor->Caption =         List->Strings[48];
LabelDisk->Caption =               List->Strings[49];
ButtonDisk->Caption =              List->Strings[50];
LabelMemory->Caption =             List->Strings[51];
ButtonMemory->Caption =            List->Strings[52];
CheckBoxMemory->Caption =          List->Strings[53];
LabelFonts->Caption =              List->Strings[54];
ButtonFonts->Caption =             List->Strings[55];
ButtonInfoAll->Caption =           List->Strings[56];
// tab2
ButtonServices->Caption =          List->Strings[58];
ButtonServiceClear->Caption =      List->Strings[59];
ButtonProcess->Caption =           List->Strings[60];
ButtonProcessClear->Caption =      List->Strings[61];
// tab3
CheckBox18->Caption =              List->Strings[63];
CheckBox19->Caption =              List->Strings[64];
CheckBox20->Caption =              List->Strings[65];
CheckBox21->Caption =              List->Strings[66];
CheckBox22->Caption =              List->Strings[67];

Label2->Caption =                  List->Strings[68];
CheckBox1->Caption =               List->Strings[69];
Button11->Caption =                List->Strings[70];
Label4->Caption =                  List->Strings[71]+"\r\n"+List->Strings[72];
CheckBox5->Caption =               List->Strings[73];
CheckBox6->Caption =               List->Strings[74];
CheckBox7->Caption =               List->Strings[75];
CheckBox8->Caption =               List->Strings[76];
CheckBox9->Caption =               List->Strings[77];
CheckBox10->Caption =              List->Strings[78];
CheckBox11->Caption =              List->Strings[79];
CheckBox12->Caption =              List->Strings[80];
CheckBox14->Caption =              List->Strings[81];
CheckBox15->Caption =              List->Strings[82];
CheckBox16->Caption =              List->Strings[83];
CheckBox17->Caption =              List->Strings[84];
CheckBox13->Caption =              List->Strings[85];
Label3->Caption =                  List->Strings[86];
CheckBox3->Caption =               List->Strings[87];
CheckBox4->Caption =               List->Strings[88];
LabelRBinName->Caption =           List->Strings[89];
Button6->Caption =                 List->Strings[90];
Button7->Caption =                 List->Strings[91];
LabelRBinHint->Caption =           List->Strings[92];
Button8->Caption =                 List->Strings[93];
Button9->Caption =                 List->Strings[94];

LabelKeyPreset->Caption =          List->Strings[95];
ButtonKeyboardPreset->Caption =    List->Strings[96];
ButtonKeyboardPresetRe->Caption =  List->Strings[97];
RzBitBtnLangRus->Caption =         List->Strings[98];
RzBitBtnLangUkr->Caption =         List->Strings[99];
RzBitBtnLangEng->Caption =         List->Strings[100];

// tab4
RzBitBtnReg->Caption =             List->Strings[102];
RzButtonRegEdit->Caption =         List->Strings[103];

// tab5
RzRGLang->Caption =                List->Strings[105];
RzRGLang->Items->Strings[0] =      List->Strings[106];
RzRGLang->Items->Strings[1] =      List->Strings[107];
RzRGLang->Items->Strings[2] =      List->Strings[108];

RzRGControlSkin->Caption =         List->Strings[109];
RzRGControlSkin->Items->Strings[0] = List->Strings[110];
RzRGControlSkin->Items->Strings[1] = List->Strings[111];
RzRGControlSkin->Items->Strings[2] = List->Strings[112];
RzRGControlSkin->Items->Strings[3] = List->Strings[113];
RzRGControlSkin->Items->Strings[4] = List->Strings[114];
RzRGControlSkin->Items->Strings[5] = List->Strings[115];
RzRGToolButtonPict->Caption =        List->Strings[116];
RzRGToolButtonPict->Items->Strings[0] = List->Strings[117];
RzRGToolButtonPict->Items->Strings[1] = List->Strings[118];
RzRGToolButtonPict->Items->Strings[2] = List->Strings[119];

RzGroupBoxView->Caption =               List->Strings[120];
RzCheckListView->Items->Strings[0] =    List->Strings[121];
RzCheckListView->Items->Strings[1] =    List->Strings[122];
RzCheckListView->Items->Strings[2] =    List->Strings[123];
RzCheckListView->Items->Strings[3] =    List->Strings[124];
RzRGrunning->Caption =                  List->Strings[125];
RzRGrunning->Items->Strings[0] =        List->Strings[126];
RzRGrunning->Items->Strings[1] =        List->Strings[127];
RzRGPageControl->Caption =              List->Strings[128];
RzRGPageControl->Items->Strings[0] =    List->Strings[129];
RzRGPageControl->Items->Strings[1] =    List->Strings[130];
RzRGPageControl->Items->Strings[2] =    List->Strings[131];
RzRGPageControl->Items->Strings[3] =    List->Strings[132];

RzGroupBoxTTW->Caption =                List->Strings[133];
RzCheckListAdd->Items->Strings[0] =     List->Strings[134];
RzCheckListAdd->Items->Strings[1] =     List->Strings[135];
RzCheckListAdd->Items->Strings[2] =     List->Strings[136];
RzCheckListAdd->Items->Strings[3] =     List->Strings[137];
RzCheckListAdd->Items->Strings[4] =     List->Strings[138];
RzCheckListAdd->Items->Strings[5] =     List->Strings[139];

delete List;
}

//-----влючити очистку списку відкривавшихся документів-----------------------
void __fastcall TForm1::CheckBox1Click(TObject *Sender)
{
 TRegistry *Registry=new  TRegistry;
 try
{
 Registry->RootKey=HKEY_CURRENT_USER; // коренева гілка
 Registry->OpenKey("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer",true); // відкрити гілку

 if (CheckBox1->Checked)
 {
  Registry->WriteBool("ClearRecentDocsOnExit",1); // запис значення 1
 }
 else
 {
  Registry->WriteBool("ClearRecentDocsOnExit",0); // запис значення 0
 }
 Registry->CloseKey();
}
__finally
{
delete  Registry;
}

}

//-----очистка списку команд Виконати----------------------------------
void __fastcall TForm1::Button11Click(TObject *Sender)
{
TRegistry *Registry=new  TRegistry;
try
{
 Registry->RootKey=HKEY_CURRENT_USER;
 Registry->OpenKey("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RunMRU",true);
 Registry->WriteString("MRUList"," ");
 Registry->CloseKey();
}
__finally
{
 delete  Registry;
}

}
//-------заборона підказок ярликів-------------------------------------------

void __fastcall TForm1::CheckBox3Click(TObject *Sender)
{
 TRegistry *Registry=new  TRegistry;
 try
 {
  Registry->RootKey=HKEY_CURRENT_USER;
  Registry->OpenKey("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",true);
  if (CheckBox3->Checked)
  {
   Registry->WriteBool("ShowInfoTip",0);
  }
  else
  {
   Registry->WriteBool("ShowInfoTip",1);
  }
 }
__finally
{
delete  Registry;
}

}

//-------відімкнути майстер очистки робочого столу---------------------------
void __fastcall TForm1::CheckBox4Click(TObject *Sender)
 {
 TRegistry *Registry=new  TRegistry;
 try
 {
 Registry->RootKey=HKEY_CURRENT_USER;
 Registry->OpenKey("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer",true);
 if (CheckBox4->Checked)
{
 Registry->WriteBool("NoDesktopCleanupWizard",0);
}
 else
{
 Registry->WriteBool("NoDesktopCleanupWizard",1);
}

}
__finally
{
delete  Registry;
}

}

//-----сховати аплет паролі---------------------------------------
void __fastcall TForm1::CheckBox5Click(TObject *Sender)
{
 TRegistry *Registry=new  TRegistry;
try
{
 Registry->RootKey=HKEY_CURRENT_USER; 
 Registry->OpenKey("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System",true); 
 if (CheckBox5->Checked)
 {
  Registry->WriteBool("NoSecCPL",1);
 }
  else
 {
  Registry->WriteBool("NoSecCPL",0); 
 }
 Registry->CloseKey();
}
__finally
{
delete  Registry;
}

}

//-----сховати аплет зміна паролей-------------------------------------------

void __fastcall TForm1::CheckBox6Click(TObject *Sender)
{
 TRegistry *Registry=new  TRegistry;
try
{
 Registry->RootKey=HKEY_CURRENT_USER; 
 Registry->OpenKey("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System",true); 
 if (CheckBox6->Checked)
 {
  Registry->WriteBool("NoPwdPage",1);
 }
 else
 {
  Registry->WriteBool("NoPwdPage",0); 
 }
 Registry->CloseKey();
}
__finally
{
delete  Registry;
}

}

//----сховати аплет налаштування користувацьких профілів-----------------------

void __fastcall TForm1::CheckBox7Click(TObject *Sender)
{
 TRegistry *Registry=new  TRegistry;
try
{
Registry->RootKey=HKEY_CURRENT_USER; 
Registry->OpenKey("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System",true); 
if (CheckBox7->Checked)
{
 Registry->WriteBool("NoProfilePage",1);
}
 else
{
 Registry->WriteBool("NoProfilePage",0); 
}
 Registry->CloseKey();
}
__finally
{
delete  Registry;
}

}


//------сховати аплет видалене керування-------------------------------------

void __fastcall TForm1::CheckBox8Click(TObject *Sender)
{
 TRegistry *Registry=new  TRegistry;
try
{
Registry->RootKey=HKEY_CURRENT_USER; 
Registry->OpenKey("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System",true); 
if (CheckBox8->Checked)
{
 Registry->WriteBool("NoAdminPage",1);
}
 else
{
 Registry->WriteBool("NoAdminPage",0); 
}

 Registry->CloseKey();
}
__finally
{
delete  Registry;
}

}


//----сховати аплет екран------------------------------

void __fastcall TForm1::CheckBox9Click(TObject *Sender)
{
 TRegistry *Registry=new  TRegistry;
try
{
Registry->RootKey=HKEY_CURRENT_USER; 
Registry->OpenKey("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System",true); 
if (CheckBox9->Checked)
{
 Registry->WriteBool("NoDispCPL",1);
}
 else
{
 Registry->WriteBool("NoDispCPL",0); 
}

 Registry->CloseKey();
}
__finally
{
delete  Registry;
}

}


//------сховати аплет фон--------------------------------------

void __fastcall TForm1::CheckBox10Click(TObject *Sender)
{
 TRegistry *Registry=new  TRegistry;
try
{
Registry->RootKey=HKEY_CURRENT_USER; 
Registry->OpenKey("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System",true); 
if (CheckBox10->Checked)
{
 Registry->WriteBool("NoDispBackgroundPage",1);
}
 else
{
 Registry->WriteBool("NoDispBackgroundPage",0); 
}

 Registry->CloseKey();
}
__finally
{
delete  Registry;
}

}


//----сховати аплет заставка-------------------------------------------

void __fastcall TForm1::CheckBox11Click(TObject *Sender)
{
  TRegistry *Registry=new  TRegistry;
try
{
Registry->RootKey=HKEY_CURRENT_USER; 
Registry->OpenKey("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System",true); 
if (CheckBox11->Checked)
{
 Registry->WriteBool("NoDispScrSavPage",1);
}
 else
{
 Registry->WriteBool("NoDispScrSavPage",0); 
}

 Registry->CloseKey();
}
__finally
{
delete  Registry;
}

}

//----сховати аплет налаштування--------------------------------------

void __fastcall TForm1::CheckBox12Click(TObject *Sender)
{
  TRegistry *Registry=new  TRegistry;
try
{
Registry->RootKey=HKEY_CURRENT_USER; 
Registry->OpenKey("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System",true); 
if (CheckBox12->Checked)
{
 Registry->WriteBool("NoDispSettingsPage",1);
}
 else
{
 Registry->WriteBool("NoDispSettingsPage",0); 
}

 Registry->CloseKey();
}
__finally
{
delete  Registry;
}

}


//----сховати всі аплети--------------------------------------------

void __fastcall TForm1::CheckBox13Click(TObject *Sender)
{
TRegistry *Registry=new  TRegistry;
try
{
Registry->RootKey=HKEY_CURRENT_USER; 
Registry->OpenKey("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer",true); 
if (CheckBox13->Checked)
{
 Registry->WriteBool("NoControlPanel ",1);
}
else
{
 Registry->WriteBool("NoControlPanel ",0); 
}

 Registry->CloseKey();
}
__finally
{
delete  Registry;
}


}


//----сховати аплет мій компютер---------------------------

void __fastcall TForm1::CheckBox14Click(TObject *Sender)
{
TRegistry *Registry=new  TRegistry;
try
{
Registry->RootKey=HKEY_CURRENT_USER; 
Registry->OpenKey("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\NonEnum",true); 
if (CheckBox14->Checked)
{
 Registry->WriteBool("{20D04FE0-3AEA-1069-A2D8-08002B30309D}",1);
}
else
{
 Registry->WriteBool("{20D04FE0-3AEA-1069-A2D8-08002B30309D}",0); 
}

 Registry->CloseKey();
}
__finally
{
delete  Registry;
}

}


//-----сховати аплет мережеві зєднання-----------------------------------

void __fastcall TForm1::CheckBox15Click(TObject *Sender)
{
TRegistry *Registry=new  TRegistry;
try
{
Registry->RootKey=HKEY_CURRENT_USER; 
Registry->OpenKey("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer",true); 
if (CheckBox15->Checked)
{
 Registry->WriteBool("NoNetworkConnections",1);
}
else
{
 Registry->WriteBool("NoNetworkConnections",0); 
}

 Registry->CloseKey();
}
__finally
{
delete  Registry;
}

}


//-----сховати аплет мережеві оточення---------------------------------------

void __fastcall TForm1::CheckBox16Click(TObject *Sender)
{
TRegistry *Registry=new  TRegistry;
try
{
Registry->RootKey=HKEY_CURRENT_USER; 
Registry->OpenKey("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer",true); 
if (CheckBox16->Checked)
{
 Registry->WriteBool("NoStartMenuNetworkPlaces",1);
}
else
{
 Registry->WriteBool("NoStartMenuNetworkPlaces",0); 
}

 Registry->CloseKey();
}
__finally
{
delete  Registry;
}

}


//------сховати аплет принтери і факси------------------------------

void __fastcall TForm1::CheckBox17Click(TObject *Sender)
{
TRegistry *Registry=new  TRegistry;
try
{
Registry->RootKey=HKEY_CURRENT_USER; 
Registry->OpenKey("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",true); 
if (CheckBox17->Checked)
{
 Registry->WriteBool("Start_ShowPrinters",1);
}
else
{
 Registry->WriteBool("Start_ShowPrinters",0); 
}

 Registry->CloseKey();
}
__finally
{
delete  Registry;
}

}


//-------нове імя корзини------------------------------------------

void __fastcall TForm1::Button6Click(TObject *Sender)
{
AnsiString headInp = "Ввод нового названия корзины";
AnsiString headPrompt = "Название:";

TRegistry *Registry=new  TRegistry;
try
{
Registry->RootKey=HKEY_CLASSES_ROOT; 
Registry->OpenKey("CLSID\\{645FF040-5081-101B-9F08-00AA002F954E}",true); 

 AnsiString str = InputBox(headInp, headPrompt , "Корзина");
 if (str != "")
 {
  Registry->WriteString("LocalizedString",str);

 }
 Registry->CloseKey();
}
__finally
{
delete  Registry;
}

}
//------імя корзини по замовчуванню-------------------------------------------

void __fastcall TForm1::Button7Click(TObject *Sender)
{

TRegistry *Registry=new  TRegistry;
try
{
Registry->RootKey=HKEY_CLASSES_ROOT; 
Registry->OpenKey("CLSID\\{645FF040-5081-101B-9F08-00AA002F954E}",true); 
Registry->WriteString("LocalizedString","@%SystemRoot%\\system32\\SHELL32.dll,-8964"); 
Registry->CloseKey();
}
__finally
{
delete  Registry;
}

}
//-----нова підказка корзини-------------------------------------------

void __fastcall TForm1::Button8Click(TObject *Sender)
{
AnsiString gInp = "Ввод новой подсказки";
AnsiString gPrompt = "Подсказка:";

TRegistry *Registry=new  TRegistry;
try
{
Registry->RootKey=HKEY_CLASSES_ROOT; 
Registry->OpenKey("CLSID\\{645FF040-5081-101B-9F08-00AA002F954E}",true); 

 AnsiString strg = InputBox(gInp, gPrompt , "используется для временного хранение файлов");
 if (strg != "")
 {
  Registry->WriteString("InfoTip",strg);
 }

 Registry->CloseKey();
}
__finally
{
delete  Registry;
}

}

//------підказка корзини по замовчуванню--------------------------------------

void __fastcall TForm1::Button9Click(TObject *Sender)
{
TRegistry *Registry=new  TRegistry;
try
{
Registry->RootKey=HKEY_CLASSES_ROOT; 
Registry->OpenKey("CLSID\\{645FF040-5081-101B-9F08-00AA002F954E}",true); 
Registry->WriteString("InfoTip","@%SystemRoot%\\system32\\SHELL32.dll,-22915");
Registry->CloseKey();
}
__finally
{
delete  Registry;
}
}
//---відключення дебагера Ватсона---------------------------------------------

void __fastcall TForm1::CheckBox18Click(TObject *Sender)
{
 TRegistry *Registry=new  TRegistry;
try
{
Registry->RootKey=HKEY_LOCAL_MACHINE; 
Registry->OpenKey("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug",true); 
if (CheckBox18->Checked)
{
 Registry->WriteString("Auto","1"); // запис 1-ці в рядковий розділ
}
else
{
 Registry->WriteString("Auto","0"); // запис 0 в рядковий розділ
}

 Registry->CloseKey();
}
__finally
{
delete  Registry;
}

}

//-----автозаввершення зависших процесів---------------------------------------

void __fastcall TForm1::CheckBox19Click(TObject *Sender)
{
 TRegistry *Registry=new  TRegistry;
try
{
Registry->RootKey=HKEY_CURRENT_USER; 
Registry->OpenKey("Control Panel\\Desktop",true); 

if (CheckBox19->Checked)
{
 Registry->WriteString("AutoEndTasks","1"); // запис 1-ці в рядковий розділ
}
else
{
 Registry->WriteString("AutoEndTasks","0"); // запис 0 в рядковий розділ
}

 Registry->CloseKey();
}

__finally
{
delete  Registry;
}
}

//------відключення стандартного автозапуску дисків----------------------------

void __fastcall TForm1::CheckBox20Click(TObject *Sender)
{
 TRegistry *Registry=new  TRegistry;
try
{
Registry->RootKey=HKEY_LOCAL_MACHINE; 
Registry->OpenKey("SYSTEM\\CurrentControlSet\\Services\\CDRom",true); 
if (CheckBox20->Checked)
{
 Registry->WriteInteger("AutoRun",0); //створення числового розділу
}
else
{
 Registry->WriteInteger("AutoRun",1);
}

 Registry->CloseKey();
}
__finally
{
delete  Registry;
}

}


//------відключення повідомленнь про недостатній об'єм місця на диску----------

void __fastcall TForm1::CheckBox21Click(TObject *Sender)
{
 TRegistry *Registry=new  TRegistry;
try
{
Registry->RootKey=HKEY_CURRENT_USER; 
Registry->OpenKey("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer",true); 
if (CheckBox21->Checked)
{
 Registry->WriteBool("NoLowDiskSpaceChecks",1);
}
else
{
 Registry->WriteBool("NoLowDiskSpaceChecks",0); 
}

 Registry->CloseKey();
}
__finally
{
delete  Registry;
}

}

//------зменшення дефрагментації--------------------------------------------

void __fastcall TForm1::CheckBox22Click(TObject *Sender)
{
TRegistry *Registry=new  TRegistry;
try
{
Registry->RootKey=HKEY_LOCAL_MACHINE; 
Registry->OpenKey("System\\CurrentControlSet\\Control\\FileSystem",true);
//  Прочитать или создать новую ветвь в реестре
if (CheckBox22->Checked)
{
 Registry->WriteBool("ContigFileAllocSize",00000200); // запис даних типу DWORD
}
else
{
 Registry->DeleteValue("ContigFileAllocSize"); // Видалення параметра
}

 Registry->CloseKey();
}
__finally
{
delete  Registry;
}

}
//--------зчитування раскладки клавіатури------------------------------------------

void __fastcall TForm1::ButtonKeyboardPresetReClick(TObject *Sender)
{
 AnsiString str;
 TRegistry *Registry = new TRegistry;
 TStringList *StrVal = new TStringList();

 RzCheckListLangStart->Clear();

 try
 {
  Registry->RootKey=HKEY_USERS; 
  Registry->OpenKey(".DEFAULT\\Keyboard Layout\\Preload",false); //  Прочитать ветвь в реестре
  Registry->GetValueNames(StrVal);
  if (StrVal->Count >0)
  {
   for(int i=0;i<StrVal->Count;i++)
   {
    str = Registry->ReadString(StrVal->Strings[i]);
    RzCheckListLangStart->Add(str);
    RzCheckListLangStart->CheckAll();
   };
  }

 Registry->CloseKey();
 }
 __finally
 {
  delete Registry;
  delete StrVal;
 }

 ActionCheckLangStartExecute(Sender);

 for(int i=0; i<3; i++) // перебір lang
 {
  if( ((LangStart>>i)& 0x001) != 1 )
  {
   RzCheckListLangStart->Add(LANGarr[i]);
  }
 }


}
//--------встановлення в певну позицію---------------------------------------



void __fastcall TForm1::RzCheckListLangStartChange(TObject *Sender, int Index,
      TCheckBoxState NewState)
{

 if (NewState != cbChecked)
 {
  RzCheckListLangStart->Delete(Index);
 }

 ActionCheckLangStartExecute(Sender);
}
//--------ініціалізація списку мов-----------------------------------------

void __fastcall TForm1::RzCheckListLangStartClick(TObject *Sender)
{
//  Label8->Caption = IntToStr(RzCheckListLangStart->ItemIndex);
 if (RzCheckListLangStart->ItemIndex>-1)
 {
  RzSpinButtonsLang->Enabled = LangStart>0;
 }
}
//---------переміщення вниз значень----------------------------------

void __fastcall TForm1::RzSpinButtonsLangDownLeftClick(TObject *Sender)
{
  int numChB;
  AnsiString strChB;
  TCheckBoxState stChB;

  numChB = RzCheckListLangStart->ItemIndex;

  if (numChB < RzCheckListLangStart->Items->Count-1)
  {
   strChB = RzCheckListLangStart->Items->Strings[numChB];
   stChB = RzCheckListLangStart->ItemState[numChB];
   RzCheckListLangStart->Delete(numChB);
   numChB++;
   RzCheckListLangStart->Insert(numChB, strChB);
   RzCheckListLangStart->ItemIndex = numChB;
   RzCheckListLangStart->ItemState[numChB] = stChB;
  }
}
//--------переміщення вгору значень----------------------------------


void __fastcall TForm1::RzSpinButtonsLangUpRightClick(TObject *Sender)
{
  int numChB;
  AnsiString strChB;
  TCheckBoxState stChB;

  numChB = RzCheckListLangStart->ItemIndex;

  if (numChB > 0)
  {
   strChB = RzCheckListLangStart->Items->Strings[numChB];
   stChB = RzCheckListLangStart->ItemState[numChB];
   RzCheckListLangStart->Delete(numChB);
   numChB--;
   RzCheckListLangStart->Insert(numChB, strChB);
   RzCheckListLangStart->ItemIndex = numChB;
   RzCheckListLangStart->ItemState[numChB] = stChB;
  }
}
//----------заповнення шаблону-------------------------------------------

void __fastcall TForm1::ActionCheckLangStartExecute(TObject *Sender)
{
 LangStart = 0;

 for(int i=0; i<RzCheckListLangStart->Items->Count; i++)
 {
  bool flag;
  flag = RzCheckListLangStart->Items->Strings[i]==LANGENG;
  if (flag)
  {
   LangStart = LangStart + 1;
  }

  flag = RzCheckListLangStart->Items->Strings[i]==LANGRUS;
  if (flag)
  {
   LangStart = LangStart + 2;
  }

  flag = RzCheckListLangStart->Items->Strings[i]==LANGUKR;
  if (flag)
  {
   LangStart = LangStart + 4;
  }
 };

 if (RzCheckListLangStart->ItemIndex>-1)
 {
  RzSpinButtonsLang->Enabled = LangStart>0;
 }

 DWORD temp = LangStart;
 for(int i=0; i<3; i++) // перебір lang
 {
  switch (i)
  {
   case 0:
   {
    RzBitBtnLangEng->Enabled =  ((temp>>i)& 0x001) != 1;
    break;
   }
   case 1:
   {
    RzBitBtnLangRus->Enabled =  ((temp>>i)& 0x001) != 1;
    break;
   }
   case 2:
   {
    RzBitBtnLangUkr->Enabled =  ((temp>>i)& 0x001) != 1;
    break;
   }
  }

 }

}
//------------заповнення шаблону англ мови------------------------------------

void __fastcall TForm1::RzBitBtnLangEngClick(TObject *Sender)
{
 RzCheckListLangStart->Add(LANGENG);
 RzCheckListLangStart->ItemIndex = RzCheckListLangStart->Count-1;
 RzCheckListLangStart->ItemState[RzCheckListLangStart->Count-1] = cbChecked;
}
//------------заповнення шаблону укр мови------------------------------

void __fastcall TForm1::RzBitBtnLangUkrClick(TObject *Sender)
{
 RzCheckListLangStart->Add(LANGUKR);
 RzCheckListLangStart->ItemIndex = RzCheckListLangStart->Count-1;
 RzCheckListLangStart->ItemState[RzCheckListLangStart->Count-1] = cbChecked;

}
//---------------заповнення шаблону рос мови--------------------------------

void __fastcall TForm1::RzBitBtnLangRusClick(TObject *Sender)
{
 RzCheckListLangStart->Add(LANGRUS);
 RzCheckListLangStart->ItemIndex = RzCheckListLangStart->Count-1;
 RzCheckListLangStart->ItemState[RzCheckListLangStart->Count-1] = cbChecked;

}
//----------виведення даних про системні папки---------------------------------

void __fastcall TForm1::ButtonFoldersClick(TObject *Sender)
{
RzTabbedListBox1->Items->Clear(); // очищення списку
TRegistry *reg = new TRegistry(); // створення змінної для роботи з регістром
try
{
 reg->RootKey = HKEY_CURRENT_USER;
 reg->OpenKeyReadOnly("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");
  // встановлення в потрібну позицію в дереві реєстра
 TStringList *stl = new TStringList(); // створення екземпляру рядкового списку
 reg->GetValueNames(stl); // отримання даних з потрібної гілки регістра
 for (int i=0;i<stl->Count;i++)
 {
  AnsiString stlT = stl->Strings[i] +
                    " = " +
                    reg->ReadString(stl->Strings[i]);
  RzTabbedListBox1->Items->Add(stlT); // виведення даних про системні папки
 };
 delete stl;       // знищення екземпляру рядкового списку
 reg->CloseKey();  // закриття реєстру
}
__finally
{
delete reg;        // знищення екземпляру реєстру
}

}
//------------виведення інформації про процесор-------------------------------

void __fastcall TForm1::ButtonProcessorClick(TObject *Sender)
{
TRegistry *reg = new TRegistry(); // ствоерння екземпляру регістру
AnsiString cpu,
           vendor,
           ident;
int      mhz;
try
{
 reg->RootKey = HKEY_LOCAL_MACHINE; // вибір кореня в дереві регістру
 if(reg->OpenKeyReadOnly("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"))
              // відкриття потрібної гілки регістра
 {
  cpu = reg->ReadString("ProcessorNameString"); // зчитування імені процесора
  vendor = reg->ReadString("VendorIdentifier"); // зчитування вендора
  ident = reg->ReadString("Identifier"); // зчитування ідентифікатора
  mhz = reg->ReadInteger("~MHz"); // зчитування частоти процесора
 };
 reg->CloseKey(); // закриття дерева регістру
}
__finally
{
delete reg; // видалення екземпляру регістру
};

  Edit2proc->Text = cpu;     // виведення інформації про назву процесора
  Edit2vendor->Text = vendor;// виведення інформації про вендор
  Edit2ident->Text = ident;  // виведення інформації про ідентифікатор
  Edit2mhz->Text = mhz;      // виведення інформації про частоту процесора

}
//----------виведення інформації про диски-----------------------------------

void __fastcall TForm1::ButtonDiskClick(TObject *Sender)
{

int typeD,
    countDisk = 0,
    countDrive = 0;
__int64 totalSp,
        emptySp,
        usedSp;
char dd[4];
DWORD dr = GetLogicalDrives(); // отримання масиву (бітова маска) імен дисків

for( int i = 0; i < 26; i++ ) // перебір дисків
 {
  if( ((dr>>i)&0x00000001) == 1 )
   {
    dd[0] =  char(65+i);  // формування строкового імені диску
    dd[1] = ':';
    dd[2] = '\\';
    dd[3] = 0;
    typeD = GetDriveType(dd); // отримання типу диску

    arrDisk[countDisk].name = AnsiString(dd); // запис імені диску в масив
    arrDisk[countDisk].type = typeD;          // запис типу диску в масив

    countDisk++; // перехід до наступного разряду в масці

   }; //end if
 }; // end for

int countHard = 3;
for( int i=0; i<countDisk; i++ )
 {
  if (arrDisk[i].type==3 || (arrDisk[i].type==2 && i>1))
  // якщо диск фіксований або змінний, але не дискета
  {
  arrDisk[i].totalSpace = DiskSize(countHard)/1024;
                         // отримуємо загальний розмір диску
  arrDisk[i].emptySpace = DiskFree(countHard)/1024;
                         // отримуємо вільне місце на диску
  arrDisk[i].usedSpace = arrDisk[i].totalSpace - arrDisk[i].emptySpace;
                         // розраховуємо зайняте місце на диску
  countHard++;
  };
 };

  DISK_GEOMETRY pdg;
  BOOL bResult;
  __int64 DiskSize1;
  char buf[100];
  int p;

// Отримання списку дисків
  DWORD len = GetLogicalDriveStringsA(sizeof(buf)/sizeof(char), buf);
//  LPCSTR sDrivePath;
  for (char* s = buf; *s; s+=strlen(s)+1)
  {
    bResult = GetDriveGeometry (&pdg, s); // отримання структури з параметрами диску
    if (bResult) // якщо параметри отримані успішно
    {
     DiskSize1 = pdg.Cylinders.QuadPart * pdg.TracksPerCylinder *
                 pdg.SectorsPerTrack * pdg.BytesPerSector;
     // розрахунок обєму носія за фізичними параментрами диску
     if ((arrDisk[countDrive].type==2)) // якщо носій змінний
      {
       arrDisk[countDrive].totalSpace = DiskSize1/1024;
       // запис обєму диску в КілоБайтах
      }
    };

   countDrive++;
  };

  // формування розмірів таблиці
StringGridHDD->RowCount = countDisk+1/*header*/+1/*total*/;
StringGridHDD->ColCount = 6;

__int64 totalHDD = 0,
        emptyHDD = 0,
        usedHDD = 0;
AnsiString namesHDD = "";

//  totalCountHDD=0;

for( int i=0; i<countDisk; i++ ) // перебір масиву дисків
 {
  if (arrDisk[i].type == 3) // якщо диск фіксований
  {
//   totalCountHDD++; // підрахунок кількості логічних дисків вінчестера
   totalHDD += arrDisk[i].totalSpace; // накопичення загального обєму
   emptyHDD += arrDisk[i].emptySpace; // накопичення вільного обєму
   usedHDD  += arrDisk[i].usedSpace;  // накопичення використаного обєму
   namesHDD += arrDisk[i].name + " "; // зберігання імен логічних дисків
  }
    // виведення в таблицю інформації про диски
  StringGridHDD->Cells[0][i+1] = i+1;
  StringGridHDD->Cells[1][i+1] = arrDisk[i].name;
  StringGridHDD->Cells[2][i+1] = TypeDiskString(arrDisk[i].type);
  if (arrDisk[i].totalSpace!=0)
  StringGridHDD->Cells[3][i+1] = FormatFloat("000000000,",arrDisk[i].totalSpace);
  else
  StringGridHDD->Cells[3][i+1] = "-";
  if (arrDisk[i].emptySpace!=0)
  StringGridHDD->Cells[4][i+1] = FormatFloat("000000000,",arrDisk[i].emptySpace);
  else
  StringGridHDD->Cells[4][i+1] = "-";
  if (arrDisk[i].usedSpace!=0)
  StringGridHDD->Cells[5][i+1] = FormatFloat("000000000,",arrDisk[i].usedSpace);
  else
  StringGridHDD->Cells[5][i+1] = "-";
 };

    // виведення в таблицю інформації про жорсткий диск
  StringGridHDD->Cells[0][countDisk+1] = "Total";
  StringGridHDD->Cells[1][countDisk+1] = "HDD";
  StringGridHDD->Cells[2][countDisk+1] = namesHDD;
  StringGridHDD->Cells[3][countDisk+1] = FormatFloat("000000000,",totalHDD);
  StringGridHDD->Cells[4][countDisk+1] = FormatFloat("000000000,",emptyHDD);
  StringGridHDD->Cells[5][countDisk+1] = FormatFloat("000000000,",usedHDD);

}
//------------формування таблиці--------------------------------

void __fastcall TForm1::ActionCreateStrGridHDDExecute(TObject *Sender)
{
// встановлення діапазону виділення за межами таблиці
StringGridHDD->Selection = gr;

// порожня таблиця має два рядки з рядком заголовку включно
StringGridHDD->RowCount=2;

// обчислення пропорційних розмірів колонок таблиці
int w = StringGridHDD->Width/12;
StringGridHDD->ColWidths[0] = w-3;
StringGridHDD->ColWidths[1] = w;
StringGridHDD->ColWidths[2] = 4*w;
StringGridHDD->ColWidths[3] = 2*w;
StringGridHDD->ColWidths[4] = 2*w;
StringGridHDD->ColWidths[5] = 2*w-7;

// підписи рядка заголовка
StringGridHDD->Cells[0][0] = "#";
StringGridHDD->Cells[1][0] = "name";
StringGridHDD->Cells[2][0] = "type";
StringGridHDD->Cells[3][0] = "total, KB";
StringGridHDD->Cells[4][0] = "free, KB";
StringGridHDD->Cells[5][0] = "used, KB";
}
//---------виведення інформації про память------------------------------------

void __fastcall TForm1::ButtonMemoryClick(TObject *Sender)
{
int mem_size;
_MEMORYSTATUS mem; // змінна-структура, що зберігає інформаю про память
AnsiString strT;

mem.dwLength = sizeof(MEMORYSTATUS); // встановлення розміру структури
GlobalMemoryStatus(&mem);

ListBoxMemory->Items->Clear(); // очищення списку

mem_size=mem.dwTotalPhys/(1024); // отримання загального обєму памяті в Кб
//strT = "physical memory: " + AnsiString(mem_size)+" Mбайт";
strT = "physical memory: " + AnsiString(FormatFloat("#,",mem_size))+" Kбайт";
ListBoxMemory->Items->Add(strT);

mem_size=mem.dwAvailPhys/(1024*1024); // отримання вільного обєму памяті в Мб
strT = "free physical memory: " + AnsiString(mem_size)+" Mбайт";
ListBoxMemory->Items->Add(strT);

mem_size=mem.dwMemoryLoad;  // відсоток використаної памяті
strT = "memory in use: " + AnsiString(mem_size)+" %";
ListBoxMemory->Items->Add(strT);

mem_size=mem.dwTotalPageFile/(1024*1024);  // отримання обєму page-файлу в Мб
strT = "total paging file: " + AnsiString(mem_size)+" Мбайт";
ListBoxMemory->Items->Add(strT);

mem_size=mem.dwAvailPageFile/(1024*1024);  // отримання выльного обєму page-файлу в Мб
strT = "free bytes of paging file: " + AnsiString(mem_size)+" Мбайт";
ListBoxMemory->Items->Add(strT);

mem_size=mem.dwTotalVirtual/(1024*1024);  // отримання загального обєму адресного простору в Мб
strT = "total user size of address space: " + AnsiString(mem_size)+" Мбайт";
ListBoxMemory->Items->Add(strT);

mem_size=mem.dwAvailVirtual/(1024*1024);  // отримання вільного обєму адресного простору в Мб
strT = "free user size of address space: " + AnsiString(mem_size)+" Мбайт";
ListBoxMemory->Items->Add(strT);

CheckBoxMemory->Enabled = true; // дозвіл прапорця

}
//--------оновлення інформації про память-------------------------------------

void __fastcall TForm1::Timer1Timer(TObject *Sender)
{
 ButtonMemoryClick(Sender); // таймер запускає оновлення інформаії про память
}
//--------звязок з перемикачем стану-----------------------------------------

void __fastcall TForm1::CheckBoxMemoryClick(TObject *Sender)
{
 Timer1->Enabled = CheckBoxMemory->Checked;
}
//-----------виведення інформації про шрифти------------------------------------

void __fastcall TForm1::ButtonFontsClick(TObject *Sender)
{
 ListBoxFonts->Items->Clear(); // очищення списку
for (int i = 0; i < Screen->Fonts->Count; i++)
{
 ListBoxFonts->Items->Add(IntToStr(i+1)+ ". " + Screen->Fonts->Strings[i]);
  // виведення імен шрифтів з масиву шрифтів поточного вікна системи
}

}
//--------виведення інформації про діючі сервіси-----------------------------


void __fastcall TForm1::ButtonServicesClick(TObject *Sender)
{
ListBoxServices->Clear();

        ENUM_SERVICE_STATUS Status[SIZE_BUF];
	DWORD Size = sizeof(Status);
	DWORD Needed = 0;
	DWORD Return = 0;
	DWORD Handle = 0;
	SC_HANDLE Manager;

	Manager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);

	if (Manager!=NULL)
	{
		if (EnumServicesStatus(Manager,SERVICE_WIN32,SERVICE_ACTIVE,
		(LPENUM_SERVICE_STATUS)&Status,Size,&Needed,&Return,&Handle))
		{
			for (unsigned int x=0; x < Return; x++)
//				cout << Status[x].lpServiceName << endl;
                        ListBoxServices->Items->Add(Status[x].lpServiceName);
		}
		else Application->MessageBox("Error Open Manager!", "Error", MB_OK); //cout << "Error Open Manager " << endl;
	}
	else Application->MessageBox("Error enum Services!", "Error", MB_OK); //cout << "Error enum Services" << endl;
}
//------зміна кольору фону елементів програми-------------------------------


void __fastcall TForm1::RzRGControlSkinClick(TObject *Sender)
{
 TColor ccolor;

 switch (RzRGControlSkin->ItemIndex)
 {
  case 0:
  {
   ccolor = clBtnFace;
   break;
  }
  case 1:
  {
   ccolor = clAqua;
   break;
  }
  case 2:
  {
   ccolor = clWhite;
   break;
  }
  case 3:
  {
   ccolor = clTeal;
   break;
  }
  case 4:
  {
   ccolor = clSilver;
   break;
  }
  case 5:
  {
   ccolor = clSkyBlue;
   break;
  }
  default:
  {
   break;
  }
 }
 Form1->Color = ccolor;
 RzPanel1->Color = ccolor;
 RzPanelToolBar->Color = ccolor;
 RzStatusBar1->Color = ccolor;
}
//------визначення місця розміщення елементів----------------------------------

void __fastcall TForm1::RzRGPageControlClick(TObject *Sender)
{
 TRzTabOrientation tabor;

 switch (RzRGPageControl->ItemIndex)
 {
  case 0:
  {
   tabor = toTop;
   break;
  }
  case 1:
  {
   tabor = toBottom;
   break;
  }
  case 2:
  {
   tabor = toLeft;
   break;
  }
  case 3:
  {
   tabor = toRight;
   break;
  }
  default:
  {
   break;
  }
 }

   RzPageControl0Main->TabOrientation = tabor;
}
//------------ініціювання таймеру--------------------------------------


void __fastcall TForm1::TimerRunningTimer(TObject *Sender)
{

//  LabelRunning->Caption = LabelRunning->Caption.SubString(2, (LabelRunning->Caption.Length() - 1));
//  if(LabelRunning->Caption.Length()<1)
//   LabelRunning->Caption = InitialCaption;

 LabelRunning->Left-=3;
 if (LabelRunning->Left < -LabelRunning->Width)
  LabelRunning->Left = RzPanel1->ClientWidth;
}
//-------включення таймеру------------------------------------------

void __fastcall TForm1::RzRGrunningClick(TObject *Sender)
{
 switch (RzRGrunning->ItemIndex)
 {
  case 0:
  {
   TimerRunning->Enabled = true;
   break;
  }
  case 1:
  {
   TimerRunning->Enabled = false;
   break;
  }
  default:
  {
   break;
  }
 }
}
//-----------процедура налагодження параметрів вікна---------------------------

void __fastcall TForm1::RzCheckListAddChange(TObject *Sender, int Index,
      TCheckBoxState NewState)
{

TBorderIcons tempBI = Form1->BorderIcons;

switch (Index)
{
 case 0:
 {
  RzTrayIconMain->Enabled = !RzCheckListAdd->ItemChecked[Index];
  break;
 }
 case 1:
 {
  break;
 }
 case 2:
 {
  if (!RzCheckListAdd->ItemChecked[Index])
  {    Form1->BorderStyle = bsSizeable;  }
  else
  {   Form1->BorderStyle = bsSingle;     }
  break;
 }
 case 3:
 {
  if (RzCheckListAdd->ItemChecked[Index])
  {
   tempBI >> biSystemMenu;
   Form1->BorderIcons = tempBI;
  }
  else
  {
   tempBI << biSystemMenu;
   Form1->BorderIcons = tempBI;
  }
  break;
 }
 case 4:
 {
  if (RzCheckListAdd->ItemChecked[Index])
  {
   tempBI >> biMinimize;
   Form1->BorderIcons = tempBI;
  }
  else
  {
   tempBI << biMinimize;
   Form1->BorderIcons = tempBI;
  }
  break;
 }
 case 5:
 {
  if (RzCheckListAdd->ItemChecked[Index])
  {
   tempBI >> biMaximize;
   Form1->BorderIcons = tempBI;
  }
  else
  {
   tempBI << biMaximize;
   Form1->BorderIcons = tempBI;
  }
  break;
 }
 default:
 {
  break;
 }
}
}
//---------------------------------------------------------------------------


void __fastcall TForm1::Exit2Click(TObject *Sender)
{
 Form1->Close();
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Restore1Click(TObject *Sender)
{
 Application->Restore();
}
//-------------виведення підказки в нижньому куту------------------------------



void __fastcall TForm1::RzTrayIconMainMinimizeApp(TObject *Sender)
{
 if (!RzCheckListAdd->ItemChecked[1])
 {
  RzTrayIconMain->ShowBalloonHint("Внимание", "Работает ВинТ", 0x1, 0xa);
 }
}
//--------створення інтерфейсу системних кнопок---------------------------------


void __fastcall TForm1::RzRGToolButtonPictClick(TObject *Sender)
{
 buttpicFlag = RzRGToolButtonPict->ItemIndex;
 AnsiString pathT;

 pathT = progPath + "img\\save.bmp";
 if (FileExists(pathT))
  RzShB1Save->Bitmap->LoadFromFile(pathT);
 else
  ShowMessage("Error. " + pathT + "not found!");

 pathT = progPath + "img\\reg.bmp";
 if (FileExists(pathT))
  RzShB2Reg->Bitmap->LoadFromFile(pathT);
 else
  ShowMessage("Error. " + pathT + "not found!");

 pathT = progPath + "img\\config.bmp";
 if (FileExists(pathT))
  RzShB3Config->Bitmap->LoadFromFile(pathT);
 else
  ShowMessage("Error. " + pathT + "not found!");

 pathT = progPath + "img\\help.bmp";
 if (FileExists(pathT))
  RzShB4Help->Bitmap->LoadFromFile(pathT);
 else
  ShowMessage("Error. " + pathT + "not found!");

 pathT = progPath + "img\\about.bmp";
 if (FileExists(pathT))
  RzShB5About->Bitmap->LoadFromFile(pathT);
 else
  ShowMessage("Error. " + pathT + "not found!");

 pathT = progPath + "img\\exit.bmp";
 if (FileExists(pathT))
  RzShB6Exit->Bitmap->LoadFromFile(pathT);
 else
  ShowMessage("Error. " + pathT + "not found!");

 RzShB1Save->Caption = "save";
 RzShB2Reg->Caption = "reg";
 RzShB3Config->Caption = "config";
 RzShB4Help->Caption = "help";
 RzShB5About->Caption = "about";
 RzShB6Exit->Caption = "exit";

switch (buttpicFlag)
{
case 1:
 {
 RzShB1Save->Caption = "";
 RzShB2Reg->Caption = "";
 RzShB3Config->Caption = "";
 RzShB4Help->Caption = "";
 RzShB5About->Caption = "";
 RzShB6Exit->Caption = "";
 break;
 }

case 2:
 {
 RzShB1Save->Bitmap = NULL;
 RzShB2Reg->Bitmap = NULL;
 RzShB3Config->Bitmap = NULL;
 RzShB4Help->Bitmap = NULL;
 RzShB5About->Bitmap = NULL;
 RzShB6Exit->Bitmap = NULL;
 break;
 }
} // end case

}
//------виведення інформації про запущені процеси-----------------------------------------------

void __fastcall TForm1::ButtonProcessClick(TObject *Sender)
{
 RzListBoxProcess->Clear();

TStringList *slist = new TStringList();
try
{
 GetActiveAppList(slist);
// ListBoxServices->Items->Add(Status[x].lpServiceName);
 RzListBoxProcess->Items = slist;
// Mem_Info->Lines->Text = slist->Text;
}
__finally
{
 delete slist;
}
}
//------------очищення списку запущених сервісів------------------------------

void __fastcall TForm1::ButtonServiceClearClick(TObject *Sender)
{
 ListBoxServices->Clear();
}
//---------------очищення списку запущених процесів--------------------------

void __fastcall TForm1::ButtonProcessClearClick(TObject *Sender)
{
 RzListBoxProcess->Clear();
}
//------------виведення сумарної системної інформації------------------------

void __fastcall TForm1::ButtonInfoAllClick(TObject *Sender)
{
// для отримання сумарної інформації ініціюються всі окремі процедури
  ButtonFoldersClick(Sender);
  ButtonProcessorClick(Sender);
  ButtonMemoryClick(Sender);
  ButtonDiskClick(Sender);
  ButtonFontsClick(Sender);

// заголовок інформації
  pRich->Lines->Clear();
   pRich->Paragraph->Alignment = taCenter;
    pRich->SelAttributes->Color = clRed;
    pRich->SelAttributes->Style = pRich->SelAttributes->Style << fsBold;
  pRich->Lines->Add("System Information.");
  pRich->Lines->Add("");

// заголовок інформаційного розділу "Апаратна частина"
   pRich->Paragraph->Alignment = taCenter;
    pRich->SelAttributes->Color = clGreen;
    pRich->SelAttributes->Style = pRich->SelAttributes->Style << fsBold;
  pRich->Lines->Add("Hardware (CPU & Memory) Information:");
  pRich->Lines->Add("");
// інформаційна частина розділу "Апаратна частина"
   pRich->Paragraph->Alignment = taLeftJustify;
    pRich->SelAttributes->Color = clBlack;
    pRich->SelAttributes->Style = pRich->SelAttributes->Style << fsBold;
  pRich->Lines->Add("Processor Name:");
   pRich->Paragraph->FirstIndent = 10;
  pRich->Lines->Add(Edit2proc->Text);
   pRich->Paragraph->FirstIndent = 0;
    pRich->SelAttributes->Style = pRich->SelAttributes->Style << fsBold;
  pRich->Lines->Add("CPU Vendor:");
   pRich->Paragraph->FirstIndent = 10;
  pRich->Lines->Add(Edit2vendor->Text);
   pRich->Paragraph->FirstIndent = 0;
    pRich->SelAttributes->Style = pRich->SelAttributes->Style << fsBold;
  pRich->Lines->Add("CPU Identifier:");
   pRich->Paragraph->FirstIndent = 10;
  pRich->Lines->Add(Edit2ident->Text);
   pRich->Paragraph->FirstIndent = 0;
    pRich->SelAttributes->Style = pRich->SelAttributes->Style << fsBold;
  pRich->Lines->Add("Initial CPU Frequency:");
   pRich->Paragraph->FirstIndent = 10;
  pRich->Lines->Add(Edit2mhz->Text + " MHz");
   pRich->Paragraph->FirstIndent = 0;
    pRich->SelAttributes->Style = pRich->SelAttributes->Style << fsBold;
  pRich->Lines->Add("Memory Information:");
   pRich->Paragraph->FirstIndent = 10;
  pRich->Lines->AddStrings(ListBoxMemory->Items);
   pRich->Paragraph->FirstIndent = 0;
  pRich->Lines->Add("");
  pRich->Lines->Add("");

// заголовок інформаційного розділу "Диски"
   pRich->Paragraph->Alignment = taCenter;
    pRich->SelAttributes->Color = clGreen;
    pRich->SelAttributes->Style = pRich->SelAttributes->Style << fsBold;
  pRich->Lines->Add("Disks Information:");
  pRich->Lines->Add("");
// інформаційна частина розділу "Диски"
   pRich->Paragraph->Alignment = taLeftJustify;
    pRich->SelAttributes->Color = clBlack;
    pRich->SelAttributes->Style = pRich->SelAttributes->Style << fsBold;
  pRich->Lines->Add("Number of Disks:");
   pRich->Paragraph->FirstIndent = 10;
  pRich->Lines->Add(IntToStr(StringGridHDD->RowCount-2));
   pRich->Paragraph->FirstIndent = 0;
    pRich->SelAttributes->Style = pRich->SelAttributes->Style << fsBold;
  pRich->Lines->Add("Disks:");
   pRich->Paragraph->FirstIndent = 10;
  pRich->Lines->Add("");
 for (int i=1;i<StringGridHDD->RowCount-1;i++)
 {
   pRich->Paragraph->FirstIndent = 10;
  pRich->Lines->Add("Letter: " + StringGridHDD->Cells[1][i] + "   " +
                    "Type: " + StringGridHDD->Cells[2][i] + "    " +
                    "Total Size: " + StringGridHDD->Cells[3][i] + " KB = " +
                    "Free Space: " + StringGridHDD->Cells[4][i] + " KB + " +
                    "Used Space: " + StringGridHDD->Cells[5][i] + " KB");
    pRich->Paragraph->FirstIndent = 0;
  }
  pRich->Lines->Add("");
   pRich->SelAttributes->Style = pRich->SelAttributes->Style << fsBold;
  pRich->Lines->Add("HDD:");
   pRich->Paragraph->FirstIndent = 10;
  pRich->Lines->Add("Logical Disks Letters: " +
                     StringGridHDD->Cells[2][StringGridHDD->RowCount-1]);
  pRich->Lines->Add("Total Size: " +
                     StringGridHDD->Cells[3][StringGridHDD->RowCount-1] + " KB");
  pRich->Lines->Add("Free Space: " +
                     StringGridHDD->Cells[4][StringGridHDD->RowCount-1] + " KB");
  pRich->Lines->Add("Used Space: " +
                     StringGridHDD->Cells[5][StringGridHDD->RowCount-1] + " KB");
    pRich->Paragraph->FirstIndent = 0;
  pRich->Lines->Add("");
  pRich->Lines->Add("");

// заголовок інформаційного розділу "Шрифти"
   pRich->Paragraph->Alignment = taCenter;
    pRich->SelAttributes->Color = clGreen;
    pRich->SelAttributes->Style = pRich->SelAttributes->Style << fsBold;
  pRich->Lines->Add("Fonts Information:");
  pRich->Lines->Add("");
// інформаційна частина розділу "Шрифти"
   pRich->Paragraph->Alignment = taLeftJustify;
    pRich->SelAttributes->Color = clBlack;
    pRich->SelAttributes->Style = pRich->SelAttributes->Style << fsBold;
  pRich->Lines->Add("System Fonts:");
   pRich->Paragraph->FirstIndent = 10;
  pRich->Lines->AddStrings(ListBoxFonts->Items);
   pRich->Paragraph->FirstIndent = 0;
  pRich->Lines->Add("");
  pRich->Lines->Add("");

 pRich->SelStart = pRich->Perform(EM_LINEINDEX, 0, 0) + 0;
 Application->ProcessMessages();
 pRich->SetFocus();
 //очищаються всі поля
 RzTabbedListBox1->Items->Clear(); // очищення списку
  Edit2proc->Text = "";
  Edit2vendor->Text = "";
  Edit2ident->Text = "";
  Edit2mhz->Text = "";
  ListBoxMemory->Items->Clear(); // очищення списку
  ListBoxFonts->Items->Clear(); // очищення списку

  //очищення таблиці з інформацією про диски
  StringGridHDD->RowCount = 2;
  StringGridHDD->ColCount = 6;
  StringGridHDD->Cells[0][1] = " ";
  StringGridHDD->Cells[1][1] = " ";
  StringGridHDD->Cells[2][1] = " ";
  StringGridHDD->Cells[3][1] = " ";
   StringGridHDD->Cells[4][1] = " ";
   StringGridHDD->Cells[5][1] = " ";
}
//-------------збереження інформації у файл----------------------------------

void __fastcall TForm1::RzShB1SaveClick(TObject *Sender)
{
 if (SaveDialogInfo->Execute()) // якщо було введено імя файла
 {

  ButtonInfoAllClick(Sender); // ініційовано отримання сумарної інформації

  switch (SaveDialogInfo->FilterIndex) // який тип файлу обрано
  {
  case 2:
   {
    pRich->PlainText = true; // для текстового файлу вимикається форматування
    pRich->Lines->SaveToFile(SaveDialogInfo->FileName+".txt");
             // сумарна інформація записуєтсья у файл
    RzStatusBar1->SimpleCaption = // виводиться інформація в рядок стану
               "SysInfo was saved to file <" + SaveDialogInfo->FileName + ".txt>";
    break;
   }
  default:
//  case 1,3:
   {
    pRich->PlainText = false; // для rtf файлу вмикається форматування
    pRich->Lines->SaveToFile(SaveDialogInfo->FileName+".rtf");
             // сумарна інформація записуєтсья у файл
    RzStatusBar1->SimpleCaption = // виводиться інформація в рядок стану
               "SysInfo was saved to file <" + SaveDialogInfo->FileName + ".rtf>";
    break;
   }
//  default:
//   {
//    break;
//   }
  } // end switch

 } // end if for SaveDialog

}
//--------ініціалізація клавіатури--------------------------------------------



void __fastcall TForm1::ButtonKeyboardPresetClick(TObject *Sender)
{
 AnsiString str;
 TRegistry *Registry = new TRegistry;
 TStringList *StrVal = new TStringList();

 try
 {
  Registry->RootKey=HKEY_USERS; 
  Registry->OpenKey(".DEFAULT\\Keyboard Layout\\Preload",true); //  Прочитать ветвь в реестре
  Registry->GetValueNames(StrVal);
  if (StrVal->Count >0)
  {
   for(int i=0;i<StrVal->Count;i++)
   {
    Registry->DeleteValue(StrVal->Strings[i]);
   };
  }

   for(int i=0;i<RzCheckListLangStart->Count;i++)
   {
    Registry->WriteString(IntToStr(i+1),
                          RzCheckListLangStart->Items->Strings[i]);
   };

  Registry->CloseKey();
 }
 __finally
 {
  delete Registry;
  delete StrVal;
 }

}
//---------------------------------------------------------------------------


//------зчитування з регістру---------------------------------------


void __fastcall TForm1::ActionRegDebugExecute(TObject *Sender)
{

TRegistry *Registry=new  TRegistry;
try
{
Registry->RootKey=HKEY_LOCAL_MACHINE; 
Registry->OpenKey("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug",false); 
if (Registry->ValueExists("Auto"))
 {
   if (Registry->ReadString("Auto")=="1")    //зчитування рядкових даних
   {
    CheckBox18->Checked=true;
   }
   else
   {
    CheckBox18->Checked=false;
   }
 }
 else
 {
  CheckBox18->Checked=false;
 }

 Registry->CloseKey();
}
__finally
{
delete  Registry;
}

}
//---------------------------------------------------------------------------

void __fastcall TForm1::ActionRegKillProcessExecute(TObject *Sender)
{

TRegistry *Registry=new  TRegistry;
try
{
Registry->RootKey=HKEY_CURRENT_USER; 
Registry->OpenKey("Control Panel\\Desktop",false); 
if (Registry->ValueExists("AutoEndTasks"))
 {
   if (Registry->ReadString("AutoEndTasks")=="1")
   {
    CheckBox19->Checked=true;
   }
   else
   {
    CheckBox19->Checked=false;
   }
 }
 else
 {
    CheckBox19->Checked=false;
 }

 Registry->CloseKey();
}
__finally
{
delete  Registry;
}

}
//---------------------------------------------------------------------------

void __fastcall TForm1::ActionRegAutorunExecute(TObject *Sender)
{

TRegistry *Registry=new  TRegistry;
try
{
Registry->RootKey=HKEY_LOCAL_MACHINE; 
Registry->OpenKey("SYSTEM\\CurrentControlSet\\Services\\CDRom",false); 
if (Registry->ValueExists("AutoRun"))
 {
   if (Registry->ReadInteger("AutoRun")==0)   //зчитування числових даних
   {
    CheckBox20->Checked=true;
   }
   else
   {
    CheckBox20->Checked=false;
   }
 }
 else
 {
  CheckBox20->Checked=false;
 }

 Registry->CloseKey();
}
__finally
{
delete  Registry;
}

}
//---------------------------------------------------------------------------

void __fastcall TForm1::ActionRegSpaceMessageExecute(TObject *Sender)
{

TRegistry *Registry=new  TRegistry;
try
{
Registry->RootKey=HKEY_CURRENT_USER; 
Registry->OpenKey("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer",false); 
if (Registry->ValueExists("NoLowDiskSpaceChecks"))
 {
   if (Registry->ReadInteger("NoLowDiskSpaceChecks")==0)
   {
    CheckBox21->Checked=true;
   }
   else
   {
    CheckBox21->Checked=false;
   }
 }
 else
 {
  CheckBox21->Checked=false;
 }

 Registry->CloseKey();
}
__finally
{
delete  Registry;
}

}
//---------------------------------------------------------------------------

void __fastcall TForm1::ActionRegDefragExecute(TObject *Sender)
{
TRegistry *Registry=new  TRegistry;
try
{
Registry->RootKey=HKEY_LOCAL_MACHINE; 
Registry->OpenKey("System\\CurrentControlSet\\Control\\FileSystem",false); 
if (Registry->ValueExists("ContigFileAllocSize"))
 {
  CheckBox22->Checked=true;
 }
 else
 {
  CheckBox22->Checked=false;
 }

 Registry->CloseKey();
}
__finally
{
delete  Registry;
}
}
//---------------------------------------------------------------------------

void __fastcall TForm1::ActionRegAutoCleanExecute(TObject *Sender)
{
 TRegistry *Registry=new  TRegistry;
 try
{
 Registry->RootKey=HKEY_CURRENT_USER; 
 Registry->OpenKey("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer",false); 
 if (Registry->ValueExists("ClearRecentDocsOnExit"))
 {
   if (Registry->ReadInteger("ClearRecentDocsOnExit")==1)
   {
    CheckBox1->Checked=true;
   }
   else
   {
   CheckBox1->Checked=false;
   }
 }
 else
 {
   CheckBox1->Checked=false;
 }

 Registry->CloseKey();
}
__finally
{
delete  Registry;
}

}
//---------------------------------------------------------------------------

void __fastcall TForm1::ActionRegPassExecute(TObject *Sender)
{

 TRegistry *Registry=new  TRegistry;
 try
{
 Registry->RootKey=HKEY_CURRENT_USER; 
 Registry->OpenKey("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System",false); 
 if (Registry->ValueExists("NoSecCPL"))
 {
   if (Registry->ReadInteger("NoSecCPL")==1)
   {
    CheckBox5->Checked=true;
   }
   else
   {
   CheckBox5->Checked=false;
   }
 }
 else
 {
   CheckBox5->Checked=false;
 }

 Registry->CloseKey();
}
__finally
{
delete  Registry;
}

}
//---------------------------------------------------------------------------

void __fastcall TForm1::ActionRegPassChangeExecute(TObject *Sender)
{

 TRegistry *Registry=new  TRegistry;
 try
{
 Registry->RootKey=HKEY_CURRENT_USER; 
 Registry->OpenKey("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System",false); 
 if (Registry->ValueExists("NoPwdPage"))
 {
   if (Registry->ReadInteger("NoPwdPage")==1)
   {
    CheckBox6->Checked=true;
   }
   else
   {
   CheckBox6->Checked=false;
   }
 }
 else
 {
   CheckBox6->Checked=false;
 }

 Registry->CloseKey();
}
__finally
{
delete  Registry;
}

}
//---------------------------------------------------------------------------

void __fastcall TForm1::ActionRegProfExecute(TObject *Sender)
{

 TRegistry *Registry=new  TRegistry;
 try
{
 Registry->RootKey=HKEY_CURRENT_USER; 
 Registry->OpenKey("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System",false); 
 if (Registry->ValueExists("NoProfilePage"))
 {
   if (Registry->ReadInteger("NoProfilePage")==1)
   {
    CheckBox7->Checked=true;
   }
   else
   {
   CheckBox7->Checked=false;
   }
 }
 else
 {
   CheckBox7->Checked=false;
 }

 Registry->CloseKey();
}
__finally
{
delete  Registry;
}

}
//---------------------------------------------------------------------------

void __fastcall TForm1::ActionRegRemoteExecute(TObject *Sender)
{

 TRegistry *Registry=new  TRegistry;
 try
{
 Registry->RootKey=HKEY_CURRENT_USER; 
 Registry->OpenKey("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System",false); 
 if (Registry->ValueExists("NoAdminPage"))
 {
   if (Registry->ReadInteger("NoAdminPage")==1)
   {
    CheckBox8->Checked=true;
   }
   else
   {
   CheckBox8->Checked=false;
   }
 }
 else
 {
   CheckBox8->Checked=false;
 }

 Registry->CloseKey();
}
__finally
{
delete  Registry;
}

}
//---------------------------------------------------------------------------

void __fastcall TForm1::ActionRegMonitorExecute(TObject *Sender)
{

 TRegistry *Registry=new  TRegistry;
 try
{
 Registry->RootKey=HKEY_CURRENT_USER; 
 Registry->OpenKey("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System",false); 
 if (Registry->ValueExists("NoDispCPL"))
 {
   if (Registry->ReadInteger("NoDispCPL")==1)
   {
    CheckBox9->Checked=true;
   }
   else
   {
   CheckBox9->Checked=false;
   }
 }
 else
 {
   CheckBox9->Checked=false;
 }

 Registry->CloseKey();
}
__finally
{
delete  Registry;
}

}
//---------------------------------------------------------------------------

void __fastcall TForm1::ActionRegMonitorBackExecute(TObject *Sender)
{

 TRegistry *Registry=new  TRegistry;
 try
{
 Registry->RootKey=HKEY_CURRENT_USER; 
 Registry->OpenKey("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System",false); 
 if (Registry->ValueExists("NoDispBackgroundPage"))
 {
   if (Registry->ReadInteger("NoDispBackgroundPage")==1)
   {
    CheckBox10->Checked=true;
   }
   else
   {
   CheckBox10->Checked=false;
   }
 }
 else
 {
   CheckBox10->Checked=false;
 }

 Registry->CloseKey();
}
__finally
{
delete  Registry;
}

}
//---------------------------------------------------------------------------

void __fastcall TForm1::ActionRegMonitorScreenExecute(TObject *Sender)
{

 TRegistry *Registry=new  TRegistry;
 try
{
 Registry->RootKey=HKEY_CURRENT_USER; 
 Registry->OpenKey("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System",false); 
 if (Registry->ValueExists("NoDispScrSavPage"))
 {
   if (Registry->ReadInteger("NoDispScrSavPage")==1)
   {
    CheckBox11->Checked=true;
   }
   else
   {
   CheckBox11->Checked=false;
   }
 }
 else
 {
   CheckBox11->Checked=false;
 }

 Registry->CloseKey();
}
__finally
{
delete  Registry;
}

}
//---------------------------------------------------------------------------

void __fastcall TForm1::ActionRegMonitorOptionExecute(TObject *Sender)
{

 TRegistry *Registry=new  TRegistry;
 try
{
 Registry->RootKey=HKEY_CURRENT_USER; 
 Registry->OpenKey("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System",false); 
 if (Registry->ValueExists("NoDispSettingsPage"))
 {
   if (Registry->ReadInteger("NoDispSettingsPage")==1)
   {
    CheckBox12->Checked=true;
   }
   else
   {
   CheckBox12->Checked=false;
   }
 }
 else
 {
   CheckBox12->Checked=false;
 }

 Registry->CloseKey();
}
__finally
{
delete  Registry;
}

}
//---------------------------------------------------------------------------


void __fastcall TForm1::ActionRegNetConnectExecute(TObject *Sender)
{

 TRegistry *Registry=new  TRegistry;
 try
{
 Registry->RootKey=HKEY_CURRENT_USER; 
 Registry->OpenKey("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer",false); 
 if (Registry->ValueExists("NoNetworkConnections"))
 {
   if (Registry->ReadInteger("NoNetworkConnections")==1)
   {
    CheckBox15->Checked=true;
   }
   else
   {
   CheckBox15->Checked=false;
   }
 }
 else
 {
   CheckBox15->Checked=false;
 }

 Registry->CloseKey();
}
__finally
{
delete  Registry;
}

}
//---------------------------------------------------------------------------

void __fastcall TForm1::ActionRegMyCompExecute(TObject *Sender)
{

 TRegistry *Registry=new  TRegistry;
 try
{
 Registry->RootKey=HKEY_CURRENT_USER; 
 Registry->OpenKey("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\NonEnum",false); 
 if (Registry->ValueExists("{20D04FE0-3AEA-1069-A2D8-08002B30309D}"))
 {
   if (Registry->ReadInteger("{20D04FE0-3AEA-1069-A2D8-08002B30309D}")==1)
   {
    CheckBox14->Checked=true;
   }
   else
   {
   CheckBox14->Checked=false;
   }
 }
 else
 {
   CheckBox14->Checked=false;
 }

 Registry->CloseKey();
}
__finally
{
delete  Registry;
}
        
}
//---------------------------------------------------------------------------

void __fastcall TForm1::ActionRegNetExecute(TObject *Sender)
{

 TRegistry *Registry=new  TRegistry;
 try
{
 Registry->RootKey=HKEY_CURRENT_USER; 
 Registry->OpenKey("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer",false);
 if (Registry->ValueExists("NoStartMenuNetworkPlaces"))
 {
   if (Registry->ReadInteger("NoStartMenuNetworkPlaces")==1)
   {
    CheckBox16->Checked=true;
   }
   else
   {
   CheckBox16->Checked=false;
   }
 }
 else
 {
   CheckBox16->Checked=false;
 }

 Registry->CloseKey();
}
__finally
{
delete  Registry;
}

}
//---------------------------------------------------------------------------

void __fastcall TForm1::ActionRegPrintFaxExecute(TObject *Sender)
{

 TRegistry *Registry=new  TRegistry;
 try
{
 Registry->RootKey=HKEY_CURRENT_USER; 
 Registry->OpenKey("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",false); 
 if (Registry->ValueExists("Start_ShowPrinters"))
 {
   if (Registry->ReadInteger("Start_ShowPrinters")==1)
   {
    CheckBox17->Checked=true;
   }
   else
   {
   CheckBox17->Checked=false;
   }
 }
 else
 {
   CheckBox17->Checked=false;
 }

 Registry->CloseKey();
}
__finally
{
delete  Registry;
}

}
//---------------------------------------------------------------------------

void __fastcall TForm1::ActionRegAllHideExecute(TObject *Sender)
{

 TRegistry *Registry=new  TRegistry;
 try
{
 Registry->RootKey=HKEY_CURRENT_USER; 
 Registry->OpenKey("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer",false); 
 if (Registry->ValueExists("NoControlPanel"))
 {
   if (Registry->ReadInteger("NoControlPanel")==1)
   {
    CheckBox13->Checked=true;
   }
   else
   {
   CheckBox13->Checked=false;
   }
 }
 else
 {
   CheckBox13->Checked=false;
 }

 Registry->CloseKey();
}
__finally
{
delete  Registry;
}

}
//---------------------------------------------------------------------------

void __fastcall TForm1::ActionRegHintExecute(TObject *Sender)
{

 TRegistry *Registry=new  TRegistry;
 try
{
 Registry->RootKey=HKEY_CURRENT_USER; 
 Registry->OpenKey("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",false); 
 if (Registry->ValueExists("ShowInfoTip"))
 {
   if (Registry->ReadInteger("ShowInfoTip")==0)
   {
    CheckBox3->Checked=true;
   }
   else
   {
   CheckBox3->Checked=false;
   }
 }
 else
 {
   CheckBox3->Checked=false;
 }

}
__finally
{
delete  Registry;
}

}
//---------------------------------------------------------------------------

void __fastcall TForm1::ActionRegMasterCleanExecute(TObject *Sender)
{

 TRegistry *Registry=new  TRegistry;
 try
{
 Registry->RootKey=HKEY_CURRENT_USER;
 Registry->OpenKey("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer",false);
 if (Registry->ValueExists("NoDesktopCleanupWizard"))
 {
   if (Registry->ReadInteger("NoDesktopCleanupWizard")==0)
   {
    CheckBox4->Checked=true;
   }
   else
   {
    CheckBox4->Checked=false;
   }
 }
 else
 {
    CheckBox4->Checked=false;
 }

}
__finally
{
delete  Registry;
}

}

//--- запуск редактору реєстру -----------------------------------------------
void __fastcall TForm1::RzButtonRegEditClick(TObject *Sender)
{
HWND Regedit=FindWindow("RegEdit_RegEdit",NULL), Tree, List;
if(Regedit==NULL) // якщо редактор реєстру ще не запущено
  {
  SHELLEXECUTEINFO info;
  ZeroMemory(&info,sizeof(SHELLEXECUTEINFO));
  info.cbSize=sizeof info;
  info.fMask=SEE_MASK_NOCLOSEPROCESS;
  info.lpVerb="open";
  info.lpFile="Regedit.exe";
  info.nShow=SW_SHOWNORMAL;
  ShellExecuteEx(&info);
  WaitForInputIdle(info.hProcess,INFINITE);
  Regedit=FindWindow("RegEdit_RegEdit",NULL);
  }
if(Regedit==NULL)
  {
  MessageDlg("\nНеможливо запустити Regedit.exe.",mtError,TMsgDlgButtons()<<mbOK,0);
   // помілка відкриття
  return;
  }
ShowWindow(Regedit,SW_MAXIMIZE); // показати вікно в розвернутому стані
SetForegroundWindow(Regedit); // активізувати вікно
}

//--- команда виклику інформації про автора ----------------------------------
void __fastcall TForm1::RzShB5AboutClick(TObject *Sender)
{
 Form2->ShowModal(); // показати вікно в модальному режимі
}

//--- завантаження інформації про ліцензію -----------------------------------
void __fastcall TForm1::ActionInfoLicenseExecute(TObject *Sender)
{
 AnsiString pathT;

 pathT = progPath + "docs\\license.rtf";
 if (FileExists(pathT)) // якщо файл існує
  RzRichLicense->Lines->LoadFromFile(pathT); // завантажити дані
 else
  ShowMessage("Error loading file =" + pathT + "=. License info not found!");
  // повідомлення про помилку
}

//--- завантаження інформації про автора --------------------------------------
void __fastcall TForm1::ActionInfoAuthorExecute(TObject *Sender)
{
 AnsiString pathT;

 pathT = progPath + "docs\\author.rtf";
 if (FileExists(pathT)) // якщо файл існує
  RzRichAuthor->Lines->LoadFromFile(pathT); // завантажити дані
 else
  ShowMessage("Error loading file =" + pathT + "=. Author info not found!");
  // повідомлення про помилку
}

//--- оновлення прапорців реєстру першої вкладки -----------------------------
void __fastcall TForm1::Button1Click(TObject *Sender)
{
 ActionRegDebugExecute(Sender);
 ActionRegKillProcessExecute(Sender);
 ActionRegAutorunExecute(Sender);
 ActionRegSpaceMessageExecute(Sender);
 ActionRegDefragExecute(Sender);
}

//--- оновлення прапорців реєстру другої вкладки -----------------------------
void __fastcall TForm1::Button2Click(TObject *Sender)
{
 ActionRegAutoCleanExecute(Sender);
 ActionRegPassExecute(Sender);
 ActionRegPassChangeExecute(Sender);
 ActionRegProfExecute(Sender);
 ActionRegRemoteExecute(Sender);
 ActionRegMonitorExecute(Sender);
 ActionRegMonitorBackExecute(Sender);
 ActionRegMonitorScreenExecute(Sender);
 ActionRegMonitorOptionExecute(Sender);
 ActionRegAllHideExecute(Sender);
 ActionRegMyCompExecute(Sender);
 ActionRegNetConnectExecute(Sender);
 ActionRegNetExecute(Sender);
 ActionRegPrintFaxExecute(Sender);
 ActionRegHintExecute(Sender);
 ActionRegMasterCleanExecute(Sender);        
}

//--- команда створення рег-файлу --------------------------------------------
void __fastcall TForm1::RzShB2RegClick(TObject *Sender)
{
 // save reg-file
 ActionRegCreateExecute(Sender);
}

//--- команда запису конфізурації --------------------------------------------
void __fastcall TForm1::RzShB3ConfigClick(TObject *Sender)
{
 //save config
 ActionConfigSaveExecute(Sender);
}

//--- виклик вікна допомоги ---------------------------------------------------
void __fastcall TForm1::RzShB4HelpClick(TObject *Sender)
{
 Form3->Show(); // показати вікно допомоги
}

//--- формування файлу конфігурації -------------------------------------------
void __fastcall TForm1::ActionConfigSaveExecute(TObject *Sender)
{
 TStringList *List = new TStringList;
 AnsiString strTemp;
 AnsiString filename = progPath + "\\config\\config.ini";

List->Add("[WinT config file]");        // 0 row -

 // RzRGLang
 List->Add("{Language}");               // 1nd row -
 strTemp = IntToStr(RzRGLang->ItemIndex);
 // 0 - eng
 // 1 - rus
 // 2 - ukr
 List->Add(strTemp);                    // 2nd row +

 // RzRGControlSkin
 List->Add("{Window Control Skin}");    // 3th row -
 strTemp = IntToStr(RzRGControlSkin->ItemIndex);
 // 0 - system
 // 1 - aqua
 // 2 - white
 // 3 - teal
 // 4 - silver
 // 5 - skyblue
 List->Add(strTemp);                    // 4nd row +

 // RzRGToolButtonPict
 List->Add("{Tool Buttons}");           // 5th row -
 strTemp = IntToStr(RzRGToolButtonPict->ItemIndex);
 // 0 - pict + text
 // 1 - pict
 // 2 - text
 List->Add(strTemp);                    // 6nd row +

 // RzCheckListView
 List->Add("{Controls}");               // 7th row -
 if (RzCheckListView->ItemChecked[0])
  strTemp = "1";
 else
  strTemp = "0";
 List->Add(strTemp);                    // 8th row +
 if (RzCheckListView->ItemChecked[1])
  strTemp = "1";
 else
  strTemp = "0";
 List->Add(strTemp);                    // 9th row +
 if (RzCheckListView->ItemChecked[2])
  strTemp = "1";
 else
  strTemp = "0";
 List->Add(strTemp);                    // 10th row +
 if (RzCheckListView->ItemChecked[3])
  strTemp = "1";
 else
  strTemp = "0";
 List->Add(strTemp);                    // 11th row +

 // RzRGrunning
 List->Add("{Running String}");         // 12th row -
 strTemp = IntToStr(RzRGrunning->ItemIndex);
 // 0 - on
 // 1 - off
 List->Add(strTemp);                    // 13th row +

 // RzRGPageControl
 List->Add("{Tab Orientation}");        // 14th row -
 strTemp = IntToStr(RzRGPageControl->ItemIndex);
 // 0 - top
 // 1 - bottom
 // 2 -  left
 // 3 - right
 List->Add(strTemp);                    // 15th row +

 // RzCheckListAdd
 List->Add("{Tray+TaskBar+Window}");    // 16th row -
 if (RzCheckListAdd->ItemChecked[0])
  strTemp = "1";
 else
  strTemp = "0";
 List->Add(strTemp);                    // 17th row +
 if (RzCheckListAdd->ItemChecked[1])
  strTemp = "1";
 else
  strTemp = "0";
 List->Add(strTemp);                    // 18th row +
 if (RzCheckListAdd->ItemChecked[2])
  strTemp = "1";
 else
  strTemp = "0";
 List->Add(strTemp);                    // 19th row +
 if (RzCheckListAdd->ItemChecked[3])
  strTemp = "1";
 else
  strTemp = "0";
 List->Add(strTemp);                    // 20th row +
 if (RzCheckListAdd->ItemChecked[4])
  strTemp = "1";
 else
  strTemp = "0";
 List->Add(strTemp);                    // 21th row +
 if (RzCheckListAdd->ItemChecked[5])
  strTemp = "1";
 else
  strTemp = "0";
 List->Add(strTemp);                    // 22th row +

 List->Add("[End config]");             // 23th row -

 List->SaveToFile(filename);

 delete List;
}

//--- завантаження конфігурації -----------------------------------------------
void __fastcall TForm1::ActionConfigLoadExecute(TObject *Sender)
{
 TStringList *List = new TStringList;
 AnsiString strTemp;
 AnsiString filename = progPath + "\\config\\config.ini";
 if (FileExists(filename))
 {
  List->LoadFromFile(filename); // завантаження інформацї з файлу

 // мова
  strTemp = List->Strings[2];
  RzRGLang->ItemIndex = StrToInt(strTemp);
 // кольори
  strTemp = List->Strings[4];
  RzRGControlSkin->ItemIndex = StrToInt(strTemp);
 // кнопки панелі інструментів
  strTemp = List->Strings[6];
  RzRGToolButtonPict->ItemIndex = StrToInt(strTemp);
 // відображення панелей
  strTemp = List->Strings[8];
  RzCheckListView->ItemChecked[0] = (strTemp=="1");
  strTemp = List->Strings[9];
  RzCheckListView->ItemChecked[1] = (strTemp=="1");
  strTemp = List->Strings[10];
  RzCheckListView->ItemChecked[2] = (strTemp=="1");
  strTemp = List->Strings[11];
  RzCheckListView->ItemChecked[3] = (strTemp=="1");
 // рухомий рядок
  strTemp = List->Strings[13];
  RzRGrunning->ItemIndex = StrToInt(strTemp);
 // положення вкладок
  strTemp = List->Strings[15];
  RzRGPageControl->ItemIndex = StrToInt(strTemp);
 // параметри вікна
  strTemp = List->Strings[17];
  RzCheckListAdd->ItemChecked[0] = (strTemp=="1");
  strTemp = List->Strings[18];
  RzCheckListAdd->ItemChecked[1] = (strTemp=="1");
  strTemp = List->Strings[19];
  RzCheckListAdd->ItemChecked[2] = (strTemp=="1");
  strTemp = List->Strings[20];
  RzCheckListAdd->ItemChecked[3] = (strTemp=="1");
  strTemp = List->Strings[21];
  RzCheckListAdd->ItemChecked[4] = (strTemp=="1");
  strTemp = List->Strings[22];
  RzCheckListAdd->ItemChecked[5] = (strTemp=="1");
  } // end if

  delete List;
}

//--- формування рег-файлу ----------------------------------------------------
void __fastcall TForm1::ActionRegCreateExecute(TObject *Sender)
{
if (SaveDialogReg->Execute()) // вибір або введення імені файлу
{

 TStringList *List = new TStringList;
 AnsiString filename = SaveDialogReg->FileName; // формування імені файлу

// додавання рядків
 List->Add("Windows Registry Editor Version 5.00");
 List->Add("");

// tab <system>
// debugger dr Watson
  List->Add("[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug]");
 if (CheckBox18->Checked)
  List->Add("\"Auto\"=\"1\"");
 else
  List->Add("\"Auto\"=\"0\"");
  List->Add("");

// autokill crashed process
  List->Add("[HKEY_CURRENT_USER\\Control Panel\\Desktop]");
 if (CheckBox19->Checked)
  List->Add("\"AutoEndTasks\"=\"1\"");
 else
  List->Add("\"AutoEndTasks\"=\"0\"");
  List->Add("");

// disable autorun
  List->Add("[HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\CDRom]");
 if (CheckBox20->Checked)
  List->Add("\"AutoRun\"=dword:0");
 else
  List->Add("\"AutoRun\"=dword:1");
  List->Add("");

// disable message about space
  List->Add("[HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explоrer]");
 if (CheckBox21->Checked)
  List->Add("\"NoLowDiskSpaceChecks\"=dword:1");
 else
  List->Add("\"NoLowDiskSpaceChecks\"=dword:0");
  List->Add("");

// defragmentation
  List->Add("[HKEY_LOCAL_MACHINE\\System\\CurrentControlSet\\Control\\FileSystem]");
 if (CheckBox22->Checked)
  List->Add("\"ContigFileAllocSize\"=dword:00000200");
 else
  List->Add("\"ContigFileAllocSize\"=-");
  List->Add("");

// tab <visual>
// autoclean
  List->Add("[HHKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explоrer]");
 if (CheckBox1->Checked)
  List->Add("\"ClearRecentDocsOnExit\"=dword:1");
 else
  List->Add("\"ClearRecentDocsOnExit\"=dword:0");
  List->Add("");

// passwords
  List->Add("[HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System]");
 if (CheckBox5->Checked)
  List->Add("\"NoSecCPL\"=dword:1");
 else
  List->Add("\"NoSecCPL\"=dword:0");
  List->Add("");

// change password
  List->Add("[HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System]");
 if (CheckBox6->Checked)
  List->Add("\"NoPwdPage\"=dword:1");
 else
  List->Add("\"NoPwdPage\"=dword:0");
  List->Add("");

// profiles
  List->Add("[HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System]");
 if (CheckBox7->Checked)
  List->Add("\"NoProfilePage\"=dword:1");
 else
  List->Add("\"NoProfilePage\"=dword:0");
  List->Add("");

// remote control
  List->Add("[HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System]");
 if (CheckBox8->Checked)
  List->Add("\"NoAdminPage\"=dword:1");
 else
  List->Add("\"NoAdminPage\"=dword:0");
  List->Add("");

// monitor
  List->Add("[HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System]");
 if (CheckBox9->Checked)
  List->Add("\"NoDispCPL\"=dword:1");
 else
  List->Add("\"NoDispCPL\"=dword:0");
  List->Add("");

// monitor background
  List->Add("[HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System]");
 if (CheckBox10->Checked)
  List->Add("\"NoDispBackgroundPage\"=dword:1");
 else
  List->Add("\"NoDispBackgroundPage\"=dword:0");
  List->Add("");

// monitor screensaver
  List->Add("[HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System]");
 if (CheckBox11->Checked)
  List->Add("\"NoDispScrSavPage\"=dword:1");
 else
  List->Add("\"NoDispScrSavPage\"=dword:0");
  List->Add("");

// monitor option
  List->Add("[HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System]");
 if (CheckBox12->Checked)
  List->Add("\"NoDispSettingsPage\"=dword:1");
 else
  List->Add("\"NoDispSettingsPage\"=dword:0");
  List->Add("");

// my computer
  List->Add("[HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\NonEnum]");
 if (CheckBox14->Checked)
  List->Add("\"{20D04FE0-3AEA-1069-A2D8-08002B30309D}\"=dword:1");
 else
  List->Add("\"{20D04FE0-3AEA-1069-A2D8-08002B30309D}\"=dword:0");
  List->Add("");

// net connection
  List->Add("[HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer]");
 if (CheckBox15->Checked)
  List->Add("\"NoNetworkConnections\"=dword:1");
 else
  List->Add("\"NoNetworkConnections\"=dword:0");
  List->Add("");

// network
  List->Add("[HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer]");
 if (CheckBox16->Checked)
  List->Add("\"NoStartMenuNetworkPlaces\"=dword:1");
 else
  List->Add("\"NoStartMenuNetworkPlaces\"=dword:0");
  List->Add("");

// printers and fax
  List->Add("[HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced]");
 if (CheckBox17->Checked)
  List->Add("\"Start_ShowPrinters\"=dword:1");
 else
  List->Add("\"Start_ShowPrinters\"=dword:0");
  List->Add("");

// all aplettes
  List->Add("[HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer]");
 if (CheckBox13->Checked)
  List->Add("\"NoControlPanel \"=dword:1");
 else
  List->Add("\"NoControlPanel \"=dword:0");
  List->Add("");

// disable pictogram hint
  List->Add("[HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced]");
 if (CheckBox3->Checked)
  List->Add("\"ShowInfoTip\"=dword:0");
 else
  List->Add("\"ShowInfoTip\"=dword:1");
  List->Add("");

// disable desktop clean-master
  List->Add("[HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer]");
 if (CheckBox4->Checked)
  List->Add("\"NoDesktopCleanupWizard\"=dword:0");
 else
  List->Add("\"NoDesktopCleanupWizard\"=dword:1");
  List->Add("");

// keyboard preset
  List->Add("[-HKEY_USERS\\.DEFAULT\\Keyboard Layout\\Preload]");
  List->Add("");

  List->Add("[HKEY_USERS\\.DEFAULT\\Keyboard Layout\\Preload]");
  List->Add("@=\"\"");
for(int i=0; i<RzCheckListLangStart->Items->Count; i++)
 {
  List->Add("\""+IntToStr(i+1)+"\"=\""+RzCheckListLangStart->Items->Strings[i]+"\"");
 }
  List->Add("");


  List->SaveToFile(filename); // запису у файл
} // end if save dialog execute


}

//---------------------------------------------------------------------------
void __fastcall TForm1::RzShB6ExitClick(TObject *Sender)
{
   Form1->IBQuery2->Close();
   Form1->IBQuery2->SQL->Clear();
   Form1->IBQuery2->SQL->Text = "";
   Form1->IBQuery2->SQL->Add("delete");
   Form1->IBQuery2->SQL->Add("from ID");
   Form1->IBQuery2->SQL->Add("where ADRESS='"+Form1->Label1->Caption+"'");
   Form1->IBQuery2->ExecSQL();
   Form1->IBTransaction1->CommitRetaining();

   Form1->Close(); // закрити програму
   Form1->IBDatabase1->Connected=false;
   delete ini;

}
//---------------------------------------------------------------------------

void __fastcall TForm1::FormActivate(TObject *Sender)
{

    // пошук та відображення  IP адреси
    WORD wVersionRequested;
  WSADATA wsaData;
   wVersionRequested = MAKEWORD(1, 0);
   int err = WSAStartup(wVersionRequested, &wsaData);
    if(err == 0)
  {        char hn[1024];
        struct hostent *adr;
        if(gethostname((char *)&hn, 1024))
 {            int err = WSAGetLastError();
            Beep();
        };
        adr = gethostbyname(hn);
       if(adr)        {          char *LocalIp = new char[15];
          sprintf(LocalIp,"%d.%d.%d.%d",
        (unsigned char)adr->h_addr_list[0][0],
       (unsigned char)adr->h_addr_list[0][1],
      (unsigned char)adr->h_addr_list[0][2],
      (unsigned char)adr->h_addr_list[0][3]);
         Label1->Caption=LocalIp; //Виводимо IP адресу
      }
   }

   
   //зчитуємо інформацію з ini-файлу
   AnsiString server, path_bd, name;
   server = ini->ReadString("System", "server", "ERROR_SERVER");
  path_bd = ini->ReadString("System", "path", "ERROR_PATH_BD");
  if ((server!="ERROR_SERVER") && (path_bd!="ERROR_PATH_BD"))
  {
        name = server+":"+path_bd;
        Form1->IBDatabase1->DatabaseName = name;
        Form1->IBDatabase1->Connected=true;
   }
   else
   {
   ShowMessage ("В файлі "+ini->FileName+" не вказан шлях до БД. Робота не можлива!");
   Form1->Close();
   }
   //відображення форми для вибору кліент чи сервер меню
   if (server == Form1->Label1->Caption)
   {
    Form4->ShowModal();
    }
   else
   {
    Form4->Visible = false;
    Form1->TabSheet1_7->TabVisible=false;
    Form1->Select1->Enabled = false;
   }

   //записуємо в таблицю значення IP адреси
   Form1->IBTable1->Close();
   Form1->IBTable1->Open();
   Form1->IBTable1->Insert();
   Form1->IBQuery1->Close();
   Form1->IBQuery1->Open();
   Form1->IBTable1ADRESS->AsString=Form1->Label1->Caption;
   Form1->IBTable1PP->AsInteger=Form1->IBQuery1MAXID->AsInteger +1;
   Form1->IBTable1->Post();
   Form1->IBTransaction1->CommitRetaining();
   Form1->IBTable1->Close();
   Form1->IBTable1->Open();

  // для отримання сумарної інформації ініціюються всі окремі процедури

  ButtonFoldersClick(Sender);
  ButtonProcessorClick(Sender);
  ButtonDiskClick(Sender);
  Form5->Inf->Active = false;
  Form5->Inf->Active = true;
  Form5->Inf->Insert();
  Form5->IBQuery2->Close();
  Form5->IBQuery2->Open();
  Form5->InfADR->AsString = Form1->Label1->Caption;
  Form5->InfDD->AsInteger = Form5->IBQuery2MAXID->AsInteger +1;
  Form5->InfFREQ->AsString = Edit2mhz->Text + " MHz";
  Form5->InfHDD->AsString = "Logical Disks Letters: " +
                     StringGridHDD->Cells[2][StringGridHDD->RowCount-1]
                     + "Total Size: " +
                     StringGridHDD->Cells[3][StringGridHDD->RowCount-1] + " KB; "
                     + "Free Space: " +
                     StringGridHDD->Cells[4][StringGridHDD->RowCount-1] + " KB; "
                     + "Used Space: " +
                     StringGridHDD->Cells[5][StringGridHDD->RowCount-1] + " KB; ";
 // for (int i=1;i<StringGridHDD->RowCount-1;i++)
// {
 /* Form5->InfDISK->AsString=("Letter: " + StringGridHDD->Cells[1][i] + "   " +
                    "Type: " + StringGridHDD->Cells[2][i] + "    " +
                    "Total Size: " + StringGridHDD->Cells[3][i] + " KB = " +
                    "Free Space: " + StringGridHDD->Cells[4][i] + " KB + " +
                    "Used Space: " + StringGridHDD->Cells[5][i] + " KB"); */
//  }
  Form5->InfIDEN->AsString = Edit2ident->Text;
  Form5->InfPROC->AsString = Edit2proc->Text;
  Form5->InfVEND->AsString = Edit2vendor->Text;
  int mem_size;
_MEMORYSTATUS mem; // змінна-структура, що зберігає інформаю про память
AnsiString strT, strT1, strT2, strT3, strT4, strT5, strT6;

mem.dwLength = sizeof(MEMORYSTATUS); // встановлення розміру структури
GlobalMemoryStatus(&mem);

ListBoxMemory->Items->Clear(); // очищення списку

mem_size=mem.dwTotalPhys/(1024); // отримання загального обєму памяті в Кб
//strT = "physical memory: " + AnsiString(mem_size)+" Mбайт";
strT = "physical memory: " + AnsiString(FormatFloat("#,",mem_size))+" Kбайт;  ";

mem_size=mem.dwAvailPhys/(1024*1024); // отримання вільного обєму памяті в Мб
strT1 = "free physical memory: " + AnsiString(mem_size)+" Mбайт;  ";

mem_size=mem.dwMemoryLoad;  // відсоток використаної памяті
strT2 = "memory in use: " + AnsiString(mem_size)+" %;  ";

mem_size=mem.dwTotalPageFile/(1024*1024);  // отримання обєму page-файлу в Мб
strT3 = "total paging file: " + AnsiString(mem_size)+" Мбайт;  ";

mem_size=mem.dwAvailPageFile/(1024*1024);  // отримання выльного обєму page-файлу в Мб
strT4 = "free bytes of paging file: " + AnsiString(mem_size)+" Мбайт;  ";

mem_size=mem.dwTotalVirtual/(1024*1024);  // отримання загального обєму адресного простору в Мб
strT5 = "total user size of address space: " + AnsiString(mem_size)+" Мбайт;  ";


mem_size=mem.dwAvailVirtual/(1024*1024);  // отримання вільного обєму адресного простору в Мб
strT6 = "free user size of address space: " + AnsiString(mem_size)+" Мбайт;  ";
Form5->InfMEMOR->AsString = strT+strT1+strT2+strT3+strT4+strT5+strT6;

  Form5->Inf->Post();
  Form1->IBTransaction1->CommitRetaining();
  Form5->Inf->Close();
  Form5->Inf->Open();

  RzTabbedListBox1->Items->Clear(); // очищення списку
  Edit2proc->Text = "";
  Edit2vendor->Text = "";
  Edit2ident->Text = "";
  Edit2mhz->Text = "";
  ListBoxMemory->Items->Clear(); // очищення списку
 // ListBoxFonts->Items->Clear(); // очищення списку

  //очищення таблиці з інформацією про диски
  StringGridHDD->RowCount = 2;
  StringGridHDD->ColCount = 6;
  StringGridHDD->Cells[0][1] = " ";
  StringGridHDD->Cells[1][1] = " ";
  StringGridHDD->Cells[2][1] = " ";
  StringGridHDD->Cells[3][1] = " ";
   StringGridHDD->Cells[4][1] = " ";
   StringGridHDD->Cells[5][1] = " ";



}
//---------------------------------------------------------------------------


void __fastcall TForm1::ClientServer1Click(TObject *Sender)
{
  Form4->ShowModal();
  Form4->Button3->Visible=false;
  Form4->Label1->Visible=false;
  Form4->Edit1->Visible=false;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::FormClose(TObject *Sender, TCloseAction &Action)
{
   Form1->IBQuery2->Close();
   Form1->IBQuery2->SQL->Clear();
   Form1->IBQuery2->SQL->Text = "";
   Form1->IBQuery2->SQL->Add("delete");
   Form1->IBQuery2->SQL->Add("from ID");
   Form1->IBQuery2->SQL->Add("where ADRESS='"+Form1->Label1->Caption+"'");
   Form1->IBQuery2->ExecSQL();
   Form1->IBTransaction1->CommitRetaining();
  // Form1->IBDatabase1->Connected=false;


   Form5->IBQuery3->Close();
   Form5->IBQuery3->SQL->Clear();
   Form5->IBQuery3->SQL->Text = "";
   Form5->IBQuery3->SQL->Add("delete");
   Form5->IBQuery3->SQL->Add("from NEW_TABLE");
   Form5->IBQuery3->SQL->Add("where ADR='"+Form1->Label1->Caption+"'");
   Form5->IBQuery3->ExecSQL();
   Form1->IBTransaction1->CommitRetaining();
   Form1->IBDatabase1->Connected=false;

   delete ini;
}
//---------------------------------------------------------------------------


void __fastcall TForm1::DBGrid1DblClick(TObject *Sender)
{
   Form5->ShowModal();

}
//---------------------------------------------------------------------------


