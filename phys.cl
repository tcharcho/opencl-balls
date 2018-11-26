int ballCollision(int balli, int ballj, __global float4* ball_array_data);
float dotProduct(float x1, float y1, float x2, float y2);
void resolveCollision(int i, int j, __global float4* ball_array_data, __global float4* update_array_data);

// determine if two balls in ballArray collide
// calcualte distance between the two balls and compare to the
//	sum of the radii
// use balli and ballj to identify elements in ballArray[]
int ballCollision(int balli, int ballj, __global float4* ball_array_data) {
  float distance;
  float radiiSum;

  // Pythagorean distance
	distance = sqrt(pow((ball_array_data[balli][0]-ball_array_data[ballj][0]),2)
		 + pow((ball_array_data[balli][1]-ball_array_data[ballj][1]),2));
	radiiSum = 1.0 + 1.0;
  // if the sum of the two radii is less than the distance
  // between the balls then they collide, otherwise they
  // do not collide
	if (distance < radiiSum) return(1);
	else return(0);
}

// calculate the dot product between two vectors
float dotProduct(float x1, float y1, float x2, float y2) {
   return(x1 * x2 + y1 * y2);
}

// calculate effects of collision between ballArray[i][] and
// ballArray[j][] where i and j are the parameters to the function
void resolveCollision(int i, int j, __global float4* ball_array_data, __global float4* update_array_data) {
  float rvx, rvy;
  float nx, ny;
  float distance;
  float vnormal;
  float impulse;
  float ix, iy;

	// calculate relative velocity
  rvx = ball_array_data[j][2] - ball_array_data[i][2];
  rvy = ball_array_data[j][3] - ball_array_data[i][3];

	// calculate collision normal
  nx = ball_array_data[j][0] - ball_array_data[i][0];
  ny = ball_array_data[j][1] - ball_array_data[i][1];

	// Pythagorean distance
  distance = sqrt(pow((ball_array_data[j][0]-ball_array_data[i][0]),2)
		 + pow((ball_array_data[j][1]-ball_array_data[i][1]),2));
  if (distance == 0) return;

  nx = nx / distance;
  ny = ny / distance;

	// Calculate relative velocity in terms of the normal direction
  vnormal = dotProduct(rvx, rvy, nx, ny);

	// Do not resolve if velocities are separating
  if(vnormal > 0)
    return;

	// Calculate impulse scalar
  impulse = -(1 + 0.5) * vnormal;
  impulse /= ((1 / 1.0) + (1 / 1.0));

	// Apply impulse
  ix = impulse * nx;
  iy = impulse * ny;
  update_array_data[i][0] = ball_array_data[i][2] - ((1/1.0) * impulse);
  update_array_data[i][1] = ball_array_data[i][3] - ((1/1.0) * impulse);
  update_array_data[j][0] = ball_array_data[j][2] + ((1/1.0) * impulse);
  update_array_data[j][1] = ball_array_data[j][3] + ((1/1.0) * impulse);

}


__kernel void update_velocity(__global float4* ball_array, __global float4* update_array) {
  uint global_addr, pop_size, j;

  pop_size = 50;
  global_addr = get_global_id(0);

  for (j=global_addr+1; j<pop_size; j++) {
    if (ballCollision(global_addr, j, ball_array) == 1) {
      resolveCollision(global_addr, j, ball_array, update_array);
    }
  }
}

__kernel void update_balls(__global float4* update_array, __global float4* ball_array) {
  // move balls by calculating updating velocity and position
  uint global_addr = get_global_id(0);

  // update velocity for each ball
  if (update_array[global_addr][0] != 0.0)
    ball_array[global_addr][2] = update_array[global_addr][0];
  if (update_array[global_addr][1] != 0.0)
    ball_array[global_addr][3] = update_array[global_addr][1];

  // enforce maximum velocity of 2.0 in each axis
  // done to make it easier to see collisions
  if (ball_array[global_addr][2] > 2.0) ball_array[global_addr][2] = 2.0;
  if (ball_array[global_addr][3] > 2.0) ball_array[global_addr][3] = 2.0;

  // update position for each ball
  ball_array[global_addr][0] += ball_array[global_addr][2];
  ball_array[global_addr][1] += ball_array[global_addr][3];

  // if ball moves off the screen then reverse velocity so it bounces
  // back onto the screen, and move it onto the screen
  if (ball_array[global_addr][0] > (100-1)) {
    ball_array[global_addr][2] *= -1.0;
    ball_array[global_addr][0] = 100 - 1.5;
  }
  if (ball_array[global_addr][0] < 0.0) {
    ball_array[global_addr][2] *= -1.0;
    ball_array[global_addr][0] = 0.5;
  }
  if (ball_array[global_addr][1] > (100-1)) {
    ball_array[global_addr][3] *= -1.0;
    ball_array[global_addr][1] = 100 - 1.5;
  }
  if (ball_array[global_addr][1] < 0.0) {
    ball_array[global_addr][3] *= -1.0;
    ball_array[global_addr][1] = 0.5;
  }
}
