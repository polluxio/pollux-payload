// SPDX-FileCopyrightText: 2023 Pollux authors <https://github.com/polluxio/pollux-payload/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#ifndef __POLLUX_METHODS_H_
#define __POLLUX_METHODS_H_

class PolluxPayload;

class Pollux {
  public:
    static int Main(int argc, char** argv, PolluxPayload* payload);
};

#endif /* __POLLUX_METHODS_H_ */
