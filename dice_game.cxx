#include <iostream>

#include "TROOT.h"
#include "TMath.h"
#include "TH1F.h"
#include "TColor.h"
#include "TLine.h"
#include "TPaveText.h"
#include "TRandom3.h"
#include "TSystem.h"
#include "TCanvas.h"
#include "TApplication.h"
#include "TRootEmbeddedCanvas.h"
#include "TGResourcePool.h"
#include "TGFrame.h"
#include "TGDockableFrame.h"
#include "TGCanvas.h"
#include "TGButton.h"
#include "TGTextEntry.h"
#include "TGNumberEntry.h"
#include "TGComboBox.h"
#include "TVirtualPadEditor.h"

using namespace std;

class InputDialog : public TGMainFrame {

  private:
    TGCompositeFrame     *entryFrame, *buttonsFrame, *histFrame;
    TGNumberEntry        *numberEntry;
    TGTextButton         *fAddB, *fClearB, *fSampleB;
    TGCheckButton        *fixScaleButton, *uncertButton, *expButton;
    TGNumberEntry        *fixScaleUp, *fixScaleDown;
    TGLayoutHints        *entryLayout, *buttonsLayout, *histLayout, *uncertLayout, *rigValueLayout;
    TGComboBox           *rigValueMenu;
    TRootEmbeddedCanvas  *fEmbCanv;
    TH1F                 *fH;
    TLine                *expLine;
    TBox                 *uncertBox;
    int                  cnt;
    Int_t                entry;
    bool                 fixScale;
    bool                 uncert;
    bool                 exp;
    Double_t             fScUp;
    Double_t             fScDn;
    int                  riggedValue;

  public:

    InputDialog(const TGWindow *p, UInt_t w, UInt_t h);
    virtual ~InputDialog(){}

    virtual void CloseWindow();
    virtual Bool_t ProcessMessage(Long_t msg, Long_t parm1, Long_t);

    void AddEntry();
    void ClearEntries();
    void SampleEntries();
    void SetupHist();
    void DrawExpectedLine();
    void UpdateCanvas();
    int  GetValue();
};

void InputDialog::SetupHist(){
  fH = new TH1F("h","Dice throw values",6,0,6);
  for (int b=0; b<fH->GetNbinsX(); b++){
    fH->GetXaxis()->SetBinLabel(b+1,Form("%d",b+1));
  }
  fH->SetLineColor(kBlue-7);
  fH->SetLineWidth(2);
  fH->SetFillColor(kBlue-7);
  fH->SetBarWidth(0.9);
  fH->SetBarOffset(0.05);
  fH->GetXaxis()->SetTitle("Dice value");
  fH->GetYaxis()->SetTitle("Number of times");
  fH->GetYaxis()->SetTitleOffset(1.4);
  fH->SetStats(0);

  expLine = new TLine();
  expLine->SetLineColor(kRed);
  expLine->SetLineWidth(3);
  expLine->SetLineStyle(kDashed);

  uncertBox = new TBox();
  TColor *col = gROOT->GetColor(kRed-7);
  col->SetAlpha(0.4);
  uncertBox->SetLineColor(col->GetNumber());
  uncertBox->SetFillColor(col->GetNumber());

}

InputDialog::InputDialog(const TGWindow *p, UInt_t w, UInt_t h):
  TGMainFrame(p, w, h),
  fH(0),
  cnt(0),
  entry(0),
  uncert(false),
  exp(false),
  fScUp(0.),
  fScDn(0.),
  fixScale(false),
  riggedValue(0)
{
  SetupHist();

  SetCleanup(kDeepCleanup);

  entryFrame  = new TGCompositeFrame(this, 800, 50, kHorizontalFrame);
  numberEntry = new TGNumberEntry(entryFrame, entry, 10, 39, (TGNumberFormat::kNESInteger));
  numberEntry->Associate(this);
  numberEntry->SetState();
  entryLayout = new TGLayoutHints(kLHintsTop | kLHintsLeft, 5, 5, 5, 5);
  entryFrame->AddFrame(numberEntry,entryLayout);

  buttonsFrame = new TGCompositeFrame(this, 800, 50, kHorizontalFrame);
  fAddB    = new TGTextButton(buttonsFrame, "Add &Entry", 40);
  fClearB  = new TGTextButton(buttonsFrame, "&Clear Entries", 41);
  fSampleB = new TGTextButton(buttonsFrame, "Sample Entries", 42);
  fAddB->Associate(this);
  fClearB->Associate(this);
  fSampleB->Associate(this);
  buttonsLayout = new TGLayoutHints(kLHintsTop | kLHintsLeft | kLHintsExpandX, 5, 5, 5, 5);
  buttonsFrame->AddFrame(fAddB, buttonsLayout);
  buttonsFrame->AddFrame(fClearB,  buttonsLayout);
  buttonsFrame->AddFrame(fSampleB,  buttonsLayout);

  uncertLayout = new TGLayoutHints(kLHintsTop | kLHintsLeft, 5, 5, 5, 5);
  expButton = new TGCheckButton(buttonsFrame, "&Expected Line", 51);
  expButton->Associate(this);
  uncertButton = new TGCheckButton(buttonsFrame, "&Uncertainty Band", 50);
  uncertButton->Associate(this);
  buttonsFrame->AddFrame(expButton,uncertLayout);
  buttonsFrame->AddFrame(uncertButton,uncertLayout);

  rigValueLayout = new TGLayoutHints(kLHintsTop | kLHintsLeft | kLHintsExpandX | kLHintsExpandY, 5, 5, 5, 5);
  rigValueMenu = new TGComboBox(buttonsFrame,52);
  rigValueMenu->Associate(this);
  rigValueMenu->AddEntry("Fair",0);
  rigValueMenu->AddEntry("Rig 1",1);
  rigValueMenu->AddEntry("Rig 2",2);
  rigValueMenu->AddEntry("Rig 3",3);
  rigValueMenu->AddEntry("Rig 4",4);
  rigValueMenu->AddEntry("Rig 5",5);
  rigValueMenu->AddEntry("Rig 6",6);
  buttonsFrame->AddFrame(rigValueMenu, rigValueLayout);

  histFrame = new TGCompositeFrame(this, 800, 700, kHorizontalFrame);
  histLayout = new TGLayoutHints(kLHintsTop | kLHintsLeft | kLHintsExpandX | kLHintsExpandY, 5, 5, 5, 5);

  fEmbCanv = new TRootEmbeddedCanvas("ec",histFrame,800,700);
  histFrame->AddFrame(fEmbCanv,histLayout);

  AddFrame(entryFrame,entryLayout);
  AddFrame(buttonsFrame,buttonsLayout);
  AddFrame(histFrame,histLayout);

  fEmbCanv->GetCanvas()->SetBorderMode(0);

  SetWindowName("Dice Game Gui");
  MapSubwindows();
  Resize();
  MapWindow();
  UpdateCanvas();
}

void InputDialog::UpdateCanvas(){

  TCanvas *canv = fEmbCanv->GetCanvas();
  if (fixScale && fScUp>fScDn) {
    fH->GetYaxis()->SetRangeUser(fScDn,fScUp);
  }
  fH->SetTitle(Form("Dice Values (after %d throws)",int(fH->GetEntries())));
  fH->Draw("HIST");
  DrawExpectedLine();
  canv->Modified();
  canv->Update();
}

void InputDialog::AddEntry(){
  if (entry>0 && entry<7) {
    fH->Fill(entry-1);
  }
  UpdateCanvas();
}

void InputDialog::ClearEntries(){
  fH->Reset();
  UpdateCanvas();
}

int InputDialog::GetValue(){
  int val = gRandom->Integer(6);
  if (riggedValue>=1 && riggedValue<=6) {
    int riggingProb = gRandom->Integer(6);
    if (riggingProb==1) {
      return riggedValue-1;
    }
    else {
      return val;
    }
  }
  else {
    return val;
  }
}

void InputDialog::SampleEntries(){

  for (int i=0; i<entry; i++){
    int val = GetValue();
    fH->Fill(val);
    if (i%(int(TMath::Ceil((entry/10.))))==0) {
      UpdateCanvas();
    }
  }
  UpdateCanvas();
}

void InputDialog::DrawExpectedLine(){
  TCanvas *canv = fEmbCanv->GetCanvas();
  double expVal = fH->GetEntries()/6.;
  if (exp) {
    expLine->DrawLine(0,expVal,6,expVal);
  }

  if (uncert){
    double up = expVal+TMath::Sqrt(expVal);
    double dn = expVal-TMath::Sqrt(expVal);
    uncertBox->DrawBox(0,dn,6,up);
  }
  canv->Update();
  canv->Modified();
}

void InputDialog::CloseWindow(){

   if (TVirtualPadEditor::GetPadEditor(kFALSE) != 0)
      TVirtualPadEditor::Terminate();
   DeleteWindow();
}

Bool_t InputDialog::ProcessMessage(Long_t msg, Long_t parm1, Long_t parm2){

  if (parm1==39) {
    entry = numberEntry->GetIntNumber();
    if (GET_MSG(msg)==kC_TEXTENTRY && GET_SUBMSG(msg)==kTE_ENTER) {
      if (entry>0 && entry<7) {
        AddEntry();
      }
      else {
        SampleEntries();
      }
    }
  }
  if (parm1==40) {
    AddEntry();
  }
  if (parm1==41) {
    ClearEntries();
  }
  if (parm1==42) {
    SampleEntries();
  }
  if (parm1==43) {
    fixScale =  !fixScale;
    UpdateCanvas();
  }
  if (parm1==44) {
    fScUp = fixScaleUp->GetNumber();
  }
  if (parm1==45) {
    fScDn = fixScaleDown->GetNumber();
  }
  if (parm1==50) {
    uncert = !uncert;
    UpdateCanvas();
  }
  if (parm1==51) {
    exp = !exp;
    UpdateCanvas();
  }
  if (parm1==52) {
    riggedValue = parm2;
  }
  return kTRUE;
}

int main(int argc, char **argv){

  TApplication theApp("App", &argc, argv);

  InputDialog input(gClient->GetRoot(), 800, 800);

  theApp.Run();

  return 0;

}

