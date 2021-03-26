#include <iostream>

#include <EventIO.hh>

/*
 * 
 * Function: GetInputs
 * 
 * Receives an EventIO::Item object of type 1212 and read the CORSIKA
 * input lines from it.
 * 
 * @param  item  Pointer to Item object of type 1212
 * @return (none)
 * 
 */
void GetInputs(eventio::EventIO::Item * item, bool dump)
{
  int n; // Number of lines
  item->GetInt32(n);
  for (int i=0; i<n; i++)
  {
    // Loop over input lines
    std::string line = item->GetString16();
    if (dump) std::cout << line << std::endl;
  }
  return;
}
