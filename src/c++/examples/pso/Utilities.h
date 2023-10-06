#pragma once
#include <array>
#include <functional>
#include <random>

// SPDX-FileCopyrightText: 2023 Pollux authors <https://github.com/polluxio/pollux-payload/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0
// Inspired by:
// https://github.com/Swistusmen/Particle-Swarm-Optimization

#define EPSILON 0.001
#define EPSILON_EXP 1000
#define VELOCITY_SCOPE 100
#define SIGMA 0.5

struct SwarmInputData {
	std::function<double(double, double)> goalFunction;
	std::vector<double> X;
	std::vector<double> Y;
	int iterations;
	int noParticles;
	int threads;
};

struct SwarmOutputata {
	double x, y, z;
	std::vector<std::vector<double>> minimums;
};