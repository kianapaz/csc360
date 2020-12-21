To compile the all the files run: make
To execute Part 1 (diskinfo.c):
./diskget <given image disk>

To execute Part 2 (disklist.c): 
./disklist <given image disk>

To execute Part 3 (diskget.c): 
./diskget <given image disk> <file from the given root directory>

To execute Part 4 (diskput.c): 
./diskput <given image disk> <path to file to copy>


Example of Part3:
./diskget disk3.IMA ANS1.PDF

Example of Part4:
./diskget Image2020.IMA /SUB2/FIGURE1.JPG

Helper Functions include:
a3functions.c and a3functions.h
queue.c and queue.h