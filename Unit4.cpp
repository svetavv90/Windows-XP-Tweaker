//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "Unit3.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "RzPanel"
#pragma link "RzEdit"
#pragma resource "*.dfm"
TForm3 *Form3;
//---------------------------------------------------------------------------
__fastcall TForm3::TForm3(TComponent* Owner)
        : TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TForm3::FormCreate(TObject *Sender)
{
 AnsiString progPath = ExtractFilePath(Application->ExeName);
 AnsiString pathT;

 pathT = progPath + "docs\\help.rtf";
 if (FileExists(pathT))
  RzRichHelp->Lines->LoadFromFile(pathT);
 else
  ShowMessage("Error loading file =" + pathT + "=. Help info not found!");

}
//---------------------------------------------------------------------------
