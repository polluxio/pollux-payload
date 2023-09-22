#include <sstream>
#include <thread>
#include "spdlog/spdlog.h"

#include "pollux.h"

#include "OptimizationFunctions.h"
#include "SavePoints.h"

namespace {

size_t numPixels = 1000;

std::map<size_t, std::pair<double, double> > _particlesData; 

class PolluxPayLoadPSO: public PolluxPayload {
  public:
    PolluxPayLoadPSO(): PolluxPayload("pollux-payload-pso") {}

    void init() override {
      auto Himmelblau = [](double x, double y) {return std::pow(x * x + y - 11, 2) + std::pow(x + y * y - 7, 2); };
    
      auto Levi = [](double x, double y) {
          return std::pow(std::sin(3 * M_PI * x), 2) + std::pow(x - 1, 2) * (1 + std::pow(std::sin(3 * y * M_PI), 2))
              + std::pow(y - 1, 2) * (1 + std::pow(std::sin(2 * M_PI * y), 2));
      };
      auto Rastrign = [](double x, double y) {
          return
              20 + std::pow( x - 5, 2) - 10 * std::cos(2 * M_PI * ( x - 5)) + (y - 5)* (y - 5)- 10 * std::cos(2 * M_PI * (y - 5));
      };
      auto parabole = [](double x, double y) {
          return
              std::pow(x, 2) + std::pow(y, 2);
      };
      auto multimodal  = [](double x, double y) {
          return
              -std::abs(sin(x) * cos(y) * std::exp(std::abs(1 - (sqrt(x*2 + y*2)/M_PI))));
      };
      auto eggholder  = [](double x, double y) {
          return
              -(y+47)*(sin(std::sqrt(std::abs(x/2 + (y + 47))))) - x*sin(std::sqrt(std::abs(x + (y + 47))));
      };
      auto beale  = [](double x, double y) {
          return std::pow(1.5-x + x*y, 2) + std::pow(2.25-x + x*std::pow(y,2), 2)
            + std::pow(2.65-x + x*std::pow(y,3), 2);
      };
      func_ = eggholder;

      std::vector<double> real_solutions;
      const int n = 100;
      std::vector<double> x;
    
      std::vector<double> y;
      x.push_back(512.0);
      x.push_back(-512.0);
      y.push_back(512.0);
      y.push_back(-512.0);
      for (int i = 0; i < 1; i++) {
        particle_ = std::make_unique<Particle>(n, func_, x, y );
      }
      std::vector<std::pair<double, double> > pixels;
      for (int i = 0; i < numPixels; i++) {
        for (int j = 0; j < numPixels; j++) {
          pixels.push_back(std::pair<double, double>(i*std::abs(x[0]-x[1])/numPixels +x[1],
            j*std::abs(y[0]-y[1])/numPixels +y[1]));
        }
      }
      int numParticles = getNumberOfPayloads();
      if (getLocalID() == 0) {
        particle_->setCurrentPosition(pixels[0]);
      } else if (getLocalID() == numParticles) {
        particle_->setCurrentPosition(pixels[pixels.size() - 1]);
      } else {
        particle_->setCurrentPosition(pixels[getLocalID()*numPixels/numParticles]);
      }
      
      bestSolution_ = particle_->GetBestLocalPosition();
      for (int j = 0; j < 1; j++) {
        particle_->SetBestGlobalPosition(bestSolution_);
      }
    }

    void loop(ZebulonPayloadClient* client) override {
      //assert(!(particle_->getCurrentPosition().first > maxX[0] || particle_->getCurrentPostition().first < maxX[1]));
	    //assert(!(particle_->getCurrentPosition().second > maxY[0] || particle_->getCurrentPostition().second  < maxY[1]));
      std::string filename("./example" + std::to_string(getLocalID()) + ".txt");
      std::ofstream myfile;
      //myfile.open(filename);
      myfile.open(filename, std::ios_base::app | std::ios_base::out);
      //std::ofstream myfile("./example.txt"/* + std::to_string(localID) + ".txt"*/);
      //myfile.open ("/home/nonoc/cloud/hello/polluxapptest100/polluxapptest/example" + std::to_string(localID) + ".txt", std::fstream::out);
      myfile << particle_->getCurrentPosition().first << " " << particle_->getCurrentPosition().second << std::endl;
      myfile.close();

      particle_->CalculateNextPosition();
      spdlog::info("ITER after pso: {} {}",
          particle_->getCurrentPosition().first,
          particle_->getCurrentPosition().second);

      _particlesData.clear();

      spdlog::info("Main loop started iteration: {}", iteration_);
      spdlog::info("Particle before SA: {},{}", particle_->getCurrentPosition().first, particle_->getCurrentPosition().second);
      //particle_->runSimulatedAnealing();
      spdlog::info("Particle position after SA: {},{}", particle_->getCurrentPosition().first, particle_->getCurrentPosition().second);
      //while (_particlesData.size() < control_.partids().size()) {
         std::ostringstream PLOG_INFO;
         PLOG_INFO <<  "sending " << particle_->GetBestLocalPosition().first << " " << (particle_->GetBestLocalPosition().second);

         PLOG_INFO <<  "sending string " << std::to_string(particle_->GetBestLocalPosition().first) << " " << std::to_string(particle_->GetBestLocalPosition().second);
         client->polluxCommunication(-1, std::to_string(particle_->GetBestLocalPosition().first), std::to_string(particle_->GetBestLocalPosition().second));
         PLOG_INFO << "size: " << _particlesData.size();
         spdlog::info("{}", PLOG_INFO.str());
         //sleep(1);
      //}
      {
        std::ostringstream PLOG_INFO;
        PLOG_INFO << "continue";
        spdlog::info("{}", PLOG_INFO.str());
      }
      for (int i = 0; i < getNumberOfPayloads(); i++) {
        if (func_(_particlesData[i].first, _particlesData[i].second) < func_(bestSolution_.first, bestSolution_.second)) {
          bestSolution_ = _particlesData[i];
          std::ostringstream PLOG_INFO;
          PLOG_INFO <<  "FOUND NEW BEST" <<  " " << bestSolution_.first <<  " " << bestSolution_.second << " " << 
          func_(bestSolution_.first, bestSolution_.second);
          spdlog::info("{}", PLOG_INFO.str());
          particle_->SetBestGlobalPosition(bestSolution_);
        }
      }

      if (iterationsNum_ == iteration_) {
        client->sendPayloadLoopEnd(iteration_++);
      } else {
        client->sendPayloadLoopReadyForNextIteration(iteration_++);
      }
    }

    void polluxCommunication(const pollux::PolluxMessage* message) override {
      std::ostringstream logMessage;
      logMessage << "Pollux Message received: "
        << "origin=" << message->origin();
      logMessage << ", key=" << message->key();
      switch (message->value_case()) {
        case pollux::PolluxMessage::kStrValue:
          logMessage << ", value=" << message->strvalue() << std::endl;
          _particlesData[message->origin()].first = std::stod(message->key());
          _particlesData[message->origin()].second = std::stod(message->strvalue());;
          break;
        default:
          break;
      }
      spdlog::info("{}", logMessage.str());
    }

  private:
    int   iteration_ = 0;
    size_t iterationsNum_ = 100;
    std::vector<double> x_;
    std::vector<double> y_;
    std::unique_ptr<Particle> particle_;
    std::vector<std::pair<double, double>> solutions_;
    std::pair<double, double> bestSolution_;
    std::function<double(double, double)> func_;
};

}

int main(int argc, char** argv) {
  auto payload = std::make_unique<PolluxPayLoadPSO>();
  return Pollux::Main(argc, argv, payload.get());
}