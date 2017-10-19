
#include <cassert>
#include <iostream>

#include "Parameters.h"
#include "Environment.h"

#define DEFAULT_SIMFILE "data/in/SimFile.txt"
#define DEFAULT_OUTPREFIX "default"

// Input Files
std::string Parameters::NamePftFile; 							// trait file for experiment species
std::string Parameters::NameSimFile = DEFAULT_SIMFILE; 	// file with simulation scenarios
std::string Parameters::outputPrefix = DEFAULT_OUTPREFIX;

Parameters Parameters::params;

Parameters::Parameters() :
		weekly(0), ind_out(0), PFT_out(2), srv_out(1), trait_out(1), aggregated_out(1),
		AboveCompMode(asympart), BelowCompMode(sym), stabilization(version1), mode(communityAssembly),
		Tmax_monoculture(10),
		ITV(off), ITVsd(0),
		GridSize(173),
		Tmax(100),
		seedMortality(0.5), winterDieback(0.5), backgroundMortality(0.007), litterDecomp(0.5),
		meanARes(100), meanBRes(100),
		rametEstab(1),
		AbvGrazProb(0), AbvPropRemoved(0), BiteSize(0.5), MassUngrazable(15300),
		BelGrazProb(0), BelGrazPerc(0), BelGrazResidualPerc(0),
		BelGrazAlpha(2), BelGrazHistorySize(60),
		CutHeight(0), NCut(0),
		CatastrophicDistYear(100), CatastrophicDistWeek(20),
		CatastrophicPlantMortality(0),
		Aampl(0), Bampl(0),
		SeedInput(0), SeedRainType(0)
{

}

//-----------------------------------------------------------------------------

std::string Parameters::getSimID()
{

	std::string s =
			std::to_string(Environment::SimID) + "_" +
			std::to_string(Environment::ComNr) + "_" +
			std::to_string(Environment::RunNr);

	return s;
}

//-----------------------------------------------------------------------------

//
//  Because the constructor already sets the default filename we check if the name has its default content
//  and overwrite it. This is like a statemachine using the state of the filenames as state variable.
void Parameters::ProcessArgs(std::string aArg) {
    if (Parameters::NameSimFile == DEFAULT_SIMFILE) {
        Parameters::NameSimFile = aArg;
    } else if (Parameters::outputPrefix == DEFAULT_OUTPREFIX) {
        Parameters::outputPrefix = aArg;
    } else {
        std::cerr << "Do not except more than two file names.\nTake a look at the help with -h or --help\n";
    }
}
