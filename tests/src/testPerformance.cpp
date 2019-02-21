#include "UnidirectionalProcessArray.h"
#include <boost/thread/thread.hpp>
#include <cstdint>
#include <random>
#include <signal.h>

using namespace ChimeraTK;

int main() {

  constexpr size_t dataSize = 16384;
  constexpr size_t nVars = 20;
  constexpr size_t nSendsPerVar = 200;

  // create process variable pairs
  std::vector<std::pair<boost::shared_ptr<ProcessArray<std::uint64_t>>,
                        boost::shared_ptr<ProcessArray<std::uint64_t>>>>
      pvars;
  for (size_t k = 0; k < nVars; ++k) {
    std::string name = "var" + std::to_string(k);
    pvars.push_back(
        createSynchronizedProcessArray<std::uint64_t>(dataSize, name));
  }

  // create sender thread
  size_t sum_sender = 0;
  bool dataWasLost = false;
  boost::thread sender([pvars, &sum_sender, &dataWasLost] {
    for (size_t i = 0; i < nSendsPerVar; ++i) {
      size_t k = 0;
      for (auto &pv : pvars) {
        for (size_t j = 0; j < dataSize; ++j) {
          pv.first->accessData(j) = k * nSendsPerVar + i + j;
          sum_sender += k * nSendsPerVar + i + j;
        }
        while (true) {
          bool dataLost = pv.first->write();
          if (!dataLost)
            break;
          dataWasLost = true;
        }
      }
    }
  }); // end of sender thread definition via lambda

  auto start = std::chrono::steady_clock::now();

  size_t sum = 0;
  for (size_t i = 0; i < nSendsPerVar; ++i) {
    for (auto &pv : pvars) {
      pv.second->read();
      for (size_t j = 0; j < dataSize; ++j)
        sum += pv.second->accessData(j);
    }
  }
  std::cout << "SUM = " << sum << std::endl;
  sender.join();
  bool failed = false;
  if (sum_sender != sum) {
    std::cout << "ERROR sender sum = " << sum_sender << std::endl;
    if (dataWasLost) {
      std::cout << "-> data was overwritten on sender side, sums are expected "
                   "not to match."
                << std::endl;
    } else {
      std::cout << "-> no data was overwritten on sender side, sums are "
                   "expected to match. Failing the test!"
                << std::endl;
      failed = true;
    }
  }

  auto end = std::chrono::steady_clock::now();

  auto nTransfers = nVars * nSendsPerVar;

  std::chrono::duration<double> diff = end - start;
  std::cout << "Time for " << nTransfers << " transfers: " << diff.count()
            << " s\n";
  std::cout << "Average time per transfer: "
            << diff.count() / (double)nTransfers * 1e6 << " us\n";

  return failed;
}
