dnl $Id: config.m4 262896 2014-07-20 19:11:16Z bgriffith $
dnl config.m4 for extension fsevents

PHP_ARG_ENABLE(fsevents, whether to enable fsevents support,
[  --enable-fsevents        Enable fsevents support])

if test "$PHP_FSEVENTS" != "no"; then

  AC_TRY_RUN([
   #include <CoreServices/CoreServices.h>
   void testfunc(int (*passedfunc)()) {
   }
   int main() {
    testfunc(FSEventStreamCreate);
    return 0;
   }
  ],[],[
   AC_MSG_ERROR(Your system does not support fsevents)
  ])

  PHP_NEW_EXTENSION(fsevents, fsevents.c, $ext_shared)
fi
