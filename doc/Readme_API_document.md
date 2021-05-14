# PMlib API documentation


Detail specification of PMlib public class and private class APIs
is documented in Doxygen format.
This directory contains the Doxygen format documentation for PMlib public
and private class APIs.
	CMakeLists.txt
	Doxyfile
	api_cpp/
	api_fortran/

The HTML format documentation for Web browser viewing can be generated
in simple steps as described below.


1. Create a symbolic link to C++ headers. They are written according to
 the doxygen style format.

~~~
$ ln -s ../include api_cpp
~~~

2. Run doxygen command

~~~
$ doxygen
~~~

3. View the html/index.html file using a Web browser such as Firefox/Safari.

~~~
$ open html/index.html
~~~

The C++ Public Member Functions (C++ class API) can be found in
PMlib->Classes->Class List->pm_lib->PerfMonitor menu.

The Fortran Member Functions (Fortran subroutines) can be found in
PMlib->Files->File List->api_fortran->PMlib_Fortra_api.f90 menu.

