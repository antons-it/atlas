/*
 * (C) Copyright 1996-2017 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#define BOOST_TEST_MODULE TestGmsh
#include "ecbuild/boost_test_framework.h"

#include "tests/TestMeshes.h"
#include "atlas/mesh/Mesh.h"
#include "atlas/output/Output.h"
#include "atlas/output/Gmsh.h"

#include "tests/AtlasFixture.h"

namespace atlas {
namespace test {

BOOST_GLOBAL_FIXTURE( AtlasFixture );

BOOST_AUTO_TEST_CASE( test_gmsh_output )
{
  mesh::Mesh::Ptr mesh = test::generate_mesh( grid::Grid("N128") );

  atlas::output::GmshFileStream file("bs.msh","w");
  output::Gmsh gmsh ( "test_gmsh_output.msh", util::Config
      ("binary",true)
      ("file","test_gmsh_output.msh") );
  gmsh.write(*mesh);
}

} // namespace test
} // namespace atlas
