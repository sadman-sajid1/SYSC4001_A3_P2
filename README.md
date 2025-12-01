This assignment simulates multiple TAs marking exams wiht the used of shared memory
part2a doesn't use semaphores
part2b uses semaphores to ensure TA synchronization

// FOLDER ARCH
part2a
part2b
rubric.txt
exams/ containing 20 exam.txt files for 20 students

//COMPILING (bash)
gcc part2a.c -o part2a
gcc part2b.c -o part2b -lpthread

//USE
./part2a "number of TAs wanted"
./part2b "number of TAs wanted"

//EXAMPLE
./part2a 3
./part2b 3
