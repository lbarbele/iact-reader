#include <iostream>
#include <string>

#include <EventIO.hh>

#include <TObject.h>
#include <TFile.h>
#include <TTree.h>
#include <TGraph.h>

#include <iact-reader.h>

/*
 * 
 * Function: GetProfiles
 * 
 * Receives an EventIO::Item object of type 1211 and read the profiles
 * within this data block.
 * 
 * @param  item  Pointer to Item object of type 1211
 * @return (none)
 * 
 */
void GetProfiles(eventio::EventIO::Item *item, TFile* rootFile)
{
  // Just in case
  if (rootFile == nullptr) return;
  
  // For the profile names
  int   evtNumber = global::thisEvent.GetEventNumber();
  int   runNumber = global::corHeader.GetRunNumber();
  std::string particleType[9] = {"gamma","e+","e-","mu+","mu-","hadrons","charged","nuclei","cherenkov"};
  
  
  int   event, type;
  short np, nthick;
  float thickstep;
  
  item->GetInt32(event);
  item->GetInt32(type);
  item->GetInt16(np);
  item->GetInt16(nthick);
  item->GetReal(thickstep);
  
  // Profiles are ordered as:
  // gamma, e+, e-, mu+, mu-, hadrons, charged, nuclei, cerenkov
  int nPoints = (int)nthick;
  
  // Atmospheric depths
  float depth[nPoints];
  for (int i=0; i<nPoints; i++) depth[i]=(i+1)*thickstep;
  
  // Loop over profiles:
  // read profile from input buffer; create a tgraph; store into .root.
  float * profile[np];
  for (int i=0; i<np; i++)
  {
    profile[i] = new float[nthick];
    item->GetReal(profile[i],nthick);
    
    // Create a graph with the profile and save it into the root file
    std::string profName = "run" + std::to_string(runNumber) + "_event" + std::to_string(evtNumber) + "_" + particleType[i];
    TGraph g(nPoints,depth,profile[i]);
    g.SetName(profName.c_str());
    g.SetTitle(profName.c_str());
    rootFile->cd("Profiles");
    g.Write();
  }

  // Or save into a tree instead... (more difficult to read)
  //~ // Create the tree and its branches
  //~ TTree * tree = (TTree*)rootFile->Get("profiles");
  //~ if (!tree) tree = new TTree("profiles","profiles");
  //~ tree->Branch("nPoints"  ,&nPoints,"nPoints/I");
  //~ tree->Branch("gamma"    ,profile[0],"gamma[nPoints]/F");
  //~ tree->Branch("ePlus"    ,profile[1],"ePlus[nPoints]/F");
  //~ tree->Branch("eMinus"   ,profile[2],"eMinus[nPoints]/F");
  //~ tree->Branch("muPlus"   ,profile[3],"muPlus[nPoints]/F");
  //~ tree->Branch("muMinus"  ,profile[4],"muMinus[nPoints]/F");
  //~ tree->Branch("hadrons"  ,profile[5],"hadrons[nPoints]/F");
  //~ tree->Branch("charged"  ,profile[6],"charged[nPoints]/F");
  //~ tree->Branch("nuclei"   ,profile[7],"nuclei[nPoints]/F");
  //~ tree->Branch("cherenkov",profile[8],"cherenkov[nPoints]/F");
  
  //~ tree->Fill();
  //~ rootFile->cd();
  //~ tree->Write(0,TObject::kSingleKey+TObject::kWriteDelete);
  
  return;
}
