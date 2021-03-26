#include <iostream>
#include <cstdio>
#include <sstream>

#include <EventIO.hh>

#include <TFile.h>
#include <TNtuple.h>
#include <TMath.h>

#include <iact-reader.h>
#include <getInputs.h>
#include <getProfiles.h>
#include <atmosphericTransmission.h>
#include <analyzeBunches.h>
#include <getOptions.h>
#include <makeHeader.h>


/*
 * 
 * Global namespace: instantiates classes that may be used inside
 * other analysis functions. It appears in iact2root.h where all objects
 * are referenced as extern.
 * 
 */
namespace global
{
  // To store specific blocks from the iobufer
  CorsikaRunHeader    corHeader;
  CorsikaRunEnd       corEnd;
  CorsikaEventHeader  thisEvent;
  CorsikaEventEnd     thisEventEnd;
  TelescopeDefinition telDef;
  TelescopeOffsets    telOffsets;
  
  // A random number generator from ROOT
  TRandom1 ranlux(time(NULL));
  
  // Options from comand line
  std::string onlyTelescopes = "";
  std::string inputFileName = "";
  std::string outputFileName = "output.root";
  std::string atmTransFile = "atmtrans/atm_trans_2150_1_10_0_0_2150.dat";
  long  iniBufSize = 100000000;  // 100 MB
  long  maxBufSize = 1000000000; // 1 GB
  int   nMaxEvents = -1;
  bool  dumpTelPos = false;
  bool  dumpInputs = false;
  bool  saveLongi  = false;
  int   binsX      = 100;
  int   binsY      = 100;
  float xMin       = -500.;
  float xMax       = +500.;
  float yMin       = 0.;
  float yMax       = 1100.;
};



/*
 * 
 * Function: main()
 * 
 * Main function of the program. It is only an interface to iterate over
 * the input file (from CORSIKA IACT) and call the correspondent analy-
 * sis functions.
 * 
 */
int main (int argc, char** argv)
{
  using std::cerr;
  using std::cout;
  using std::endl;

  // Get options from command line
  if (!GetOptions(argc, argv)) return 1;
  // Read atmospheric transmission data
  //~ if (!ReadAtmosphericTransmission(global::atmTransFile)) return 1;
  
  int iEvt = 0; // Event counter
  std::vector<int> skipTypes = {0,1206,1208}; // Data block types to be skiped
  eventio::EventIO iobuf(global::iniBufSize,global::maxBufSize); // The IO buffer
  
  // Open input buffer
  if (global::inputFileName != "")
    iobuf.OpenInput(global::inputFileName);
  else
    iobuf.OpenInput(stdin);
  
  // Check if it was correctly opened
  if (!iobuf.HaveInput())
  {
    std::cerr << "Error opening input buffer!" << std::endl;
    return 1;
  }
  
  // Create a root file and two subdirectories to save the histograms
  TFile rootFile(global::outputFileName.c_str(),"recreate","",209);
  rootFile.mkdir("allPhotons");
  //~ rootFile.mkdir("detectedPhotons");
  
  // Create a folder to save the longitudinal profiles if needed
  if (global::saveLongi) rootFile.mkdir("Profiles");
  
  // Boolean to get the first event and fill the header
  bool firstEvent = true;
  
  // Boolean to exit the main loop under specified conditions
  bool done = false;
  
  // Boolean to check if input comes from stdin
  bool fromStdIn = global::inputFileName == "" ? true : false;
  
  // Read the input buffer (IACT file) until it is over
  while(iobuf.Find()==0)
  {
    // We are done if we got the required number of events
    if (iEvt>global::nMaxEvents && global::nMaxEvents>0) done = true;
    
    // If we are done, break, but avoid a broken pipe if using stdin
    if (done && !fromStdIn) break;
    if (done &&  fromStdIn) {iobuf.Skip(); continue;}
    
    // Do we want to skip the current block type?
    if (std::find(skipTypes.begin(), skipTypes.end(), iobuf.ItemType())!=skipTypes.end())
    {
      iobuf.Skip();
      continue;
    }
    
    // Read the current data block...
    iobuf.Read();
    // ... and store it in an EventIO::Item object
    eventio::EventIO::Item curItem(iobuf,"get");
    
    ShowProgress(curItem.Type());
    
    // Treat this block accordingly to its type
    switch(curItem.Type())
    {
      case 1200: /// CORSIKA run header
        global::corHeader.GetFromIACT(&curItem);
        break;
      case 1201: /// Position and sizes of telescopes within telescope array
        global::telDef.GetFromIACT(&curItem);
        if (global::onlyTelescopes!="")
          global::telDef.SetUserIDs(global::onlyTelescopes);
        if (global::dumpTelPos)
        {
          global::telDef.DumpPositions();
          done = true;
        }
        break;
      case 1202: /// CORSIKA event header
        global::thisEvent.GetFromIACT(&curItem);
        if(firstEvent) makeHeader(&rootFile);
        firstEvent = false;
        iEvt++;
        break;
      case 1203: /// Offsets of multiple telescope arrays for the present event
        global::telOffsets.GetFromIACT(&curItem);
        break;
      case 1204: /// Top level item for data from one array in one event
      {
        while(curItem.NextSubItemType()==1205)
        {
          eventio::EventIO::Item subItem(curItem,"get");
          AnalyzePhotonBunches(&subItem,&rootFile);
        }
        break;
      }
      case 1205:
      {
        AnalyzePhotonBunches(&curItem,&rootFile);
      }
      case 1206: /// Camera layout in the telescope simulation
        break;
      case 1208: /// Photo-electrons after ray-tracing and detection
        break;
      case 1209: /// CORSIKA event end
        global::thisEventEnd.GetFromIACT(&curItem);
        break;
      case 1210: /// CORSIKA run end
        global::corEnd.GetFromIACT(&curItem);
        break;
      case 1211: /// Longitudinal profiles
        if (global::saveLongi) GetProfiles(&curItem,&rootFile);
        break;
      case 1212: /// CORSIKA inputs
        GetInputs(&curItem,global::dumpInputs);
        break;
      case 1213:
        break;
      case 1214:
        break;
      default: /// Any other type is threated as unknown
        cerr << "Unknown block type " << iobuf.ItemType() << " will be skiped." << endl;
        break;
    } // Loop block over types
  } // Loop over IO buffer
  
  // Close input buffer
  iobuf.CloseInput();

  // Close root ouput file
  rootFile.Close();
  
  return 0;
}


void ShowProgress(int type)
{
  using std::cout;
  using std::cerr;
  using std::endl;
  
  switch (type)
  {
    case 1200: cout << "1200: Reading CORSIKA run header"                   << endl; break;
    case 1201: cout << "1201: Reading position and sizes of telescopes"     << endl; break;
    case 1202: cout << "1202: CORSIKA event header found"                   << endl; break;
    case 1203: cout << "1203:   Reading telescope offsets for this event"   << endl; break;
    case 1204: cout << "1204:   Start reading photon bunches"               << endl; break;
    case 1209: cout << "1209: CORSIKA event end found"                      << endl; break;
    case 1210: cout << "1210: CORSIKA run end found"                        << endl; break;
    case 1211: cout << "1211:   Longitudinal profiles block found"          << endl; break;
    case 1212: cout << "1212: Reading CORSIKA inputs"                       << endl; break;
    case 1213: cout << "1213:   Start reading photon bunches (split data)"  << endl; break;
    case 1214: cout << "1214:   Done reading photon bunches (split data)"   << endl; break;
    default: break;
  }
  
  return;
}
