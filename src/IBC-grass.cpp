#include <iostream>
#include <string>
#include <sstream>
#include <memory>
#include <cassert>

#include "GridEnvir.h"

using namespace std;

int    startseed  = -1;
int    linetoexec = -1;
int    proctoexec =  1;

string configfilename;
//   Support functions for program parameters
//
//   This is the usage dumper to the console.
static void dump_help() {
    cerr << "usage:\n"
            "\tibc <options> <simfilename> <outputprefix>\n"
            "\t\t-h/--help : print this usage information\n"
            "\t\t-c        : use this file with configuration data\n"
            "\t\t-n        : line to execute in simulation\n"
            "\t\t-p        : number of processors to use\n"
            "\t\t-s        : set a starting seed for random number generators\n";
    exit(0);

}
//
//
//  This is the processing function for long parameters.
//  It splits the argument at the equal sign into name and value string
static void process_long_parameter(string aLongParameter) {
    std::string name=aLongParameter.substr(0, aLongParameter.find_first_of('='));
    std::string value=aLongParameter.substr(aLongParameter.find_first_of('=')+1);

    if (name == "help") {
        dump_help();
    } else {
        std::cerr << "unknown parameter : " << name << "\n";
    }
}

int main(int argc, char* argv[])
{
    //
    //
    //  Beginning of a new parameter parser.
    //
    //
    int   i = 1;
    /*
      * Parse the program parameters.
      */
     while ((i<argc) && (argv[i]!=0)) {
         char *s=argv[i];

         if (*s=='-') {
             s++;
             switch (*s) {
             case 's':
                 s++;
                 if (*s!='\0') {
                     startseed=atoi(s);
                 } else {
                     i++;
                     s=argv[i];
                     if (s!=0) {
                         startseed=atoi(s);
                     } else {
                     }
                 }
                 break;
             case 'n':
                 s++;
                 if (*s!='\0') {
                     linetoexec=atoi(s);
                 } else {
                     i++;
                     s=argv[i];
                     if (s!=0) {
                         linetoexec=atoi(s);
                     } else {
                     }
                 }
                 break;
             case 'p':
                 s++;
                 if (*s!='\0') {
                     proctoexec=atoi(s);
                 } else {
                     i++;
                     s=argv[i];
                     if (s!=0) {
                         proctoexec=atoi(s);
                     } else {
                     }
                 }
                 break;
             case '-':    //  Long parameter
                 s++;     //  Move on to parameter name
                 process_long_parameter(s);
                 break;
             case 'c':
                 s++;
                 if (*s!='\0') {
                     configfilename=s;
                 } else {
                     i++;
                     s=argv[i];
                     if (s!=0) {
                         configfilename=s;
                     } else {
                     }
                 }
                 break;
             case 'h':
                 dump_help();
                 break;
             default:
                 std::cerr << "Unknown parameter. See usage\n";
                 dump_help();
                 break;
             }
         } else {
             Parameters::params.ProcessArgs(argv[i]);
         }
         i++;
     }

    cerr << "Using simfile : " << Parameters::NameSimFile << endl << "Using output prefix : " << Parameters::outputPrefix << endl;
    //
    //  This is the end of a new program parameter parser.
    //
    //

	ifstream SimFile(Parameters::NameSimFile.c_str()); // Open simulation parameterization file
	int _NRep;
    int linecounter = 1;
	// Temporary strings
	string trash;
	string data;

	getline(SimFile, data);
	std::stringstream ss(data);
	ss >> trash >> _NRep; 		// Remove "NRep" header, set NRep
	getline(SimFile, trash); 	// Remove parameterization header file

	while (getline(SimFile, data))
	{
        if ((linetoexec == -1) || (linecounter == linetoexec)) {
            for (int i = 0; i < _NRep; i++)
            {
                unique_ptr<GridEnvir> run = unique_ptr<GridEnvir>( new GridEnvir() );
                run->GetSim(data);
                run->RunNr = i;

                cout << Parameters::params.getSimID() << endl;
                cout << "Run " << run->RunNr << " \n";

                run->InitRun();
                run->OneRun();
            }
        }
        linecounter++;
	}

	SimFile.close();

	return 0;
}
