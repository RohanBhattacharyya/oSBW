# Web Troubleshooting

## PThreads build won’t start
**Symptoms:** runtime errors about SharedArrayBuffer or PThreads.

**Fix:** Ensure COOP/COEP headers are set. See [hosting.md](hosting.md).

## Canvas is blank or squashed after fullscreen
**Symptoms:** rendering looks compressed or wrong after toggling fullscreen.

**Fix:** Confirm you are using the latest shell template. The web UI schedules multiple resize passes after fullscreen changes.

## No audio until you click
Some browsers require a user gesture to start audio. Click the canvas or press Start.

## Missing fonts or broken UI text
The web build converts WOFF2 fonts to TTF. Ensure `convert_woff2_fonts.py` runs (Python 3 required) and the build output includes the converted fonts under `/overrides`.

## Mod or save data not persisting
Ensure IDBFS is available (IndexedDB supported) and that `IDBFS` sync is enabled by the shell. Check the console viewer for warnings.

## HTTP or external network access fails
HTTP requests from C++ are disabled in web builds and will return an error by design.
