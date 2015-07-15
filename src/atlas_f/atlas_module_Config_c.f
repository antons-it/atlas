! (C) Copyright 2013-2014 ECMWF.

! -----------------------------------------------------------------------------
! Config routines

function atlas_Config__ctor() result(Config)
  use atlas_Config_c_binding
  type(atlas_Config) :: Config
  Config%cpp_object_ptr = atlas__Config__new()
end function atlas_Config__ctor

function atlas_Config__ctor_from_json(json) result(Config)
  use atlas_Config_c_binding
  type(atlas_Config) :: Config
  class(atlas_JSON) :: json
  Config%cpp_object_ptr = atlas__Config__new_from_json(c_str(json%str()))
end function atlas_Config__ctor_from_json

function atlas_Config__ctor_from_file(path) result(Config)
  use atlas_Config_c_binding
  type(atlas_Config) :: Config
  class(atlas_PathName), intent(in) :: path
  Config%cpp_object_ptr = atlas__Config__new_from_file(c_str(path%str()))
end function atlas_Config__ctor_from_file

subroutine atlas_Config__delete(this)
  use atlas_Config_c_binding
  class(atlas_Config), intent(inout) :: this
  if ( c_associated(this%cpp_object_ptr) ) then
    call atlas__Config__delete(this%cpp_object_ptr)
  end if
  this%cpp_object_ptr = C_NULL_ptr
end subroutine atlas_Config__delete

subroutine atlas_ConfigList__delete(this)
  use atlas_Config_c_binding
  class(atlas_Config), intent(inout) :: this(:)
  integer :: j
  do j=1,size(this)
    call atlas_Config__delete(this(j))
  enddo
end subroutine atlas_ConfigList__delete

function atlas_Config__has(this, name) result(value)
  use atlas_Config_c_binding
  class(atlas_Config), intent(inout) :: this
  character(len=*), intent(in) :: name
  logical :: value
  integer :: value_int
  value_int =  atlas__Config__has(this%cpp_object_ptr, c_str(name) )
  if( value_int == 1 ) then
    value = .True.
  else
    value = .False.
  end if
end function atlas_Config__has

subroutine atlas_Config__set_config(this, name, value)
  use atlas_Config_c_binding
  class(atlas_Config), intent(inout) :: this
  character(len=*), intent(in) :: name
  class(atlas_Config), intent(in) :: value
  call atlas__Config__set_config(this%cpp_object_ptr, c_str(name), value%cpp_object_ptr )
end subroutine atlas_Config__set_config

subroutine atlas_Config__set_config_list(this, name, value)
  use atlas_Config_c_binding
  class(atlas_Config), intent(inout) :: this
  character(len=*), intent(in) :: name
  class(atlas_Config), intent(in) :: value(:)
  type(c_ptr), target :: value_cptrs(size(value))
  integer :: j
  if( size(value) > 0 ) then
    do j=1,size(value)
      value_cptrs(j) = value(j)%cpp_object_ptr
    enddo
    call atlas__Config__set_config_list(this%cpp_object_ptr, c_str(name), c_loc(value_cptrs(1)), size(value_cptrs) )
  endif
end subroutine atlas_Config__set_config_list

subroutine atlas_Config__set_logical(this, name, value)
  use atlas_Config_c_binding
  class(atlas_Config), intent(inout) :: this
  character(len=*), intent(in) :: name
  logical, intent(in) :: value
  integer :: value_int
  if( value ) then
    value_int = 1
  else
    value_int = 0
  end if
  call atlas__Config__set_int(this%cpp_object_ptr, c_str(name), value_int )
end subroutine atlas_Config__set_logical

subroutine atlas_Config__set_int32(this, name, value)
  use atlas_Config_c_binding
  class(atlas_Config), intent(inout) :: this
  character(len=*), intent(in) :: name
  integer, intent(in) :: value
  call atlas__Config__set_int(this%cpp_object_ptr, c_str(name), value)
end subroutine atlas_Config__set_int32

subroutine atlas_Config__set_real32(this, name, value)
  use atlas_Config_c_binding
  class(atlas_Config), intent(inout) :: this
  character(len=*), intent(in) :: name
  real(c_float), intent(in) :: value
  call atlas__Config__set_float(this%cpp_object_ptr, c_str(name) ,value)
end subroutine atlas_Config__set_real32

subroutine atlas_Config__set_real64(this, name, value)
  use atlas_Config_c_binding
  class(atlas_Config), intent(inout) :: this
  character(len=*), intent(in) :: name
  real(c_double), intent(in) :: value
  call atlas__Config__set_double(this%cpp_object_ptr, c_str(name) ,value)
end subroutine atlas_Config__set_real64

subroutine atlas_Config__set_string(this, name, value)
  use atlas_Config_c_binding
  class(atlas_Config), intent(inout) :: this
  character(len=*), intent(in) :: name
  character(len=*), intent(in) :: value
  call atlas__Config__set_string(this%cpp_object_ptr, c_str(name) , c_str(value) )
end subroutine atlas_Config__set_string

function atlas_Config__get_config(this, name, value) result(found)
  use atlas_Config_c_binding
  logical :: found
  class(atlas_Config), intent(in) :: this
  character(len=*), intent(in) :: name
  class(atlas_Config), intent(inout) :: value
  integer :: found_int
  if( .not. c_associated(value%cpp_object_ptr) ) then
    value%cpp_object_ptr = atlas__Config__new()
  endif
  found_int = atlas__Config__get_config(this%cpp_object_ptr, c_str(name), value%cpp_object_ptr )
  found = .False.
  if (found_int == 1) found = .True.
end function atlas_Config__get_config

function atlas_Config__get_config_list(this, name, value) result(found)
  use atlas_Config_c_binding
  logical :: found
  class(atlas_Config), intent(in) :: this
  character(len=*), intent(in) :: name
  type(atlas_Config), allocatable, intent(inout) :: value(:)
  type(c_ptr) :: value_list_cptr
  type(c_ptr), pointer :: value_cptrs(:)
  integer :: value_list_allocated
  integer :: value_list_size
  integer :: found_int
  integer :: j
  value_list_cptr = c_null_ptr
  found_int = atlas__Config__get_config_list(this%cpp_object_ptr, c_str(name), &
    & value_list_cptr, value_list_size, value_list_allocated )
  if( found_int == 1 ) then
    call c_f_pointer(value_list_cptr,value_cptrs,(/value_list_size/))
    if( allocated(value) ) deallocate(value)
    allocate(value(value_list_size))
    do j=1,value_list_size
      value(j)%cpp_object_ptr = value_cptrs(j)
    enddo
    if( value_list_allocated == 1 ) call atlas_free(value_list_cptr)
  endif
  found = .False.
  if (found_int == 1) found = .True.
end function atlas_Config__get_config_list

function atlas_Config__get_logical(this, name, value) result(found)
  use atlas_Config_c_binding
  logical :: found
  class(atlas_Config), intent(in) :: this
  character(len=*), intent(in) :: name
  logical, intent(inout) :: value
  integer :: value_int
  integer :: found_int
  found_int = atlas__Config__get_int(this%cpp_object_ptr,c_str(name), value_int )
  if (value_int > 0) then
    value = .True.
  else
    value = .False.
  end if
  found = .False.
  if (found_int == 1) found = .True.
end function atlas_Config__get_logical

function atlas_Config__get_int32(this, name, value) result(found)
  use atlas_Config_c_binding
  logical :: found
  class(atlas_Config), intent(in) :: this
  character(len=*), intent(in) :: name
  integer, intent(inout) :: value
  integer :: found_int
  found_int = atlas__Config__get_int(this%cpp_object_ptr, c_str(name), value )
  found = .False.
  if (found_int == 1) found = .True.
end function atlas_Config__get_int32

function atlas_Config__get_real32(this, name, value) result(found)
  use atlas_Config_c_binding
  logical :: found
  class(atlas_Config), intent(in) :: this
  character(len=*), intent(in) :: name
  real(c_float), intent(inout) :: value
  integer :: found_int
  found_int = atlas__Config__get_float(this%cpp_object_ptr, c_str(name), value )
  found = .False.
  if (found_int == 1) found = .True.
end function atlas_Config__get_real32

function atlas_Config__get_real64(this, name, value) result(found)
  use atlas_Config_c_binding
  logical :: found
  class(atlas_Config), intent(in) :: this
  character(len=*), intent(in) :: name
  real(c_double), intent(inout) :: value
  integer :: found_int
  found_int = atlas__Config__get_double(this%cpp_object_ptr, c_str(name), value )
  found = .False.
  if (found_int == 1) found = .True.
end function atlas_Config__get_real64

function atlas_Config__get_string(this, name, value) result(found)
  use atlas_Config_c_binding
  logical :: found
  class(atlas_Config), intent(in) :: this
  character(len=*), intent(in) :: name
  character(len=:), allocatable, intent(inout) :: value
  type(c_ptr) :: value_cptr
  integer :: found_int
  integer(c_int) :: value_size
  integer(c_int) :: value_allocated
  found_int = atlas__Config__get_string(this%cpp_object_ptr,c_str(name),value_cptr,value_size,value_allocated)
  if( found_int == 1 ) then
    if( allocated(value) ) deallocate(value)
    allocate(character(len=value_size) :: value )
    value = c_to_f_string_cptr(value_cptr)
    if( value_allocated == 1 ) call atlas_free(value_cptr)
  endif
  found = .False.
  if (found_int == 1) found = .True.
end function atlas_Config__get_string

subroutine atlas_Config__set_array_int32(this, name, value)
  use atlas_Config_c_binding
  class(atlas_Config), intent(in) :: this
  character(len=*), intent(in) :: name
  integer(c_int), intent(in) :: value(:)
  call atlas__Config__set_array_int(this%cpp_object_ptr, c_str(name), &
    & value, size(value) )
end subroutine atlas_Config__set_array_int32

subroutine atlas_Config__set_array_int64(this, name, value)
  use atlas_Config_c_binding
  class(atlas_Config), intent(in) :: this
  character(len=*), intent(in) :: name
  integer(c_long), intent(in) :: value(:)
  call atlas__Config__set_array_long(this%cpp_object_ptr, c_str(name), &
    & value, size(value) )
end subroutine atlas_Config__set_array_int64

subroutine atlas_Config__set_array_real32(this, name, value)
  use atlas_Config_c_binding
  class(atlas_Config), intent(in) :: this
  character(len=*), intent(in) :: name
  real(c_float), intent(in) :: value(:)
  call atlas__Config__set_array_float(this%cpp_object_ptr, c_str(name), &
    & value, size(value) )
end subroutine atlas_Config__set_array_real32

subroutine atlas_Config__set_array_real64(this, name, value)
  use atlas_Config_c_binding
  class(atlas_Config), intent(in) :: this
  character(len=*), intent(in) :: name
  real(c_double), intent(in) :: value(:)
  call atlas__Config__set_array_double(this%cpp_object_ptr, c_str(name), &
    & value, size(value) )
end subroutine atlas_Config__set_array_real64

function atlas_Config__get_array_int32(this, name, value) result(found)
  use atlas_Config_c_binding
  logical :: found
  class(atlas_Config), intent(in) :: this
  character(len=*), intent(in) :: name
  integer(c_int), allocatable, intent(inout) :: value(:)
  type(c_ptr) :: value_cptr
  integer(c_int), pointer :: value_fptr(:)
  integer :: value_size
  integer :: value_allocated
  integer :: found_int
  found_int = atlas__Config__get_array_int(this%cpp_object_ptr, c_str(name), &
    & value_cptr, value_size, value_allocated )
  if (found_int ==1 ) then
    call c_f_pointer(value_cptr,value_fptr,(/value_size/))
    if( allocated(value) ) deallocate(value)
    allocate(value(value_size))
    value(:) = value_fptr(:)
    if( value_allocated == 1 ) call atlas_free(value_cptr)
  endif
  found = .False.
  if (found_int == 1) found = .True.
end function atlas_Config__get_array_int32

function atlas_Config__get_array_int64(this, name, value) result(found)
  use atlas_Config_c_binding
  logical :: found
  class(atlas_Config), intent(in) :: this
  character(len=*), intent(in) :: name
  integer(c_long), allocatable, intent(inout) :: value(:)
  type(c_ptr) :: value_cptr
  integer(c_long), pointer :: value_fptr(:)
  integer :: value_size
  integer :: value_allocated
  integer :: found_int
  found_int = atlas__Config__get_array_long(this%cpp_object_ptr, c_str(name), &
    & value_cptr, value_size, value_allocated )
  if (found_int == 1) then
    call c_f_pointer(value_cptr,value_fptr,(/value_size/))
    if( allocated(value) ) deallocate(value)
    allocate(value(value_size))
    value(:) = value_fptr(:)
    if( value_allocated == 1 ) call atlas_free(value_cptr)
  endif
  found = .False.
  if (found_int == 1) found = .True.
end function atlas_Config__get_array_int64

function atlas_Config__get_array_real32(this, name, value) result(found)
  use atlas_Config_c_binding
  logical :: found
  class(atlas_Config), intent(in) :: this
  character(len=*), intent(in) :: name
  real(c_float), allocatable, intent(inout) :: value(:)
  type(c_ptr) :: value_cptr
  real(c_float), pointer :: value_fptr(:)
  integer :: value_size
  integer :: value_allocated
  integer :: found_int
  found_int = atlas__Config__get_array_float(this%cpp_object_ptr, c_str(name), &
    & value_cptr, value_size, value_allocated )
  if (found_int == 1 ) then
    call c_f_pointer(value_cptr,value_fptr,(/value_size/))
    if( allocated(value) ) deallocate(value)
    allocate(value(value_size))
    value(:) = value_fptr(:)
    if( value_allocated == 1 ) call atlas_free(value_cptr)
  endif
  found = .False.
  if (found_int == 1) found = .True.
end function atlas_Config__get_array_real32

function atlas_Config__get_array_real64(this, name, value) result(found)
  use atlas_Config_c_binding
  logical :: found
  class(atlas_Config), intent(in) :: this
  character(len=*), intent(in) :: name
  real(c_double), allocatable, intent(inout) :: value(:)
  type(c_ptr) :: value_cptr
  real(c_double), pointer :: value_fptr(:)
  integer :: value_size
  integer :: value_allocated
  integer :: found_int
  found_int = atlas__Config__get_array_double(this%cpp_object_ptr, c_str(name), &
    & value_cptr, value_size, value_allocated )
  if (found_int == 1) then
    call c_f_pointer(value_cptr,value_fptr,(/value_size/))
    if( allocated(value) ) deallocate(value)
    allocate(value(value_size))
    value(:) = value_fptr(:)
    if( value_allocated == 1 ) call atlas_free(value_cptr)
  endif
  found = .False.
  if (found_int == 1) found = .True.
end function atlas_Config__get_array_real64

function atlas_Config__json(this) result(json)
  use atlas_Config_c_binding
  character(len=:), allocatable :: json
  class(atlas_Config), intent(in) :: this
  type(c_ptr) :: json_cptr
  integer(c_int) :: json_size
  integer(c_int) :: json_allocated
  call atlas__Config__json(this%cpp_object_ptr,json_cptr,json_size,json_allocated)
  allocate(character(len=json_size) :: json )
  json = c_to_f_string_cptr(json_cptr)
  if( json_allocated == 1 ) call atlas_free(json_cptr)
end function atlas_Config__json

