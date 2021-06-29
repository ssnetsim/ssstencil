/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * - Neither the name of prim nor the names of its contributors may be used to
 * endorse or promote products derived from this software without specific prior
 * written permission.
 *
 * See the NOTICE file distributed with this work for additional information
 * regarding copyright ownership.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <cassert>
#include <cmath>
#include <fstream>
#include <memory>
#include <vector>

#include "prim/prim.h"
#include "tclap/CmdLine.h"

// This class provides a translation from 3D to 1D
struct Cube {
  u32 xn_;
  u32 yn_;
  u32 zn_;
  Cube(u32 _xn, u32 _yn, u32 _zn) : xn_(_xn), yn_(_yn), zn_(_zn) {}
  u32 id(u32 _x, u32 _y, u32 _z) const {
    assert(_x < xn_);
    assert(_y < yn_);
    assert(_z < zn_);
    u32 i = (((zn_ * yn_) * _z) + (yn_ * _y) + (_x));
    assert(i < (zn_ * yn_ * xn_));
    return i;
  }
};

s32 main(s32 _argc, char** _argv) {
  u32 xn;
  u32 yn;
  u32 zn;
  u32 face_msg_size;
  u32 edge_msg_size;
  u32 corner_msg_size;
  u32 bytes_per_flit;
  std::string output_file;
  u32 verbosity;
  try {
    TCLAP::CmdLine cmd(
        "Make ParaGraph representing a 27-Point Stencil Workloads", ' ', "1.0");
    TCLAP::UnlabeledValueArg<u32> xNodesArg(
        "x_nodes", "Number of nodes in the virtual x dimension", true, 0, "u32",
        cmd);
    TCLAP::UnlabeledValueArg<u32> yNodesArg(
        "y_nodes", "Number of nodes in the virtual y dimension", true, 0, "u32",
        cmd);
    TCLAP::UnlabeledValueArg<u32> zNodesArg(
        "z_nodes", "Number of nodes in the virtual z dimension", true, 0, "u32",
        cmd);
    TCLAP::UnlabeledValueArg<u32> faceMessageSizeArg(
        "face_msg_size", "Message size of face communications", true, 0, "u32",
        cmd);
    TCLAP::UnlabeledValueArg<u32> edgeMessageSizeArg(
        "edge_msg_size", "Message size of corner communications", true, 0,
        "u32", cmd);
    TCLAP::UnlabeledValueArg<u32> cornerMessageSizeArg(
        "corner_msg_size", "Message size of corner communications", true, 0,
        "u32", cmd);
    TCLAP::UnlabeledValueArg<u32> bytesPerFlitArg(
        "bytes_per_flit", "Bytes per flit", true, 0, "u32", cmd);
    TCLAP::UnlabeledValueArg<std::string> outputFileArg(
        "output_file", "Output csv file", true, "", "filename", cmd);
    TCLAP::ValueArg<u32> verbosityArg("v", "verbosity",
                                      "Configures the verbosity level", false,
                                      0, "u32", cmd);
    cmd.parse(_argc, _argv);
    xn = xNodesArg.getValue();
    yn = yNodesArg.getValue();
    zn = zNodesArg.getValue();
    face_msg_size = faceMessageSizeArg.getValue();
    edge_msg_size = edgeMessageSizeArg.getValue();
    corner_msg_size = cornerMessageSizeArg.getValue();
    bytes_per_flit = bytesPerFlitArg.getValue();
    output_file = outputFileArg.getValue();
    verbosity = verbosityArg.getValue();
  } catch (TCLAP::ArgException& e) {
    throw std::runtime_error(e.error().c_str());
  }

  if (verbosity > 0) {
    printf("xn=%u\n", xn);
    printf("yn=%u\n", yn);
    printf("zn=%u\n", zn);
    printf("face_msg_size=%u\n", face_msg_size);
    printf("edge_msg_size=%u\n", edge_msg_size);
    printf("corner_msg_size=%u\n", corner_msg_size);
    printf("bytes_per_flit=%u\n", bytes_per_flit);
    printf("output_file=%s\n", output_file.c_str());
    printf("\n");
  }
  assert(xn > 0);
  assert(yn > 0);
  assert(zn > 0);
  assert(face_msg_size > 0);
  assert(edge_msg_size > 0);
  assert(corner_msg_size > 0);
  assert(bytes_per_flit > 0);
  assert(output_file != "");

  // Configures the communication groups of the exchange operation
  if (verbosity > 0) {
    printf("Configuing communication groups for halo exchange\n");
  }
  Cube cube(xn, yn, zn);
  u32 nodes = xn * yn * zn;
  std::vector<std::vector<u32>> matrix(nodes, std::vector<u32>(nodes));
  for (u32 z = 0; z < zn; z++) {
    for (u32 y = 0; y < yn; y++) {
      for (u32 x = 0; x < xn; x++) {
        u32 me = cube.id(x, y, z);
        if (verbosity > 1) {
          printf("Node -> [%u,%u,%u] -> %u\n", x, y, z, me);
        }

        // Face neighbors
        if (x > 0) {
          u32 you = cube.id(x - 1, y, z);
          if (verbosity > 1) {
            printf("  Face -x -> [%u,%u,%u] -> %u\n", x - 1, y, z, you);
          }
          matrix.at(me).at(you) += face_msg_size;
        }
        if (x < xn - 1) {
          u32 you = cube.id(x + 1, y, z);
          if (verbosity > 1) {
            printf("  Face +x -> [%u,%u,%u] -> %u\n", x + 1, y, z, you);
          }
          matrix.at(me).at(you) += face_msg_size;
        }
        if (y > 0) {
          u32 you = cube.id(x, y - 1, z);
          if (verbosity > 1) {
            printf("  Face -y -> [%u,%u,%u] -> %u\n", x, y - 1, z, you);
          }
          matrix.at(me).at(you) += face_msg_size;
        }
        if (y < yn - 1) {
          u32 you = cube.id(x, y + 1, z);
          if (verbosity > 1) {
            printf("  Face +y -> [%u,%u,%u] -> %u\n", x, y + 1, z, you);
          }
          matrix.at(me).at(you) += face_msg_size;
        }
        if (z > 0) {
          u32 you = cube.id(x, y, z - 1);
          if (verbosity > 1) {
            printf("  Face -z -> [%u,%u,%u] -> %u\n", x, y, z - 1, you);
          }
          matrix.at(me).at(you) += face_msg_size;
        }
        if (z < zn - 1) {
          u32 you = cube.id(x, y, z + 1);
          if (verbosity > 1) {
            printf("  Face +z -> [%u,%u,%u] -> %u\n", x, y, z + 1, you);
          }
          matrix.at(me).at(you) += face_msg_size;
        }

        // Edge neighbors
        if (x > 0 && y > 0) {
          u32 you = cube.id(x - 1, y - 1, z);
          if (verbosity > 1) {
            printf("  Edge -x,-y -> [%u,%u,%u] -> %u\n", x - 1, y - 1, z, you);
          }
          matrix.at(me).at(you) += edge_msg_size;
        }
        if (x > 0 && y < yn - 1) {
          u32 you = cube.id(x - 1, y + 1, z);
          if (verbosity > 1) {
            printf("  Edge -x,+y -> [%u,%u,%u] -> %u\n", x - 1, y + 1, z, you);
          }
          matrix.at(me).at(you) += edge_msg_size;
        }
        if (x < xn - 1 && y > 0) {
          u32 you = cube.id(x + 1, y - 1, z);
          if (verbosity > 1) {
            printf("  Edge +x,-y -> [%u,%u,%u] -> %u\n", x + 1, y - 1, z, you);
          }
          matrix.at(me).at(you) += edge_msg_size;
        }
        if (x < xn - 1 && y < yn - 1) {
          u32 you = cube.id(x + 1, y + 1, z);
          if (verbosity > 1) {
            printf("  Edge +x,+y -> [%u,%u,%u] -> %u\n", x + 1, y + 1, z, you);
          }
          matrix.at(me).at(you) += edge_msg_size;
        }
        if (y > 0 && z > 0) {
          u32 you = cube.id(x, y - 1, z - 1);
          if (verbosity > 1) {
            printf("  Edge -y,-z -> [%u,%u,%u] -> %u\n", x, y - 1, z - 1, you);
          }
          matrix.at(me).at(you) += edge_msg_size;
        }
        if (y > 0 && z < zn - 1) {
          u32 you = cube.id(x, y - 1, z + 1);
          if (verbosity > 1) {
            printf("  Edge -y,+z -> [%u,%u,%u] -> %u\n", x, y - 1, z + 1, you);
          }
          matrix.at(me).at(you) += edge_msg_size;
        }
        if (y < yn - 1 && z > 0) {
          u32 you = cube.id(x, y + 1, z - 1);
          if (verbosity > 1) {
            printf("  Edge +y,-z -> [%u,%u,%u] -> %u\n", x, y + 1, z - 1, you);
          }
          matrix.at(me).at(you) += edge_msg_size;
        }
        if (y < yn - 1 && z < zn - 1) {
          u32 you = cube.id(x, y + 1, z + 1);
          if (verbosity > 1) {
            printf("  Edge +y,+z -> [%u,%u,%u] -> %u\n", x, y + 1, z + 1, you);
          }
          matrix.at(me).at(you) += edge_msg_size;
        }
        if (z > 0 && x > 0) {
          u32 you = cube.id(x - 1, y, z - 1);
          if (verbosity > 1) {
            printf("  Edge -z,-x -> [%u,%u,%u] -> %u\n", x - 1, y, z - 1, you);
          }
          matrix.at(me).at(you) += edge_msg_size;
        }
        if (z > 0 && x < xn - 1) {
          u32 you = cube.id(x + 1, y, z - 1);
          if (verbosity > 1) {
            printf("  Edge -z,+x -> [%u,%u,%u] -> %u\n", x + 1, y, z - 1, you);
          }
          matrix.at(me).at(you) += edge_msg_size;
        }
        if (z < zn - 1 && x > 0) {
          u32 you = cube.id(x - 1, y, z + 1);
          if (verbosity > 1) {
            printf("  Edge +z,-x -> [%u,%u,%u] -> %u\n", x - 1, y, z + 1, you);
          }
          matrix.at(me).at(you) += edge_msg_size;
        }
        if (z < zn - 1 && x < xn - 1) {
          u32 you = cube.id(x + 1, y, z + 1);
          if (verbosity > 1) {
            printf("  Edge +z,+x -> [%u,%u,%u] -> %u\n", x + 1, y, z + 1, you);
          }
          matrix.at(me).at(you) += edge_msg_size;
        }

        // Corner neighbors
        if (x > 0 && y > 0 && z > 0) {
          u32 you = cube.id(x - 1, y - 1, z - 1);
          if (verbosity > 1) {
            printf("  Corner -x,-y,-z -> [%u,%u,%u] -> %u\n", x - 1, y - 1,
                   z - 1, you);
          }
          matrix.at(me).at(you) += corner_msg_size;
        }
        if (x > 0 && y > 0 && z < zn - 1) {
          u32 you = cube.id(x - 1, y - 1, z + 1);
          if (verbosity > 1) {
            printf("  Corner -x,-y,+z -> [%u,%u,%u] -> %u\n", x - 1, y - 1,
                   z + 1, you);
          }
          matrix.at(me).at(you) += corner_msg_size;
        }
        if (x > 0 && y < yn - 1 && z > 0) {
          u32 you = cube.id(x - 1, y + 1, z - 1);
          if (verbosity > 1) {
            printf("  Corner -x,+y,-z -> [%u,%u,%u] -> %u\n", x - 1, y + 1,
                   z - 1, you);
          }
          matrix.at(me).at(you) += corner_msg_size;
        }
        if (x > 0 && y < yn - 1 && z < zn - 1) {
          u32 you = cube.id(x - 1, y + 1, z + 1);
          if (verbosity > 1) {
            printf("  Corner -x,+y,+z -> [%u,%u,%u] -> %u\n", x - 1, y + 1,
                   z + 1, you);
          }
          matrix.at(me).at(you) += corner_msg_size;
        }
        if (x < xn - 1 && y > 0 && z > 0) {
          u32 you = cube.id(x + 1, y - 1, z - 1);
          if (verbosity > 1) {
            printf("  Corner -x,-y,-z -> [%u,%u,%u] -> %u\n", x + 1, y - 1,
                   z - 1, you);
          }
          matrix.at(me).at(you) += corner_msg_size;
        }
        if (x < xn - 1 && y > 0 && z < zn - 1) {
          u32 you = cube.id(x + 1, y - 1, z + 1);
          if (verbosity > 1) {
            printf("  Corner -x,-y,+z -> [%u,%u,%u] -> %u\n", x + 1, y - 1,
                   z + 1, you);
          }
          matrix.at(me).at(you) += corner_msg_size;
        }
        if (x < xn - 1 && y < yn - 1 && z > 0) {
          u32 you = cube.id(x + 1, y + 1, z - 1);
          if (verbosity > 1) {
            printf("  Corner -x,+y,-z -> [%u,%u,%u] -> %u\n", x + 1, y + 1,
                   z - 1, you);
          }
          matrix.at(me).at(you) += corner_msg_size;
        }
        if (x < xn - 1 && y < yn - 1 && z < zn - 1) {
          u32 you = cube.id(x + 1, y + 1, z + 1);
          if (verbosity > 1) {
            printf("  Corner -x,+y,+z -> [%u,%u,%u] -> %u\n", x + 1, y + 1,
                   z + 1, you);
          }
          matrix.at(me).at(you) += corner_msg_size;
        }
      }
    }
  }

  // Writes the output matrix file
  if (verbosity > 0) {
    printf("Writing matrix to file: %s\n", output_file.c_str());
  }
  std::ofstream os(output_file);
  assert(os.is_open());
  assert(os.good());
  for (u32 me = 0; me < matrix.size(); me++) {
    for (u32 you = 0; you < matrix.at(me).size(); you++) {
      u32 bytes = matrix.at(me).at(you);
      u32 flits =
          static_cast<u32>(std::ceil(bytes / static_cast<f64>(bytes_per_flit)));
      os << flits;
      if (you < matrix.at(me).size() - 1) {
        os << ',';
      }
    }
    os << std::endl;
  }
  assert(os.good());

  return 0;
}
