#pragma once
#include <math.h>
#include <numeric>
#include <limits>
#include "Utilities.h"

class Particle {
public:
	explicit Particle(int numberOfIterations, std::function< double( double,  double)> fun, std::vector<double> maxX, std::vector<double> maxY);
	void CalculateNextPosition();
	void runSimulatedAnealing();
	std::pair<double, double> neighbor(const std::pair<double, double>& currentPosition);

	std::pair<double, double> GetBestPosition() { return bestPosition; }
	std::pair<double, double> GetBestLocalPosition() { return bestLocalPosition; }

	void SetBestGlobalPosition(std::pair<double, double> solution);
	const std::pair<double, double>& getCurrentPosition() const { return currentPosition; }
	void setCurrentPosition(const std::pair<double, double>& solution)  { currentPosition = solution; }

private:
	inline void RandomizeR();
	std::pair<double, double> GenerateStartingPositions(std::vector<double> x, std::vector<double> y);
	std::pair<double, double> FindBestDirection(double velocity);
private:
	double velocity;
	double vx = 1;
	double vy = 1;
	double rl, rg;
	std::pair< double,  double> currentPosition;
	std::pair<double, double> bestLocalPosition;
	std::pair< double,  double> bestPosition;
	double w; //is calculated accoridng to the https://www.matec-conferences.org/articles/matecconf/pdf/2016/26/matecconf_mmme2016_02019.pdf
	double c1, c2; //calulated in every iteration according to the Ratnaweera equation-https://web2.qatar.cmu.edu/~gdicaro/15382-Spring18/hw/hw3-files/pso-book-extract.pdf
	const double cmax = 2.5, cmin = 0.5; //according to to Ratnaweera
	double wmax = 0.9, wmin = 0.4; //accoring to menitoned method in w

	std::function< double( double,  double)> goalFunction;
	 int n=0; //current iteration
	 int T;//number of iterations

	std::vector<double> X;
	std::vector<double> Y;

	std::random_device rd;
	std::mt19937 gen{ rd() };
};
