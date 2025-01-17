//    This file is part of ASMC, developed by Pier Francesco Palamara.
//
//    ASMC is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    ASMC is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with ASMC.  If not, see <https://www.gnu.org/licenses/>.

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "Data.hpp"
#include "DecodingParams.hpp"
#include "DecodingQuantities.hpp"
#include "FileUtils.hpp"

#include "HMM.hpp"
#include "StringUtils.hpp"
#include "Timer.hpp"

using namespace std;

int main(int argc, char* argv[])
{

  srand(1234);

  const char VERSION[] = "1.0";
  const char VERSION_DATE[] = "July 1, 2018";
  const char YEAR[] = "2018";
  const char LICENSE[] = "GNU GPL v3";
  const char WEBSITE[] = "www.palamaralab.org/software/ASMC";
  const char PROGRAM[] = "Ascertained Sequentially Markovian Coalescent (ASMC)";

  DecodingParams params;

  // parse input arguments
  if (!params.processCommandLineArgs(argc, argv)) {
    cerr << "Error processing command line; exiting." << endl;
    exit(1);
  }

  cout << "\n";

  // cout << "              _____   __  __    _____ \n";
  // cout << "     /\\      / ____| |  \\/  |  / ____|\n";
  // cout << "    /  \\    | (___   | \\  / | | |     \n";
  // cout << "   / /\\ \\    \\___ \\  | |\\/| | | |     \n";
  // cout << "  / ____ \\   ____) | | |  | | | |____ \n";
  // cout << " /_/    \\_\\ |_____/  |_|  |_|  \\_____|\n";

  cout << " █████╗   ███████╗  ███╗   ███╗   ██████╗\n";
  cout << "██╔══██╗  ██╔════╝  ████╗ ████║  ██╔════╝\n";
  cout << "███████║  ███████╗  ██╔████╔██║  ██║     \n";
  cout << "██╔══██║  ╚════██║  ██║╚██╔╝██║  ██║     \n";
  cout << "██║  ██║  ███████║  ██║ ╚═╝ ██║  ╚██████╗\n";
  cout << "╚═╝  ╚═╝  ╚══════╝  ╚═╝     ╚═╝   ╚═════╝\n";

  cout << "\n" << PROGRAM << " v." << VERSION << ", " << VERSION_DATE << "\n";
  cout << LICENSE << ", Copyright (C) " << YEAR << " Pier Palamara"
       << "\n";
  cout << "Manual: " << WEBSITE << "\n"
       << "\n";

  cout << "Decoding batch " << params.jobInd << " of " << params.jobs << "\n\n";

  cout << "Will decode " << params.decodingModeString << " data." << endl;
  cout << "Output will have prefix: " << params.outFileRoot << endl;
  if (params.compress)
    cout << "Will use classic emission model (no CSFS)." << endl;
  else
    cout << "Minimum marker distance to use CSFS is set to " << params.skipCSFSdistance
         << "." << endl;
  if (params.useAncestral)
    cout << "Assuming ancestral alleles are correctly encoded." << endl;
  if (params.doPosteriorSums)
    cout << "Will output sum of posterior tables for all pairs." << endl;
  if (params.doMajorMinorPosteriorSums)
    cout << "Will output sum of posterior tables for all pairs, partitioned by "
            "major/minor alleles."
         << endl;

  // if (params.noBatches)
  //     cout << "Will not process samples in batches (slower)." << endl;
  // if (!params.withinOnly)
  //     cout << "Will only decode maternal vs. paternal haplotypes." << endl;
  // if (params.doPerPairMAP)
  //     cout << "Will output MAP for all haploid pairs (DANGER: huge files)." << endl;
  // if (params.doPerPairPosteriorMean)
  //     cout << "Will output posterior mean for all haploid pairs (DANGER: huge
  //     files)." << endl;

  // used for benchmarking
  Timer timer;

  // read decoding quantities from file
  std::string str(params.decodingQuantFile.c_str());
  DecodingQuantities decodingQuantities(params.decodingQuantFile.c_str());
  printf("Read precomputed decoding info in %.3f seconds.\n", timer.update_time());
  // cout << "CSFS samples: " << decodingQuantities.CSFSSamples << endl;

  cout << "Data will be loaded from " << params.hapsFileRoot << "*\n";
  int sequenceLength = Data::countHapLines(params.hapsFileRoot.c_str());
  Data data(params.hapsFileRoot.c_str(), sequenceLength, decodingQuantities.CSFSSamples,
      params.foldData, params.usingCSFS);
  printf("Read haps in %.3f seconds.\n", timer.update_time());

  HMM hmm(data, decodingQuantities, params, !params.noBatches);

  hmm.decodeAll(params.jobs, params.jobInd);
  const DecodingReturnValues& decodingReturnValues = hmm.getDecodingReturnValues();
  const vector<vector<float>>& sumOverPairs = decodingReturnValues.sumOverPairs;

  // output sums over pairs (if requested)
  if (params.doPosteriorSums) {
    FileUtils::AutoGzOfstream fout;
    fout.openOrExit(params.outFileRoot + ".sumOverPairs.gz");
    for (int pos = 0; pos < data.sites; pos++) {
      for (uint k = 0; k < decodingQuantities.states; k++) {
        if (k)
          fout << "\t";
        fout << sumOverPairs[pos][k];
      }
      fout << endl;
    }
    fout.close();
  }
  if (params.doMajorMinorPosteriorSums) {
    vector<vector<float>> sumOverPairs00 = decodingReturnValues.sumOverPairs00;
    vector<vector<float>> sumOverPairs01 = decodingReturnValues.sumOverPairs01;
    vector<vector<float>> sumOverPairs11 = decodingReturnValues.sumOverPairs11;
    // Sum for 00
    FileUtils::AutoGzOfstream fout00;
    fout00.openOrExit(params.outFileRoot + ".00.sumOverPairs.gz");
    for (int pos = 0; pos < data.sites; pos++) {
      for (uint k = 0; k < decodingQuantities.states; k++) {
        if (k)
          fout00 << "\t";
        if (!data.siteWasFlippedDuringFolding[pos]) {
          fout00 << sumOverPairs00[pos][k];
        } else {
          fout00 << sumOverPairs11[pos][k];
        }
      }
      fout00 << endl;
    }

    fout00.close();
    // Sum for 01
    FileUtils::AutoGzOfstream fout01;
    fout01.openOrExit(params.outFileRoot + ".01.sumOverPairs.gz");
    for (int pos = 0; pos < data.sites; pos++) {
      for (uint k = 0; k < decodingQuantities.states; k++) {
        if (k)
          fout01 << "\t";
        fout01 << sumOverPairs01[pos][k];
      }
      fout01 << endl;
    }
    fout01.close();
    // Sum for 11
    FileUtils::AutoGzOfstream fout11;
    fout11.openOrExit(params.outFileRoot + ".11.sumOverPairs.gz");
    for (int pos = 0; pos < data.sites; pos++) {
      for (uint k = 0; k < decodingQuantities.states; k++) {
        if (k)
          fout11 << "\t";
        if (!data.siteWasFlippedDuringFolding[pos]) {
          fout11 << sumOverPairs11[pos][k];
        } else {
          fout11 << sumOverPairs00[pos][k];
        }
      }
      fout11 << endl;
    }
    fout11.close();

    cout << "Done.\n\n";
  }
}
