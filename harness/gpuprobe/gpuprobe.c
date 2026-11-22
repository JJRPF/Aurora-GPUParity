// gpuprobe: a reproducer for GPU compute scheduling problems (aurora-silicon/linux#2701).
//
//   gpuprobe dispatch --ms N --count K [--inflight 2] [--reps R]
//       K submissions of R dispatches (default 1) of about N ms each, with a barrier between them, INFLIGHT submitted at a time (llama.cpp
//       keeps more than one in flight). Every result buffer is hashed and compared with the
//       first one: a wrong answer after a GPU recovery is reported.
//   gpuprobe latency --seconds S [--period-ms 16]
//       A tiny dispatch every PERIOD ms, timed from submit to fence: how long another client
//       waits for the GPU while a long job runs. Prints percentiles.
//
// One JSON line per run on stdout, details on stderr. Exit 1 on a Vulkan error or a wrong hash.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include <vulkan/vulkan.h>
#include "probe_spv.h"

#define N_INVOC (1u << 22)
static VkDevice dev; static VkQueue q; static VkCommandPool pool; static VkPipeline pipe;
static VkPipelineLayout pl; static VkDescriptorSet ds; static VkBuffer buf; static void *map;

static double now_ms(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec * 1e3 + t.tv_nsec / 1e6; }
#define VK(x) do { VkResult r_ = (x); if (r_ != VK_SUCCESS) { fprintf(stderr, "%s failed: %d (line %d)\n", #x, r_, __LINE__); printf("{\"error\":\"%s\",\"vk_result\":%d}\n", #x, r_); exit(1); } } while (0)

static void setup(void) {
  VkApplicationInfo ai = { .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO, .pApplicationName = "gpuprobe", .apiVersion = VK_API_VERSION_1_1 };
  VkInstanceCreateInfo ici = { .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, .pApplicationInfo = &ai };
  VkInstance inst; VK(vkCreateInstance(&ici, NULL, &inst));
  uint32_t n = 1; VkPhysicalDevice pd; VkResult r = vkEnumeratePhysicalDevices(inst, &n, &pd);
  if ((r != VK_SUCCESS && r != VK_INCOMPLETE) || n == 0) { fprintf(stderr, "no Vulkan device\n"); exit(1); }
  VkPhysicalDeviceProperties pp; vkGetPhysicalDeviceProperties(pd, &pp);
  fprintf(stderr, "device: %s\n", pp.deviceName);
  uint32_t nq = 0; vkGetPhysicalDeviceQueueFamilyProperties(pd, &nq, NULL);
  VkQueueFamilyProperties qf[16]; if (nq > 16) nq = 16; vkGetPhysicalDeviceQueueFamilyProperties(pd, &nq, qf);
  uint32_t fam = 0; for (uint32_t i = 0; i < nq; i++) if (qf[i].queueFlags & VK_QUEUE_COMPUTE_BIT) { fam = i; break; }
  float prio = 1.0f;
  VkDeviceQueueCreateInfo qci = { .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, .queueFamilyIndex = fam, .queueCount = 1, .pQueuePriorities = &prio };
  VkDeviceCreateInfo dci = { .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, .queueCreateInfoCount = 1, .pQueueCreateInfos = &qci };
  VK(vkCreateDevice(pd, &dci, NULL, &dev)); vkGetDeviceQueue(dev, fam, 0, &q);
  VkCommandPoolCreateInfo cpi = { .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, .queueFamilyIndex = fam };
  VK(vkCreateCommandPool(dev, &cpi, NULL, &pool));
  VkBufferCreateInfo bi = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = N_INVOC * 4, .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, .sharingMode = VK_SHARING_MODE_EXCLUSIVE };
  VK(vkCreateBuffer(dev, &bi, NULL, &buf));
  VkMemoryRequirements mr; vkGetBufferMemoryRequirements(dev, buf, &mr);
  VkPhysicalDeviceMemoryProperties mp; vkGetPhysicalDeviceMemoryProperties(pd, &mp);
  uint32_t mt = UINT32_MAX;
  for (uint32_t i = 0; i < mp.memoryTypeCount; i++) if ((mr.memoryTypeBits & (1u << i)) && (mp.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) == (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) { mt = i; break; }
  if (mt == UINT32_MAX) { fprintf(stderr, "no host-visible coherent memory\n"); exit(1); }
  VkMemoryAllocateInfo mai = { .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, .allocationSize = mr.size, .memoryTypeIndex = mt };
  VkDeviceMemory mem; VK(vkAllocateMemory(dev, &mai, NULL, &mem)); VK(vkBindBufferMemory(dev, buf, mem, 0)); VK(vkMapMemory(dev, mem, 0, VK_WHOLE_SIZE, 0, &map));
  VkDescriptorSetLayoutBinding b = { .binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT };
  VkDescriptorSetLayoutCreateInfo dli = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, .bindingCount = 1, .pBindings = &b };
  VkDescriptorSetLayout dl; VK(vkCreateDescriptorSetLayout(dev, &dli, NULL, &dl));
  VkPushConstantRange pcr = { .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT, .size = 8 };
  VkPipelineLayoutCreateInfo pli = { .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, .setLayoutCount = 1, .pSetLayouts = &dl, .pushConstantRangeCount = 1, .pPushConstantRanges = &pcr };
  VK(vkCreatePipelineLayout(dev, &pli, NULL, &pl));
  VkShaderModuleCreateInfo smi = { .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, .codeSize = sizeof(probe_spv), .pCode = probe_spv };
  VkShaderModule sm; VK(vkCreateShaderModule(dev, &smi, NULL, &sm));
  VkComputePipelineCreateInfo pci = { .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO, .layout = pl, .stage = { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_COMPUTE_BIT, .module = sm, .pName = "main" } };
  VK(vkCreateComputePipelines(dev, VK_NULL_HANDLE, 1, &pci, NULL, &pipe));
  VkDescriptorPoolSize dps = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 };
  VkDescriptorPoolCreateInfo dpi = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, .maxSets = 1, .poolSizeCount = 1, .pPoolSizes = &dps };
  VkDescriptorPool dp; VK(vkCreateDescriptorPool(dev, &dpi, NULL, &dp));
  VkDescriptorSetAllocateInfo dai = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, .descriptorPool = dp, .descriptorSetCount = 1, .pSetLayouts = &dl };
  VK(vkAllocateDescriptorSets(dev, &dai, &ds));
  VkDescriptorBufferInfo dbi = { buf, 0, VK_WHOLE_SIZE };
  VkWriteDescriptorSet w = { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = ds, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .pBufferInfo = &dbi };
  vkUpdateDescriptorSets(dev, 1, &w, 0, NULL);
}

static VkCommandBuffer record(uint32_t iters, uint32_t groups, int reps) {
  VkCommandBufferAllocateInfo ai = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, .commandPool = pool, .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, .commandBufferCount = 1 };
  VkCommandBuffer cb; VK(vkAllocateCommandBuffers(dev, &ai, &cb));
  VkCommandBufferBeginInfo bi = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
  VK(vkBeginCommandBuffer(cb, &bi));
  vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_COMPUTE, pipe);
  vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_COMPUTE, pl, 0, 1, &ds, 0, NULL);
  uint32_t pc[2] = { iters, N_INVOC }; vkCmdPushConstants(cb, pl, VK_SHADER_STAGE_COMPUTE_BIT, 0, 8, pc);
  VkMemoryBarrier mb = { .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER, .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT, .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT };
  for (int r = 0; r < reps; r++) {
    vkCmdDispatch(cb, groups, 1, 1);
    if (r + 1 < reps) vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &mb, 0, NULL, 0, NULL);
  }
  VK(vkEndCommandBuffer(cb)); return cb;
}
static VkFence new_fence(void) { VkFenceCreateInfo fi = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO }; VkFence f; VK(vkCreateFence(dev, &fi, NULL, &f)); return f; }
static void submit(VkCommandBuffer cb, VkFence f) { VkSubmitInfo si = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = &cb }; VK(vkQueueSubmit(q, 1, &si, f)); }
static void wait(VkFence f) { VK(vkWaitForFences(dev, 1, &f, VK_TRUE, UINT64_MAX)); }
static uint64_t fnv(const void *p, size_t n) { const uint8_t *b = p; uint64_t h = 1469598103934665603ull; for (size_t i = 0; i < n; i++) { h ^= b[i]; h *= 1099511628211ull; } return h; }
static double timed_run(uint32_t iters) { VkCommandBuffer cb = record(iters, N_INVOC / 64, 1); VkFence f = new_fence(); double t = now_ms(); submit(cb, f); wait(f); t = now_ms() - t; vkDestroyFence(dev, f, NULL); vkFreeCommandBuffers(dev, pool, 1, &cb); return t; }
static int cmpd(const void *a, const void *b) { double x = *(const double *)a, y = *(const double *)b; return (x > y) - (x < y); }

int main(int argc, char **argv) {
  if (argc < 2) { fprintf(stderr, "usage: gpuprobe dispatch --ms N --count K [--inflight 2] | latency --seconds S [--period-ms 16]\n"); return 2; }
  const char *mode = argv[1]; double ms = 100, secs = 10, period = 16; int count = 5, inflight = 2, reps = 1;
  for (int i = 2; i + 1 < argc; i += 2) {
    if (!strcmp(argv[i], "--ms")) ms = atof(argv[i + 1]); else if (!strcmp(argv[i], "--count")) count = atoi(argv[i + 1]);
    else if (!strcmp(argv[i], "--inflight")) inflight = atoi(argv[i + 1]); else if (!strcmp(argv[i], "--reps")) reps = atoi(argv[i + 1]); else if (!strcmp(argv[i], "--seconds")) secs = atof(argv[i + 1]);
    else if (!strcmp(argv[i], "--period-ms")) period = atof(argv[i + 1]); else { fprintf(stderr, "unknown %s\n", argv[i]); return 2; }
  }
  setup();
  if (!strcmp(mode, "latency")) {
    VkCommandBuffer cb = record(1, 1, 1); VkFence f = new_fence(); int cap = (int)(secs * 1000 / period) + 16, n = 0; double *s = malloc(cap * sizeof *s); double end = now_ms() + secs * 1000;
    while (now_ms() < end && n < cap) { double t = now_ms(); vkResetFences(dev, 1, &f); submit(cb, f); wait(f); s[n++] = now_ms() - t; double rest = period - (now_ms() - t); if (rest > 0) { struct timespec ts = { 0, (long)(rest * 1e6) }; nanosleep(&ts, NULL); } }
    double mx = 0; int over = 0; for (int i = 0; i < n; i++) { if (s[i] > mx) mx = s[i]; if (s[i] > 8.3) over++; }
    qsort(s, n, sizeof *s, cmpd);
    printf("{\"mode\":\"latency\",\"samples\":%d,\"p50_ms\":%.2f,\"p95_ms\":%.2f,\"p99_ms\":%.2f,\"max_ms\":%.2f,\"over_8_3ms_pct\":%.1f}\n", n, s[n / 2], s[(int)(n * 0.95)], s[(int)(n * 0.99)], mx, 100.0 * over / n);
    return 0;
  }
  // dispatch mode: calibrate iterations to the target duration (two passes), then run.
  uint32_t iters = 20000; double t = timed_run(iters);
  for (int pass = 0; pass < 3; pass++) { uint32_t ni = (uint32_t)fmin(4e9, fmax(1, iters * (ms / fmax(t, 0.01)))); iters = ni; t = timed_run(iters); fprintf(stderr, "calibrate: iters=%u -> %.1f ms\n", iters, t); if (fabs(t - ms) < 0.1 * ms) break; }
  uint64_t ref = 0; double *d = calloc(count, sizeof *d); int bad = 0;
  VkCommandBuffer *cb = calloc(inflight, sizeof *cb); VkFence *fe = calloc(inflight, sizeof *fe); double *t0 = calloc(inflight, sizeof *t0);
  for (int i = 0; i < inflight; i++) { cb[i] = record(iters, N_INVOC / 64, reps); fe[i] = new_fence(); }
  int sub = 0, done = 0; double start = now_ms();
  while (done < count) {
    while (sub < count && sub - done < inflight) { int s = sub % inflight; vkResetFences(dev, 1, &fe[s]); t0[s] = now_ms(); submit(cb[s], fe[s]); sub++; }
    int s = done % inflight; wait(fe[s]); d[done] = now_ms() - t0[s];
    uint64_t h = fnv(map, N_INVOC * 4); if (done == 0) ref = h; else if (h != ref) { bad++; fprintf(stderr, "dispatch %d: result hash differs\n", done); }
    fprintf(stderr, "dispatch %d: %.1f ms\n", done, d[done]); done++;
  }
  double tot = now_ms() - start, mx = 0; for (int i = 0; i < count; i++) if (d[i] > mx) mx = d[i];
  printf("{\"mode\":\"dispatch\",\"target_ms\":%.0f,\"reps\":%d,\"iters\":%u,\"count\":%d,\"inflight\":%d,\"max_ms\":%.1f,\"total_ms\":%.1f,\"hash\":\"%016llx\",\"hash_mismatches\":%d}\n", ms, reps, iters, count, inflight, mx, tot, (unsigned long long)ref, bad);
  return bad ? 1 : 0;
}
