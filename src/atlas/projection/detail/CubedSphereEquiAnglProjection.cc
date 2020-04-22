/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "CubedSphereEquiAnglProjection.h"

#include <cmath>
#include <iostream>

#include "eckit/config/Parametrisation.h"
#include "eckit/types/FloatCompare.h"
#include "eckit/utils/Hash.h"

#include "atlas/projection/detail/ProjectionFactory.h"
#include "atlas/projection/detail/ProjectionUtilities.h"
#include "atlas/runtime/Exception.h"
#include "atlas/util/Config.h"
#include "atlas/util/Constants.h"
#include "atlas/util/CoordinateEnums.h"
#include "atlas/util/Earth.h"

namespace atlas {
namespace projection {
namespace detail {

// -------------------------------------------------------------------------------------------------

CubedSphereEquiAnglProjection::CubedSphereEquiAnglProjection( const eckit::Parametrisation& params )
                                                               : CubedSphereProjectionBase(params) {
  // Get Base data
  const auto cubeNx = getCubeNx();
  auto tile1Lats = getLatArray();
  auto tile1Lons = getLonArray();

  // Cartesian coordinate starting points
  const double rsq3 = 1.0/sqrt(3.0);

  // Equiangular
  const double dp = 0.5*M_PI/static_cast<double>(cubeNx);

  double p1;
  double p2;
  double p3;
  double lonlat[2];

  for ( int ix = 0; ix < cubeNx+1; ix++ ) {
    for ( int iy = 0; iy < cubeNx+1; iy++ ) {
      // Grid points in cartesian coordinates
      double p1 = -rsq3;
      double p2 = -rsq3*tan(-0.25*M_PI+static_cast<double>(ix)*dp);
      double p3 =  rsq3*tan(-0.25*M_PI+static_cast<double>(iy)*dp);

      ProjectionUtilities::cartesianToLatLon(p1, p2, p3, lonlat);

      tile1Lons(ix, iy) = lonlat[LON] - M_PI;
      tile1Lats(ix, iy) = lonlat[LAT];
    }
  }
}

// -------------------------------------------------------------------------------------------------

void CubedSphereEquiAnglProjection::lonlat2xy( double crd[] ) const {
  CubedSphereProjectionBase::lonlat2xy(crd);
}

// -------------------------------------------------------------------------------------------------

void CubedSphereEquiAnglProjection::xy2lonlat( double crd[] ) const {
  CubedSphereProjectionBase::xy2lonlat(crd);
}

// -------------------------------------------------------------------------------------------------

CubedSphereEquiAnglProjection::Spec CubedSphereEquiAnglProjection::spec() const {
  // Fill projection specification
  Spec proj;
  proj.set( "type", static_type() );
  return proj;
}

// -------------------------------------------------------------------------------------------------

void CubedSphereEquiAnglProjection::hash( eckit::Hash& h ) const {
  // Add to hash
  h.add( static_type() );
}

// -------------------------------------------------------------------------------------------------

namespace {
static ProjectionBuilder<CubedSphereEquiAnglProjection>
       register_1( CubedSphereEquiAnglProjection::static_type() );
}

}  // namespace detail
}  // namespace projection
}  // namespace atlas
