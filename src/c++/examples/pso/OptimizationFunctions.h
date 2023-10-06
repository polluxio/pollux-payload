// SPDX-FileCopyrightText: 2023 Pollux authors <https://github.com/polluxio/pollux-payload/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0
// Inspired by:
// https://github.com/Swistusmen/Particle-Swarm-Optimization

#include "Particle.h"
#include <memory>
#include <utility>
#include "Utilities.h"

SwarmOutputata SwarmOneThread(SwarmInputData data);

SwarmOutputata SwarmMultiThread(SwarmInputData data);
