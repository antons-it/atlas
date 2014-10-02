/*
 * (C) Copyright 1996-2014 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#ifndef atlas_Metadata_h
#define atlas_Metadata_h

#include <vector>
#include <map>
#include <string>

namespace atlas {
class Field;

/// @brief Contains a list of field-pointers, no ownership
class Metadata {

public:

	template<typename ValueT>
	Metadata& set(const std::string& name, const ValueT& value);

	template<typename ValueT>
	const ValueT& get(const std::string& name) const;

	template<typename ValueT>
	const ValueT& get(const std::string& name, const ValueT& deflault_value) const;

	template<typename ValueT>
	bool has(const std::string& name) const;

private:

	// std::map< std::string, void* >   dict_;
	std::map< std::string, bool >        map_bool_;
	std::map< std::string, int >         map_int_;
	std::map< std::string, float >       map_float_;
	std::map< std::string, double >      map_double_;
	std::map< std::string, std::string > map_string_;

};

// ------------------------------------------------------------------
// C wrapper interfaces to C++ routines
extern "C"
{
	Metadata* atlas__Metadata__new ();

	void atlas__Metadata__delete (Metadata* This);

	int	atlas__Metadata__has        (Metadata* This, const char* name);

	void atlas__Metadata__add_int   (Metadata* This, const char* name, int value);
	void atlas__Metadata__add_float (Metadata* This, const char* name, float value);
	void atlas__Metadata__add_double (Metadata* This, const char* name, double value);
	void atlas__Metadata__add_string (Metadata* This, const char* name, const char* value);

	int    atlas__Metadata__get_int    (Metadata* This, const char* name);
	float  atlas__Metadata__get_float  (Metadata* This, const char* name);
	double atlas__Metadata__get_double (Metadata* This, const char* name);
	const char* atlas__Metadata__get_string (Metadata* This, const char* name);

}
// ------------------------------------------------------------------

} // namespace atlas

#endif // Metadata_h
