#include <iostream>

#include <TFile.h>
#include <TNtuple.h>
#include <TTree.h>
#include <TMath.h>

#include <EventIO.hh>

#include <iact-reader.h>

void makeHeader(TFile * rootFile)
{
  // Change directory to file pointed by argument
  rootFile->cd();
  
  // Create TNtuple object as header
  TNtuple headerTuple("Header","Header","runNumber:primaryID:primaryEnergyTeV:primaryTheta:primaryPhi");
  
  float h1 = global::corHeader.GetRunNumber();
  float h2 = global::thisEvent.GetPrimaryID();
  float h3 = global::thisEvent.GetPrimaryEnergy()*0.001;
  float h4 = global::thisEvent.GetZenithAngle()*180/TMath::Pi();
  float h5 = global::thisEvent.GetAzimuthAngle()*180/TMath::Pi();
  
  while(h5<0)   h5+=360;
  while(h5>360) h5-=360;
  
  headerTuple.Fill(h1,h2,h3,h4,h5);
  headerTuple.Write();
  
  // Create a TNtuple with telescope definitions
  TNtuple telTuple("Telescopes","Telescopes","ID:x:y:z:r");
  for (int i=0; i<global::telDef.GetN(); i++)
  {
    float id = global::telDef.GetID(i);
    float x  = 0.01*global::telDef.GetX(i);
    float y  = 0.01*global::telDef.GetY(i);
    float z  = 0.01*global::telDef.GetZ(i);
    float r  = 0.01*global::telDef.GetR(i);
    
    if (id<0) continue;
    telTuple.Fill(id,x,y,z,r);
  }
  telTuple.Write();
  
  return;
}
