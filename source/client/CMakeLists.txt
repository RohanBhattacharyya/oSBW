INCLUDE_DIRECTORIES (
    ${STAR_EXTERN_INCLUDES}
    ${STAR_CORE_INCLUDES}
    ${STAR_BASE_INCLUDES}
    ${STAR_GAME_INCLUDES}
    ${STAR_PLATFORM_INCLUDES}
    ${STAR_APPLICATION_INCLUDES}
    ${STAR_RENDERING_INCLUDES}
    ${STAR_WINDOWING_INCLUDES}
    ${STAR_FRONTEND_INCLUDES}
  )

SET (star_client_HEADERS
      StarClientApplication.hpp
      StarRenderingLuaBindings.hpp
    )

SET (star_client_SOURCES
      StarClientApplication.cpp
      StarRenderingLuaBindings.cpp
    )

IF (STAR_SYSTEM_WINDOWS)
  SET (star_client_RESOURCES
      starbound.rc
    )
ENDIF ()

ADD_EXECUTABLE (starbound WIN32
  $<TARGET_OBJECTS:star_extern> $<TARGET_OBJECTS:star_core> $<TARGET_OBJECTS:star_base> $<TARGET_OBJECTS:star_game>
  $<TARGET_OBJECTS:star_application> $<TARGET_OBJECTS:star_rendering> $<TARGET_OBJECTS:star_windowing> $<TARGET_OBJECTS:star_frontend>
  ${star_client_HEADERS} ${star_client_SOURCES} ${star_client_RESOURCES})

IF (STAR_PRECOMPILED_HEADERS)
  TARGET_PRECOMPILE_HEADERS (starbound REUSE_FROM star_core)
ENDIF()

IF(STAR_SYSTEM_MACOS)
  SET_TARGET_PROPERTIES (starbound PROPERTIES LINK_FLAGS "-Wl,-rpath,'@executable_path'")
ELSEIF (UNIX AND NOT EMSCRIPTEN)
  SET_TARGET_PROPERTIES (starbound PROPERTIES LINK_FLAGS "-Wl,--export-dynamic,-rpath,'$ORIGIN'")
ENDIF()

IF (STAR_SYSTEM_MACOS)
  SET_TARGET_PROPERTIES (starbound PROPERTIES XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME YES)
ENDIF()

TARGET_LINK_LIBRARIES (starbound ${STAR_EXT_LIBS} ${STAR_EXT_GUI_LIBS})

IF(EMSCRIPTEN)
  SET_TARGET_PROPERTIES (starbound PROPERTIES SUFFIX ".html")
  set(STAR_WEB_OFFLINE_JS "false")
  if(STAR_WEB_OFFLINE)
    set(STAR_WEB_OFFLINE_JS "true")
  endif()
  file(READ "${PROJECT_SOURCE_DIR}/../web/vendor/jszip.min.js" STAR_WEB_JSZIP_INLINE)
  set(starbound_em_shell_file "${PROJECT_SOURCE_DIR}/../web/emscripten_shell.html")
  if(STAR_WEB_OFFLINE)
    set(starbound_em_shell_file "${PROJECT_SOURCE_DIR}/../web/emscripten_offline_shell.html")
  endif()
  configure_file(
    "${starbound_em_shell_file}"
    "${CMAKE_CURRENT_BINARY_DIR}/emscripten_shell.html"
    @ONLY)
  set_property(TARGET starbound APPEND PROPERTY LINK_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/emscripten_shell.html")

  # Memory tuning for the offline build:
  # - INITIAL_MEMORY=256MB: static data section is ~71 MiB; default 16 MiB
  #   initial memory makes wasm-ld error out once MAXIMUM_MEMORY is set
  #   explicitly. 256 MiB gives headroom for static + stack + early heap.
  # - MAXIMUM_MEMORY=4GB: Emscripten defaults the cap to 2 GiB; wasm32
  #   supports a full 4 GiB address space, and the offline build's
  #   working set (682 MB pak in MEMFS + engine textures + Lua state +
  #   world data) reliably crosses 2 GiB in complex areas, causing
  #   abort() during heap grow. Lift to the wasm32 ceiling.
  SET(starbound_em_link_flags "--shell-file ${CMAKE_CURRENT_BINARY_DIR}/emscripten_shell.html -sEXPORTED_RUNTIME_METHODS=callMain,FS,setCanvasSize -sASSERTIONS=2 -sSTACK_OVERFLOW_CHECK=2 -sEXCEPTION_DEBUG=1 -sNO_DISABLE_EXCEPTION_CATCHING -sUSE_SDL=3 -sUSE_WEBGL2=1 -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=256MB -sMAXIMUM_MEMORY=4GB -sNO_EXIT_RUNTIME=1 -sFORCE_FILESYSTEM=1 -sUSE_ZLIB=1 -sUSE_LIBPNG=1 -sUSE_FREETYPE=1 -sUSE_OGG=1 -sUSE_VORBIS=1 -sSTACK_SIZE=5242880 -sDEFAULT_PTHREAD_STACK_SIZE=2097152 -lidbfs.js -lworkerfs.js -lwebsocket.js")

  if(STAR_WEB_OFFLINE)
    string(APPEND starbound_em_link_flags " -sSINGLE_FILE=1 -sMINIFY_HTML=0 --embed-file ${PROJECT_SOURCE_DIR}/../assets/opensb@/overrides")
  else()
    string(APPEND starbound_em_link_flags " --preload-file ${PROJECT_SOURCE_DIR}/../assets/opensb@/overrides")
  endif()

  if(STAR_WEB_SAFE_HEAP)
    string(APPEND starbound_em_link_flags " -sSAFE_HEAP=1")
  endif()

  if(STAR_WEB_PTHREADS)
    # Requires COOP/COEP headers (crossOriginIsolated) to run in browsers.
    # Run the game on a worker thread to avoid blocking the browser main thread.
    # This prevents main-thread mailbox/proxy failures like "Blocking on the main thread is very dangerous".
    string(APPEND starbound_em_link_flags " -sUSE_PTHREADS=1 -sPTHREAD_POOL_SIZE=16 -sPROXY_TO_PTHREAD=1 -sOFFSCREENCANVAS_SUPPORT=1")
  endif()

  # Emscripten packs the preloaded files into the .data at link time, but changes under assets/
  # aren't tracked by default. Add a stamp file that depends on the Lua scripts so edits trigger
  # a relink (and therefore an updated .data).
  file(GLOB_RECURSE opensb_lua_scripts CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/../assets/opensb/scripts/*.lua")
  set(opensb_assets_stamp "${CMAKE_CURRENT_BINARY_DIR}/opensb_assets.stamp")
  add_custom_command(
    OUTPUT "${opensb_assets_stamp}"
    COMMAND ${CMAKE_COMMAND} -E touch "${opensb_assets_stamp}"
    DEPENDS ${opensb_lua_scripts}
    VERBATIM)
  add_custom_target(opensb_assets DEPENDS "${opensb_assets_stamp}")
  add_dependencies(starbound opensb_assets)
  set_property(TARGET starbound APPEND PROPERTY LINK_DEPENDS "${opensb_assets_stamp}")

  # Web builds: convert OpenSB WOFF2 fonts to TTF, since Emscripten's FreeType build may not
  # support WOFF2 and UI (e.g. settings menu) uses OpenSB fonts like iosevka-semibold.
  file(GLOB_RECURSE opensb_woff2_fonts CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/../assets/opensb/font/*.woff2")
  set(opensb_web_fonts_dir "${CMAKE_CURRENT_BINARY_DIR}/opensb_web_fonts")
  set(opensb_web_fonts_stamp "${CMAKE_CURRENT_BINARY_DIR}/opensb_web_fonts.stamp")
  find_program(PYTHON3_EXECUTABLE python3)
  if(NOT PYTHON3_EXECUTABLE)
    message(FATAL_ERROR "python3 is required for EMSCRIPTEN web builds (font conversion step)")
  endif()

  add_custom_command(
    OUTPUT "${opensb_web_fonts_stamp}"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${opensb_web_fonts_dir}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${opensb_web_fonts_dir}"
    COMMAND ${PYTHON3_EXECUTABLE} "${PROJECT_SOURCE_DIR}/../web/convert_woff2_fonts.py"
      --src "${PROJECT_SOURCE_DIR}/../assets/opensb/font"
      --dst "${opensb_web_fonts_dir}/font"
    COMMAND ${CMAKE_COMMAND} -E touch "${opensb_web_fonts_stamp}"
    DEPENDS ${opensb_woff2_fonts} "${PROJECT_SOURCE_DIR}/../web/convert_woff2_fonts.py"
    VERBATIM)

  add_custom_target(opensb_web_fonts DEPENDS "${opensb_web_fonts_stamp}")
  add_dependencies(starbound opensb_web_fonts)
  set_property(TARGET starbound APPEND PROPERTY LINK_DEPENDS "${opensb_web_fonts_stamp}")

  # Preload converted fonts under /overrides (in addition to the standard OpenSB overrides).
  if(STAR_WEB_OFFLINE)
    string(APPEND starbound_em_link_flags " --embed-file ${opensb_web_fonts_dir}@/overrides")
  else()
    string(APPEND starbound_em_link_flags " --preload-file ${opensb_web_fonts_dir}@/overrides")
  endif()

  # Set link flags once (avoid CMake list semantics inserting ';' which breaks the shell command).
  SET_TARGET_PROPERTIES (starbound PROPERTIES LINK_FLAGS "${starbound_em_link_flags}")

  if(NOT STAR_WEB_OFFLINE)
    add_custom_command(TARGET starbound POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${PROJECT_SOURCE_DIR}/../assets/opensb/preload.config
        $<TARGET_FILE_DIR:starbound>/preload.config
      COMMENT "Copy preload.config into web output")

    add_custom_command(TARGET starbound POST_BUILD
      COMMAND ${PYTHON3_EXECUTABLE} "${PROJECT_SOURCE_DIR}/../web/patch_emscripten_package_name.py"
        --js "$<TARGET_FILE_DIR:starbound>/starbound.js"
      COMMENT "Patch starbound.js PACKAGE_NAME to be relative")
  endif()
ENDIF()
