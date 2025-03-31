# HW2_Q3 readme
## What you need
You need Visual Studio 2022 and window 11 OS.

And C/C++ should be available in VS2022.

## About
This project is about Antialiasing.
To view the result image, open result.png, 
and if you want an explanation of the code, scroll down below.

## How to run

1. Click code and download as zip file.
![image](https://github.com/user-attachments/assets/c8da34e7-1db2-47e6-a841-e0bd1dd7c911)

2. Unzip a download file
![image](https://github.com/user-attachments/assets/d7f66b32-37b6-41ca-a07a-a8ea596d7228)

3. Open hw2_Q1-master. Double click hw2_Q1-master and opne OpenglViewer.sln
![image](https://github.com/user-attachments/assets/0c2fa2b8-444e-46a4-8aee-49ec6ea07857)

4. click "F5" on your keybord. Then you will get the result.
![image](https://github.com/user-attachments/assets/b1b650f1-b991-4674-8759-d4e590195b9b)


## Code explanation

#include <random>

The code #include <random> is used for random variables.

------------

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<> dis(0.0, 1.0);

Using rd for generate float num between 0.0 and 1.0.

Mersenne Twister is high-quality random number generator.

----------

const int N = 64; // Number of samples per pixel

![image](https://github.com/user-attachments/assets/3ab8bf85-8c51-4296-92e5-94dd7a975337)

Set samples of the image within each pixel 64.

----------
vec3 color(0.0f);
for (int s = 0; s < N; ++s)
{
	float u_offset = dis(gen);
	float v_offset = dis(gen);
	Ray ray = camera.getRay(i, j, u_offset, v_offset);
	color += scene.trace(ray, 0.0f, std::numeric_limits<float>::max());
}
color /= static_cast<float>(N);

Using dis(gen) to generate random variables used to create ray. When making ray, sampling multiple points in the pixel.

After that, the scene.trace function is called, and the results are stored in the color value. 

Finally, the accumulated sample values are divided by the sample count N to calculate the average.

-----------
Ather code is same as hw2_Q2.

