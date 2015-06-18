//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "Unit5.h"
#include "Unit1.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "RzEdit"
#pragma resource "*.dfm"
TForm5 *Form5;
//---------------------------------------------------------------------------
__fastcall TForm5::TForm5(TComponent* Owner)
        : TForm(Owner)
{
}
//---------------------------------------------------------------------------

void __fastcall TForm5::FormActivate(TObject *Sender)
{
   Form5->sRich->Lines->Clear();

   Form5->IBQuery1->Close();
   Form5->IBQuery1->SQL->Clear();
   Form5->IBQuery1->SQL->Text = "";
   Form5->IBQuery1->SQL->Add("select *");
   Form5->IBQuery1->SQL->Add("from NEW_TABLE");
   Form5->IBQuery1->SQL->Add("where ADR ='"+Form1->IBTable1ADRESS->AsString+"'");
   Form5->IBQuery1->Open();
  // Form5->IBQuery1->First();
   Form1->IBTransaction1->CommitRetaining();
   Form5->Label1->Caption =Form1->IBTable1ADRESS->AsString;
   Form5->Caption = "hardware client information";
      // ԭﱬᶳꮠ Ჲ鮠 﨤ԫ "oᱠᲲ鮠"
   Form5->sRich->Paragraph->Alignment = taLeftJustify;
    Form5->sRich->SelAttributes->Color = clBlack;
    Form5->sRich->SelAttributes->Style = Form5->sRich->SelAttributes->Style << fsBold;
  Form5->sRich->Lines->Add("Processor Name:");
   Form5->sRich->Paragraph->FirstIndent = 10;
  Form5->sRich->Lines->Add(Form5->IBQuery1PROC->AsString);
   Form5->sRich->Paragraph->FirstIndent = 0;
    Form5->sRich->SelAttributes->Style = Form5->sRich->SelAttributes->Style << fsBold;
  Form5->sRich->Lines->Add("CPU Vendor:");
   Form5->sRich->Paragraph->FirstIndent = 10;
  Form5->sRich->Lines->Add(Form5->IBQuery1VEND->AsString);
   Form5->sRich->Paragraph->FirstIndent = 0;
    Form5->sRich->SelAttributes->Style = Form5->sRich->SelAttributes->Style << fsBold;
  Form5->sRich->Lines->Add("CPU Identifier:");
   Form5->sRich->Paragraph->FirstIndent = 10;
  Form5->sRich->Lines->Add(Form5->IBQuery1IDEN->AsString);
   Form5->sRich->Paragraph->FirstIndent = 0;
    Form5->sRich->SelAttributes->Style = Form5->sRich->SelAttributes->Style << fsBold;
  Form5->sRich->Lines->Add("Initial CPU Frequency:");
   Form5->sRich->Paragraph->FirstIndent = 10;
  Form5->sRich->Lines->Add(Form5->IBQuery1FREQ->AsString);
   Form5->sRich->Paragraph->FirstIndent = 0;
    Form5->sRich->SelAttributes->Style = Form5->sRich->SelAttributes->Style << fsBold;
  Form5->sRich->Lines->Add("Memory Information:");
   Form5->sRich->Paragraph->FirstIndent = 10;
  Form5->sRich->Lines->Add(Form5->IBQuery1MEMOR->AsString);
   Form5->sRich->Paragraph->FirstIndent = 0;
  Form5->sRich->Lines->Add("");
  Form5->sRich->SelAttributes->Style = Form5->sRich->SelAttributes->Style << fsBold;
  Form5->sRich->Lines->Add("HDD:");
   Form5->sRich->Paragraph->FirstIndent = 10;
  Form5->sRich->Lines->Add(Form5->IBQuery1HDD->AsString);
  Form5->sRich->Paragraph->FirstIndent = 0;

}
//---------------------------------------------------------------------------
void __fastcall TForm5::FormCreate(TObject *Sender)
{
  TColor ccolor;

 switch (Form1->RzRGControlSkin->ItemIndex)
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
    Form5->Color = ccolor;
}
//---------------------------------------------------------------------------

