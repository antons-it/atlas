/*
 * (C) Copyright 1996-2017 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#include "eckit/exception/Exceptions.h"
#include "atlas/library/config.h"
#include "atlas/numerics/Method.h"
#include "atlas/runtime/ErrorHandling.h"

namespace atlas {
namespace numerics {

//----------------------------------------------------------------------------------------------------------------------

// C wrapper interfaces to C++ routines
extern "C" {
 void atlas__Method__delete (Method* This)
{
  ATLAS_ERROR_HANDLING(
    ASSERT( This );
    delete This;
    This = 0;
  );
}

const char* atlas__Method__name (Method* This) {
  ATLAS_ERROR_HANDLING(
    ASSERT( This );
    return This->name().c_str();
  );
  return 0;
}
}

// ------------------------------------------------------------------

} // namespace numerics
} // namespace atlas

