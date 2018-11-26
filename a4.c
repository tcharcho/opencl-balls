/* Simple 2D physics for circles using ASCII graphics
	-original NCurses code from "Game Programming in C with the Ncurses Library"
	 https://www.viget.com/articles/game-programming-in-c-with-the-ncurses-library/
	 and from "NCURSES Programming HOWTO"
	 http://tldp.org/HOWTO/NCURSES-Programming-HOWTO/
	-Physics code and algorithms from "How to Create a Custom 2D Physics
	 Engine: The Basics and Impulse Resolution"
	 https://gamedevelopment.tutsplus.com/tutorials/how-to-create-a-custom-2d-physics-engine-the-basics-and-impulse-resolution--gamedev-6331
*/


#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<unistd.h>
#include<ncurses.h>
#include <OpenCL/cl.h>

// used to slow curses animation
#define DELAY 50000

// number of balls
#define POPSIZE 50

// maximum screen size, both height and width
#define SCREENSIZE 100

// ball location (x,y,z) and velocity (vx,vy,vz) in ballArray[][]
#define BX 0
#define BY 1
#define VX 2
#define VY 3

// maximum screen dimensions
int max_y = 0, max_x = 0;

// location and velocity of ball
float ballArray[POPSIZE][4];
// change in velocity is stored for each ball (x,y,z)
float ballUpdate[POPSIZE][2];

/* Find a GPU or CPU associated with the first available platform */
cl_device_id create_device() {

  cl_platform_id platform;
  cl_device_id dev;
  int err;

  /* Identify a platform */
  err = clGetPlatformIDs(1, &platform, NULL);
  if(err < 0) {
    perror("Couldn't identify a platform");
    exit(1);
  }

  /* Access a device */
  err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &dev, NULL);
  if(err == CL_DEVICE_NOT_FOUND) {
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &dev, NULL);
  }
  if(err < 0) {
    perror("Couldn't access any devices");
    exit(1);
  }

  return dev;
}

/* Create program from a file and compile it */
cl_program build_program(cl_context ctx, cl_device_id dev) {

  cl_program program;
  FILE *program_handle;
  char *program_buffer, *program_log;
  size_t program_size, log_size;
  int err;

  /* Read program file and place content into buffer */
  program_handle = fopen("phys.cl", "r");
  if(program_handle == NULL) {
    perror("Couldn't find the program file");
    exit(1);
  }

  fseek(program_handle, 0, SEEK_END);
  program_size = ftell(program_handle);
  rewind(program_handle);
  program_buffer = (char*)malloc(program_size + 1);
  program_buffer[program_size] = '\0';
  fread(program_buffer, sizeof(char), program_size, program_handle);
  fclose(program_handle);

  /* Create program from file */
  program = clCreateProgramWithSource(ctx, 1,
    (const char**)&program_buffer, &program_size, &err);
  if(err < 0) {
    perror("Couldn't create the program");
    exit(1);
  }
  free(program_buffer);

  /* Build program */
  err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
  if(err < 0) {
    /* Find size of log and print to std output */
    clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG,
      0, NULL, &log_size);
    program_log = (char*) malloc(log_size + 1);
    program_log[log_size] = '\0';
    clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG,
      log_size + 1, program_log, NULL);
    printf("%s\n", program_log);
    free(program_log);
    exit(1);
  }

   return program;
}

void initBalls() {
  int i;
	// calculate initial random locations for each ball, scaled based on the screen size
   for(i=0; i<POPSIZE; i++) {
      ballArray[i][BX] = (float) (random() % SCREENSIZE);
      ballArray[i][BY] = (float) (random() % SCREENSIZE);
      ballArray[i][VX] =  (float) ((random() % 5) - 2);
      ballArray[i][VY] = (float) ((random() % 5) - 2);
      ballUpdate[i][BX] = 0.0;
      ballUpdate[i][BY] = 0.0;
   }
}

int drawBalls() {
  int c, i;
  float multx, multy;

	// update screen maximum size
   getmaxyx(stdscr, max_y, max_x);

	// used to scale position of balls based on screen size
   multx = (float)max_x / SCREENSIZE;
   multy = (float)max_y / SCREENSIZE;

   clear();

	// display balls
   for (i=0; i<POPSIZE; i++) {
      mvprintw((int)(ballArray[i][BX]*multy), (int)(ballArray[i][BY]*multx), "o");
   }

   refresh();

   usleep(DELAY);

	// read keyboard and exit if 'q' pressed
   c = getch();
   if (c == 'q') return(1);
   else return(0);
}

void moveBalls() {
  /* OpenCL structures */
  cl_device_id device;
  cl_context context;
  cl_program program;
  cl_kernel kernel;
  cl_command_queue queue;
  cl_int i, j, err;
  size_t local_size, global_size;

  /* Data and buffers */
  cl_mem input_buffer, update_buffer;
  cl_int num_groups;

  device = create_device();
  context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
  if(err < 0) {
      perror("Couldn't create a context");
      exit(1);
  }

  /* Build program */
  program = build_program(context, device);

  /* Create data buffer */
  global_size = 50; // total WIs
  local_size = 10; // WIs per group
  num_groups = global_size/local_size;


  /****************** UPDATE VELOCITY OF BALLS BASED UPON COLLISIONS ******************/
  input_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY |
      CL_MEM_COPY_HOST_PTR, POPSIZE * sizeof(float), ballArray, &err);
  update_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE |
      CL_MEM_COPY_HOST_PTR, sizeof(ballUpdate), ballUpdate, &err);
  if(err < 0) {
    perror("Couldn't create a buffer");
    exit(1);
  }

   /* Create a command queue */
  queue = clCreateCommandQueue(context, device, 0, &err);
  if(err < 0) {
    perror("Couldn't create a command queue");
    exit(1);
  }

  /* Create a kernel */
  kernel = clCreateKernel(program, "update_velocity", &err);
  if(err < 0) {
    perror("Couldn't create a kernel");
    exit(1);
  }

  /* Create kernel arguments */
  err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &input_buffer);
  err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &update_buffer);
  if(err < 0) {
    perror("Couldn't create a kernel argument");
    exit(1);
  }

  /* Enqueue kernel */
  err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size,
      &local_size, 0, NULL, NULL);
  if(err < 0) {
    perror("Couldn't enqueue the kernel");
    exit(1);
  }

  /* Read the kernel's output */
  err = clEnqueueReadBuffer(queue, update_buffer, CL_TRUE, 0,
      sizeof(ballUpdate), &ballUpdate, 0, NULL, NULL);
  if(err < 0) {
    perror("Couldn't read the buffer");
    exit(1);
  }

  /****************** MOVE BALLS BY CALCULATING UPDATING VELOCITY AND POSITION ******************/

  /* Deallocate resources */
  clReleaseKernel(kernel);
  clReleaseMemObject(update_buffer);
  clReleaseMemObject(input_buffer);
  clReleaseCommandQueue(queue);

  input_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY |
      CL_MEM_COPY_HOST_PTR, sizeof(ballUpdate), ballUpdate, &err);
  update_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE |
      CL_MEM_COPY_HOST_PTR, sizeof(ballArray), ballArray, &err);
  if(err < 0) {
    perror("Couldn't create a buffer");
    exit(1);
  }

  /* Create a command queue */
  queue = clCreateCommandQueue(context, device, 0, &err);
  if(err < 0) {
    perror("Couldn't create a command queue");
    exit(1);
  }

  /* Create a kernel */
  kernel = clCreateKernel(program, "update_balls", &err);
  if(err < 0) {
    perror("Couldn't create a kernel");
    exit(1);
  }

  /* Create kernel arguments */
  err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &input_buffer);
  err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &update_buffer);
  if(err < 0) {
    perror("Couldn't create a kernel argument");
    exit(1);
  }

  /* Enqueue kernel */
  err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size,
      &local_size, 0, NULL, NULL);
  if(err < 0) {
    perror("Couldn't enqueue the kernel");
    exit(1);
  }

  /* Read the kernel's output */
  err = clEnqueueReadBuffer(queue, update_buffer, CL_TRUE, 0,
      sizeof(ballArray), &ballArray, 0, NULL, NULL);
  if(err < 0) {
    perror("Couldn't read the buffer");
    exit(1);
  }

  /* Deallocate resources */
  clReleaseKernel(kernel);
  clReleaseMemObject(update_buffer);
  clReleaseMemObject(input_buffer);
  clReleaseCommandQueue(queue);
  clReleaseProgram(program);
  clReleaseContext(context);
}

int main(int argc, char *argv[]) {
  // initialize curses
  initscr();
  noecho();
  cbreak();
  timeout(0);
  curs_set(FALSE);
  // Global var `stdscr` is created by the call to `initscr()`
  getmaxyx(stdscr, max_y, max_x);

  // place balls in initial position
  initBalls();

  // draw and move balls using ncurses
  while(1) {
    if (drawBalls() == 1) break;
    moveBalls();
  }

  // shut down curses
  endwin();

}
