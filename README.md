# Pacman-CPP - Multithreaded Pacman Game

A multithreaded Pacman game implementation in C++ using SFML graphics library and POSIX threads (pthreads). This project demonstrates various operating system concepts including thread synchronization, mutex locks, semaphores, and concurrent programming.

## Features

- **Classic Pacman Gameplay**: Navigate through a maze, collect dots and power pellets while avoiding ghosts
- **Multithreaded Architecture**: Separate threads for game engine, user interface, and ghost movement
- **Thread Synchronization**: Implements mutex locks and semaphores for thread-safe operations
- **Dynamic Ghost Behavior**: AI-controlled ghosts with collision detection and pathfinding
- **Power Pellet System**: Temporary ghost vulnerability mode
- **Speed Pellets**: Special pellets that change ghost behavior
- **Lives System**: Player has multiple lives with respawn functionality
- **Score Tracking**: Real-time score display
- **Game Over Screen**: End game detection with exit option

## Operating System Concepts Demonstrated

### 1. Thread Management

- **Main Thread**: Game initialization and thread creation
- **Game Engine Thread**: Core game loop and rendering
- **UI Thread**: Menu system and user interface
- **Ghost Threads**: Individual threads for each ghost's AI and movement

### 2. Synchronization Mechanisms

- **Mutex Locks**:
  - `ghostGlobalMutex`: Synchronizes ghost movement and maze access
  - `mutex1`: Protects score updates and pellet consumption
  - `uiThreadFinishedMutex`: Coordinates UI thread completion
- **Semaphores**:
  - `keySemaphore`: Controls ghost house exit permissions
  - `permitSemaphore`: Manages ghost movement permits
- **Condition Variables**: `uiThreadFinishedCond` for thread coordination

### 3. Concurrent Programming Scenarios

- **Scenario 1**: Multiple ghosts accessing shared maze data
- **Scenario 2**: Score updating with pellet consumption
- **Scenario 3**: Ghost house exit control using semaphores

## Prerequisites

### Dependencies

- **SFML (Simple and Fast Multimedia Library)**

  ```bash
  # Ubuntu/Debian
  sudo apt-get install libsfml-dev

  # macOS with Homebrew
  brew install sfml

  # Windows
  # Download from https://www.sfml-dev.org/download.php
  ```

- **GCC with pthread support**
- **Make** (optional)

## Installation and Setup

1. **Clone the repository**:

   ```bash
   git clone <repository-url>
   cd OS_Pacman
   ```

2. **Ensure all assets are in place**:
   - `pacman-art/` directory with sprites
   - `sprites/` directory with game assets
   - Font files: `pacfont-good.ttf`, `Arial.ttf`
   - Maze file: `maze.txt`

## Building and Running

### Using the provided script:

```bash
chmod +x run.sh
./run.sh
```

### Manual compilation:

```bash
g++ -pthread -c main.cpp -o main.o
g++ -pthread main.o -o pacman -lsfml-graphics -lsfml-window -lsfml-system
./pacman
```

## Controls

- **W**: Move Up
- **S**: Move Down / Start Game (in menu)
- **A**: Move Left
- **D**: Move Right
- **P**: Pause Game
- **E**: Exit (in game over screen)

## Game Mechanics

### Maze Elements

- **X**: Walls (impassable)
- **.**: Regular dots (10 points each)
- **+**: Power pellets (make ghosts vulnerable)
- **,**: Speed pellets (randomly spawned, affect ghost speed)
- **=**: Ghost house barriers

### Scoring System

- Regular dots: 1 point each
- Power pellets: 1 point each + ghost vulnerability
- Game ends when all pellets are collected or player runs out of lives

### Ghost Behavior

- **Normal Mode**: Ghosts chase player at regular speed
- **Vulnerable Mode**: Activated by power pellets, ghosts become slower and can be "eaten"
- **Speed Mode**: Some ghosts become faster when consuming speed pellets

## Technical Architecture

### Thread Structure

```
Main Thread
├── Game Engine Thread
│   ├── UI Thread (Menu)
│   └── Ghost Threads (4 individual threads)
└── Semaphore Management
```

### Synchronization Points

1. **Ghost Movement**: Protected by `ghostGlobalMutex`
2. **Score Updates**: Protected by `mutex1`
3. **Ghost House Exit**: Controlled by semaphores with count of 2
4. **UI Coordination**: Uses condition variables

## File Structure

```
pacman-cpp/
├── main.cpp              # Main game implementation
├── maze.txt              # Maze layout definition
├── run.sh                # Build and run script
├── README.md             # This file
├── pacfont-good.ttf      # Game font
├── Arial.ttf             # UI font
├── pacman-art/           # Game sprites and assets
│   ├── ghosts/
│   ├── other/
│   └── pacman-*/
└── sprites/              # Additional sprite assets
```

## Known Issues and Limitations

- Ghost AI uses simple random movement rather than sophisticated pathfinding
- Limited error handling for missing asset files
- No save/load game functionality
- Fixed maze layout (not dynamically generated)

## Future Enhancements

- [ ] Implement A\* pathfinding for ghosts
- [ ] Add multiple maze levels
- [ ] Implement high score system
- [ ] Add sound effects and background music
- [ ] Create level progression system
- [ ] Add more power-up types

## Troubleshooting

### Common Issues:

1. **SFML not found**: Ensure SFML is properly installed and linked
2. **Missing assets**: Verify all sprite files and fonts are in correct directories
3. **Compilation errors**: Check that pthread library is available
4. **Segmentation fault**: Usually caused by missing mutex initialization

### Debug Mode:

Add `-g` flag for debugging:

```bash
g++ -pthread -g -c main.cpp -o main.o
```

## License

This project is created for educational purposes as part of an Operating Systems course.

## Contributing

This is an academic project. For educational use and reference only.
