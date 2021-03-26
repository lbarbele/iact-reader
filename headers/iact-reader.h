#pragma once 

#include <iomanip>
#include <sstream>

#include <TRandom1.h>

void ShowProgress(int);

class CorsikaBlock
{
  protected:
    static const int nFields = 274;
    float fields[nFields];
    bool  status;
    
  public:
    CorsikaBlock() { status = false; }
    void GetFromIACT(eventio::EventIO::Item *item)
    {
      int firstWord;
      item->GetInt32(firstWord);
      item->GetReal(fields+1,firstWord);
      if (firstWord<nFields-1) for (int i=firstWord+1; i<nFields; i++) fields[i] = 0;
      fields[0] = firstWord;
      status = true;
    }
    bool Status()       { return status; }
    float GetField(int i) { return fields[i]; }
    void Dump()   
    {
      std::cout << 1 << " " << (char*)&fields[1] << std::endl;
      for (int i=2; i<nFields; i++) std::cout << i << " " << fields[i] << std::endl;
    }
};

class CorsikaRunHeader : public CorsikaBlock
{
  public:
    int GetRunNumber() {return fields[2];}
};

class CorsikaEventHeader : public CorsikaBlock
{
  public:
    int   GetEventNumber()   { return fields[2];  }
    int   GetPrimaryID()     { return fields[3];  }
    int   GetPrimaryEnergy() { return fields[4];  }
    float GetZenithAngle()   { return fields[11]; }
    float GetAzimuthAngle()  { return fields[12]; }
    float GetObsLevel(int i) { return fields[48+i]; }
    float GetMinWaveLength() { return fields[96]; }
    float GetMaxWaveLength() { return fields[97]; }
};

class CorsikaRunEnd : public CorsikaBlock
{
  public:
    int GetRunNumber() {return fields[2];}
};

class CorsikaEventEnd : public CorsikaBlock
{
  public:
    int GetEventNumber() {return fields[2];}
};

class TelescopeDefinition
{
  private:
  
    int ntel;
    std::vector<float> x, y, z, r;
    std::vector<int>   id;
  
  public:
  
    void GetFromIACT(eventio::EventIO::Item *item)
    {
      item->GetInt32(ntel);
      item->GetReal(x,ntel);
      item->GetReal(y,ntel);
      item->GetReal(z,ntel);
      item->GetReal(r,ntel);
      for (int i=1; i<=ntel; i++) id.push_back(i);
    }
    
    void SetUserIDs(std::string str)
    {
      std::vector<int> intSeq;
      // Parse string as an integer sequence
      int commapos=0;
      while(commapos!=std::string::npos)
      {
        commapos = str.find_first_of(',');
        std::string part = str.substr(0,commapos);
        str.erase(0,commapos+1);
        int posHyph = part.find_first_of('-');
        if(posHyph==std::string::npos) // single element
          intSeq.push_back(stoi(part));
        else // sequence (hyphen)
        {
          part.replace(posHyph,1," ");
          std::istringstream iss(part);
          int first, last;
          iss >> first >> last;
          for (int i=first; i<=last; i++) intSeq.push_back(i);
        }
      }
      // Sort elements
      std::sort(intSeq.begin(),intSeq.end());
      // Set all IDs to -1
      for (int i=0; i<id.size(); i++) id[i]=-1;
      // Set correct IDs
      int lastID = 1;
      for (int i=0; i<intSeq.size(); i++) id[intSeq[i]-1]=lastID++;
    }
    
    void  DumpPositions()
    {
      std::cout << std::endl;
      std::cout << "Telescope ID    X [m]    Y [m]    Z [m]    R [m]\n";
      std::cout << std::endl;
      for (int i=0; i<ntel; i++)
      {
        if (id[i]<0) continue;
        std::cout << std::setw(12) << id[i]     << " ";
        std::cout << std::setw(8)  << 0.01*x[i] << " ";
        std::cout << std::setw(8)  << 0.01*y[i] << " ";
        std::cout << std::setw(8)  << 0.01*z[i] << " ";
        std::cout << std::setw(8)  << 0.01*r[i] << "\n";
      }
    }
    int   GetN ()      { return ntel;  }
    float GetX (int n) { return x[n];  }
    float GetY (int n) { return y[n];  }
    float GetZ (int n) { return z[n];  }
    float GetR (int n) { return r[n];  }
    int   GetID(int n) { return id[n]; }
};

class TelescopeOffsets
{
  private:
  
    int narray;
    float toff;
    std::vector<float> xoff, yoff;
    
  public:
  
    void GetFromIACT(eventio::EventIO::Item *item)
    {
      item->GetInt32(narray);
      item->GetReal(toff);
      item->GetReal(xoff,narray);
      item->GetReal(yoff,narray);
    }
    
    void DumpOffsets() { for (int i=0; i<narray; i++) std::cout << i << " " << xoff[i] << " " << yoff[i] << std::endl; }
};

namespace global
{
  extern CorsikaRunHeader    corHeader;
  extern CorsikaRunEnd       corEnd;
  extern CorsikaEventHeader  thisEvent;
  extern CorsikaEventEnd     thisEventEnd;
  extern TelescopeDefinition telDef;
  extern TelescopeOffsets    telOffsets;
  
  extern TRandom1 ranlux;

  extern std::string onlyTelescopes;
  extern std::string inputFileName;
  extern std::string outputFileName;
  extern std::string atmTransFile;
  extern long  iniBufSize;
  extern long  maxBufSize;
  extern int   nMaxEvents;
  extern bool  dumpInputs;
  extern bool  dumpTelPos;
  extern bool  saveLongi;
  extern int   binsX;
  extern int   binsY;
  extern float xMin;
  extern float xMax;
  extern float yMin;
  extern float yMax;
};


// From sim_telarray:
//
// #define IO_TYPE_MC_BASE     1200
// #define IO_TYPE_MC_RUNH     (IO_TYPE_MC_BASE+0)
// #define IO_TYPE_MC_TELPOS   (IO_TYPE_MC_BASE+1)
// #define IO_TYPE_MC_EVTH     (IO_TYPE_MC_BASE+2)
// #define IO_TYPE_MC_TELOFF   (IO_TYPE_MC_BASE+3)
// #define IO_TYPE_MC_TELARRAY (IO_TYPE_MC_BASE+4)
// #define IO_TYPE_MC_PHOTONS  (IO_TYPE_MC_BASE+5)
// #define IO_TYPE_MC_LAYOUT   (IO_TYPE_MC_BASE+6)
// #define IO_TYPE_MC_TRIGTIME (IO_TYPE_MC_BASE+7)
// #define IO_TYPE_MC_PE       (IO_TYPE_MC_BASE+8)
// #define IO_TYPE_MC_EVTE     (IO_TYPE_MC_BASE+9)
// #define IO_TYPE_MC_RUNE     (IO_TYPE_MC_BASE+10)
// #define IO_TYPE_MC_LONGI    (IO_TYPE_MC_BASE+11)
// #define IO_TYPE_MC_INPUTCFG (IO_TYPE_MC_BASE+12)
// #define IO_TYPE_MC_TELARRAY_HEAD (IO_TYPE_MC_BASE+13)
// #define IO_TYPE_MC_TELARRAY_END (IO_TYPE_MC_BASE+14)
// #define IO_TYPE_MC_EXTRA_PARAM (IO_TYPE_MC_BASE+15)
