__kernel void mtLocal(__global float *odata, __global float *idata, int width,
                      int height, __local float *block) {
  unsigned int localHeight = get_local_size(1);
  unsigned int localWidth = get_local_size(0);

  unsigned int groupRow = get_group_id(1);
  unsigned int groupColumn = get_group_id(0);

  unsigned int row = get_global_id(1);
  unsigned int column = get_global_id(0);

  unsigned int localRow = get_local_id(1);
  unsigned int localColumn = get_local_id(0);

  unsigned int index_in = row * width + column;
  block[localRow * (localWidth)+localColumn] = idata[index_in];

  barrier(CLK_LOCAL_MEM_FENCE);

  unsigned int Z = localWidth * groupColumn * height + localHeight * groupRow;

  unsigned int localIndex = localRow * localWidth + localColumn;
  unsigned int tRow = localIndex / localHeight;
  unsigned int tColumn = localIndex % localHeight;

  unsigned int index_out = Z + height * tRow + tColumn;
  odata[index_out] = block[tColumn * localWidth + tRow];
}
