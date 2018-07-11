#include <signal.h>
#include <random>
#include <boost/thread/thread.hpp>
#include "ProcessArray.h"

using namespace ChimeraTK;

int main() {

    constexpr size_t nVars = 1000;
    constexpr size_t nSendsPerVar = 1000;

    // create process variable pairs
    std::vector< std::pair< boost::shared_ptr<ProcessArray<size_t>>, boost::shared_ptr<ProcessArray<size_t>> > > pvars;
    for(size_t k=0; k<nVars; ++k) {
      std::string name = "var"+std::to_string(k);
      pvars.push_back(createSynchronizedProcessArray<size_t>(1, name));
    }

    // create sender thread
    boost::thread sender( [pvars] {
      for(size_t i=0; i<nSendsPerVar; ++i) {
        size_t k=0;
        for(auto &pv : pvars) {
          pv.first->accessData(0) = k*nSendsPerVar + i;
          while(true) {
            bool dataLost = pv.first->write();
            if(!dataLost) break;
          }
        }
      }
    } );  // end of sender thread definition via lambda

    auto start = std::chrono::steady_clock::now();

    size_t error_count = 0;
    for(size_t i=0; i<nSendsPerVar; ++i) {
      size_t k=0;
      for(auto &pv : pvars) {
        pv.second->read();
        if( pv.second->accessData(0) < k*nSendsPerVar + i || pv.second->accessData(0) > k*nSendsPerVar + i + 1 ) ++error_count;
      }
    }

    if(error_count > 0) {
      std::cout << "*** Test FAILED. " << error_count << " errors found." << std::endl;
      return 1;
    }

    auto end = std::chrono::steady_clock::now();

    auto nTransfers = nVars * nSendsPerVar;

    std::chrono::duration<double> diff = end-start;
    std::cout << "Time for " << nTransfers << " transfers: " << diff.count() << " s\n";
    std::cout << "Average time per transfer: " << diff.count()/(double)nTransfers * 1e6 << " us\n";


    return 0;

}
