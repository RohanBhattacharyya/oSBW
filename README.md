# OpenStarbound Web Fork

<details>
<summary><b>What is this?</b></summary>
 
tl;dr: **OpenStarbound** is a mod of the latest version of Starbound, 1.4.4. It fixes many bugs, adds many new features and improves performance. This is the web version, so that users can run the Starbound engine on modern browsers.

See [openstarbound](https://github.com/OpenStarbound/OpenStarbound/).
</details>

## Building
Note: Some of these [texts](## "hi :3") are just tooltips rather than links. 

Web builds are documented in [doc/web](doc/web/README.md).

<details>
<summary>template sbinit.config for dist/ after build</summary>
<br>

```json
{
  "assetDirectories" : [
    "../assets/",
    "./mods/"
  ],

  "storageDirectory" : "./",
  "logDirectory" : "./logs/"
}
```

</details>

<details>
<summary><b>Web (Emscripten)</b></summary>

* Install and activate the [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html).
* Ensure CMake 3.23+ and Ninja are installed.
* Build from the **source/** directory:
  * Single-threaded:
    * `cmake --preset=emscripten-release`
    * `cmake --build --preset=emscripten-release`
  * PThreads (requires COOP/COEP headers):
    * `cmake --preset=emscripten-pthreads-release`
    * `cmake --build --preset=emscripten-pthreads-release`

Output is written to:
* **build/emscripten-release/** or
* **build/emscripten-pthreads-release/**

Deploy **all** generated files together (starbound.html/js/wasm/data + preload.config).

#### Hosting
PThreads builds require cross-origin isolation headers:
* `Cross-Origin-Opener-Policy: same-origin`
* `Cross-Origin-Embedder-Policy: require-corp`

For local testing with the correct headers, use:
* `python3 web/serve_dist_with_headers.py --dir /path/to/build/emscripten-release`
* or `python3 web/serve_dist_with_headers.py --pthreads`

More details: [doc/web/hosting.md](doc/web/hosting.md)

</details>
