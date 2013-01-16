CMAKE_MINIMUM_REQUIRED (VERSION 2.6)

SET (HEADERS
  util.h
  gui.h
  fdini.h
  )

SET (SOURCES
  util.c
  gui.c
  main.c
  )

ADD_EXECUTABLE (fdupves ${HEADERS} ${SOURCES} main.c)
TARGET_LINK_LIBRARIES (fdupves
  ${GTK2_LIBRARIES}
  ${AVFORMAT_LIBRARIES}
  ${AVCODEC_LIBRARIES}
  )

IF (WIN32)
  FIND_FILE (FREETYPE6 freetype6.dll)
  FIND_FILE (INTL intl.dll)
  FIND_FILE (LIBATK libatk-1.0-0.dll)
  FIND_FILE (LIBEXPAT libexpat-1.dll)
  FIND_FILE (LIBFONTCONFIG libfontconfig-1.dll)
  FIND_FILE (LIBGIO libgio-2.0-0.dll)
  FIND_FILE (LIBGTHREAD libgthread-2.0-0.dll)
  FIND_FILE (LIBCAIRO libcairo-2.dll)
  FIND_FILE (LIBPANGOCAIRO libpangocairo-1.0-0.dll)
  FIND_FILE (LIBPANGOFT2 libpangoft2-1.0-0.dll)
  FIND_FILE (LIBGDKPIXBUF libgdk_pixbuf-2.0-0.dll)
  FIND_FILE (LIBGDK libgdk-win32-2.0-0.dll)
  FIND_FILE (LIBGLIB libglib-2.0-0.dll)
  FIND_FILE (LIBGMODULE libgmodule-2.0-0.dll)
  FIND_FILE (LIBPANGO libpango-1.0-0.dll)
  FIND_FILE (LIBPNG libpng14-14.dll)
  FIND_FILE (LIBGTK libgtk-win32-2.0-0.dll)
  FIND_FILE (LIBGOBJECT libgobject-2.0-0.dll)
  FIND_FILE (LIBPANGOWIN32 libpangowin32-1.0-0.dll)

  INSTALL (FILES
    ${FREETYPE6}
    ${INTL}
    ${LIBATK}
    ${LIBEXPAT}
    ${LIBFONTCONFIG}
    ${LIBGIO}
    ${LIBGTHREAD}
    ${LIBCAIRO}
    ${LIBPANGOCAIRO}
    ${LIBGDKPIXBUF}
    ${LIBGDK}
    ${LIBGLIB}
    ${LIBGMODULE}
    ${LIBPANGO}
    ${LIBGTHREAD}
    ${LIBPNG}
    ${LIBGTK}
    ${LIBGOBJECT}
    ${LIBPANGOFT2}
    ${LIBPANGOWIN32}
    ${LIBZLIB1}
    ${LIBXML2}
    ${LIBGTKHTML3}
    ${LIBGCONF2}
    ${LIBORBIT2}
    ${LIBGNOME2}
    ${LIBBONOBO2}
    ${LIBBONOBOACTIVATION}
    ${LIBGNOMEVFS2}
    ${LIBGNURX}
    ${LIBORBITCOSNAMING2}
    ${LIBPOPT}
    DESTINATION bin)
ENDIF (WIN32)

INSTALL (TARGETS fdupves DESTINATION bin)
