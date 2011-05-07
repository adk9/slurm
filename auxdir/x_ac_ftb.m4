##*****************************************************************************
#  AUTHOR:
#    Abhishek Kulkarni <adkulkar@osl.iu.edu>
#
#  SYNOPSIS:
#    X_AC_FTB()
#
#  DESCRIPTION:
#    Determine if the FTB libraries exist,
#    updating CPPFLAGS and LDFLAGS as necessary.
##*****************************************************************************

AC_DEFUN([X_AC_FTB], [

  _x_ac_ftb_dirs="/usr /usr/local /opt/ftb"
  _x_ac_ftb_libs="lib64 lib"

  AC_ARG_WITH(
    [ftb],
    AS_HELP_STRING(--with-ftb=PATH,Specify path to FTB installation),
    [_x_ac_ftb_dirs="$withval $_x_ac_ftb_dirs"])

  AC_CACHE_CHECK(
    [for FTB installation],
    [x_ac_cv_ftb_dir],
    [
      for d in $_x_ac_ftb_dirs; do
	test -d "$d" || continue
	test -d "$d/include" || continue
	test -f "$d/include/ftb.h" || continue
	for bit in $_x_ac_ftb_libs; do
	  test -d "$d/$bit" || continue
          _x_ac_ftb_cppflags_save="$CPPFLAGS"
          CPPFLAGS="-I$d/include $CPPFLAGS"
 	  _x_ac_ftb_libs_save="$LIBS"
	  LIBS="-L$d/$bit -lftb $LIBS"
	  AC_LINK_IFELSE(
	    AC_LANG_CALL([], FTB_Connect),
	    AS_VAR_SET(x_ac_cv_ftb_dir, $d))
          CPPFLAGS="$_x_ac_ftb_cppflags_save"
	  LIBS="$_x_ac_ftb_libs_save"
	  test -n "$x_ac_cv_ftb_dir" && break
	done
	test -n "$x_ac_cv_ftb_dir" && break
      done
    ])

  if test -z "$x_ac_cv_ftb_dir"; then
    AC_MSG_WARN([unable to locate FTB installation])
  else
    FTB_CPPFLAGS="-I$x_ac_cv_ftb_dir/include"
    FTB_LDFLAGS="-L$x_ac_cv_ftb_dir/$bit"
    FTB_LIBS="-lftb"
  fi

  AC_DEFINE(HAVE_FTB, 1, [Define to 1 if FTB library found])

  AC_SUBST(FTB_LIBS)
  AC_SUBST(FTB_CPPFLAGS)
  AC_SUBST(FTB_LDFLAGS)

  AM_CONDITIONAL(WITH_FTB, test -n "$x_ac_cv_ftb_dir")
])
