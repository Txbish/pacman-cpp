g++ -pthread -c main.cpp -o main.o
g++ -pthread main.o -o pacman -lsfml-graphics -lsfml-window -lsfml-system
./pacman