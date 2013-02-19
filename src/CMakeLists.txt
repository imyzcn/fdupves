CMAKE_MINIMUM_REQUIRED (VERSION 2.6)

SET (HEADERS
  util.h
  gui.h
  ini.h
  hash.h
  find.h
  video.h
  )

SET (SOURCES
  util.c
  gui.c
  ini.c
  hash.c
  find.c
  video.c
  main.c
  )

IF (WIN32)
  SET (HEADERS
    image-win.h
    ${HEADERS}
    )
  SET (SOURCES
    main.rc
    image-win.c
    ${SOURCES}
    )
  ADD_EXECUTABLE (fdupves WIN32 ${HEADERS} ${SOURCES} main.c)
ELSE (WIN32)
  ADD_EXECUTABLE (fdupves ${HEADERS} ${SOURCES} main.c)
ENDIF (WIN32)

TARGET_LINK_LIBRARIES (fdupves
  ${GTK2_LIBRARIES}
  ${FFMPEG_LIBRARIES}
  )

IF (WIN32)
  FIND_FILE (FREETYPE6 freetype6.dll)
  FIND_FILE (INTL intl.dll)
  FIND_FILE (LIBATK libatk-1.0-0.dll)
  FIND_FILE (LIBCAIRO libcairo-2.dll)
  FIND_FILE (LIBEXPAT libexpat-1.dll)
  FIND_FILE (LIBFONTCONFIG libfontconfig-1.dll)
  FIND_FILE (LIBGIO libgio-2.0-0.dll)
  FIND_FILE (LIBPANGOCAIRO libpangocairo-1.0-0.dll)
  FIND_FILE (LIBPANGOFT2 libpangoft2-1.0-0.dll)
  FIND_FILE (LIBGDKPIXBUF libgdk_pixbuf-2.0-0.dll)
  FIND_FILE (LIBGDK libgdk-win32-2.0-0.dll)
  FIND_FILE (LIBGLIB libglib-2.0-0.dll)
  FIND_FILE (LIBGMODULE libgmodule-2.0-0.dll)
  FIND_FILE (LIBPANGO libpango-1.0-0.dll)
  FIND_FILE (LIBGTHREAD libgthread-2.0-0.dll)
  FIND_FILE (LIBPNG libpng14-14.dll)
  FIND_FILE (LIBGTK libgtk-win32-2.0-0.dll)
  FIND_FILE (LIBGOBJECT libgobject-2.0-0.dll)
  FIND_FILE (LIBPANGOWIN32 libpangowin32-1.0-0.dll)
  FIND_FILE (LIBPANGOFT2 libpangoft2-1.0-0.dll)
  FIND_FILE (LIBZLIB1 zlib1.dll)
  SET (FFMPEGDLL
    ${FFMPEG_DIR}/bin/avformat-54.dll
    ${FFMPEG_DIR}/bin/avcodec-54.dll
    ${FFMPEG_DIR}/bin/avutil-52.dll
    ${FFMPEG_DIR}/bin/swscale-2.dll
    ${FFMPEG_DIR}/bin/ffplay.exe
    )

  INSTALL (FILES
    ${FREETYPE6}
    ${INTL}
    ${LIBATK}
    ${LIBEXPAT}
    ${LIBFONTCONFIG}
    ${LIBGIO}
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
    ${LIBPANGOWIN32}
    ${LIBPANGOFT2}
    ${LIBZLIB1}
    ${FFMPEGDLL}
    DESTINATION bin)
ENDIF (WIN32)

INSTALL (TARGETS fdupves DESTINATION bin)
