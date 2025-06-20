# Conqorial

A 2D multiplayer web game inspired by territorial.io and warfront.io

## Building

Below are steps you can use in order to build and run the game (assuming you have already downloaded the code from GitHub).

* **Note:** make sure you download the code with this command, don't use github's "download" button:

```bash
git clone https://github.com/agokule/Conqorial.git --recurse-submodules
```

Note that you need the following prerequisites:

- cmake
- a LLVM C/C++ compiler (e.g. clang)
- something that can run a web server (e.g. Python)
- and a web browser

1. Install Emscripten SDK using their [instructions](https://emscripten.org/docs/getting_started/downloads.html#installation-instructions-using-the-emsdk-recommended)
2. In a terminal that has the source code directory open, run the following commands in order to build the game in release mode:

```bash
cd client
emcmake cmake -DCMAKE_BUILD_TYPE=Release -DDISTRIBUTION_MODE=OFF -S . -B build
cmake --build build --config Release
```
 * If you want to build the game in debug mode (**warning:** this mode will run agonizingly slow), replace `Release` with `Debug`.
 * If you want to build the game in distribution mode (has virtually no logging), replace `-DDISTRIBUTION_MODE=OFF` with `-DDISTRIBUTION_MODE=ON`.
3. Now we need to setup a webserver that can serve the game to the web browser. To do this, you can use the following command:

```bash
cd build
python -m http.server
```

4. Open your web browser and navigate to `http://localhost:8000`
 * If you want to run the `Debug` version, go to the `Debug` folder, otherwise, go to the `Release` folder.
  * **Tip:** If you are running Debug mode, you can increase performance a little bit by deleting the console output html element using Right Click -> Inspect -> Right Click again -> Delete Element. You will have to use the browser's console to see any output now though.
 * Then click on the html file to start the game.

