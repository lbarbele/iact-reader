#include <iostream>
#include <sstream>
#include <string>
#include <iterator>

#include <EventIO.hh>

#include <iact-reader.h>

bool GetOptions(int argc, char** argv)
{
  using namespace std;
  
	for (int i=1; i<argc; i++)
	{
		string opt=argv[i];
		string arg="";
		bool no_arg = true;
		bool missarg = false;
    bool has_space = true;
    
		int istart = 0;
		if (opt[0] == '-') istart++;
		if (opt[1] == '-') istart++;
		
		/// Get option data...
		if (istart>0)
		{
			opt.erase(0,istart);
			
			/// Look for a possible argument for current option
			if (i+1<argc && istart==2)
			{
				arg=argv[i+1];
				if(arg[0]!='-') no_arg=false;
			}
			else if (i+1<argc && istart==1 && opt.size()==1)
			{
				arg=argv[i+1];
				if(arg[0]!='-') no_arg=false;
			}
			else if (istart==1 && opt.size()>1)
			{
				arg=opt.substr(1,opt.size()-1);
				opt.erase(1,opt.size()-1);
				no_arg=false;
        has_space=false;
			}
      
      /// --------------------------------------------------------------
      /// Define the options here
			if      (opt == "h" || opt == "help")
			{
        cout << endl;
        cout << "Program to analyze CORSIKA IACT files in EventIO format and create shower face plane histograms." << endl;
        cout << endl;
        cout << "Command line options are:" << endl;
        cout << endl;
        cout << "\t-i input.iact                \tCORSIKA IACT input file name (leavy empty for stdin) [default: stdin]" << endl;
        cout << "\t-o output.root               \tROOT output file name [default: output.root]" << endl;
        //~ cout << "\t-a atmtrans.dat              \tAtmospheric transmission data file name [default: atmtrans/atm_trans_2150_1_10_0_0_2150.dat]" << endl;
        cout << "\t-m maxevents                 \tMaximum number of events to analyze [default: unlimited]" << endl;
        cout << "\t-b nX:Xmin:Xmax:nY:Ymin:Ymax \t2D histogram binning options (separated by colons) [default: 100:-500:500:100:0:100]" << endl;
        cout << "\t--longi                      \tSave longitudinal profiles to output file" << endl;
        cout << "\t--dump-telescopes            \tShow telescope positions and exit" << endl;
        cout << "\t--dump-inputs                \tShow CORSIKA inputs and exit" << endl;
        cout << "\t--only-telescopes 1,5-10,... \tAnalyze only specific telescopes from IACT file" << endl;
        cout << "\t--bufsize size               \tInitial size of IO buffer (bytes)" << endl;
        cout << "\t--maxbuf  size               \tMaximum size of IO buffer (bytes)" << endl;
        cout << endl;
				return false;
			}
			else if (opt == "i" || opt == "input")
			{
				if (no_arg) missarg = true;
				global::inputFileName = arg;
				if (has_space) i++;
			}
			else if (opt == "o" || opt == "output")
			{
				if (no_arg) missarg = true;
				global::outputFileName = arg;
				if (has_space) i++;
			}
			//~ else if (opt == "a" || opt == "atmtrans")
			//~ {
				//~ if (no_arg) missarg = true;
				//~ global::atmTransFile = arg;
				//~ if (has_space) i++;
			//~ }
			else if (opt == "m" || opt == "maxevents")
			{
				if (no_arg) missarg = true;
				global::nMaxEvents = stoi(arg);
				if (has_space) i++;
			}
      else if (opt == "bufsize")
			{
				if (no_arg) missarg = true;
				global::iniBufSize = stol(arg);
				if (has_space) i++;
			}
      else if (opt == "maxbuf")
			{
				if (no_arg) missarg = true;
				global::maxBufSize = stol(arg);
				if (has_space) i++;
			}
			else if (opt == "longi")
			{
				global::saveLongi = true;
			}
			else if (opt == "dump-telescopes")
			{
				global::dumpTelPos = true;
			}
			else if (opt == "dump-inputs")
			{
				global::dumpInputs = true;
			}
      else if (opt == "only-telescopes")
      {
				if (no_arg) missarg = true;
        global::onlyTelescopes = arg;
        if (has_space) i++;
      }
			else if (opt == "b" || opt == "bins")
			{
				if (no_arg) missarg = true;
        std::istringstream iss(arg);
        float x;
        if(iss >> x) { global::binsX = x; iss.ignore(1); }
        if(iss >> x) { global::xMin  = x; iss.ignore(1); } 
        if(iss >> x) { global::xMax  = x; iss.ignore(1); }
        if(iss >> x) { global::binsY = x; iss.ignore(1); } 
        if(iss >> x) { global::yMin  = x; iss.ignore(1); } 
        if(iss >> x) { global::yMax  = x; }
				if (has_space) i++;
			}
			else { cerr << "Invalid option \"" << opt << "\". Try ./iact2root --help." << endl; return false; }
      ///
      /// --------------------------------------------------------------

			if (missarg) { cerr << "Missing argument for option \"" << opt << "\". Try ./iact2root --help." << endl; return false; }

		}
		else { cerr << "Invalid option \"" << opt << "\". Try ./iact2root --help." << endl; return false; }
	}
  
  if (global::outputFileName == "")
  {
    cerr << "You should declare an output ROOT file name!" << endl;
    return false;
  }
  //~ else if (global::atmTransFile == "")
  //~ {
    //~ cerr << "You should declare an atmospheric transmission data file!" << endl;
    //~ return false;
  //~ }
  
	return true;
}
