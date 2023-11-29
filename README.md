# ProcessScheduler
SYSC4001 Assignment 3

README

Scheduler Simulation
Simulates the scheduler of a computer with multiple CPUs running multiple unique scheduling algorithms

HOW TO RUN:
1. Compile Assignment3.c file either manually or using the script
2. Enter ./a3 into terminal

Additional Info:
- This scheduler supports up to 240 tasks (if priority distributed evenly)
  - Each priority queue (rqX) can hold 20 tasks
- Additional tasks can be added in the input file, provided they follow the format outlined below
  - SCHEDULE_TYPE priority execution_time pid

