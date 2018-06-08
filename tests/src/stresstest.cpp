#include <signal.h>
#include <random>
#include <boost/thread/thread.hpp>
#include "ProcessArray.h"

using namespace ChimeraTK;

// std::ceil isn't constexpr for some reason...
constexpr size_t constceil(double num) {
    return (static_cast<double>(static_cast<size_t>(num)) == num)
        ? static_cast<size_t>(num)
        : static_cast<size_t>(num) + ((num > 0) ? 1 : 0);
}

std::atomic<bool> terminate;
std::atomic<size_t> nSendOps;
std::atomic<size_t> nReceiveOps;


extern "C" void sigAbortHandler(int /*signal_number*/) {
    terminate = true;
    std::cout << "SIGABORT caught. nSendOps = " << nSendOps << "  nReceiveOps = " << nReceiveOps << std::endl;
    exit(1);
}


int main() {

    // catch SIGABRT to print some useful information before terminating
    signal(SIGABRT, &sigAbortHandler);

    constexpr size_t nSenders = 100;        // same number of receivers
    constexpr size_t nVarsPerSender = 100;  // same number of variables per receiver
    constexpr size_t runForSeconds = 10;

    nSendOps = 0;
    nReceiveOps = 0;

    // create process variables and distribute them to the threads
    for(size_t i=0; i<nSenders; ++i) {

      // create process variable pairs for the current thread pair
      std::vector< std::pair< boost::shared_ptr<ProcessArray<int>>, boost::shared_ptr<ProcessArray<int>> > > pvars;
      for(size_t k=0; k<nVarsPerSender; ++k) {
        std::string name = "thread"+std::to_string(i)+"_var"+std::to_string(k);
        pvars.push_back(createSynchronizedProcessArray<int>(1, name));
      }

      // create sender thread
      boost::thread sender( [pvars] {
        std::random_device rd;
        std::mt19937 gen(rd());

        // Random value will be used to determine action on receiver side (this value will be send through the variable):
        // The value is the number of microseconds to sleep after each receive operation.
        // The value modulo 4 determines the next receive operation type:
        //   0 - read() on the next variable
        //   1 - readNonBlocking() on the next variable
        //   2 - readLatest() on the next variable
        //   3 - readAny() on all variables
        std::uniform_int_distribution<> disValue(1, 1000);

        // A second random value used to determin the number of microseconds to sleep after each send operation
        std::uniform_int_distribution<> disSleep(1, 500);

        // loop until termination request
        while(!terminate) {
          for(auto &pv : pvars) {
            pv.first->accessData(0) = disValue(gen);
            pv.first->write();
            ++nSendOps;
            usleep(disSleep(gen));
          }
        }

      } );  // end of sender thread definition via lambda
      sender.detach();

      // create receiver thread
      boost::thread receiver( [pvars] {

        // see comment in sender thread
        int mode = 0;

        // iterator pointing to the current pvar
        auto pviter = pvars.begin();

        // fill list of app receivers for readAny()
        std::list<std::reference_wrapper<mtca4u::TransferElement>> varList;
        std::map< mtca4u::TransferElementID, boost::shared_ptr<ProcessArray<int>> > varMap;
        for(auto &pvar : pvars) {
          varList.emplace_back(*(pvar.second));
          varMap[pvar.second->getId()] = pvar.second;
        }

        // loop until termination request
        while(!terminate) {

          int sleepTime;
          if(mode == 0) {
            pviter->second->read();
            sleepTime = pviter->second->accessData(0);
          }
          else if(mode == 1) {
            pviter->second->readNonBlocking();
            sleepTime = pviter->second->accessData(0);
          }
          else if(mode == 2) {
            pviter->second->readLatest();
            sleepTime = pviter->second->accessData(0);
          }
          /*else {  // mode == 3                            /// @todo enable readAny in stresstest again!
            auto id = readAny(varList);
            sleepTime = varMap[id]->accessData(0);
          }*/
          ++nReceiveOps;
          mode = (sleepTime % 3); //4);

          // iterate to next variable
          ++pviter;
          if(pviter == pvars.end()) pviter = pvars.begin();

          usleep(sleepTime);
        }

      } );  // end of sender thread definition via lambda
      receiver.detach();

    } // thread-launching loop

    sleep(runForSeconds);
    terminate = true;

    return 0;

}
