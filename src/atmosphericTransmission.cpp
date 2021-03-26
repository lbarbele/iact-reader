#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <iterator>

#include <EventIO.hh>

#include <atmosphericTransmission.h>

#include <iact-reader.h>

/*
 * 
 * The namespace atmTrans is used to store values readed from the
 * atmospheric trasmission input file in th function
 * ReadAtmosphericTransmission(). As it does not appear in any header
 * file, it is only visible within the present translation unit.
 * 
 */
namespace atmTrans
{
  int nh1, nwl; // Number of atmospheric heights and wavelengths in tabulated data
  double h2;    // Ground altitude above sea level

  std::vector<int>    wl;       // Vector with tabulated wavelengths
  std::vector<double> h1;       // Vector with tabulated atmospheric heights
  std::vector<double> logh1cm;  // Vector with tabulated logarithms of atmospheric heights
  
  std::vector<std::vector<double>> trans; // Table of optical depths as a function of wavelength and atmospheric height
};



/*
 * 
 * Function: ReadAtmosphericTransmission
 * 
 * Read data from an atmospheric transmission data file and store
 * into the vectors of the atmTrans namespace, later to be used to
 * calculate the survival probability of individual photons.
 * 
 * @param  filename  Name of file containing atmospheric transmission data
 * @return "true" in case of success, otherwise return "false".
 * 
 */
bool ReadAtmosphericTransmission(std::string filename)
{
  std::string line;
  std::istringstream iss;
  std::ifstream file;
  
  // Open atmospheric transmission file and check for errors
  file.open(filename);
  if(!file.is_open())
  {
    std::cerr << "Unable to open atmospheric transmission file " << filename << ". Quit.\n";
    return false;
  }
  
  // Seek for the first line of data in the input file
  while(getline(file,line)) if (line.substr(0,5)=="# H2=") break;
  if (file.eof())
  {
    std::cerr << "Invalid atmospheric transmission file " << filename << ". Quit.\n";
    return false;
  }
  
  /// Get data from the first line: atmospheric heights
  // Ignore first five characters
  iss.str(line.substr(5));
  // Get h2 value (which is ground altitude)
  iss >> atmTrans::h2;
  // First element of h1 is h2
  atmTrans::h1.push_back(atmTrans::h2);
  // Ignore nex six characters
  iss.ignore(6);
  // Get all h1 elements by using an iterator<double> over the istringstream
  atmTrans::h1.insert(atmTrans::h1.end(),std::istream_iterator<double>(iss),std::istream_iterator<double>());
  // Resize logh1cm to the same size of h1
  atmTrans::logh1cm.resize(atmTrans::h1.size());
  // Copy all elements of h1 transformed as log10(100.*h1[i]) using a lambda expression
  std::transform(atmTrans::h1.begin(),atmTrans::h1.end(),atmTrans::logh1cm.begin(),[](double arg)->double{return log10(arg*1.e5);});

  /// Count number of atmospheric heights and check for errors
  atmTrans::nh1 = atmTrans::h1.size();
  if (atmTrans::nh1==0 || atmTrans::nh1 != atmTrans::logh1cm.size())
  {
    std::cerr << "Unable to read first line of atmospheric transmission data from " << filename << ". Quit.\n";
    return false;
  }
  
  /// Now read the file until eof() to get wavelenghts and optical depth values
  while(getline(file,line))
  {
    iss.clear();
    iss.str(line);
    // An interator over iss<double> and an eof iterator
    std::istream_iterator<double> it(iss), eof;
    // Get atmospheric transmission and increment the iterator
    atmTrans::wl.push_back((int)*it);
    it++;
    // Use the iterators to explicitly construct a vector an push it back to trans vector
    atmTrans::trans.push_back(std::vector<double>(it,eof));
  }

  /// Count number of wavelenghts and check for errors
  atmTrans::nwl = atmTrans::wl.size();
  for (int i=0; i<atmTrans::nwl; i++)
  {
    if (atmTrans::trans[i].size() != atmTrans::nh1-1 || atmTrans::trans.size() != atmTrans::nwl)
    {
      std::cerr << "Error reading atmospheric transmission data from " << filename << ". Quit.\n";
      return false;
    }
  }
  
  file.close();
  return true;
}



/*
 * 
 * Function: AtmosphericTransmission
 * 
 * Calculates the survival probability (0 < p < 1) of a photon with
 * a given wavelength, emitted at a given atmospheric height with a
 * specific direction.
 * 
 * @param  waveLength Photon's wavelength
 * @param  zEmission  Atmospheric height of photon's emission
 * @param  relAirmass Relative air mass for calculating the optical depth (actually, 1/cos(theta))
 * @return Survival probability
 * 
 */
double AtmosphericTransmission(double waveLength, double zEmission, double relAirmass)
{
  /// If photon were emitted below the observation level, it is not transmitted
  if (zEmission < atmTrans::h2*1.e5) return 0;
  
  int iwl = lrint(waveLength) - atmTrans::wl[0];
  
  if (iwl < 0    ) iwl = 0;
  if (iwl > atmTrans::nwl-1) iwl = atmTrans::nwl-1;
  
  int iBelow=-1;
  double logzEm = log10(zEmission);
  
  
  double rFrac;
  double opticalDepth;
  
  if (logzEm<atmTrans::logh1cm[1])
  {
    rFrac = (logzEm-atmTrans::logh1cm[0])/(atmTrans::logh1cm[1]-atmTrans::logh1cm[0]);
    opticalDepth = rFrac*atmTrans::trans[iwl][0];
  }
  else if (logzEm>atmTrans::logh1cm[atmTrans::nh1-1])
  {
    opticalDepth = atmTrans::trans[iwl][atmTrans::nh1-2];
  }
  else
  {
    for (int i=0; i<atmTrans::nh1; i++)
    {
      if(atmTrans::logh1cm[i] > logzEm) break;
      iBelow = i;
    }
    
    rFrac = (logzEm-atmTrans::logh1cm[iBelow])/(atmTrans::logh1cm[iBelow+1]-atmTrans::logh1cm[iBelow]);
    opticalDepth = rFrac*(atmTrans::trans[iwl][iBelow]-atmTrans::trans[iwl][iBelow-1]) + atmTrans::trans[iwl][iBelow-1];
  }
  
  return exp(-1.*opticalDepth*relAirmass);
}
