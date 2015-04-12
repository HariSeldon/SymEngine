__kernel void reduce(__global uint *input, __global uint *output,
                     __local uint *localStorage) {
  unsigned int localId = get_local_id(0);
  unsigned int groupId = get_group_id(0);
  unsigned int globalId = get_global_id(0);

  unsigned int localSize = get_local_size(0);
  unsigned int stride = globalId * 2;
  localStorage[localId] = input[stride] + input[stride + 1];

  barrier(0);
  for (unsigned int s = localSize >> 1; s > 0; s >>= 1) {
    if (localId < s) {
      localStorage[localId] += localStorage[localId + s];
    }
    barrier(0);
  }

  if (localId == 0)
    output[groupId] = localStorage[0];
}
