#include "connect.h"
#include "mpc.h"
#include "protocol.h"
#include "util.h"
#include "NTL/ZZ_p.h"

#include <cstdlib>
#include <fstream>
#include <map>
#include <iostream>
#include <sstream>

using namespace NTL;
using namespace std;

void send_masked_matrix(MPCEnv& mpc, string name, Mat<ZZ_p>& matrix,
                 size_t n_rows, size_t n_cols, int other_pid) {
  Mat<ZZ_p> mask;
  mpc.RandMat(mask, n_rows, n_cols);
  matrix -= mask;
  mpc.SendMat(matrix, other_pid);
  tcout() << "Sent masked matrix for " << name << " to " << other_pid << endl;
}

void recv_masked_matrix(string data_dir, MPCEnv& mpc, string name,
                    size_t n_rows, size_t n_cols, int other_pid) {
  Mat<ZZ_p> matrix;
  fstream fs;
  string fname = data_dir + name + "_masked.bin";
  fs.open(fname.c_str(), ios::out | ios::binary);
  mpc.ReceiveMat(matrix, other_pid, n_rows, n_cols);
  mpc.WriteToFile(matrix, fs);
  fs.close();
  tcout() << "Received masked matrix from " << other_pid
    << " and wrote it to " << fname << endl;
}

bool mask_matrix(string data_dir, MPCEnv& mpc, string name,
                 size_t n_rows, size_t n_cols, int other_pid) {
  /* Open file. */
  string fname = data_dir + name;
  ifstream fin(fname.c_str());
  if (!fin.is_open()) {
    tcout() << "Error: could not open " << fname << endl;
    return false;
  }

  /* Read in matrix. */
  Mat<ZZ_p> matrix;
  Init(matrix, n_rows, n_cols);

  string line;
  int i = 0;
  while(getline(fin, line)) {
    if (i % 1000 == 0) {
      tcout() << "Reading line " << i << endl;
    }
    for (int j = 0; j < n_cols; j++) {
      double val = ((double) line[j]) - 48;
      ZZ_p val_fp;
      DoubleToFP(val_fp, val, Param::NBIT_K, Param::NBIT_F);
      matrix[i][j] = val_fp;
    }
    i++;
  }

  if (i != n_rows) {
    tcout() << "Error: Invalid number of rows: " << i << endl;
    return false;
  }
  fin.close();

  /* Send and receive a masked matrix.
   * Order of operations is swapped depending on the party,
   * to avoid a deadlock. */
  if (other_pid == 2) {
    send_masked_matrix(mpc, name, matrix, n_rows, n_cols, other_pid);
    recv_masked_matrix(data_dir, mpc, name, n_rows, n_cols, other_pid);
  } else {
    recv_masked_matrix(data_dir, mpc, name, n_rows, n_cols, other_pid);
    send_masked_matrix(mpc, name, matrix, n_rows, n_cols, other_pid);
  }

  return true;
}

bool mask_data(string data_dir, MPCEnv& mpc, int other_pid) {
  vector<string> suffixes;
  suffixes = load_suffixes(Param::TRAIN_SUFFIXES);

  mpc.SwitchSeed(other_pid);

  fstream fs;
  string fname;
  for (int i = 0; i < suffixes.size(); i++) {
    /* Save seed state to file for each batch. */
    fname = cache(other_pid, "seed" + suffixes[i]);
    fs.open(fname.c_str(), ios::out | ios::binary);
    if (fs.is_open()) {
      tcout() << "Saved seed to " << fname << endl;
    } else {
      tcout() << "Error: could not open " << fname << endl;
      return false;
    }
    mpc.ExportSeed(fs);
    fs.close();

    /* Write batch to file. */
    if (!mask_matrix(data_dir, mpc, "X" + suffixes[i],
                     Param::N_FILE_BATCH, Param::FEATURE_RANK, other_pid))
      return false;

    if (!mask_matrix(data_dir, mpc, "y" + suffixes[i],
                     Param::N_FILE_BATCH, Param::N_CLASSES - 1, other_pid))
      return false;
  }

  mpc.RestoreSeed();

  return true;
}

int main(int argc, char* argv[]) {
  if (argc < 3) {
    tcout() << "Usage: ShareData party_id param_file [data_dir (for SPs)]" << endl;
    return 1;
  }

  /* Load party id. */
  string pid_str(argv[1]);
  int pid;
  if (!Param::Convert(pid_str, pid, "party_id") || pid < 0 || pid > 2) {
    tcout() << "Error: party_id should be 0, 1, or 3" << endl;
    return 1;
  }

  /* Load values in parameter file. */
  if (!Param::ParseFile(argv[2])) {
    tcout() << "Could not finish parsing parameter file" << endl;
    return 1;
  }

  /* Load data directory name. */
  string data_dir;
  if (pid == 1 || pid == 2) {
    if (argc < 4) {
      tcout() << "Error: for SPs, data directory should be provided as the last argument" << endl;
      return 1;
    }
    data_dir = argv[3];
    if (data_dir[data_dir.size() - 1] != '/') {
      data_dir += "/";
    }
    tcout() << "Data directory: " << data_dir << endl;
  }

  /* Initalize MPC environment. */
  vector< pair<int, int> > pairs;
  pairs.push_back(make_pair(0, 1));
  pairs.push_back(make_pair(0, 2));
  pairs.push_back(make_pair(1, 2));

  MPCEnv mpc;
  if (!mpc.Initialize(pid, pairs)) {
    tcout() << "MPC environment initialization failed" << endl;
    return 1;
  }

  /* Mask the data and save to file. */
  bool success = true;
  if (pid == 1 || pid == 2) {
    const int other_pid = 3 - pid; // 1 -> 2, 2 -> 1
    success = mask_data(data_dir, mpc, other_pid);
    if (!success) {
      tcout() << "Data masking failed." << endl;
    } else {
      tcout() << "Party " << pid << " done streaming data." << endl;
    }
  }

  /* Keep party 0 online until end of data masking. */
  if (pid == 0) {
    mpc.ReceiveBool(2);
  } else if (pid == 2) {
    mpc.SendBool(true, 0);
  }

  mpc.CleanUp();

  if (success) {
    tcout() << "Protocol successfully completed." << endl;
    return 0;
  } else {
    tcout() << "Protocol abnormally terminated." << endl;
    return 1;
  }
}
