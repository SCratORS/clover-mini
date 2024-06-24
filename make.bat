taskkill /f /im clover.exe
g++ -O4  nes\*.cpp LuPng\*.c inih\*.c inih\cpp\*.cpp *.cpp  -o clover.exe -lmingw32 -lSDL2main -lSDL2 -mwindows