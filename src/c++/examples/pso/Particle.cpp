#include "Particle.h"
#include <iostream>
#include <ctime>
#include <cassert>

Particle::Particle(int numberOfIterations, std::function< double( double,  double)> fun, std::vector<double> maxX, std::vector<double> maxY)
{
	goalFunction = fun;
	T = numberOfIterations;
	X = maxX;
	Y = maxY;
	//currentPosition = GenerateStartingPositions(maxX, maxY);
	std::normal_distribution<> distx((X[0] - X[1])/2 + X[1], abs(X[0] - X[1])/2);
	std::normal_distribution<> disty((Y[0] - Y[1])/2 + Y[1], abs(Y[0] - Y[1])/2);

	double x = maxX[0]+1;
	while (x > maxX[0] || x < maxX[1])
		x = distx(rd);
	double y = maxY[0]+1;
	while (y > maxY[0] || y < maxY[1])
		y = disty(rd);
	currentPosition = std::pair<double, double>(x,y);
	bestLocalPosition = currentPosition;
	bestPosition = currentPosition;
	
	RandomizeR();
}

std::pair<double, double> Particle::neighbor(const std::pair<double, double>& currentPosition) {
	std::normal_distribution<> dist(0, 1);
	return std::pair<double, double>(std::fmod(std::abs(currentPosition.first + dist(rd)*(X[0] -X[1])/1000 -X[1]), X[0] -X[1]) + X[1]
		, std::fmod(std::abs(currentPosition.second + dist(rd)*(Y[0] -Y[1])/1000 -Y[1]), Y[0] -Y[1]) + Y[1]);
}
 
//https://www.geeksforgeeks.org/simulated-annealing/
void Particle::runSimulatedAnealing() {
	/*
	int T = 1;
	float Tmin = 0.0001;
	float alpha = 0.9;
	int numIterations = 100;*/
	double Tempr = 1;
	double Tmin = 0.0001;
	double alpha = 0.8;
	size_t numIterationsSA = 100;
	//assert(!(currentPosition.first > maxX[0] || currentPosition.first < maxX[1]));
	//assert(!(currentPosition.second > maxY[0] || currentPosition.second  < maxY[1]));
	while (Tempr > Tmin) {
        for (size_t i = 0; i < numIterationsSA; i++) {
            // Reassigns global minimum accordingly
            if (goalFunction(currentPosition.first, currentPosition.second) < goalFunction(bestLocalPosition.first, bestLocalPosition.second)) {
                bestLocalPosition = currentPosition;
            }
            auto newSol = neighbor(currentPosition);
            //assert(!(newSol.first > maxX[0] || newSol.first < maxX[1]));
            //assert(!(newSol.second > maxY[0] || newSol.second  < maxY[1]));
            float ap = exp((goalFunction(currentPosition.first, currentPosition.second) - goalFunction(newSol.first, newSol.second)) / Tempr);
            srand( (unsigned)time( NULL ) );
            if (ap > (float) rand()/RAND_MAX) {
                currentPosition = newSol;
            }
        }
        Tempr *= alpha; // Decreases T, cooling phase
    }
}

void Particle::CalculateNextPosition()
{
	w = wmax - ((wmax - wmin) * n / T);
	std::normal_distribution<> dist(0, 1);
	double rp = dist(rd);
	double rg = dist(rd);
	double phip = 0.5;
	double phig = 0.5;
	double vx = w*vx + phip*rp*(bestLocalPosition.first - currentPosition.first) +
		phig*rg*(bestPosition.first - currentPosition.first);
	double vy = w*vy + phip*rp*(bestLocalPosition.second - currentPosition.second) +
		phig*rg*(bestPosition.second - currentPosition.second);
	
	assert(vx);
	assert(vy);
	currentPosition.first = std::fmod(std::abs(currentPosition.first + vx - X[1]), X[0] - X[1]) + X[1];
	currentPosition.second = std::fmod(std::abs(currentPosition.second + vy - Y[1]), Y[0] - Y[1]) + Y[1];
	if (w > wmax - (wmax - wmin)/2 && dist(rd) > 0) {
		currentPosition.first = std::fmod(rand(), X[0] - X[1]) + X[1];
		currentPosition.second = std::fmod(rand(), Y[0] - Y[1]) + Y[1];
		//currentPosition.first = /*currentPosition.first +*/ rand()%((int)(X[0] -X[1])) + X[1];
		//currentPosition.second = /*currentPosition.second +*/ rand()%((int)(Y[0] -Y[1])) + Y[1];
	}
	if (w > wmax - (wmax - wmin)/2) {
		runSimulatedAnealing();
	}
	if (goalFunction(currentPosition.first, currentPosition.second) < goalFunction(bestLocalPosition.first, bestLocalPosition.second)) {
		bestLocalPosition = currentPosition;
	}
	if (goalFunction(currentPosition.first, currentPosition.second) < goalFunction(bestPosition.first, bestPosition.second)) {
		bestPosition = currentPosition;
	}
	n++;
}

std::pair<double, double> Particle::FindBestDirection(double velocity)
{
	std::vector<double> solutions(4);
	std::vector<std::pair<double, double>> directions(4);
	std::fill(solutions.begin(), solutions.end(), std::numeric_limits<double>::max());
	
	double newPositiveX = currentPosition.first + velocity;
	double newNegativeX = currentPosition.first - velocity;
	double newPositiveY = currentPosition.second + velocity;
	double newNegativeY = currentPosition.second - velocity;
	
	if (newPositiveX < X[0] && newPositiveX > X[1] && newPositiveY < Y[0] && newPositiveY > Y[1])
	{
		solutions[0] = goalFunction(newPositiveX, newPositiveY);
		directions[0] = { newPositiveX,newPositiveY };
	}
	if (newPositiveX < X[0] && newPositiveX > X[1] && newNegativeY < Y[0] && newNegativeY > Y[1])
	{
		solutions[1] = goalFunction(newPositiveX, newNegativeY);
		directions[1] = { newPositiveX,newNegativeY };
	}
	if (newNegativeX < X[0] && newNegativeX > X[1] && newNegativeY < Y[0] && newNegativeY > Y[1])
	{
		solutions[2] = goalFunction(newNegativeX, newNegativeY);
		directions[2] = { newNegativeX,newNegativeY };
	}
	if (newNegativeX < X[0] && newNegativeX > X[1] && newPositiveY < Y[0] && newPositiveY > Y[1])
	{
		solutions[3] = goalFunction(newNegativeX, newPositiveY);
		directions[3] = { newNegativeX,newPositiveY };
	}
	int index = std::min_element(solutions.begin(), solutions.end()) - solutions.begin();
	return directions[index];
}

std::pair<double, double> Particle::GenerateStartingPositions(std::vector<double> x, std::vector<double> y)
{
	double scopeX = std::abs(x[0] - x[1]);
	double scopeY = std::abs(y[0]-y[1]);

	int X = scopeX * EPSILON_EXP;
	int Y = scopeY * EPSILON_EXP;

	std::pair< double, double> result;

	std::normal_distribution<> dist(15000 , 10000);
	srand(std::time(NULL));
	while (true)
	{
		result.first = std::abs(dist(rd));
		result.first /= EPSILON_EXP;
		result.first += x[1];
		if (result.first <= x[0] && result.first > x[1])
			break;
	}
	
	while (true)
	{
		result.second = std::abs(dist(rd));
		result.second /= EPSILON_EXP;
		result.second += y[1];
		if (result.second <= y[0] && result.second > y[1])
			break;
	}

	return result;
}

void Particle::RandomizeR()
{
	std::normal_distribution<> dist(500 , 250 );
	while (true)
	{
		rl = std::abs(dist(rd));
		if (rl < 1000 && rl>0)
			break;
	}
	while (true)
	{
		rg = std::abs(dist(rd));
		if (rg < 1000 && rg>0)
			break;
	}
	rl /= 1000;
	rg /= 1000;
}

void Particle::SetBestGlobalPosition(std::pair<double, double> solution)
{
	double newSolution = goalFunction(solution.first, solution.second);
	double oldSolution = goalFunction(bestPosition.first, bestPosition.second);
	if (oldSolution > newSolution)
		bestPosition = solution;
}
