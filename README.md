# WordleReader
Program that autosolve the wordle

# Building
Its a basic cmake project but there is git submodules:
```bash
git clone --recurse-submodules https://github.com/3l0w/WordleReader.git
```
Or
```bash
git clone https://github.com/3l0w/WordleReader.git
cd WordleReader
git submodule init
git submodule update
```
Then build it
```
mkdir build
cd build
cmake ..
make
```
# Usage
```bash
./WordleReader
```
Its very easy to use simply execute the binary and place your mouse on the top left corner of the worlde press right ctrl and redo the same for the bottom right corner of the wordle
# Note
This program use my lib [WordleSolver](https://github.com/3l0w/WordleSolver) this lib use two file called words.txt (the list of words used in the game) and result.txt (computation of the best starting word) they are need is the local directory where the program is run, i've provided them into this project
