struct Chunk {
    Vec3i coord;
    VulkanBuffer computeBuffer;
    VulkanBuffer vertexBuffer;
    u32 vertexCount;
};

const u32 computeWidth = 32;
const u32 computeHeight = computeWidth;
const u32 computeDepth = computeWidth;
const u32 computeCount = computeWidth * computeHeight * computeDepth;
const u32 computeVerticesPerExecution = 15;
const u32 computeVertexCount = computeVerticesPerExecution * computeCount;
const u32 computeVertexWidth = sizeof(Vertex);
const int computeSize = computeVertexCount * computeVertexWidth;

void chunkTriangulate(Vulkan& vk, Chunk& chunk) {
    // Init & execute compute shader.
    {
        VulkanPipeline pipeline;
        initVKPipelineCompute(
            vk,
            "cs",
            pipeline
        );
        createStorageBuffer(
            vk.device,
            vk.memories,
            vk.computeQueueFamily,
            computeSize,
            chunk.computeBuffer
        );
        updateStorageBuffer(
            vk.device,
            pipeline.descriptorSet,
            0,
            chunk.computeBuffer.handle
        );
        Params params = {
            {chunk.coord.x * 32.f, chunk.coord.y * 32.f, chunk.coord.z * 32.f, 0}
        };
        dispatchCompute(
            vk,
            pipeline,
            computeWidth, computeHeight, computeDepth,
            sizeof(params), &params
        );
        vkQueueWaitIdle(vk.computeQueue);
    }
}

void chunkPack(
    Vulkan& vk,
    Chunk& chunk
) {
    createVertexBuffer(
        vk.device,
        vk.memories,
        vk.queueFamily,
        computeSize, //TODO: this is way too big
        chunk.vertexBuffer
    );

    u32 vertexCount = 0;
    {
        auto src = (Vertex*)mapMemory(vk.device, chunk.computeBuffer.memory);
        auto dst = (Vertex*)mapMemory(vk.device, chunk.vertexBuffer.memory);
        for (int it = 0; it < computeVertexCount; it++) {
            if ((src->position.x != 0.f) ||
                (src->position.y != 0.f) ||
                (src->position.z != 0.f)) {
                *dst = *src;
                dst++;
                vertexCount++;
            }
            src++;
        }
        unMapMemory(vk.device, chunk.computeBuffer.memory);
        unMapMemory(vk.device, chunk.vertexBuffer.memory);
        destroyBuffer(vk, chunk.computeBuffer);
        chunk.computeBuffer = {};
    }

    chunk.vertexCount = vertexCount;
}

struct PackParams {
    Vulkan* vk;
    Chunk* chunk;
};

DWORD WINAPI PackThread(LPVOID param) {
    auto params = (PackParams*)param;
    auto& chunk = *params->chunk;
    chunkPack(*params->vk, *params->chunk);
    delete params;
    return 0;
}

void generateChunk(
    Vulkan& vk,
    Vec3i chunkCoord,
    Chunk& chunk
) {
    chunk = {};
    chunk.coord = chunkCoord;

    chunkTriangulate(vk, chunk);
    auto params = new PackParams;
    params->vk = &vk;
    params->chunk = &chunk;
    CreateThread(
        NULL,
        0,
        PackThread,
        params,
        0,
        NULL
    );
}
