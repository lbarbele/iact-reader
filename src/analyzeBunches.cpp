#include <iostream>

#include <EventIO.hh>

/// Include ROOT headers as needed

#include <TRandom1.h>
#include <TFile.h>
#include <TH2.h>

#include <atmosphericTransmission.h>
#include <iact-reader.h>
#include <TVector3.h>



/*
 * 
 * Function: AnalyzePhotonBunches
 * 
 * Receives an IACT data block of type 1205 with photon bunches from
 * CORSIKA simulation and loops over data. Each call to this function
 * corresponds to a block of data from one single telescope in one
 * event.
 * 
 * @param  item Eventio::Item object of type 1205
 * @return (none)
 * 
 */
void AnalyzePhotonBunches(eventio::EventIO::Item * item, TFile *rootFile)
{
  using namespace std;
  
  int16_t arrayNumber;
  int16_t telNumber;
  int32_t nBunches;
  float photonSum;
  int16_t data[8];
  
  int   evtNumber = global::thisEvent.GetEventNumber();
  int   runNumber = global::corHeader.GetRunNumber();
  float wlMin = global::thisEvent.GetMinWaveLength();
  float wlMax = global::thisEvent.GetMaxWaveLength();
  
  item->GetInt16(arrayNumber);
  item->GetInt16(telNumber);
  item->GetReal(photonSum);
  item->GetInt32(nBunches);
  
  /// Skip telescopes with ID -1
  if (global::telDef.GetID((int)telNumber)<0) return;
  

  /// General variables
  // Observation level altitude
  float obsLev     = global::thisEvent.GetObsLevel(0);
  // Primary particle properties
  float thetaPrim  = global::thisEvent.GetZenithAngle();
  float phiPrim    = global::thisEvent.GetAzimuthAngle();
  // Telescope position (in cm)
  float telX       = global::telDef.GetX(telNumber);
  float telY       = global::telDef.GetY(telNumber);
  float telZ       = global::telDef.GetZ(telNumber);

  /// ------------------------------------------------------------------
  /// Declare histograms here and fill them inside photon loop (rather than in bunch loop)
  string histoNameAll = "run" + to_string(runNumber) + "_event" + to_string(evtNumber) + "_tel" + to_string(global::telDef.GetID(telNumber)) + "_all";
  //~ string histoNameDet = "run" + to_string(runNumber) + "_event" + to_string(evtNumber) + "_tel" + to_string(global::telDef.GetID(telNumber)) + "_detected";
  TH2F  histoAll(histoNameAll.c_str(),"",global::binsX,global::xMin,global::xMax,global::binsY,global::yMin,global::yMax);
  //~ TH2F  histoDet(histoNameDet.c_str(),"",global::binsX,global::xMin,global::xMax,global::binsY,global::yMin,global::yMax);
  /// ------------------------------------------------------------------
  
  // Before entering the per-bunch loop, determine the shower face plane
  float vertX, vertY, vertZ;
  float horiX, horiY, horiZ;
  float normX, normY, normZ;
  float aux;
  
  vertX = -sin(thetaPrim)*cos(phiPrim);
  vertY = -sin(thetaPrim)*sin(phiPrim);
  vertZ =  cos(thetaPrim);
  
  horiX = -telY*vertZ+telZ*vertY;
  horiY = -telZ*vertX+telX*vertZ;
  horiZ = -telX*vertY+telY*vertX;
  
  aux = TMath::Sqrt(horiX*horiX+horiY*horiY+horiZ*horiZ);
  
  horiX /= aux;
  horiY /= aux;
  horiZ /= aux;
  
  normX = -vertY*horiZ+vertZ*horiY;
  normY = -vertZ*horiX+vertX*horiZ;
  normZ = -vertX*horiY+vertY*horiX;
  
  // Loop over bunches
  for (int i=0; i<nBunches; i++)
  {
    item->GetInt16(data,8);
    
    /// Bunch specific variables
    // Number of photons in bunch
    float nPhotons = 0.01*data[6];
    // Arrival time (in ns)
    float time       = 0.1*data[4];
    // Emission altitude (in cm)
    float zEmission  = pow(10,0.001*data[5]);
    // sin(theta)cos(phi) and sin(theta)sin(phi)
    float cx         = (1./3.e4)*data[2];
    float cy         = (1./3.e4)*data[3];
    // Check sines and cosines, just in case...
    if      (cx>1.) cx=1.; else if (cx<-1.) cx=-1.;
    if      (cy>1.) cx=1.; else if (cy<-1.) cx=-1.;
    // cos(theta) (downards!)
    float cz         = -sqrt(1.-cx*cx-cy*cy);
    // Bunch arrival position in the CORSIKA frame (in cm)
    float photX      = 0.1*data[0] + telX;
    float photY      = 0.1*data[1] + telY;
    float photZ      = telZ;
    
    // Check if photon bunch direction lies within telescope f.o.v.
    // Assuming f.o.v. diameter is 10 deg.
    // cos(5º) = 0.99619469809
    float thetaToAxis = vertX*cx + vertY*cy +vertZ*cz;
    if (fabs(thetaToAxis) < 0.99619469809) continue; // Skip this bunch
    

    /// --------------------------------------------------------------
    /// Analyze photons here
    /// ~~~~~~~ ~~~~~~~ ~~~~
    ///
    ///   This is a loop over each photon, therefore the relative
    /// weight of each photon is 1. Here, we determine the survival
    /// probability of each incoming photon as well as their position
    /// relative to the CORSIKA frame.
    ///
    /// Available variables are:
    ///
    /// obsLev      Observation level as defined in CORSIKA inputs
    /// thetaPrim   Primary particle zenithal angle
    /// phiPrim     Primaty particle azimuthal angle
    /// telX        Telescope position X in CORSIKA frame
    /// telY        Telescope position Y in CORSIKA frame
    /// telZ        Telescope position Z in CORSIKA frame
    /// nPhotons    Number of photons in current bunch
    /// time        Arrival time (see sim_telarray user guide)
    /// zEmission   Height of bunch emission in CORSIKA frame
    /// cx          sin(theta)cos(phi) of bunch direction in CORSIKA frame
    /// cy          sin(theta)sin(phi) of bunch direction in CORSIKA frame
    /// cz          -sqrt(1.-cx*cx-cy*cy) of bunch direction in CORSIKA frame
    /// photX       Bunch arrival position X in CORSIKA XY plane
    /// photY       Bunch arrival position Y in CORSIKA XY plane
    /// photZ       Bunch arrival position Z in CORSIKA XY plane (always 0)
    /// waveLength  Wavelenght of current photon
    /// survProb    Survival probability due to atmospheric absortion of current photon
    /// 

    //definition of depth parameters
    float a0,b0,c0,a1,b1,c1,a2,b2,c2,a3,b3,c3,a4, b4, c4;
    a0 =     -138.717; b0 = 1165.33; c0 =      994186;
    a1 =     -28.0547; b1 = 1204.64; c1 =      746232;
    a2 =     0.466743; b2 = 1345.62; c2 =      636143;
    a3 = -0.000530414; b3 = 557.063; c3 =      772170;
    a4 =   0.00157474; b4 =       1; c4 = 7.43224e+09;

    float lateral, depth, slant;
    float intX, intY, intZ;
    
    float parD = -(normX*photX+normY*photY+normZ*photZ)/(normX*cx+normY*cy+normZ*cz);

    // Intersecion point in corsika frame
    intX = parD*cx+photX;
    intY = parD*cy+photY;
    intZ = parD*cz+photZ;
    
    if (intZ<0) continue;
    
    // Lateral distance of intersection point within shower plane
    lateral = intX*horiX + intY*horiY + intZ*horiZ;
    
    // Calculate vertical depth of intersection point
    if      (intZ <=   900000.) depth = a0 + b0*TMath::Exp(-(intZ)/c0);
    else if (intZ <=  1800000.) depth = a1 + b1*TMath::Exp(-(intZ)/c1);
    else if (intZ <=  4600000.) depth = a2 + b2*TMath::Exp(-(intZ)/c2);
    else if (intZ <= 10500000.) depth = a3 + b3*TMath::Exp(-(intZ)/c3);
    else if (intZ <= 11704000.) depth = a4 - ((b4*(intZ))/c4);

    // Calculate slant depth
    slant = depth/TMath::Cos(thetaPrim);
    
    // Histograms with every photon arriving observation level
    histoAll.Fill(lateral/100.,slant, nPhotons);
      
    // Loop over each photon
    //~ while(nPhotons>0)
    //~ {
      
      //~ /// Photon specific variables
      //~ // Wavelength
      //~ float waveLength;
      //~ if      (data[7] == 0)   waveLength = 1 / ( (1/wlMin) - global::ranlux.Uniform()*((1/wlMin) - (1/wlMax)) );
      //~ else if (data[7] > 9900) break;
      //~ else if (data[7] < 0)    break;
      //~ else                     waveLength = data[7];
      //~ // Survival probability according to atmospheric transmission
      //~ float survProb   = AtmosphericTransmission(waveLength, zEmission, -1./cz);

      //~ // Histograms only with photons that survived atmospheric transmission
      //~ if (survProb>global::ranlux.Uniform()) histoDet.Fill(lateral/100,slant);
      
      //~ nPhotons--;
    //~ } // Loop over photons in one bunch
  } // Loop over bunches in the block

  /// Write the histograms to the root file
  rootFile->cd("allPhotons");
  histoAll.Write();
  
  //~ rootFile->cd("detectedPhotons");
  //~ histoDet.Write();

}
