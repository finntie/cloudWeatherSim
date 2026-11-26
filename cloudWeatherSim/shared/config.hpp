#pragma once
#include <stdint.h> // uint32_t
#include <stdio.h>

//Do we want to use the GPU? (CPU is very unstable if not working at all atm)
#define USE_GPU 1


#define MAXGRIDRESOLUTIONS 4
// Holds data of the simulation sizes
// If multiple resolutions, the 'grand parent' is the main grid parent with children following
struct simParams
{
	int grids; // Total amount of grid resolutions
	float voxelSize; // Voxel size of grand parent

	int resDecrease[MAXGRIDRESOLUTIONS]; // Resolution decrease starting from the grand parent

	// X and Z resolution starting from parent to children
	int sizeX[MAXGRIDRESOLUTIONS];
	int sizeZ[MAXGRIDRESOLUTIONS];
	int sizeY[MAXGRIDRESOLUTIONS]; // Size Y gets adjusted automatically to be the same height as the grand parent

	// Sizes of each grid
	int size[MAXGRIDRESOLUTIONS];
	int sizeGround[MAXGRIDRESOLUTIONS];
	int totalSize;
	int totalGroundSize;

	// Offset on the X and Z axis for each grid. The grand parent does not have an offset.
	int offsetX[MAXGRIDRESOLUTIONS];
	int offsetZ[MAXGRIDRESOLUTIONS];

	// Get filled when calculating for block and grid dimensions
	int gridDimX[MAXGRIDRESOLUTIONS];
	int gridDimY[MAXGRIDRESOLUTIONS];
	int gridDimZ[MAXGRIDRESOLUTIONS];


	// Creates instance and initializes the grand parent
	static simParams create(float _voxelsize, int x, int y, int z)
	{
		return { 1, _voxelsize, {1}, {x}, {y}, {z}, {x * y * z}, {x * z}, {x * y * z}, {x * z}, {0}, {0}, {0}, {0}, {0} };
	}
	// Add new grid resolution inside the current grid
	void addResolution(int resolutionDecrease, int x, int z, int _offsetX, int _offsetZ)
	{
		// TODO: add more checks
		int invalid = 0;

		if (grids == 0) invalid = 1;
		if (grids >= MAXGRIDRESOLUTIONS) invalid = 2;

		if (invalid == 0)
		{
			// New size must be dividible with the resolution Decrease so its alligned correctly
			if (x % resolutionDecrease != 0) invalid = 3;
			else if (z % resolutionDecrease != 0) invalid = 3;
			// Room for 'Ghost' cells (or the relaxation zone)
			else if (x <= resolutionDecrease * 2) invalid = 4;
			else if (z <= resolutionDecrease * 2) invalid = 4;
			// The new size cant be bigger than the parent size, leaving at least 2 spaces on both sides
			else if (x / resolutionDecrease - 2 < 0 || z / resolutionDecrease - 2 < 0) invalid = 5;
		}


		if (invalid == 0)
		{
			resDecrease[grids] = resolutionDecrease;
			sizeX[grids] = x;
			sizeZ[grids] = z;
			sizeY[grids] = sizeY[grids - 1] * resolutionDecrease; // For now we increases Y size, TODO: y size always stay the same resolution. 
		}

		// Resolution check
		float vSize = voxelSize;
		for (int i = 1; i < grids + 1; i++)
		{
			vSize /= resDecrease[i];
		}
		if (vSize < 1.0f) invalid = 6;

		if (invalid == 0 && (_offsetX == 0 || _offsetZ == 0 ||
			_offsetX >= sizeX[grids - 1] - (x / resolutionDecrease) ||
			_offsetZ >= sizeZ[grids - 1] - (z / resolutionDecrease))) invalid = 7;

		if (invalid == 0)
		{
			size[grids] = x * sizeY[grids] * z;
			sizeGround[grids] = x * z;

			totalSize += size[grids];
			totalGroundSize += sizeGround[grids];

			offsetX[grids] = _offsetX;
			offsetZ[grids] = _offsetZ;

			grids++;
		}

		// Switch on error
		switch (invalid)
		{
		case 1:
			printf("CLOUDLIB WARNING: addResolution(): grid was not created yet, make sure to use the 'create()' function first\n");
			break;
		case 2:
			printf("CLOUDLIB WARNING: addResolution(): unable to add another resolution, max resolutions reached\n");
			break;
		case 3:
			printf("CLOUDLIB WARNING: addResolution(): Size xz is not dividible with previous grid, causing remaining cells\n");
			break;
		case 4:
			printf("CLOUDLIB WARNING: addResolution(): Size xz is not large enough for a relaxation zone\n");
			break;
		case 5:
			printf("CLOUDLIB WARNING: addResolution(): New grid is too big\n");
			break;
		case 6:
			printf("CLOUDLIB WARNING: addResolution(): Too many resolution decreases, resolution is too small\n");
			break;
		case 7:
			printf("CLOUDLIB WARNING: addResolution(): The offset of the new grid is invalid\n");
			break;
		default:
			break;
		}
	}
	// Re-calculate the size of the grid
	void recalcSize()
	{
		totalSize = 0;
		totalGroundSize = 0;
		for (int i = 0; i < grids; i++)
		{
			totalSize += size[i];
			totalGroundSize += sizeGround[i];
		}
	}
};


// Data graph
#define MAXGRAPHLENGTH 100

#define BOUNDSTEMP boundsEnv{ CUSTOM, CUSTOM, CUSTOM}
#define BOUNDSVAPOR boundsEnv{ CUSTOM, CUSTOM, CUSTOM}
#define BOUNDSVELXZ boundsEnv{ CUSTOM, CUSTOM, DIRICHLET}
#define BOUNDSVELY boundsEnv{ NEUMANN, NEUMANN, DIRICHLET}
#define BOUNDSAPPLYA boundsEnv{ NEUMANN, DIRICHLET, DIRICHLET }
#define BOUNDSDENSITY boundsEnv{ NEUMANN, NEUMANN, DIRICHLET }
#define BOUNDSBUOYANCY boundsEnv{ DIRICHLET, DIRICHLET, DIRICHLET }
#define BOUNDSPRESPROJ boundsEnv{ NEUMANN, NEUMANN, NEUMANN } // NEUMANN in presProj means solid boundaries, DIRICHLET means free boundaries 
#define BOUNDSMIXINGRATIOS boundsEnv{ DIRICHLET, DIRICHLET, DIRICHLET}

// Wrapper for CPU/GPU functions
#include "gpu_defines.h"
#ifdef GPU_COMPILER
#include "msvc_defines.h"
#include <hip/hip_runtime.h>
#endif

//Storage array for setting boundary conditions easier.
enum envType : unsigned char
{
	SKY = 0 << 0,
	GROUND = 1 << 0,
	GHOST = 1 << 1, // In-between cells of nested grids
	// NOTE: if adding 1 more type, 'Neigh' needs to increase in size
};

struct singleNeigh
{
	singleNeigh() = default;
	singleNeigh(bool outside, envType type) : outside(outside), type(type) {}
	inline bool valid() { return (!outside && type == SKY); }
	bool outside{ false };
	envType type{ SKY };
};

//Struct containing neighbouring values
struct Neigh
{
	//With forward being +z and backward -z
	//singleNeigh left, right, up, down, forward, backward, current;
	static constexpr uint32_t resetMask = 7;
	static constexpr uint32_t envTypeMask = 3;

	// To set values:
	// 1. Reset part of the data we want to set, using a mask (for example: ~7u << 3 means grab inverse of 3 bits, move this 3 bits to the left, which creates a mask for our backward)
	// 2. Set the bool, simply using a or bitwise operation, shifting the bool first to the correct point.
	// 3. Only use first 2 bits of the envType, shift to correct place in data, then use or bitwise operation to set the data.
	__host__ __device__ void setCurrent(bool outside, envType type) { data &= ~(resetMask << 0); data |= (uint32_t(outside) << 0); data |= ((uint32_t(type) & envTypeMask) << 1); }
	__host__ __device__ void setBackward(bool outside, envType type) { data &= ~(resetMask << 3); data |= (uint32_t(outside) << 3); data |= ((uint32_t(type) & envTypeMask) << 4); }
	__host__ __device__ void setForward(bool outside, envType type)  { data &= ~(resetMask << 6); data |= (uint32_t(outside) << 6); data |= ((uint32_t(type) & envTypeMask) << 7); }
	__host__ __device__ void setDown(bool outside, envType type)     { data &= ~(resetMask << 9); data |= (uint32_t(outside) << 9); data |= ((uint32_t(type) & envTypeMask) << 10); }
	__host__ __device__ void setUp(bool outside, envType type)       { data &= ~(resetMask << 12); data |= (uint32_t(outside) << 12); data |= ((uint32_t(type) & envTypeMask) << 13); }
	__host__ __device__ void setRight(bool outside, envType type)    { data &= ~(resetMask << 15); data |= (uint32_t(outside) << 15); data |= ((uint32_t(type) & envTypeMask) << 16); }
	__host__ __device__ void setLeft(bool outside, envType type)     { data &= ~(resetMask << 18); data |= (uint32_t(outside) << 18); data |= ((uint32_t(type) & envTypeMask) << 19); }

	// Get if value is outside the grid
	__host__ __device__ bool getOutsideCurrent()  const { return bool(data & (1u << 0)); }
	__host__ __device__ bool getOutsideBackward() const { return bool(data & (1u << 3)); }
	__host__ __device__ bool getOutsideForward()  const { return bool(data & (1u << 6)); }
	__host__ __device__ bool getOutsideDown()     const { return bool(data & (1u << 9)); }
	__host__ __device__ bool getOutsideUp()       const { return bool(data & (1u << 12)); }
	__host__ __device__ bool getOutsideRight()    const { return bool(data & (1u << 15)); }
	__host__ __device__ bool getOutsideLeft()     const { return bool(data & (1u << 18)); }

	// Get the environment type of the value
	__host__ __device__ envType getEnvTypeCurrent()  const { return envType(char(data >> 1 & 3u)); }
	__host__ __device__ envType getEnvTypeBackward() const { return envType(char(data >> 4 & 3u)); }
	__host__ __device__ envType getEnvTypeForward()  const { return envType(char(data >> 7 & 3u)); }
	__host__ __device__ envType getEnvTypeDown()     const { return envType(char(data >> 10 & 3u)); }
	__host__ __device__ envType getEnvTypeUp()       const { return envType(char(data >> 13 & 3u)); }
	__host__ __device__ envType getEnvTypeRight()    const { return envType(char(data >> 16 & 3u)); }
	__host__ __device__ envType getEnvTypeLeft()     const { return envType(char(data >> 19 & 3u)); }

	__host__ __device__ uint32_t getData() { return data; }

private:
	uint32_t data;


};

enum direction
{
	LEFT, RIGHT, UP, DOWN, FORWARD, BACKWARD
};

//Boundary condition
enum boundCon
{
	NEUMANN, DIRICHLET, CUSTOM
};

///- Sides 
///- Up
///- Ground
///Holds bounds for different direction, downwards is ALWAYS ground
struct boundsEnv
{
	//Sides meaning left, right, forward and backwards
	boundCon sides;
	boundCon up;
	//Ground includes downwards and possibly left and righ when there is ground
	boundCon ground;
};

// Parameter type
enum parameter
{
	POTTEMP, QV, QW, QC, QR, QS, QI, WINDX, WINDY, WINDZ, PGROUND, PRESSURE, DEBUG1, DEBUG2, DEBUG3
};

enum parameterGround
{
	TEMP, QRS, QGR, QGS, QGI
};

struct simInfo
{
	Neigh* neighbourData{ nullptr }; // Neighbouring data
};