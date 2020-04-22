/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <vector>

#include "eckit/utils/Hash.h"

#include "atlas/field/Field.h"
#include "atlas/grid/CubedSphereGrid.h"
#include "atlas/grid/Distribution.h"
#include "atlas/grid/Partitioner.h"
#include "atlas/library/config.h"
#include "atlas/mesh/ElementType.h"
#include "atlas/mesh/Elements.h"
#include "atlas/mesh/Mesh.h"
#include "atlas/mesh/Nodes.h"
#include "atlas/meshgenerator/detail/MeshGeneratorFactory.h"
#include "atlas/meshgenerator/detail/CubedSphereMeshGenerator.h"
#include "atlas/parallel/mpi/mpi.h"
#include "atlas/runtime/Exception.h"
#include "atlas/runtime/Log.h"
#include "atlas/util/Constants.h"
#include "atlas/util/CoordinateEnums.h"

#define DEBUG_OUTPUT 0
#define DEBUG_OUTPUT_DETAIL 0

using Topology = atlas::mesh::Nodes::Topology;

namespace atlas {
namespace meshgenerator {

// -------------------------------------------------------------------------------------------------

CubedSphereMeshGenerator::CubedSphereMeshGenerator( const eckit::Parametrisation& p ) {}

// -------------------------------------------------------------------------------------------------

void CubedSphereMeshGenerator::configure_defaults() {}

// -------------------------------------------------------------------------------------------------

void CubedSphereMeshGenerator::generate( const Grid& grid, Mesh& mesh ) const {


  // Check for proper grid and need for mesh
  ATLAS_ASSERT( !mesh.generated() );
  const CubedSphereGrid csg = CubedSphereGrid( grid );
  if ( !csg ) {
      throw_Exception( "CubedSphereMeshGenerator can only work with a cubed-sphere grid", Here() );
  }

  // Number of processors
  //size_t nb_parts = options.get<size_t>( "nb_parts" );

  // Decomposition type
  std::string partitioner_type = "checkerboard";
  options.get( "checkerboard", partitioner_type );

  // Partitioner
  grid::Partitioner partitioner( partitioner_type, 1 );
  grid::Distribution distribution( partitioner.partition( grid ) );

  generate( grid, distribution, mesh );

}

// -------------------------------------------------------------------------------------------------

void CubedSphereMeshGenerator::setNeighbours(int it, int ix, int iy, int icell, idx_t quad_nodes[],
                   array::ArrayView<int, 3> NodeArray, atlas::mesh::HybridElements::Connectivity node_connectivity,
                   array::ArrayView<int, 1> cells_part) const {

  quad_nodes[0] = NodeArray(it, ix  , iy  );
  quad_nodes[1] = NodeArray(it, ix  , iy+1);
  quad_nodes[2] = NodeArray(it, ix+1, iy+1);
  quad_nodes[3] = NodeArray(it, ix+1, iy  );

  node_connectivity.set( icell, quad_nodes );
  cells_part( icell ) = 0;

  ++icell;

}

// -------------------------------------------------------------------------------------------------

void CubedSphereMeshGenerator::generate( const Grid& grid, const grid::Distribution& distribution,
                                         Mesh& mesh ) const {

  const auto csgrid = CubedSphereGrid( grid );

  int cubeNx = csgrid.GetCubeNx();

  ATLAS_TRACE( "Number of faces per tile = " + std::to_string(cubeNx) );

  // Number of nodes
  int nnodes = 6*cubeNx*cubeNx+2;             // Number of unique grid nodes
  int nnodes_all = 6*(cubeNx+1)*(cubeNx+1);   // Number of grid nodes including edge and corner duplicates
  int ncells = 6*cubeNx*cubeNx;               // Number of unique grid cells

  // Construct mesh nodes
  // --------------------
  mesh.nodes().resize( nnodes );
  mesh::Nodes& nodes = mesh.nodes();
  auto xy         = array::make_view<double, 2>( nodes.xy() );
  auto lonlat     = array::make_view<double, 2>( nodes.lonlat() );
  auto glb_idx    = array::make_view<gidx_t, 1>( nodes.global_index() );
  auto remote_idx = array::make_indexview<idx_t, 1>( nodes.remote_index() );
  auto part       = array::make_view<int, 1>( nodes.partition() );
  auto ghost      = array::make_view<int, 1>( nodes.ghost() );
  auto flags      = array::make_view<int, 1>( nodes.flags() );

  int inode = 0;
  double xy_[3];
  double lonlat_[2];

  int it;
  int ix;
  int iy;

  // Loop over entire grid
  // ---------------------
  array::ArrayT<int> NodeArrayT( 6, cubeNx+1, cubeNx+1 ); // All grid points including duplicates
  array::ArrayView<int, 3> NodeArray = array::make_view<int, 3>( NodeArrayT );

  for ( it = 0; it < 6; it++ ) {
    for ( ix = 0; ix < cubeNx+1; ix++ ) {
      for ( iy = 0; iy < cubeNx+1; iy++ ) {
        NodeArray(it, ix, iy) = -9999;
      }
    }
  }

  for ( it = 0; it < 6; it++ ) {               // 0, 1, 2, 3, 4, 5
    for ( ix = 0; ix < cubeNx; ix++ ) {        // 0, 1, ..., cubeNx-1
      for ( iy = 0; iy < cubeNx; iy++ ) {      // 0, 1, ..., cubeNx-1

        // Get xy from global xy grid array
        csgrid.xy( ix, iy, it, xy_ );

        xy( inode, LON ) = xy_[LON];
        xy( inode, LAT ) = xy_[LAT];

        csgrid.lonlat( ix, iy, it, lonlat_ );

        lonlat( inode, LON ) = lonlat_[LON] * util::Constants::radiansToDegrees();
        lonlat( inode, LAT ) = lonlat_[LAT] * util::Constants::radiansToDegrees();

        // Ghost nodes
        ghost(inode) = 0; // No ghost nodes

        glb_idx(inode) = inode;
        remote_idx(inode) = inode;
        part(inode) = 0;

        NodeArray(it, ix, iy) = inode;

        ++inode;

      }
    }
  }

  // Extra point 1
  // -------------
  it = 0;
  ix = 0;
  iy = cubeNx;

  csgrid.xy( ix, iy, it, xy_ );
  xy( inode, LON ) = xy_[LON];
  xy( inode, LAT ) = xy_[LAT];
  csgrid.lonlat( ix, iy, it, lonlat_ );
  lonlat( inode, LON ) = lonlat_[LON] * util::Constants::radiansToDegrees();
  lonlat( inode, LAT ) = lonlat_[LAT] * util::Constants::radiansToDegrees();
  ghost(inode) = 0;
  glb_idx(inode) = inode;
  remote_idx(inode) = inode;
  part(inode) = 0;
  NodeArray(it, ix, iy) = inode;

  ++inode;

  // Extra point 2
  // -------------
  it = 3;
  ix = cubeNx;
  iy = 0;

  csgrid.xy( ix, iy, it, xy_ );
  xy( inode, LON ) = xy_[LON];
  xy( inode, LAT ) = xy_[LAT];
  csgrid.lonlat( ix, iy, it, lonlat_ );
  lonlat( inode, LON ) = lonlat_[LON] * util::Constants::radiansToDegrees();
  lonlat( inode, LAT ) = lonlat_[LAT] * util::Constants::radiansToDegrees();
  ghost(inode) = 0;
  glb_idx(inode) = inode;
  remote_idx(inode) = inode;
  part(inode) = 0;
  NodeArray(it, ix, iy) = inode;

  ++inode;

  // Assert that the correct number of nodes have been set
  ATLAS_ASSERT( nnodes == inode, "Insufficient nodes" );

  // Fill duplicate points in the corners
  // ------------------------------------
  NodeArray(0, cubeNx, cubeNx) = NodeArray(2, 0, 0); ++inode;
  NodeArray(1, cubeNx, cubeNx) = NodeArray(3, 0, 0); ++inode;
  NodeArray(2, cubeNx, cubeNx) = NodeArray(4, 0, 0); ++inode;
  NodeArray(3, cubeNx, cubeNx) = NodeArray(5, 0, 0); ++inode;
  NodeArray(4, cubeNx, cubeNx) = NodeArray(0, 0, 0); ++inode;
  NodeArray(5, cubeNx, cubeNx) = NodeArray(1, 0, 0); ++inode;

  // Special points have two duplicates each
  NodeArray(2, 0, cubeNx) = NodeArray(0, 0, cubeNx); ++inode;
  NodeArray(4, 0, cubeNx) = NodeArray(0, 0, cubeNx); ++inode;
  NodeArray(1, cubeNx, 0) = NodeArray(3, cubeNx, 0); ++inode;
  NodeArray(5, cubeNx, 0) = NodeArray(3, cubeNx, 0); ++inode;

  // Top & right duplicates
  // ----------------------

  // Tile 1
  for ( ix = 1; ix < cubeNx; ix++ ) {
    NodeArray(0, ix, cubeNx) = NodeArray(2, 0, cubeNx-ix); ++inode;
  }
  for ( iy = 0; iy < cubeNx; iy++ ) {
    NodeArray(0, cubeNx, iy) = NodeArray(1, 0, iy); ++inode;
  }

  // Tile 2
  for ( ix = 0; ix < cubeNx; ix++ ) {
    NodeArray(1, ix, cubeNx) = NodeArray(2, ix, 0); ++inode;
  }
  for ( iy = 1; iy < cubeNx; iy++ ) {
    NodeArray(1, cubeNx, iy) = NodeArray(3, cubeNx-iy, 0); ++inode;
  }

  // Tile 3
  for ( ix = 1; ix < cubeNx; ix++ ) {
    NodeArray(2, ix, cubeNx) = NodeArray(4, 0, cubeNx-ix); ++inode;
  }
  for ( iy = 0; iy < cubeNx; iy++ ) {
    NodeArray(2, cubeNx, iy) = NodeArray(3, 0, iy); ++inode;
  }

  // Tile 4
  for ( ix = 0; ix < cubeNx; ix++ ) {
    NodeArray(3, ix, cubeNx) = NodeArray(4, ix, 0); ++inode;
  }
  for ( iy = 1; iy < cubeNx; iy++ ) {
    NodeArray(3, cubeNx, iy) = NodeArray(5, cubeNx-iy, 0); ++inode;
  }

  // Tile 5
  for ( ix = 1; ix < cubeNx; ix++ ) {
    NodeArray(4, ix, cubeNx) = NodeArray(0, 0, cubeNx-ix); ++inode;
  }
  for ( iy = 0; iy < cubeNx; iy++ ) {
    NodeArray(4, cubeNx, iy) = NodeArray(5, 0, iy); ++inode;
  }

  // Tile 6
  for ( ix = 0; ix < cubeNx; ix++ ) {
    NodeArray(5, ix, cubeNx) = NodeArray(0, ix, 0); ++inode;
  }
  for ( iy = 1; iy < cubeNx; iy++ ) {
    NodeArray(5, cubeNx, iy) = NodeArray(1, cubeNx-iy, 0); ++inode;
  }

  // Assert that the correct number of nodes have been set when duplicates are added
  ATLAS_ASSERT( nnodes_all == inode, "Insufficient nodes" );

  for ( it = 0; it < 6; it++ ) {
    for ( ix = 0; ix < cubeNx+1; ix++ ) {
      for ( iy = 0; iy < cubeNx+1; iy++ ) {
        ATLAS_ASSERT( NodeArray(it, ix, iy) != -9999, "Node Array Not Set Properly" );
      }
    }
  }

  // Cells in mesh
  mesh.cells().add( new mesh::temporary::Quadrilateral(), 6*cubeNx*cubeNx );
  int quad_begin  = mesh.cells().elements( 0 ).begin();
  array::ArrayView<int, 1> cells_part = array::make_view<int, 1>( mesh.cells().partition() );
  atlas::mesh::HybridElements::Connectivity& node_connectivity = mesh.cells().node_connectivity();

  int icell = 0;
  idx_t quad_nodes[4];

  for ( int it = 0; it < 6; it++ ) {
    for ( int ix = 0; ix < cubeNx; ix++ ) {
      for ( int iy = 0; iy < cubeNx; iy++ ) {

        //this->setNeighbours(it, ix, iy, icell, quad_nodes, NodeArray, node_connectivity, cells_part);

        quad_nodes[0] = NodeArray(it, ix  , iy  );
        quad_nodes[1] = NodeArray(it, ix  , iy+1);
        quad_nodes[2] = NodeArray(it, ix+1, iy+1);
        quad_nodes[3] = NodeArray(it, ix+1, iy  );

        node_connectivity.set( icell, quad_nodes );
        cells_part( icell ) = 0;

        ++icell;

      }
    }
  }

  // Assertion that correct number of cells are set
  ATLAS_ASSERT( ncells == icell, "Insufficient nodes" );

  // Parallel
  generateGlobalElementNumbering( mesh );
  nodes.metadata().set( "parallel", true );

}

// -------------------------------------------------------------------------------------------------

void CubedSphereMeshGenerator::hash( eckit::Hash& h ) const {
    h.add( "CubedSphereMeshGenerator" );
    options.hash( h );
}

// -------------------------------------------------------------------------------------------------

namespace {
  static MeshGeneratorBuilder<CubedSphereMeshGenerator> CubedSphereMeshGenerator( "cubedsphere" );
}

// -------------------------------------------------------------------------------------------------

}  // namespace meshgenerator
}  // namespace atlas
