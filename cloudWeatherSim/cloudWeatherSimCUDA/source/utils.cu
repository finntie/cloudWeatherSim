#include "utils.cuh"

#include "msvc_defines.h"
#include <hip/hip_runtime.h>

// Cu file needed for the hip implementation.

// Retrieve grid and block dimensions based on current hardware
__host__ void getGridBlockDims(simParams p, dim3& _gridDim, dim3& _blockDim)
{
    // Reset current ones
    _gridDim = dim3(1,1,1);
    _blockDim = dim3(1,1,1);
    // Set block and grid dimension based on GPU specs.
    hipDeviceProp_t prop;
    hipGetDeviceProperties(&prop, 0);
    unsigned int totalThreadsPerBlock = prop.maxThreadsPerBlock / 4;  // Use smaller threads so we have more leeway
    unsigned int totalBlocksPerGrid = prop.maxBlocksPerMultiProcessor * prop.multiProcessorCount;
    // unsigned int totalRegsPerThread = prop.regsPerBlock / totalThreadsPerBlock;
    //  Set block dimensions to be square root of the max threads available. Making use of as much threads as possible
    const unsigned int threadsBlock = uint32_t(floor(sqrt(totalThreadsPerBlock)));
    // If one side is smaller than 1 block, we still want to use as many threads as possible per block
    if (totalThreadsPerBlock > unsigned(p.sizeX[0]))
    {
        _blockDim.x = p.sizeX[0];
        _blockDim.y = std::min(uint32_t(p.sizeY[0]), uint32_t(double(totalThreadsPerBlock) / double(_blockDim.x)));
    }
    // else if (threadsBlock > GRIDSIZESKYY)
    //{
    //	_blockDim.y = GRIDSIZESKYY;
    //	_blockDim.x = std::min(uint32_t(GRIDSIZESKYX), uint32_t(double(totalThreadsPerBlock) / double(_blockDim.y)));
    // }
    else
    {
        // TODO: Just put all to x axis instead?
        _blockDim.x = std::min(uint32_t(p.sizeX[0]), threadsBlock);
        _blockDim.y = std::min(uint32_t(p.sizeY[0]), threadsBlock);
    }

    // Amount of blocks on the x and y axis is determined by how big our grid is
    _gridDim.x = unsigned(ceil(double(p.sizeX[0]) / double(_blockDim.x)));
    _gridDim.y = unsigned(ceil(double(p.sizeY[0]) / double(_blockDim.y)));
    // Error Check
    if (_gridDim.x * _gridDim.y >= totalBlocksPerGrid)
    {
        printf(
            "Error: current simulation size is greater than available theads and blocks on the x and y axis, use a smaller "
            "simulation size!\n");
        return;
    }
    // Depth is determined by how many blocks we have in total and use per z slice.
    _gridDim.z =
        unsigned(std::min(double(p.sizeZ[0]), floor(double(totalBlocksPerGrid) / double(_gridDim.x * _gridDim.y))));
}

__host__ void getGridBlockDimsMultRes(simParams& p, dim3& _gridDim, dim3& _blockDim)
{
    // Reset current ones
    _gridDim = dim3(1, 1, 1);
    _blockDim = dim3(1, 1, 1);

    // Set default block size
    _blockDim = dim3(16, 16, 1);

    // Set block and grid dimension based on GPU specs.
    hipDeviceProp_t prop;
    hipGetDeviceProperties(&prop, 0);
    unsigned int totalThreadsPerBlock = prop.maxThreadsPerBlock / 4;  // Use smaller threads so we have more leeway
    unsigned int totalBlocksPerGrid = prop.maxBlocksPerMultiProcessor * prop.multiProcessorCount;

    // Divide total available by 2 to have some leeway
    if (_blockDim.x * _blockDim.y * _blockDim.z > prop.maxThreadsPerBlock / 2)
    {
        printf("CLOUDLIB WARNING: default blocksize is greater than available blocks\n");
        return;
    }

    // Sum up blocks needed for every grid resolution.
    int blocksNeeded = 0;
    for (int i = 0; i < p.grids; i++)
    {
        // Z is not included in block size, reduce with resolution decrease to not count the relexation zone.
        int blocksX = int(std::ceil(float(p.sizeX[i] - (p.resDecrease[i] * 2)) / float(_blockDim.x)));
        int blocksY = int(std::ceil(float(p.sizeY[i] - (p.resDecrease[i] * 2)) / float(_blockDim.y)));
        int blocksZ = p.sizeZ[i];
        _gridDim.x += blocksX;
        _gridDim.y += blocksY;
        _gridDim.z += blocksZ;
        blocksNeeded += blocksX * blocksY * blocksZ;

        p.gridDimX[i] = blocksX;
        p.gridDimY[i] = blocksY;
        p.gridDimZ[i] = blocksZ;
    }

    // If we have more than enough threads, lets not worry about dividing them evenly
    if (blocksNeeded >= totalBlocksPerGrid) 
    {
        //... Grids are already set in previous loop
    }
    else
    {
        // Now we need to divide them somehow ...
    }
}

__global__ void validityCheck(float* array, float minValid, float maxValid, int depth, int* valid)
{
    const int x = threadIdx.x;
    const int y = blockIdx.x;
    int z = 0;

    for (z = 0; z < depth; z++)
    {
        const int idx = x + y * blockDim.x + z * blockDim.x * gridDim.x;
        if (array[idx] < minValid || array[idx] > maxValid) *valid = 1;
    }
}
